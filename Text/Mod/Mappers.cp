MODULE TextMappers;
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

	IMPORT Strings, Views, Dialog, TextModels;

	CONST
		(** Scanner.opts **)
		returnCtrlChars* = 1;
		returnQualIdents* = 2; returnViews* = 3;
		interpretBools* = 4; interpretSets* = 5;
		maskViews* = 6;

		(** Scanner.type **)
		char* = 1; string* = 3; int* = 4; real* = 5;
		bool* = 6;	(** iff interpretBools IN opts **)
		set* = 7;	(** iff interpretSets IN opts **)
		view* = 8;	(** iff returnViews IN opts **)
		tab* = 9; line* = 10; para* = 11;	(** iff returnCtrlChars IN opts **)
		lint* = 16;
		eot* = 30;
		invalid* = 31;	(** last Scan hit lexically invalid sequence **)

		(** Formatter.WriteIntForm base **)
		charCode* = Strings.charCode; decimal* = Strings.decimal; hexadecimal* = Strings.hexadecimal;

		(** Formatter.WriteIntForm showBase **)
		hideBase* = Strings.hideBase; showBase* = Strings.showBase;

		VIEW = TextModels.viewcode;
		TAB = TextModels.tab; LINE = TextModels.line; PARA = TextModels.para;

		acceptUnderscores = TRUE;

	TYPE
		String* = ARRAY 256 OF CHAR;

		Scanner* = RECORD
			opts-: SET;
			rider-: TextModels.Reader;	(** prefetch state for single character look-ahead **)

			type*: INTEGER;
			start*, lines*, paras*: INTEGER;	(** update by Skip **)

			char*: CHAR;	(** valid iff type = char **)
			int*: INTEGER;	(** valid iff type = int **)
			base*: INTEGER;	(** valid iff type IN {int, lint} **)
			lint*: LONGINT;	(** valid iff type IN {int, lint} **)
			real*: REAL;	(** valid iff type = real **)
			bool*: BOOLEAN;	(** valid iff type = bool **)
			set*: SET;	(** valid iff type = set **)
			len*: INTEGER;	(** valid iff type IN {string, int, lint} **)
			string*: String;	(** valid iff type IN {string, int, lint, bool, char} **)
			view*: Views.View; w*, h*: INTEGER	(** valid iff type = view **)
		END;

		Formatter* = RECORD
			rider-: TextModels.Writer
		END;


	(** Scanner **)

	PROCEDURE ^ (VAR s: Scanner) SetPos* (pos: INTEGER), NEW;
	PROCEDURE ^ (VAR s: Scanner) SetOpts* (opts: SET), NEW;
	PROCEDURE ^ (VAR s: Scanner) Skip* (OUT ch: CHAR), NEW;
	PROCEDURE ^ (VAR s: Scanner) Scan*, NEW;


	PROCEDURE Get (VAR s: Scanner; OUT ch: CHAR);
	BEGIN
		s.rider.ReadChar(ch)
	END Get;

	PROCEDURE Real (VAR s: Scanner);
		VAR res: INTEGER; ch: CHAR;
	BEGIN
		s.type := real;
		s.string[s.len] := "."; INC(s.len); Get(s, ch);
		WHILE ("0" <= ch) & (ch <= "9") & (s.len < LEN(s.string) - 1) DO
			s.string[s.len] := ch; INC(s.len); Get(s, ch)
		END;
		IF (ch = "E") OR (ch = "D") THEN
			s.string[s.len] := ch; INC(s.len); Get(s, ch);
			IF (ch = "-") OR (ch = "+") THEN s.string[s.len] := ch; INC(s.len); Get(s,ch) END;
			WHILE ("0" <= ch) & (ch <= "9") & (s.len < LEN(s.string) - 1) DO
				s.string[s.len] := ch; INC(s.len); Get(s, ch)
			END
		END;
		s.string[s.len] := 0X;
		Strings.StringToReal(s.string, s.real, res);
		IF res # 0 THEN s.type := invalid END
	END Real;

	PROCEDURE Integer (VAR s: Scanner);
		VAR n, k, res: INTEGER; ch: CHAR; hex: BOOLEAN;
	BEGIN
		s.type := int; hex := FALSE; ch := s.rider.char;
		IF ch = "%" THEN
			s.string[s.len] := "%"; INC(s.len); Get(s, ch); n:= 0;
			IF ("0" <= ch) & (ch <= "9") THEN
				k := ORD(ch) - ORD("0");
				REPEAT
					n := 10*n + k; s.string[s.len] := ch; INC(s.len);
					Get(s, ch); k := ORD(ch) - ORD("0")
				UNTIL (ch < "0") OR (ch > "9") OR (n > (MAX(INTEGER) - k) DIV 10) OR (s.len = LEN(s.string));
				IF ("0" <= ch) & (ch <= "9") THEN s.type := invalid ELSE s.base := n END
			ELSE s.type := invalid
			END
		ELSIF (ch = "H") OR (ch = "X") THEN
			hex := TRUE; s.base := 16;
			s.string[s.len] := ch; INC(s.len); Get(s, ch)
		ELSE
			s.base := 10
		END;
		s.string[s.len] := 0X;
		IF s.type # invalid THEN
			Strings.StringToInt(s.string, s.int, res);
			IF res = 0 THEN s.type := int;
				IF hex THEN (* Strings.StringToLInt(s.string, s.lint, res); ASSERT(res = 0, 100); *)
					IF s.int < 0 THEN s.lint := s.int + (LONG(MAX(INTEGER)) + 1) * 2
					ELSE s.lint := s.int
					END
				ELSE s.lint := s.int
				END
			ELSIF res = 1 THEN (* INTEGER overflow *)
				Strings.StringToLInt(s.string, s.lint, res);
				IF res = 0 THEN s.type := lint ELSE s.type := invalid END
			ELSE	(* syntax error *)
				s.type := invalid
			END
		END
	END Integer;

	PROCEDURE Number (VAR s: Scanner; neg: BOOLEAN);
		VAR m: INTEGER; ch: CHAR;
	BEGIN
		s.len := 0; m := 0; ch := s.rider.char;
		IF neg THEN s.string[s.len] := "-"; INC(s.len) END;
		REPEAT
			IF (m > 0) OR (ch # "0") THEN	(* ignore leading zeroes *)
				s.string[s.len] := ch; INC(s.len); INC(m)
			END;
			Get(s, ch)
		UNTIL (ch < "0") OR (ch > "9") & (ch < "A") OR (ch > "F")
			OR (s.len = LEN(s.string) - 1) OR s.rider.eot;
		IF (s.len = 0) OR (s.len = 1) & (s.string[0] = "-") THEN	(* compensate for ignoring leading zeroes *)
			s.string[s.len] := "0"; INC(s.len)
		END;
		s.string[s.len] := 0X;
		IF ch = "." THEN Real(s) ELSE Integer(s) END
	END Number;

	PROCEDURE Cardinal (VAR s: Scanner; OUT n: INTEGER);
		VAR k: INTEGER; ch: CHAR;
	BEGIN
		n := 0; s.Skip(ch);
		IF ("0" <= ch) & (ch <= "9") THEN
			k := ORD(ch) - ORD("0");
			REPEAT
				n := n * 10 + k;
				Get(s, ch); k := ORD(ch) - ORD("0")
			UNTIL (ch < "0") OR (ch > "9") OR (n > (MAX(INTEGER) - k) DIV 10);
			IF ("0" <= ch) & (ch <= "9") THEN s.type := invalid END
		ELSE s.type := invalid
		END
	END Cardinal;

	PROCEDURE Set (VAR s: Scanner);
		VAR n, m: INTEGER; ch: CHAR;
	BEGIN
		s.type := set; Get(s, ch); s.Skip(ch); s.set := {};
		WHILE ("0" <= ch) & (ch <= "9") & (s.type = set) DO
			Cardinal(s, n); s.Skip(ch);
			IF (MIN(SET) <= n) & (n <= MAX(SET)) THEN
				INCL(s.set, n);
				IF ch = "," THEN
					Get(s, ch); s.Skip(ch)
				ELSIF ch = "." THEN
					Get(s, ch);
					IF ch = "." THEN
						Get(s, ch); s.Skip(ch); Cardinal(s, m); s.Skip(ch);
						IF ch = "," THEN Get(s, ch); s.Skip(ch) END;
						IF (n <= m) & (m <= MAX(SET)) THEN
							WHILE m > n DO INCL(s.set, m); DEC(m) END
						ELSE s.type := invalid
						END
					ELSE s.type := invalid
					END
				END
			ELSE s.type := invalid
			END
		END;
		IF s.type = set THEN
			s.Skip(ch);
			IF ch = "}" THEN Get(s, ch) ELSE s.type := invalid END
		END
	END Set;

	PROCEDURE Boolean (VAR s: Scanner);
		VAR ch: CHAR;
	BEGIN
		s.type := bool; Get(s, ch);
		IF (ch = "T") OR (ch = "F") THEN
			s.Scan;
			IF (s.type = string) & (s.string = "TRUE") THEN s.type := bool; s.bool := TRUE
			ELSIF (s.type = string) & (s.string = "FALSE") THEN s.type := bool; s.bool := FALSE
			ELSE s.type := invalid
			END
		ELSE s.type := invalid
		END
	END Boolean;

	PROCEDURE Name (VAR s: Scanner);
		VAR max: INTEGER; ch: CHAR;
	BEGIN
		s.type := string; s.len := 0; ch := s.rider.char; max := LEN(s.string);
		REPEAT
			s.string[s.len] := ch; INC(s.len); Get(s, ch)
		UNTIL
			~(	("0" <= ch) & (ch <= "9")
			  OR ("A" <= CAP(ch)) & (CAP(ch) <= "Z")
			  OR (0C0X <= ch) & (ch <= 0FFX) & (ch # 0D7X) & (ch # 0F7X)
			  OR acceptUnderscores & (ch = "_"))
		OR (s.len = max);
		IF (returnQualIdents IN s.opts) & (ch = ".") & (s.len < max) THEN
			REPEAT
				s.string[s.len] := ch; INC(s.len); Get(s, ch)
			UNTIL
			~(	("0" <= ch) & (ch <= "9")
			  OR ("A" <= CAP(ch)) & (CAP(ch) <= "Z")
			  OR (0C0X <= ch) & (ch <= 0FFX) & (ch # 0D7X) & (ch # 0F7X)
			  OR acceptUnderscores & (ch = "_") )
			OR (s.len = max)
		END;
		IF s.len = max THEN DEC(s.len); s.type := invalid END;	(* ident too long *)
		s.string[s.len] := 0X
	END Name;

	PROCEDURE DoubleQuotedString (VAR s: Scanner);
		VAR max, pos: INTEGER; ch: CHAR;
	BEGIN
		pos := s.rider.Pos();
		s.type := string; s.len := 0; max := LEN(s.string) - 1; Get(s, ch);
		WHILE (ch # '"') & (ch # 0X) & (s.len < max) DO
			s.string[s.len] := ch; INC(s.len);
			Get(s, ch)
		END;
		s.string[s.len] := 0X;
		IF ch = '"' THEN Get(s, ch)
		ELSE s.type := invalid; s.rider.SetPos(pos (* s.rider.Pos() - s.len - 1 *)); Get(s, ch)
		END
	END DoubleQuotedString;

	PROCEDURE SingleQuotedString (VAR s: Scanner);
		VAR max, pos: INTEGER; ch: CHAR;
	BEGIN
		pos := s.rider.Pos();
		s.type := string; s.len := 0; max := LEN(s.string) - 1; Get(s, ch);
		WHILE (ch # "'") & (ch # 0X) & (s.len < max) DO
			s.string[s.len] := ch; INC(s.len);
			Get(s, ch)
		END;
		s.string[s.len] := 0X;
		IF s.len = 1 THEN s.type := char; s.char := s.string[0] END;
		IF ch = "'" THEN Get(s, ch)
		ELSE s.type := invalid; s.rider.SetPos(pos (* s.rider.Pos() - s.len - 1 *)); Get(s, ch)
		END
	END SingleQuotedString;

	PROCEDURE Char (VAR s: Scanner);
		VAR ch: CHAR;
	BEGIN
		ch := s.rider.char;
		IF ch # 0X THEN
			s.type := char; s.char := ch; s.string[0] := ch; s.string[1] := 0X; Get(s, ch)
		ELSE s.type := invalid
		END
	END Char;

	PROCEDURE View (VAR s: Scanner);
		VAR ch: CHAR;
	BEGIN
		s.type := view; s.view := s.rider.view; s.w := s.rider.w; s.h := s.rider.h;
		IF maskViews IN s.opts THEN
			IF s.rider.char # TextModels.viewcode THEN
				s.type := char; s.char := s.rider.char; s.string[0] := s.char; s.string[1] := 0X
			END
		END;
		Get(s, ch)
	END View;


	PROCEDURE (VAR s: Scanner) ConnectTo* (text: TextModels.Model), NEW;
	BEGIN
		IF text # NIL THEN
			s.rider := text.NewReader(s.rider); s.SetPos(0); s.SetOpts({})
		ELSE
			s.rider := NIL
		END
	END ConnectTo;

	PROCEDURE (VAR s: Scanner) SetPos* (pos: INTEGER), NEW;
	BEGIN
		s.rider.SetPos(pos); s.start := pos;
		s.lines := 0; s.paras := 0; s.type := invalid
	END SetPos;

	PROCEDURE (VAR s: Scanner) SetOpts* (opts: SET), NEW;
	BEGIN
		s.opts := opts
	END SetOpts;

	PROCEDURE (VAR s: Scanner) Pos* (): INTEGER, NEW;
	BEGIN
		RETURN s.rider.Pos()
	END Pos;

	PROCEDURE (VAR s: Scanner) Skip* (OUT ch: CHAR), NEW;
		VAR c, v: BOOLEAN;
	BEGIN
		IF s.opts * {returnCtrlChars, returnViews} = {} THEN
			ch := s.rider.char;
			WHILE ((ch <= " ") OR (ch = TextModels.digitspace) OR (ch = TextModels.nbspace))
			& ~s.rider.eot DO
				IF ch = LINE THEN INC(s.lines)
				ELSIF ch = PARA THEN INC(s.paras)
				END;
				Get(s, ch)
			END
		ELSE
			c := returnCtrlChars IN s.opts;
			v := returnViews IN s.opts;
			ch := s.rider.char;
			WHILE ((ch <= " ") OR (ch = TextModels.digitspace) OR (ch = TextModels.nbspace))
			& ~s.rider.eot
			& (~c OR (ch # TAB) & (ch # LINE) & (ch # PARA))
			& (~v OR (ch # VIEW) OR (s.rider.view = NIL)) DO
				IF ch = LINE THEN INC(s.lines)
				ELSIF ch = PARA THEN INC(s.paras)
				END;
				Get(s, ch)
			END
		END;
		IF ~s.rider.eot THEN s.start := s.rider.Pos() - 1
		ELSE s.start := s.rider.Base().Length(); s.type := eot
		END
	END Skip;

	PROCEDURE (VAR s: Scanner) Scan*, NEW;
		VAR sign, neg: BOOLEAN; ch: CHAR;
	BEGIN
		s.Skip(ch);
		IF s.type # eot THEN
			neg := (ch = "-"); sign := neg OR (ch = "+");
			IF sign THEN s.char := ch; Get(s, ch) END;
			IF ("0" <= ch) & (ch <= "9") THEN Number(s, neg)
			ELSIF sign THEN s.type := char;	(* return prefetched sign w/o trailing number *)
				s.string[0] := s.char; s.string[1] := 0X
			ELSE
				CASE ch OF
				| "A" .. "Z", "a" .. "z", 0C0X .. 0D6X, 0D8X .. 0F6X, 0F8X .. 0FFX: Name(s)
				| '"': DoubleQuotedString(s)
				| "'": SingleQuotedString(s)
				| TAB: s.type := tab; Get(s, ch)
				| LINE: s.type := line; Get(s, ch)
				| PARA: s.type := para; Get(s, ch)
				| VIEW:
					IF s.rider.view # NIL THEN View(s) ELSE Char(s) END
				| "{":
					IF interpretSets IN s.opts THEN Set(s) ELSE Char(s) END
				| "$":
					IF interpretBools IN s.opts THEN Boolean(s) ELSE Char(s) END
				| "_":
					IF acceptUnderscores THEN Name(s) ELSE Char(s) END
				ELSE Char(s)
				END
			END
		END
	END Scan;


	(** scanning utilities **)

	PROCEDURE IsQualIdent* (IN s: ARRAY OF CHAR): BOOLEAN;
		VAR i: INTEGER; ch: CHAR;
	BEGIN
		ch := s[0]; i := 1;
		IF ("A" <= CAP(ch)) & (CAP(ch) <= "Z")
		OR (0C0X <= ch) & (ch <= 0FFX) & (ch # 0D0X) & (ch # 0D7X) & (ch # 0F7X) THEN
			REPEAT
				ch := s[i]; INC(i)
			UNTIL
				~(	("0" <= ch) & (ch <= "9")
				  OR ("A" <= CAP(ch)) & (CAP(ch) <= "Z")
				  OR (0C0X <= ch) & (ch <= 0FFX) & (ch # 0D0X) & (ch # 0D7X) & (ch # 0F7X)
				  OR (ch = "_") );
			IF ch = "." THEN
				INC(i);
				REPEAT
					ch := s[i]; INC(i)
				UNTIL
					~(	("0" <= ch) & (ch <= "9")
					  OR ("A" <= CAP(ch)) & (CAP(ch) <= "Z")
					  OR (0C0X <= ch) & (ch <= 0FFX) & (ch # 0D0X) & (ch # 0D7X) & (ch # 0F7X)
					  OR (ch = "_") );
				RETURN ch = 0X
			ELSE
				RETURN FALSE
			END
		ELSE
			RETURN FALSE
		END
	END IsQualIdent;

	PROCEDURE ScanQualIdent* (VAR s: Scanner; OUT x: ARRAY OF CHAR; OUT done: BOOLEAN);
		VAR mod: String; i, j, len, start: INTEGER; ch: CHAR;
	BEGIN
		done := FALSE;
		IF s.type = string THEN
			IF IsQualIdent(s.string) THEN
				IF s.len < LEN(x) THEN
					x := s.string$; done := TRUE
				END
			ELSE
				mod := s.string; len := s.len; start := s.start;
				s.Scan;
				IF (s.type = char) & (s.char = ".") THEN
					s.Scan;
					IF (s.type = string) & (len + 1 + s.len < LEN(x)) THEN
						i := 0; ch := mod[0]; WHILE ch # 0X DO x[i] := ch; INC(i); ch := mod[i] END;
						x[i] := "."; INC(i);
						j := 0; ch := s.string[0];
						WHILE ch # 0X DO x[i] := ch; INC(i); INC(j); ch := s.string[j] END;
						x[i] := 0X; done := TRUE
					END
				END;
				IF ~done THEN s.SetPos(start); s.Scan() END
			END
		END
	END ScanQualIdent;


	(** Formatter **)

	PROCEDURE ^ (VAR f: Formatter) SetPos* (pos: INTEGER), NEW;
	PROCEDURE ^ (VAR f: Formatter) WriteIntForm* (x: LONGINT;
														base, minWidth: INTEGER; fillCh: CHAR; showBase: BOOLEAN), NEW;
	PROCEDURE ^ (VAR f: Formatter) WriteRealForm* (x: REAL;
														precision, minW, expW: INTEGER; fillCh: CHAR), NEW;
	PROCEDURE ^ (VAR f: Formatter) WriteViewForm* (v: Views.View; w, h: INTEGER), NEW;


	PROCEDURE (VAR f: Formatter) ConnectTo* (text: TextModels.Model), NEW;
	BEGIN
		IF text # NIL THEN
			f.rider := text.NewWriter(f.rider); f.SetPos(text.Length())
		ELSE
			f.rider := NIL
		END
	END ConnectTo;

	PROCEDURE (VAR f: Formatter) SetPos* (pos: INTEGER), NEW;
	BEGIN
		f.rider.SetPos(pos)
	END SetPos;

	PROCEDURE (VAR f: Formatter) Pos* (): INTEGER, NEW;
	BEGIN
		RETURN f.rider.Pos()
	END Pos;


	PROCEDURE (VAR f: Formatter) WriteChar* (x: CHAR), NEW;
	BEGIN
		IF (x >= " ") & (x # 7FX) THEN
			f.rider.WriteChar(x)
		ELSE
			f.rider.WriteChar(" ");
			f.WriteIntForm(ORD(x), charCode, 3, "0", showBase);
			f.rider.WriteChar(" ")
		END
	END WriteChar;

	PROCEDURE (VAR f: Formatter) WriteInt* (x: LONGINT), NEW;
	BEGIN
		f.WriteIntForm(x, decimal, 0, TextModels.digitspace, hideBase)
	END WriteInt;

	PROCEDURE (VAR f: Formatter) WriteSString* (x: ARRAY OF SHORTCHAR), NEW;
		VAR i: INTEGER;
	BEGIN
		i := 0; WHILE x[i] # 0X DO f.WriteChar(x[i]); INC(i) END
	END WriteSString;

	PROCEDURE (VAR f: Formatter) WriteString* (x: ARRAY OF CHAR), NEW;
		VAR i: INTEGER;
	BEGIN
		i := 0; WHILE x[i] # 0X DO f.WriteChar(x[i]); INC(i) END
	END WriteString;

	PROCEDURE (VAR f: Formatter) WriteReal* (x: REAL), NEW;
		VAR m: ARRAY 256 OF CHAR;
	BEGIN
		Strings.RealToString(x, m); f.WriteString(m)
	END WriteReal;

	PROCEDURE (VAR f: Formatter) WriteBool* (x: BOOLEAN), NEW;
	BEGIN
		IF x THEN f.WriteString("$TRUE") ELSE f.WriteString("$FALSE") END
	END WriteBool;

	PROCEDURE (VAR f: Formatter) WriteSet* (x: SET), NEW;
		VAR i: INTEGER;
	BEGIN
		f.WriteChar("{"); i := MIN(SET);
		WHILE x # {} DO
			IF i IN x THEN f.WriteInt(i); EXCL(x, i);
				IF (i + 2 <= MAX(SET)) & (i+1 IN x) & (i+2 IN x) THEN f.WriteString("..");
					x := x - {i+1, i+2}; INC(i, 3);
					WHILE (i <= MAX(SET)) & (i IN x) DO EXCL(x, i); INC(i) END;
					f.WriteInt(i-1)
				END;
				IF x # {} THEN f.WriteString(", ") END
			END;
			INC(i)
		END;
		f.WriteChar("}")
	END WriteSet;

	PROCEDURE (VAR f: Formatter) WriteTab*, NEW;
	BEGIN
		f.rider.WriteChar(TAB)
	END WriteTab;

	PROCEDURE (VAR f: Formatter) WriteLn*, NEW;
	BEGIN
		f.rider.WriteChar(LINE)
	END WriteLn;

	PROCEDURE (VAR f: Formatter) WritePara*, NEW;
	BEGIN
		f.rider.WriteChar(PARA)
	END WritePara;

	PROCEDURE (VAR f: Formatter) WriteView* (v: Views.View), NEW;
	BEGIN
		f.WriteViewForm(v, Views.undefined, Views.undefined)
	END WriteView;


	PROCEDURE (VAR f: Formatter) WriteIntForm* (x: LONGINT;
		base, minWidth: INTEGER; fillCh: CHAR; showBase: BOOLEAN
	), NEW;
		VAR s: ARRAY 80 OF CHAR;
	BEGIN
		Strings.IntToStringForm(x, base, minWidth, fillCh, showBase, s);
		f.WriteString(s)
	END WriteIntForm;

	PROCEDURE (VAR f: Formatter) WriteRealForm* (x: REAL;
		precision, minW, expW: INTEGER; fillCh: CHAR
	), NEW;
		VAR s: ARRAY 256 OF CHAR;
	BEGIN
		Strings.RealToStringForm(x, precision, minW, expW, fillCh, s); f.WriteString(s)
	END WriteRealForm;


	PROCEDURE (VAR f: Formatter) WriteViewForm* (v: Views.View; w, h: INTEGER), NEW;
	BEGIN
		f.rider.WriteView(v, w, h)
	END WriteViewForm;

	PROCEDURE (VAR f: Formatter) WriteParamMsg* (msg, p0, p1, p2: ARRAY OF CHAR), NEW;
		VAR s: ARRAY 256 OF CHAR; i: INTEGER; ch: CHAR;
	BEGIN
		Dialog.MapParamString(msg, p0, p1, p2, s);
		i := 0; ch := s[0];
		WHILE ch # 0X DO
			IF ch = LINE THEN f.WriteLn
			ELSIF ch = PARA THEN f.WritePara
			ELSIF ch = TAB THEN f.WriteTab
			ELSIF ch >= " " THEN f.WriteChar(ch)
			END;
			INC(i); ch := s[i]
		END
	END WriteParamMsg;

	PROCEDURE (VAR f: Formatter) WriteMsg* (msg: ARRAY OF CHAR), NEW;
	BEGIN
		f.WriteParamMsg(msg, "", "", "")
	END WriteMsg;

END TextMappers.
