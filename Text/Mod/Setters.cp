MODULE TextSetters;
(**
	project	= "BlackBox"
	organization	= "www.oberon.ch"
	contributors	= "Oberon microsystems"
	version	= "System/Rsrc/About"
	copyright	= "System/Rsrc/About"
	license	= "Docu/BB-License"
	changes	= ""
	issues	= ""

**)

	(* correct NextPage postcond in docu *)
	(* make s.r, s.rd reducible? *)
	(* paraShutoff needs to be controlled by an approx flag to certain ops (later ...?) *)

	IMPORT
		Fonts, Ports, Printers, Stores, Models, Views, Properties,
		TextModels, TextRulers;

	CONST
		(** Pref.opts, options of setter-aware views; 0 overrides 1 **)
		lineBreak* = 0; wordJoin* = 1; wordPart* = 2; flexWidth* = 3;

		tab = TextModels.tab; line = TextModels.line; para = TextModels.para;
		zwspace = TextModels.zwspace; nbspace = TextModels.nbspace;
		hyphen = TextModels.hyphen; nbhyphen = TextModels.nbhyphen;
		digitspace = TextModels.digitspace;
		softhyphen = TextModels.softhyphen;

		mm = Ports.mm;
		minTabWidth = 2 * Ports.point; stdTabWidth = 4 * mm;
		leftLineGap = 2 * Ports.point; rightLineGap = 3 * Ports.point;
		adjustMask = {TextRulers.leftAdjust, TextRulers.rightAdjust};
		centered = {}; leftFlush = {TextRulers.leftAdjust}; rightFlush = {TextRulers.rightAdjust};
		blocked = adjustMask;

		boxCacheLen = 64;
		seqCacheLen = 16;

		paraShutoff = MAX(INTEGER);	(* longest stretch read backwards to find start of paragraph *)
															(* unsafe: disabled *)
		cachedRulers = FALSE;	(* caching ruler objects trades speed against GC effectiveness *)
		periodInWords = FALSE;
		colonInWords = FALSE;

		minVersion = 0; maxVersion = 0; maxStdVersion = 0;


	TYPE
		Pref* = RECORD (Properties.Preference)
			opts*: SET;
			endW*: INTEGER;	(** preset (to width of view) **)
			dsc*: INTEGER	(** preset (to dominating line descender) **)
		END;


		Reader* = POINTER TO ABSTRACT RECORD
			r-: TextModels.Reader;	(** look-ahead state **)
			(** unit **)
			string*: ARRAY 64 OF CHAR;	(** single chars in string[0] **)
			view*: Views.View;
			(** unit props **)
			textOpts*: SET;
			mask*: CHAR;
			setterOpts*: SET;
			w*, endW*, h*, dsc*: INTEGER;
			attr*: TextModels.Attributes;
			(** reading state **)
			eot*: BOOLEAN;
			pos*: INTEGER;
			x*: INTEGER;	(** to be advanced by client! **)
			adjStart*: INTEGER;
			spaces*: INTEGER;
			tabIndex*: INTEGER;	(** tabs being processed; initially -1 **)
			tabType*: SET;	(** type of tab being processed; initially {} **)
			(** line props **)
			vw*: INTEGER;
			hideMarks*: BOOLEAN;
			ruler*: TextRulers.Ruler;
			rpos*: INTEGER
		END;

		Setter* = POINTER TO ABSTRACT RECORD (Stores.Store)
			text-: TextModels.Model;	(** connected iff text # NIL **)
			defRuler-: TextRulers.Ruler;
			vw-: INTEGER;
			hideMarks-: BOOLEAN
		END;


		LineBox* = RECORD
			len*: INTEGER;
			ruler*: TextRulers.Ruler;
			rpos*: INTEGER;
			left*, right*, asc*, dsc*: INTEGER;
			rbox*, bop*, adj*, eot*: BOOLEAN;	(** adj => adjW > 0; adj & blocked => spaces > 0 **)
			views*: BOOLEAN;
			skipOff*: INTEGER;	(** chars in [skipOff, len) take endW **)
			adjOff*: INTEGER;	(** offset of last block in box - adjust only this block **)
			spaces*: INTEGER;	(** valid, > 0 if adj & blocked **)
			adjW*: INTEGER;	(** valid if adj - to be distributed over spaces **)
			tabW*: ARRAY TextRulers.maxTabs OF INTEGER	(** delta width of tabs (<= 0) **)
		END;


		Directory* = POINTER TO ABSTRACT RECORD END;


		Worder = RECORD
			box: LineBox; next: INTEGER;
			i: INTEGER
		END;

		StdReader = POINTER TO RECORD (Reader) END;

		StdSetter = POINTER TO RECORD (Setter)
			rd: Reader;	(* subject to reduction? *)
			r: TextModels.Reader;	(* subject to reduction? *)
			ruler: TextRulers.Ruler;
			rpos: INTEGER;
			key: INTEGER
		END;

		StdDirectory = POINTER TO RECORD (Directory) END;


	VAR
		dir-, stdDir-: Directory;

		nextKey: INTEGER;
		boxIndex, seqIndex: INTEGER;
		boxCache: ARRAY boxCacheLen OF RECORD
			key: INTEGER;	(* valid iff key > 0 *)
			start: INTEGER;
			line: LineBox	(* inv ruler = NIL *)
		END;
		seqCache: ARRAY seqCacheLen OF RECORD
			key: INTEGER;	(* valid iff key > 0 *)
			start, pos: INTEGER	(* sequence [start, end), end >= pos *)
		END;


	(** Reader **)

	PROCEDURE (rd: Reader) Set* (
		old: TextModels.Reader;
		text: TextModels.Model; x, pos: INTEGER;
		ruler: TextRulers.Ruler; rpos: INTEGER; vw: INTEGER; hideMarks: BOOLEAN
	), NEW, EXTENSIBLE;
	BEGIN
		ASSERT(text # NIL, 20);
		ASSERT(ruler # NIL, 22);
		rd.r := text.NewReader(old); rd.r.SetPos(pos); rd.r.Read;
		rd.string[0] := 0X; rd.view := NIL;
		rd.textOpts := {};
		rd.setterOpts := {}; rd.w := 0; rd.endW := 0; rd.h := 0; rd.dsc := 0;
		rd.attr := NIL;
		rd.eot := FALSE; rd.pos := pos; rd.x := x;
		rd.tabIndex := -1; rd.tabType := {};
		rd.adjStart := pos; rd.spaces := 0;
		rd.ruler := ruler; rd.rpos := rpos; rd.vw := vw; rd.hideMarks := hideMarks
	END Set;

	PROCEDURE (rd: Reader) Read*, NEW, EXTENSIBLE;
	(** pre: rd set **)
	(** post: rd.pos = rd.pos' + Length(rd.string) **)
	BEGIN
		rd.string[0] := rd.r.char; rd.string[1] := 0X;
		rd.view := rd.r.view;
		rd.textOpts := {};
		rd.setterOpts := {};
		rd.w := rd.r.w; rd.endW := rd.w; rd.h := rd.r.h; rd.dsc := 0;
		rd.attr := rd.r.attr;
		rd.eot := rd.r.eot;
		INC(rd.pos);
		rd.r.Read
	END Read;

	PROCEDURE (rd: Reader) AdjustWidth* (start, pos: INTEGER; IN box: LineBox;
		VAR w: INTEGER
	), NEW, ABSTRACT;

	PROCEDURE (rd: Reader) SplitWidth* (w: INTEGER): INTEGER, NEW, ABSTRACT;


	(** Setter **)

	PROCEDURE (s: Setter) CopyFrom- (source: Stores.Store), EXTENSIBLE;
	BEGIN
		WITH source: Setter DO
			s.text := source.text; s.defRuler := source.defRuler;
			s.vw := source.vw;  s.hideMarks := source.hideMarks
		END
	END CopyFrom;

	PROCEDURE (s: Setter) Internalize- (VAR rd: Stores.Reader), EXTENSIBLE;
		VAR thisVersion: INTEGER;
	BEGIN
		s.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxVersion, thisVersion)
	END Internalize;

	PROCEDURE (s: Setter) Externalize- (VAR wr: Stores.Writer), EXTENSIBLE;
	BEGIN
		s.Externalize^(wr);
		wr.WriteVersion(maxVersion)
	END Externalize;

	PROCEDURE (s: Setter) ConnectTo* (text: TextModels.Model;
		defRuler: TextRulers.Ruler; vw: INTEGER; hideMarks: BOOLEAN
	), NEW, EXTENSIBLE;
	BEGIN
		IF text # NIL THEN
			s.text := text; s.defRuler := defRuler; s.vw := vw; s.hideMarks := hideMarks
		ELSE
			s.text := NIL; s.defRuler := NIL
		END
	END ConnectTo;


	PROCEDURE (s: Setter) ThisPage* (pageH: INTEGER; pageNo: INTEGER): INTEGER, NEW, ABSTRACT;
	(** pre: connected, 0 <= pageNo **)
	(** post: (result = -1) & (pageNo >= maxPageNo) OR (result = pageStart(pageNo)) **)

	PROCEDURE (s: Setter) NextPage* (pageH: INTEGER; start: INTEGER): INTEGER, NEW, ABSTRACT;
	(** pre: connected, ThisPage(pageH, pageNo) = start [with pageNo = NumberOfPageAt(start)] **)
	(** post: (result = start) & last-page(start) OR result = next-pageStart(start) **)


	PROCEDURE (s: Setter) ThisSequence* (pos: INTEGER): INTEGER, NEW, ABSTRACT;
	(** pre: connected, 0 <= pos <= s.text.Length() **)
	(** post: (result = 0) OR (char(result - 1) IN {line, para}) **)

	PROCEDURE (s: Setter) NextSequence* (start: INTEGER): INTEGER, NEW, ABSTRACT;
	(** pre: connected, ThisSequence(start) = start **)
	(** post: (result = start) & last-line(start) OR (ThisSequence(t, result - 1) = start) **)

	PROCEDURE (s: Setter) PreviousSequence* (start: INTEGER): INTEGER, NEW, ABSTRACT;
	(** pre: connected, ThisSequence(t, start) = start **)
	(** post: (result = 0) & (start = 0) OR (result = ThisSequence(t, start - 1)) **)


	PROCEDURE (s: Setter) ThisLine* (pos: INTEGER): INTEGER, NEW, ABSTRACT;
	(** pre: connected, 0 <= pos <= s.text.Length() **)
	(** post: result <= pos, (pos < NextLine(result)) OR last-line(result) **)

	PROCEDURE (s: Setter) NextLine* (start: INTEGER): INTEGER, NEW, ABSTRACT;
	(** pre: connected, ThisLine(start) = start **)
	(** post: (result = 0) & (start = 0) OR
				(result = start) & last-line(start) OR
				(ThisLine(result - 1) = start) **)

	PROCEDURE (s: Setter) PreviousLine* (start: INTEGER): INTEGER, NEW, ABSTRACT;
	(** pre: connected, ThisLine(start) = start **)
	(** post: (result = 0) & (start = 0) OR (result = ThisLine(start - 1)) **)


	PROCEDURE (s: Setter) GetWord* (pos: INTEGER; OUT beg, end: INTEGER), NEW, ABSTRACT;
	(** pre: connected, 0 <= pos <= s.text.Length() **)
	(** post: c set, beg <= pos <= end **)

	PROCEDURE (s: Setter) GetLine* (start: INTEGER; OUT box: LineBox), NEW, ABSTRACT;
	(** pre: connected, ThisLine(start) = start, 0 <= start <= s.text.Length() **)
	(** post: (c, box) set (=> box.ruler # NIL), (box.len > 0) OR box.eot,
		 0 <= box.left <= box.right <= ruler.right **)

	PROCEDURE (s: Setter) GetBox* (start, end, maxW, maxH: INTEGER;
		OUT w, h: INTEGER
	), NEW, ABSTRACT;
	(** pre: connected, ThisLine(start) = start, 0 <= start <= end <= s.text.Length() **)
	(** post: c set, maxW > undefined => w <= maxW, maxH > undefined => h <= maxH **)


	PROCEDURE (s: Setter) NewReader* (old: Reader): Reader, NEW, ABSTRACT;
	(** pre: connected **)


	PROCEDURE (s: Setter) GridOffset* (dsc: INTEGER; IN box: LineBox): INTEGER, NEW, ABSTRACT;
	(** pre: connected, dsc >= 0: dsc is descender of previous line; dsc = -1 for first line **)
	(** post: dsc + GridOffset(dsc, box) + box.asc = k*ruler.grid (k >= 0) >= ruler.asc + ruler.grid **)


	(** Directory **)

	PROCEDURE (d: Directory) New* (): Setter, NEW, ABSTRACT;


	(* line box cache *)

	PROCEDURE InitCache;
		VAR i: INTEGER;
	BEGIN
		nextKey := 1; boxIndex := 0; seqIndex := 0;
		i := 0; WHILE i < boxCacheLen DO boxCache[i].key := -1; INC(i) END;
		i := 0; WHILE i < seqCacheLen DO seqCache[i].key := -1; INC(i) END
	END InitCache;

	PROCEDURE ClearCache (key: INTEGER);
		VAR i, j: INTEGER;
	BEGIN
		i := 0; j := boxIndex;
		WHILE i < boxCacheLen DO
			IF boxCache[i].key = key THEN boxCache[i].key := -1; j := i END;
			INC(i)
		END;
		boxIndex := j;
		i := 0; j := seqIndex;
		WHILE i < seqCacheLen DO
			IF seqCache[i].key = key THEN seqCache[i].key := -1; j := i END;
			INC(i)
		END;
		seqIndex := j
	END ClearCache;


	PROCEDURE CacheIndex (key, start: INTEGER): INTEGER;
		VAR i: INTEGER;
	BEGIN
RETURN -1;
		i := 0;
		WHILE (i < boxCacheLen) & ~((boxCache[i].key = key) & (boxCache[i].start = start)) DO
			INC(i)
		END;
		IF i = boxCacheLen THEN i := -1 END;
		RETURN i
	END CacheIndex;

	PROCEDURE GetFromCache (s: StdSetter; i: INTEGER; VAR l: LineBox);
	BEGIN
		l := boxCache[i].line;
		IF ~cachedRulers THEN
			IF l.rpos >= 0 THEN
				s.r := s.text.NewReader(s.r); s.r.SetPos(l.rpos); s.r.Read;
				l.ruler := s.r.view(TextRulers.Ruler)
			ELSE l.ruler := s.defRuler
			END
		END
	END GetFromCache;

	PROCEDURE AddToCache (key, start: INTEGER; VAR l: LineBox);
		VAR i: INTEGER;
	BEGIN
		i := boxIndex; boxIndex := (i + 1) MOD boxCacheLen;
		boxCache[i].key := key; boxCache[i].start := start; boxCache[i].line := l;
		IF ~cachedRulers THEN
			boxCache[i].line.ruler := NIL
		END
	END AddToCache;


	PROCEDURE CachedSeqStart (key, pos: INTEGER): INTEGER;
		VAR start: INTEGER; i: INTEGER;
	BEGIN
		i := 0;
		WHILE (i < seqCacheLen)
		& ~((seqCache[i].key = key) & (seqCache[i].start <= pos) & (pos <= seqCache[i].pos)) DO
			INC(i)
		END;
		IF i < seqCacheLen THEN start := seqCache[i].start ELSE start := -1 END;
		RETURN start
	END CachedSeqStart;

	PROCEDURE AddSeqStartToCache (key, pos, start: INTEGER);
		VAR i: INTEGER;
	BEGIN
		i := 0;
		WHILE (i < seqCacheLen) & ~((seqCache[i].key = key) & (seqCache[i].start = start)) DO
			INC(i)
		END;
		IF i < seqCacheLen THEN
			IF seqCache[i].pos < pos THEN seqCache[i].pos := pos END
		ELSE
			i := seqIndex; seqIndex := (i + 1) MOD seqCacheLen;
			seqCache[i].key := key; seqCache[i].pos := pos; seqCache[i].start := start
		END
	END AddSeqStartToCache;


	(* StdReader *)

(*
	PROCEDURE WordPart (ch, ch1: CHAR): BOOLEAN;
	(* needs more work ... put elsewhere? *)
	BEGIN
		CASE ORD(ch) OF
		  ORD("0") .. ORD("9"), ORD("A") .. ORD("Z"), ORD("a") .. ORD("z"),
		  ORD(digitspace), ORD(nbspace), ORD(nbhyphen), ORD("_"),
		  0C0H .. 0C6H, 0E0H .. 0E6H,	(* ~ A *)
		  0C7H, 0E7H,	(* ~ C *)
		  0C8H .. 0CBH, 0E8H .. 0EBH,	(* ~ E *)
		  0CCH .. 0CFH, 0ECH .. 0EFH,	(* ~ I *)
		  0D1H, 0F1H,	(* ~ N *)
		  0D2H .. 0D6H, 0D8H, 0F2H .. 0F6H, 0F8H,	(* ~ O *)
		  0D9H .. 0DCH, 0F9H .. 0FCH,	(* ~ U *)
		  0DDH, 0FDH, 0FFH,	(* ~ Y *)
		  0DFH:	(* ~ ss *)
			RETURN TRUE
		| ORD("."), ORD(":"):
			IF (ch = ".") & periodInWords OR (ch = ":") & colonInWords THEN
				CASE ch1 OF
				  0X, TextModels.viewcode, tab, line, para, " ":
					RETURN FALSE
				ELSE RETURN TRUE
				END
			ELSE RETURN FALSE
			END
		ELSE RETURN FALSE
		END
	END WordPart;
*)

	PROCEDURE WordPart (ch, ch1: CHAR): BOOLEAN;
		(* Same as .net function System.Char.IsLetterOrDigit(ch)
				+ digit space, nonbreaking space, nonbreaking hyphen, & underscore
			ch1 unused *)
		VAR low: INTEGER;
	BEGIN
		low := ORD(ch) MOD 256;
		CASE ORD(ch) DIV 256 OF
		| 001H, 015H, 034H..04CH, 04EH..09EH, 0A0H..0A3H, 0ACH..0D6H, 0F9H, 0FCH: RETURN TRUE
		| 000H: CASE low OF
			| 030H..039H, 041H..05AH, 061H..07AH, 0AAH, 0B5H, 0BAH, 0C0H..0D6H, 0D8H..0F6H, 0F8H..0FFH,
				ORD(digitspace), ORD(nbspace), ORD(nbhyphen), ORD("_"): RETURN TRUE
			ELSE
			END
		| 002H: CASE low OF
			| 000H..041H, 050H..0C1H, 0C6H..0D1H, 0E0H..0E4H, 0EEH: RETURN TRUE
			ELSE
			END
		| 003H: CASE low OF
			| 07AH, 086H, 088H..08AH, 08CH, 08EH..0A1H, 0A3H..0CEH, 0D0H..0F5H, 0F7H..0FFH: RETURN TRUE
			ELSE
			END
		| 004H: CASE low OF
			| 000H..081H, 08AH..0CEH, 0D0H..0F9H: RETURN TRUE
			ELSE
			END
		| 005H: CASE low OF
			| 000H..00FH, 031H..056H, 059H, 061H..087H, 0D0H..0EAH, 0F0H..0F2H: RETURN TRUE
			ELSE
			END
		| 006H: CASE low OF
			| 021H..03AH, 040H..04AH, 060H..069H, 06EH..06FH, 071H..0D3H, 0D5H, 0E5H..0E6H, 0EEH..0FCH, 0FFH: RETURN TRUE
			ELSE
			END
		| 007H: CASE low OF
			| 010H, 012H..02FH, 04DH..06DH, 080H..0A5H, 0B1H: RETURN TRUE
			ELSE
			END
		| 009H: CASE low OF
			| 004H..039H, 03DH, 050H, 058H..061H, 066H..06FH, 07DH, 085H..08CH, 08FH..090H, 093H..0A8H, 0AAH..0B0H, 0B2H, 0B6H..0B9H, 0BDH, 0CEH, 0DCH..0DDH, 0DFH..0E1H, 0E6H..0F1H: RETURN TRUE
			ELSE
			END
		| 00AH: CASE low OF
			| 005H..00AH, 00FH..010H, 013H..028H, 02AH..030H, 032H..033H, 035H..036H, 038H..039H, 059H..05CH, 05EH, 066H..06FH, 072H..074H, 085H..08DH, 08FH..091H, 093H..0A8H, 0AAH..0B0H, 0B2H..0B3H, 0B5H..0B9H, 0BDH, 0D0H, 0E0H..0E1H, 0E6H..0EFH: RETURN TRUE
			ELSE
			END
		| 00BH: CASE low OF
			| 005H..00CH, 00FH..010H, 013H..028H, 02AH..030H, 032H..033H, 035H..039H, 03DH, 05CH..05DH, 05FH..061H, 066H..06FH, 071H, 083H, 085H..08AH, 08EH..090H, 092H..095H, 099H..09AH, 09CH, 09EH..09FH, 0A3H..0A4H, 0A8H..0AAH, 0AEH..0B9H, 0E6H..0EFH: RETURN TRUE
			ELSE
			END
		| 00CH: CASE low OF
			| 005H..00CH, 00EH..010H, 012H..028H, 02AH..033H, 035H..039H, 060H..061H, 066H..06FH, 085H..08CH, 08EH..090H, 092H..0A8H, 0AAH..0B3H, 0B5H..0B9H, 0BDH, 0DEH, 0E0H..0E1H, 0E6H..0EFH: RETURN TRUE
			ELSE
			END
		| 00DH: CASE low OF
			| 005H..00CH, 00EH..010H, 012H..028H, 02AH..039H, 060H..061H, 066H..06FH, 085H..096H, 09AH..0B1H, 0B3H..0BBH, 0BDH, 0C0H..0C6H: RETURN TRUE
			ELSE
			END
		| 00EH: CASE low OF
			| 001H..030H, 032H..033H, 040H..046H, 050H..059H, 081H..082H, 084H, 087H..088H, 08AH, 08DH, 094H..097H, 099H..09FH, 0A1H..0A3H, 0A5H, 0A7H, 0AAH..0ABH, 0ADH..0B0H, 0B2H..0B3H, 0BDH, 0C0H..0C4H, 0C6H, 0D0H..0D9H, 0DCH..0DDH: RETURN TRUE
			ELSE
			END
		| 00FH: CASE low OF
			| 000H, 020H..029H, 040H..047H, 049H..06AH, 088H..08BH: RETURN TRUE
			ELSE
			END
		| 010H: CASE low OF
			| 000H..021H, 023H..027H, 029H..02AH, 040H..049H, 050H..055H, 0A0H..0C5H, 0D0H..0FAH, 0FCH: RETURN TRUE
			ELSE
			END
		| 011H: CASE low OF
			| 000H..059H, 05FH..0A2H, 0A8H..0F9H: RETURN TRUE
			ELSE
			END
		| 012H: CASE low OF
			| 000H..048H, 04AH..04DH, 050H..056H, 058H, 05AH..05DH, 060H..088H, 08AH..08DH, 090H..0B0H, 0B2H..0B5H, 0B8H..0BEH, 0C0H, 0C2H..0C5H, 0C8H..0D6H, 0D8H..0FFH: RETURN TRUE
			ELSE
			END
		| 013H: CASE low OF
			| 000H..010H, 012H..015H, 018H..05AH, 080H..08FH, 0A0H..0F4H: RETURN TRUE
			ELSE
			END
		| 014H: IF low >= 001H THEN RETURN TRUE END
		| 016H: CASE low OF
			| 000H..06CH, 06FH..076H, 081H..09AH, 0A0H..0EAH: RETURN TRUE
			ELSE
			END
		| 017H: CASE low OF
			| 000H..00CH, 00EH..011H, 020H..031H, 040H..051H, 060H..06CH, 06EH..070H, 080H..0B3H, 0D7H, 0DCH, 0E0H..0E9H: RETURN TRUE
			ELSE
			END
		| 018H: CASE low OF
			| 010H..019H, 020H..077H, 080H..0A8H: RETURN TRUE
			ELSE
			END
		| 019H: CASE low OF
			| 000H..01CH, 046H..06DH, 070H..074H, 080H..0A9H, 0C1H..0C7H, 0D0H..0D9H: RETURN TRUE
			ELSE
			END
		| 01AH: IF low < 017H THEN RETURN TRUE END
		| 01DH: IF low < 0C0H THEN RETURN TRUE END
		| 01EH: CASE low OF
			| 000H..09BH, 0A0H..0F9H: RETURN TRUE
			ELSE
			END
		| 01FH: CASE low OF
			| 000H..015H, 018H..01DH, 020H..045H, 048H..04DH, 050H..057H, 059H, 05BH, 05DH, 05FH..07DH, 080H..0B4H, 0B6H..0BCH, 0BEH, 0C2H..0C4H, 0C6H..0CCH, 0D0H..0D3H, 0D6H..0DBH, 0E0H..0ECH, 0F2H..0F4H, 0F6H..0FCH: RETURN TRUE
			ELSE
			END
		| 020H: CASE low OF
			| 071H, 07FH, 090H..094H: RETURN TRUE
			ELSE
			END
		| 021H: CASE low OF
			| 002H, 007H, 00AH..013H, 015H, 019H..01DH, 024H, 026H, 028H, 02AH..02DH, 02FH..031H, 033H..039H, 03CH..03FH, 045H..049H: RETURN TRUE
			ELSE
			END
		| 02CH: CASE low OF
			| 000H..02EH, 030H..05EH, 080H..0E4H: RETURN TRUE
			ELSE
			END
		| 02DH: CASE low OF
			| 000H..025H, 030H..065H, 06FH, 080H..096H, 0A0H..0A6H, 0A8H..0AEH, 0B0H..0B6H, 0B8H..0BEH, 0C0H..0C6H, 0C8H..0CEH, 0D0H..0D6H, 0D8H..0DEH: RETURN TRUE
			ELSE
			END
		| 030H: CASE low OF
			| 005H..006H, 031H..035H, 03BH..03CH, 041H..096H, 09DH..09FH, 0A1H..0FAH, 0FCH..0FFH: RETURN TRUE
			ELSE
			END
		| 031H: CASE low OF
			| 005H..02CH, 031H..08EH, 0A0H..0B7H, 0F0H..0FFH: RETURN TRUE
			ELSE
			END
		| 04DH: IF low < 0B6H THEN RETURN TRUE END
		| 09FH: IF low < 0BCH THEN RETURN TRUE END
		| 0A4H: IF low < 08DH THEN RETURN TRUE END
		| 0A8H: CASE low OF
			| 000H..001H, 003H..005H, 007H..00AH, 00CH..022H: RETURN TRUE
			ELSE
			END
		| 0D7H: IF low < 0A4H THEN RETURN TRUE END
		| 0FAH: CASE low OF
			| 000H..02DH, 030H..06AH, 070H..0D9H: RETURN TRUE
			ELSE
			END
		| 0FBH: CASE low OF
			| 000H..006H, 013H..017H, 01DH, 01FH..028H, 02AH..036H, 038H..03CH, 03EH, 040H..041H, 043H..044H, 046H..0B1H, 0D3H..0FFH: RETURN TRUE
			ELSE
			END
		| 0FDH: CASE low OF
			| 000H..03DH, 050H..08FH, 092H..0C7H, 0F0H..0FBH: RETURN TRUE
			ELSE
			END
		| 0FEH: CASE low OF
			| 070H..074H, 076H..0FCH: RETURN TRUE
			ELSE
			END
		| 0FFH: CASE low OF
			| 010H..019H, 021H..03AH, 041H..05AH, 066H..0BEH, 0C2H..0C7H, 0CAH..0CFH, 0D2H..0D7H, 0DAH..0DCH: RETURN TRUE
			ELSE
			END
		ELSE
		END;
		RETURN FALSE		
	END WordPart;

(*
	PROCEDURE ExtendToEOL (x, right: INTEGER): INTEGER;
	BEGIN
		IF right - x > 5 * mm THEN RETURN right - x ELSE RETURN 5 * mm END
	END ExtendToEOL;
*)

	PROCEDURE Right (ra: TextRulers.Attributes; vw: INTEGER): INTEGER;
	BEGIN
		IF TextRulers.rightFixed IN ra.opts THEN
			RETURN ra.right
		ELSE
			RETURN vw
		END
	END Right;

	PROCEDURE GetViewPref (rd: StdReader);
		CONST maxH = 1600 * Ports.point;
		VAR ra: TextRulers.Attributes; tp: TextModels.Pref; sp: Pref;
	BEGIN
		ra := rd.ruler.style.attr;
		tp.opts := {}; Views.HandlePropMsg(rd.view, tp);
		rd.textOpts := tp.opts; rd.mask := tp.mask;
		sp.opts := {}; sp.dsc := ra.dsc; sp.endW := rd.w; Views.HandlePropMsg(rd.view, sp);
		rd.setterOpts := sp.opts; rd.dsc := sp.dsc; rd.endW := sp.endW;
		IF rd.w >= 10000 * mm THEN rd.w := 10000 * mm END;
		IF (TextModels.hideable IN tp.opts) & rd.hideMarks THEN
			rd.h := 0; sp.dsc := 0;
(*
rd.w := 0;
*)
			IF ~( (rd.view IS TextRulers.Ruler)
					OR (TextModels.maskChar IN rd.textOpts) & (rd.mask = para) ) THEN
				rd.w := 0
			END
(**)
		ELSIF rd.h > maxH THEN rd.h := maxH
		END;
		IF TextModels.maskChar IN rd.textOpts THEN
			rd.string[0] := rd.mask; rd.string[1] := 0X
		ELSE rd.string[0] := TextModels.viewcode
		END
	END GetViewPref;

	PROCEDURE GatherString (rd: StdReader);
		VAR i, len: INTEGER; ch: CHAR;
	BEGIN
		i := 1; len := LEN(rd.string) - 1; ch := rd.r.char;
		WHILE (i < len)
			& (rd.r.view = NIL) & (rd.r.attr = rd.attr)
			& (	 (" " < ch) & (ch <= "~") & (ch # "-")
				OR  (ch = digitspace)
				OR  (ch >= nbspace) & (ch < 100X) & (ch # softhyphen)
				)
		DO	(* rd.r.char > " " => ~rd.eot *)
			rd.string[i] := ch; INC(i);
			rd.eot := rd.r.eot;
			rd.r.Read; ch := rd.r.char; INC(rd.pos)
		END;
		rd.string[i] := 0X; rd.setterOpts := {wordJoin};
		IF i = 1 THEN
			IF WordPart(rd.string[0], 0X) THEN INCL(rd.setterOpts, wordPart) END
		END;
		rd.w := rd.attr.font.StringWidth(rd.string); rd.endW := rd.w
	END GatherString;

	PROCEDURE SpecialChar (rd: StdReader);
		VAR ra: TextRulers.Attributes; i, tabs, spaceW, dW: INTEGER; type: SET;
	BEGIN
		ra := rd.ruler.style.attr;
		CASE ORD(rd.string[0]) OF
		  ORD(tab):
			rd.textOpts := {TextModels.hideable};
			rd.endW := minTabWidth;
			rd.adjStart := rd.pos; rd.spaces := 0;
			(*
			i := 0; WHILE (i < ra.tabs.len) & (ra.tabs.tab[i].stop < rd.x + minTabWidth) DO INC(i) END;
			*)
			i := rd.tabIndex + 1;
			IF i < ra.tabs.len THEN
				type := ra.tabs.tab[i].type;
				rd.w := MAX(minTabWidth, ra.tabs.tab[i].stop - rd.x);
				IF TextRulers.barTab IN type THEN
					IF TextRulers.rightTab IN type THEN
						rd.w := MAX(minTabWidth, rd.w - leftLineGap)
					ELSIF ~(TextRulers.centerTab IN type) THEN
						INC(rd.w, rightLineGap)
					END
				END;
				rd.tabIndex := i; rd.tabType := type
			ELSE	(* for "reasonable" fonts: round to closest multiple of spaces of this font *)
				spaceW := rd.attr.font.SStringWidth(" ");
				IF (1 <= spaceW) & (spaceW <= stdTabWidth) THEN
					rd.w := (stdTabWidth + spaceW DIV 2) DIV spaceW * spaceW
				ELSE
					rd.w := stdTabWidth
				END;
				rd.tabIndex := TextRulers.maxTabs; rd.tabType := {}
			END
		| ORD(line):
			rd.setterOpts := {lineBreak}; rd.w := 0; rd.endW := 0
		| ORD(para):
(*
			IF rd.hideMarks THEN
				rd.w := 0; rd.h := 0; rd.dsc := 0
			ELSE
				rd.w := ExtendToEOL(rd.x, Right(ra, rd.vw)) + 1
			END;
			INC(rd.h, ra.lead);
			rd.textOpts := {TextModels.hideable};
			rd.endW := rd.w
*)
(*
			rd.setterOpts := {lineBreak};
*)
			IF rd.hideMarks THEN rd.h := 0; rd.dsc := 0 END;
			INC(rd.h, ra.lead); rd.textOpts := {TextModels.hideable};
			IF (rd.view = NIL) OR ~(rd.view IS TextRulers.Ruler) THEN
				rd.w := 10000 * Ports.mm (* ExtendToEOL(rd.x, Right(ra, rd.vw)) + 1 *)
			END;
			rd.endW := rd.w
(**)
		| ORD(" "):
			rd.setterOpts := {flexWidth};
			rd.w := rd.attr.font.StringWidth(rd.string); rd.endW := 0; INC(rd.spaces)
		| ORD(zwspace):
			rd.w := 0; rd.endW := 0
		| ORD(digitspace):
			rd.setterOpts := {wordPart};
			rd.w := rd.attr.font.StringWidth("0"); rd.endW := rd.w
		| ORD("-"):
			rd.setterOpts := {};
			rd.w := rd.attr.font.StringWidth("-"); rd.endW := rd.w
		| ORD(hyphen):
			rd.setterOpts := {};
			rd.string[0] := "-" (*softhyphen*);
			rd.w := rd.attr.font.StringWidth("-" (*softhyphen*)); rd.endW := rd.w
		| ORD(nbhyphen):
			rd.setterOpts := {wordJoin, wordPart};
			rd.string[0] := "-" (*softhyphen*);
			rd.w := rd.attr.font.StringWidth("-" (*softhyphen*)); rd.endW := rd.w
		| ORD(softhyphen):
			rd.setterOpts := {wordPart}; rd.textOpts := {TextModels.hideable};
			rd.string[0] := "-";
			rd.endW := rd.attr.font.StringWidth("-" (*softhyphen*));
			IF rd.hideMarks THEN rd.w := 0 ELSE rd.w := rd.endW END
		ELSE
			rd.setterOpts := {wordJoin};
			IF WordPart(rd.string[0], rd.r.char) THEN INCL(rd.setterOpts, wordPart) END;
			rd.w := rd.attr.font.StringWidth(rd.string); rd.endW := rd.w
		END
	END SpecialChar;
(*
	PROCEDURE LongChar (rd: StdReader);
		VAR ra: TextRulers.Attributes;
	BEGIN
		ra := rd.ruler.style.attr;
		rd.setterOpts := {wordJoin, wordPart};
		rd.w := rd.attr.font.StringWidth(rd.string); rd.endW := rd.w
	END LongChar;
*)

	PROCEDURE (rd: StdReader) Read;
	(* pre: connected *)
		VAR ra: TextRulers.Attributes; asc, dsc, w: INTEGER; ch: CHAR;
	BEGIN
		rd.Read^;
		IF ~rd.eot THEN
			IF rd.view = NIL THEN
				rd.attr.font.GetBounds(asc, dsc, w);
				rd.h := asc + dsc; rd.dsc := dsc
			ELSE
				GetViewPref(rd)
			END;
			IF (rd.view = NIL) OR (TextModels.maskChar IN rd.textOpts) THEN
				ch := rd.string[0];
				IF (rd.view = NIL)
				 & (	(" " < ch) & (ch < "~") & (ch # "-")
				 	OR (ch = digitspace)
				 	OR (ch >= nbspace) & (ch # softhyphen)
				  	)
				THEN
					GatherString(rd)
				ELSE
					SpecialChar(rd)
				END
			END
		ELSE
			ra := rd.ruler.style.attr;
			rd.w := 0; rd.endW := 0; rd.h := ra.asc + ra.dsc; rd.dsc := ra.dsc
		END
	END Read;

	PROCEDURE (rd: StdReader) AdjustWidth (start, pos: INTEGER; IN box: LineBox; VAR w: INTEGER);
		VAR i: INTEGER; form: SET;
	BEGIN
		IF box.adj & (pos >= start + box.adjOff) THEN
			form := box.ruler.style.attr.opts * adjustMask;
			IF (form = blocked) & (rd.string[0] = " ") THEN
				INC(w, box.adjW DIV box.spaces)
			ELSIF (form # blocked) & (rd.string[0] = tab) THEN
				INC(w, box.adjW)	(* is this correct ??? *)
			END
		END;
		i := rd.tabIndex;	(* rd.string[0] = tab  =>  i >= 0 *)
		IF (rd.string[0] = tab) & (i < box.ruler.style.attr.tabs.len) THEN
			w := box.tabW[i]
		END
	END AdjustWidth;

	PROCEDURE (rd: StdReader) SplitWidth (w: INTEGER): INTEGER;
	BEGIN
		IF (rd.string[1] = 0X) & (rd.view = NIL) THEN
			RETURN (w + 1) DIV 2
		ELSE RETURN w
		END
	END SplitWidth;


	(* Worder *)

	PROCEDURE SetWorder (VAR w: Worder; s: StdSetter; pos: INTEGER; OUT start: INTEGER);
		CONST wordCutoff = LEN(s.rd.string);
	BEGIN
		start := s.ThisSequence(pos);
		IF pos - start >= wordCutoff THEN
			start := pos; WHILE pos - start < wordCutoff DO start := s.PreviousLine(start) END
		END;
		s.GetLine(start, w.box); w.next := start + w.box.len;
		s.rd.Set(s.r, s.text, w.box.left, start, w.box.ruler, w.box.rpos, s.vw, s.hideMarks);
		w.i := 0; s.rd.string[0] := 0X
	END SetWorder;

	PROCEDURE StepWorder (VAR w: Worder; s: StdSetter; VAR part: BOOLEAN);
		VAR rd: Reader;
	BEGIN
		rd := s.rd;
		IF rd.string[w.i] = 0X THEN
			IF rd.pos < w.next THEN
				rd.Read; w.i := 0
			ELSE
				IF ~w.box.eot THEN
					s.GetLine(w.next, w.box);
					s.rd.Set(s.r, s.text, w.box.left, w.next, w.box.ruler, w.box.rpos, s.vw, s.hideMarks);
					rd.Read; w.i := 0;
					INC(w.next, w.box.len)
				ELSE
					rd.string[0] := 0X
				END
			END
		END;
		IF rd.string[0] = 0X THEN	(* end of text *)
			part := TRUE
		ELSIF rd.string[1] = 0X THEN	(* special character *)
			part := wordPart IN rd.setterOpts; INC(w.i)
		ELSE	(* gathered sString *)
			part := WordPart(rd.string[w.i], rd.string[w.i + 1]); INC(w.i)
		END
	END StepWorder;


	(* StdSetter *)

	PROCEDURE (s: StdSetter) CopyFrom (source: Stores.Store);
	BEGIN
		s.CopyFrom^(source);
		WITH source: StdSetter DO
			s.ruler := source.ruler; s.rpos := source.rpos; s.key := source.key;
			s.rd := NIL; s.r := NIL
		END
	END CopyFrom;

	PROCEDURE (s: StdSetter) Externalize (VAR wr: Stores.Writer);
	BEGIN
		s.Externalize^(wr);
		wr.WriteVersion(maxStdVersion)
	END Externalize;

	PROCEDURE (s: StdSetter) Internalize (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		s.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxStdVersion, thisVersion);
		IF rd.cancelled THEN RETURN END;
		s.text := NIL; s.defRuler := NIL; s.ruler := NIL; s.rd := NIL; s.r := NIL
	END Internalize;


	PROCEDURE (s: StdSetter) ConnectTo (text: TextModels.Model;
		defRuler: TextRulers.Ruler; vw: INTEGER; hideMarks: BOOLEAN
	);
	BEGIN
		s.ConnectTo^(text, defRuler, vw, hideMarks);
		ClearCache(s.key);
		IF text # NIL THEN
			s.ruler := defRuler; s.rpos := -1; s.key := nextKey; INC(nextKey)
		ELSE
			s.ruler := NIL
		END
	END ConnectTo;


	PROCEDURE (s: StdSetter) ThisPage (pageH: INTEGER; pageNo: INTEGER): INTEGER;
	(* pre: connected, 0 <= pageNo *)
	(* post: (result = -1) & (pageNo >= maxPageNo) OR (result = pageStart(pageNo)) *)
		VAR start, prev: INTEGER;
	BEGIN
		ASSERT(s.text # NIL, 20); ASSERT(pageNo >= 0, 21);
		start := 0;
		WHILE pageNo > 0 DO
			prev := start; DEC(pageNo); start := s.NextPage(pageH, start);
			IF start = prev THEN start := -1; pageNo := 0 END
		END;
		RETURN start
	END ThisPage;

	PROCEDURE (s: StdSetter) NextPage (pageH: INTEGER; start: INTEGER): INTEGER;
	(* pre: connected, ThisPage(pageH, x) = start *)
	(* post: (result = s.text.Length()) OR result = next-pageStart(start) *)
		CONST
			noBreakInside = TextRulers.noBreakInside;
			pageBreak = TextRulers.pageBreak;
			parJoin = TextRulers.parJoin;
			regular = 0; protectInside = 1; joinFirst = 2; joinNext = 3; confirmSpace = 4;	(* state *)
		VAR
			box: LineBox; ra: TextRulers.Attributes;
			h, asc, dsc, backup, pos, state: INTEGER; isRuler: BOOLEAN;

		PROCEDURE FetchNextLine;
		BEGIN
			s.GetLine(pos, box);
			IF box.len > 0 THEN
				ra := box.ruler.style.attr; isRuler := box.rpos = pos;
				asc := box.asc + s.GridOffset(dsc, box); dsc := box.dsc; h := asc + dsc
			END
		END FetchNextLine;

		PROCEDURE HandleRuler;
			CONST norm = 0; nbi = 1; pj = 2;
			VAR strength: INTEGER;
		BEGIN
			IF isRuler & (pos > start) & ~(pageBreak IN ra.opts) THEN
				IF parJoin IN ra.opts THEN strength := pj
				ELSIF noBreakInside IN ra.opts THEN strength := nbi
				ELSE strength := norm
				END;
				CASE state OF
				| regular:
					CASE strength OF
					| norm:
					| nbi: state := protectInside; backup := pos
					| pj: state := joinFirst; backup := pos
					END
				| protectInside:
					CASE strength OF
					| norm: state := regular
					| nbi: backup := pos
					| pj: state := joinFirst; backup := pos
					END
				| joinFirst:
					CASE strength OF
					| norm: state := confirmSpace
					| nbi: state := protectInside
					| pj: state := joinNext
					END
				| joinNext:
					CASE strength OF
					| norm: state := confirmSpace
					| nbi: state := protectInside
					| pj:
					END
				| confirmSpace:
					CASE strength OF
					| norm: state := regular
					| nbi: state := protectInside; backup := pos
					| pj: state := joinFirst; backup := pos
					END
				END
			END
		END HandleRuler;

		PROCEDURE IsEmptyLine (): BOOLEAN;
		BEGIN
			RETURN (box.right = box.left)  OR  s.hideMarks & isRuler & ~(pageBreak IN ra.opts)
		END IsEmptyLine;

	BEGIN
		ASSERT(s.text # NIL, 20);
		ASSERT(0 <= start, 21); ASSERT(start <= s.text.Length(), 22);
		pos := start; dsc := -1;
		FetchNextLine;
		IF box.len > 0 THEN
			state := regular;
			REPEAT	(* at least one line per page *)
				HandleRuler; DEC(pageH, h); INC(pos, box.len);
				IF (state = confirmSpace) & ~IsEmptyLine() THEN state := regular END;
				FetchNextLine
			UNTIL (box.len = 0)  OR  (pageH - h < 0)  OR  isRuler & (pageBreak IN ra.opts);
			IF ~isRuler OR ~(pageBreak IN ra.opts) THEN
				WHILE (box.len > 0) & IsEmptyLine() DO	(* skip empty lines at top of page *)
					HandleRuler; INC(pos, box.len); FetchNextLine
				END
			END;
			HandleRuler;
			IF (state # regular) & ~(isRuler & (pageBreak IN ra.opts) OR (box.len = 0)) THEN pos := backup END
		END;
		RETURN pos
	END NextPage;


	PROCEDURE (s: StdSetter) NextSequence (start: INTEGER): INTEGER;
	(* pre: connected, ThisSequence(start) = start *)
	(* post: (result = start) & last-line(start) OR (ThisSequence(t, result - 1) = start) *)
		VAR rd: TextModels.Reader; ch: CHAR;
	BEGIN
		ASSERT(s.text # NIL, 20);
		s.r := s.text.NewReader(s.r); rd := s.r; rd.SetPos(start);
		REPEAT rd.ReadChar(ch) UNTIL rd.eot OR (ch = line) OR (ch = para);
		IF rd.eot THEN RETURN start ELSE RETURN rd.Pos() END
	END NextSequence;

	PROCEDURE (s: StdSetter) ThisSequence (pos: INTEGER): INTEGER;
	(* pre: connected, 0 <= pos <= t.Length() *)
	(* post: (result = 0) OR (char(result - 1) IN {line, para}) *)
		VAR rd: TextModels.Reader; start, limit: INTEGER; ch: CHAR;
	BEGIN
		ASSERT(s.text # NIL, 20); ASSERT(0 <= pos, 21); ASSERT(pos <= s.text.Length(), 22);
		IF pos = 0 THEN
			RETURN 0
		ELSE
			start := CachedSeqStart(s.key, pos);
			IF start < 0 THEN
				s.r := s.text.NewReader(s.r); rd := s.r; rd.SetPos(pos);
				limit := paraShutoff;
				REPEAT rd.ReadPrevChar(ch); DEC(limit)
				UNTIL rd.eot OR (ch = line) OR (ch = para) OR (limit = 0);
				IF rd.eot THEN start := 0 ELSE start := rd.Pos() + 1 END;
				AddSeqStartToCache(s.key, pos, start)
			END;
			RETURN start
		END
	END ThisSequence;

	PROCEDURE (s: StdSetter) PreviousSequence (start: INTEGER): INTEGER;
	(* pre: connected, ThisSequence(t, start) = start *) 
	(* post: (result = 0) & (start = 0) OR (result = ThisSequence(t, start - 1)) *)
	BEGIN
		IF start <= 1 THEN RETURN 0 ELSE RETURN s.ThisSequence(start - 1) END
	END PreviousSequence;


	PROCEDURE (s: StdSetter) ThisLine (pos: INTEGER): INTEGER;
	(* pre: connected *)
		VAR start, next: INTEGER;
	BEGIN
		next := s.ThisSequence(pos);
		REPEAT start := next; next := s.NextLine(start) UNTIL (next > pos) OR (next = start);
		RETURN start
	END ThisLine;

	PROCEDURE (s: StdSetter) NextLine (start: INTEGER): INTEGER;
	(* pre: connected, ThisLine(start) = start *)
	(* post: (result = 0) & (start = 0) OR
				(result = start) & last-line(start) OR
				(ThisLine(result - 1) = start) *)
		VAR box: LineBox; len: INTEGER; i: INTEGER; eot: BOOLEAN;
	BEGIN
		i := CacheIndex(s.key, start);
		IF i >= 0 THEN
			len := boxCache[i].line.len; eot := boxCache[i].line.eot
		ELSE
			s.GetLine(start, box); len := box.len; eot := box.eot
		END;
		IF ~eot THEN RETURN start + len ELSE RETURN start END
	END NextLine;

	PROCEDURE (s: StdSetter) PreviousLine (start: INTEGER): INTEGER;
	(* pre: connected, ThisLine(start) = start *)
	(* post: (result = 0) & (start = 0) OR (result = ThisLine(start - 1)) *)
	BEGIN
		IF start <= 1 THEN start := 0 ELSE start := s.ThisLine(start - 1) END;
		RETURN start
	END PreviousLine;


	PROCEDURE (s: StdSetter) GetWord (pos: INTEGER; OUT beg, end: INTEGER);
	(* pre: connected, 0 <= pos <= s.text.Length() *)
	(* post: beg <= pos <= end *)
		CONST wordCutoff = LEN(s.rd.string);
		VAR w: Worder; part: BOOLEAN;
	BEGIN
		ASSERT(s.text # NIL, 20); ASSERT(0 <= pos, 21); ASSERT(pos <= s.text.Length(), 22);
		SetWorder(w, s, pos, beg); end := beg;
		REPEAT
			StepWorder(w, s, part); INC(end);
			IF ~part THEN beg := end END
		UNTIL end >= pos;
		DEC(end);
		REPEAT
			StepWorder(w, s, part); INC(end)
		UNTIL ~part OR (s.rd.string[0] = 0X) OR (end - beg > wordCutoff)
	END GetWord;

	PROCEDURE (s: StdSetter) GetLine (start: INTEGER; OUT box: LineBox);
		VAR rd: Reader; ra: TextRulers.Attributes; brk: LineBox;
			d, off, right, w: INTEGER; i, tabsN: INTEGER; form: SET; adj: BOOLEAN; ch: CHAR;

		PROCEDURE TrueW (VAR b: LineBox; w: INTEGER): INTEGER;
			VAR i: INTEGER; type: SET;
		BEGIN
			i := rd.tabIndex;
			IF (0 <= i ) & (i < TextRulers.maxTabs) & (rd.string[0] # tab) THEN
				type := rd.tabType * {TextRulers.centerTab, TextRulers.rightTab};
				IF type = {TextRulers.centerTab} THEN
					DEC(w, b.tabW[i] - MAX(minTabWidth, b.tabW[i] - w DIV 2))
				ELSIF type = {TextRulers.rightTab} THEN
					DEC(w, b.tabW[i] - MAX(minTabWidth, b.tabW[i] - w))
				END
			END;
			RETURN w
		END TrueW;

		PROCEDURE Enclose (VAR b: LineBox; w: INTEGER);
			VAR off, i, d: INTEGER; type: SET;
		BEGIN
			b.len := rd.pos - start; INC(b.right, w);
			off := rd.attr.offset; i := rd.tabIndex;
			IF rd.h - rd.dsc + off > b.asc THEN b.asc := rd.h - rd.dsc + off END;
			IF rd.dsc - off > b.dsc THEN b.dsc := rd.dsc - off END;
			IF rd.view # NIL THEN b.views := TRUE END;
			IF (0 <= i ) & (i < TextRulers.maxTabs) THEN
				IF rd.string[0] = tab THEN
					b.tabW[i] := w
				ELSE
					type := rd.tabType * {TextRulers.centerTab, TextRulers.rightTab};
					IF type = {TextRulers.centerTab} THEN
						d := b.tabW[i] - MAX(minTabWidth, b.tabW[i] - w DIV 2);
						DEC(b.tabW[i], d); DEC(b.right, d)
					ELSIF type = {TextRulers.rightTab} THEN
						d := b.tabW[i] - MAX(minTabWidth, b.tabW[i] - w);
						DEC(b.tabW[i], d); DEC(b.right, d)
					END
				END
			END
		END Enclose;

	BEGIN
		ASSERT(s.text # NIL, 20); ASSERT(0 <= start, 21); ASSERT(start <= s.text.Length(), 22);
		i := CacheIndex(s.key, start);
		IF i >= 0 THEN
			GetFromCache(s, i, box)
		ELSE
			TextRulers.GetValidRuler(s.text, start, s.rpos, s.ruler, s.rpos);
			IF s.rpos > start THEN s.ruler := s.defRuler; s.rpos := -1 END;
			box.ruler := s.ruler; box.rpos := s.rpos;
			ra := s.ruler.style.attr; tabsN := ra.tabs.len; right := Right(ra, s.vw);
			s.r := s.text.NewReader(s.r);
			IF start = 0 THEN s.r.SetPos(start); ch := para
			ELSE s.r.SetPos(start - 1); s.r.ReadChar(ch)
			END;
			s.r.Read;

(*
			IF s.r.char = para THEN box.rbox := ~s.hideMarks; box.bop := s.hideMarks; box.left := 0
			ELSIF ch = para THEN box.rbox := FALSE; box.bop := TRUE; box.left := ra.first
			ELSE box.rbox := FALSE; box.bop := FALSE; box.left := ra.left
			END;
*)
			IF s.r.char = para THEN box.rbox := TRUE; box.bop := FALSE; box.left := 0
			ELSIF ch = para THEN box.rbox := FALSE; box.bop := TRUE; box.left := ra.first
			ELSE box.rbox := FALSE; box.bop := FALSE; box.left := ra.left
			END;
(**)
			box.views := FALSE;
			box.asc := 0; box.dsc := 0; box.right := box.left;
			box.len := 0; box.adjOff := 0; box.spaces := 0;
			brk.right := 0;
	
			s.rd := s.NewReader(s.rd); rd := s.rd;
			rd.Set(s.r, s.text, box.left, start, box.ruler, box.rpos, s.vw, s.hideMarks);
			rd.Read;
			WHILE ~rd.eot & (box.right + (*rd.w*) TrueW(box, rd.w) <= right)
			& ~(lineBreak IN rd.setterOpts) DO
				IF ~(wordJoin IN rd.setterOpts) & (box.right + rd.endW <= right) THEN
					(*brk := box;*)
					brk.len := box.len; brk.ruler := box.ruler; brk.rpos := box.rpos;
					brk.left := box.left; brk.right := box.right; brk.asc := box.asc; brk.dsc := box.dsc;
					brk.rbox := box.rbox; brk.bop := box.bop; brk.adj := box.adj; brk.eot := box.eot;
					brk.views := box.views; brk.skipOff := box.skipOff; brk.adjOff := box.adjOff;
					brk.spaces := box.spaces; brk.adjW := box.adjW;
					i := 0; WHILE i < tabsN DO brk.tabW[i] := box.tabW[i]; INC(i) END;
					(*---*)
					Enclose(brk, rd.endW);
					brk.eot := rd.r.eot	(* rd.r.eot one ahead of rd.eot *)
				END;
				box.adjOff := rd.adjStart - start; box.spaces := rd.spaces;
				Enclose(box, rd.w);
				rd.x := box.right; rd.Read
			END;
			IF (lineBreak IN rd.setterOpts) (* & ~box.rbox *) THEN Enclose(box, 0) END;
			box.eot := rd.eot; adj := FALSE; box.skipOff := box.len;
			IF box.right + rd.w > right THEN	(* rd.w > 0 => ~rd.eot & ~(lineBreak IN setterOpts) *)
				IF ~(wordJoin IN rd.setterOpts) & (box.right + rd.endW <= right) THEN
					IF rd.string[0] = " " THEN DEC(box.spaces) END;
					Enclose(box, rd.endW);
					adj := TRUE
				ELSIF brk.right > 0 THEN
					(*box := brk;*)
					box.len := brk.len; box.ruler := brk.ruler; box.rpos := brk.rpos;
					box.left := brk.left; box.right := brk.right; box.asc := brk.asc; box.dsc := brk.dsc;
					box.rbox := brk.rbox; box.bop := brk.bop; box.adj := brk.adj; box.eot := brk.eot;
					box.views := brk.views; box.skipOff := brk.skipOff; box.adjOff := brk.adjOff;
					box.spaces := brk.spaces; box.adjW := brk.adjW;
					i := 0; WHILE i < tabsN DO box.tabW[i] := brk.tabW[i]; INC(i) END;
					(*---*)
					box.skipOff := box.len - 1; adj := TRUE
				ELSIF box.right = box.left THEN
					Enclose(box, rd.w)	(* force at least one per line *)
				END
			ELSIF (box.right = box.left) & box.eot THEN
				box.asc := ra.asc; box.dsc := ra.dsc	(* force empty line to ruler's default height *)
			END;
	
			box.adj := FALSE;
			d := right - box.right;
			IF d > 0 THEN
				form := ra.opts * adjustMask;
				IF form = blocked THEN
					IF adj & (box.spaces > 0) THEN
						box.right := right; box.adj := TRUE; box.adjW := d
					END
				ELSIF form = rightFlush THEN
					IF box.adjOff > 0 THEN
						box.adjW := d; box.adj := TRUE
					ELSE
						INC(box.left, d)
					END;
					box.right := right
				ELSIF form = centered THEN
					IF box.adjOff > 0 THEN
						box.adjW := d DIV 2; box.adj := TRUE
					ELSE
						INC(box.left, d DIV 2)
					END;
					INC(box.right, d DIV 2)
				END
			END;

			AddToCache(s.key, start, box)
		END;

		ASSERT(box.eot OR (box.len > 0), 100)
	END GetLine;


	PROCEDURE (s: StdSetter) GetBox (start, end, maxW, maxH: INTEGER; OUT w, h: INTEGER);
		VAR box: LineBox; asc, dsc: INTEGER;
	BEGIN
		ASSERT(s.text # NIL, 20);
		ASSERT(0 <= start, 21);
		ASSERT(start <= end, 22);
		ASSERT(end <= s.text.Length(), 23);
		w := 0; h := 0; dsc := -1;
		IF maxW <= Views.undefined THEN maxW := MAX(INTEGER) END;
		IF maxH <= Views.undefined THEN maxH := MAX(INTEGER) END;
		WHILE (start < end) & (h < maxH) DO
			s.GetLine(start, box);
			IF box.rbox THEN w := MAX(w, Right(box.ruler.style.attr, s.vw))
			ELSE w := MAX(w, box.right)
			END;
			asc := box.asc + s.GridOffset(dsc, box); dsc := box.dsc;
			INC(start, box.len); INC(h, asc + dsc)
		END;
		w := MIN(w, maxW); h := MIN(h, maxH)
	END GetBox;


	PROCEDURE (s: StdSetter) NewReader (old: Reader): Reader;
	(* pre: connected *)
		VAR rd: StdReader;
	BEGIN
		ASSERT(s.text # NIL, 20);
		IF (old # NIL) & (old IS StdReader) THEN RETURN old
		ELSE NEW(rd); RETURN rd
		END
	END NewReader;


	PROCEDURE (s: StdSetter) GridOffset (dsc: INTEGER; IN box: LineBox): INTEGER;
		VAR ra: TextRulers.Attributes; h, h0: INTEGER;
		(* minimal possible line spacing h0, minimal legal line spacing h *)
	BEGIN
		IF ~box.rbox THEN
			ra := box.ruler.style.attr;
			IF dsc < 0 THEN
RETURN 0	(* no longer try to correct first line's grid position -- should be done when printing... *)
(*
				h0 := box.asc; h := ra.asc;
				IF h < h0 THEN	(* override legal spacing if to small *)
					h := h - (h - h0) DIV ra.grid * ra.grid	(* adjust to next larger grid line *)
				END;
				RETURN h - h0
*)
			ELSE
				h0 := box.asc + dsc; h := ra.asc + ra.dsc;
				IF h < h0 THEN h := h0 END;	(* override legal spacing if to small *)
				RETURN - (-h) DIV ra.grid * ra.grid - h0	(* adjust to next larger grid line *)
			END
		ELSE
			RETURN 0
		END
	END GridOffset;


	(* StdDirectory *)

	PROCEDURE (d: StdDirectory) New (): Setter;
		VAR s: StdSetter;
	BEGIN
		NEW(s); s.text := NIL; RETURN s
	END New;


	(** miscellaneous **)

	PROCEDURE Init;
		VAR d: StdDirectory;
	BEGIN
		InitCache;
		NEW(d); dir := d; stdDir := d
	END Init;

	PROCEDURE SetDir* (d: Directory);
	BEGIN
		ASSERT(d # NIL, 20); dir := d
	END SetDir;

BEGIN
	Init
END TextSetters.
