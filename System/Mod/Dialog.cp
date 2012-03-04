MODULE Dialog;
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

	IMPORT SYSTEM, Kernel, Files;

	CONST
		pressed* = 1; released* = 2; changed* = 3; included* = 5; excluded* = 6; set* = 7;	(** notify ops **)
		ok* = 1; yes* = 2; no* = 3; cancel* = 4;	(** GetOK forms & results **)
		persistent* = TRUE; nonPersistent* = FALSE;	(** constants for SetLanguage **)

		stringLen = 256;
		bufLen = 252;

		rsrcDir = "Rsrc";
		stringFile = "Strings";
		TAB = 09X; CR = 0DX;
		update = 2;	(* notify options *)
		listUpdate = 3;
		guardCheck = 4;

		windows32s* = 11;
		windows95* = 12;
		windowsNT3* = 13;
		windowsNT4* = 14;
		windows2000* = 15;
		windows98* = 16;
		windowsXP* = 17;
		windowsVista* = 18;
		macOS* = 21;
		macOSX* = 22;
		linux* = 30;
		tru64* = 40;

		firstPos* = 0;
		lastPos* = -1;

	TYPE
		String* = ARRAY stringLen OF CHAR;

		Buf = POINTER TO RECORD
			next: Buf;
			s: ARRAY bufLen OF CHAR
		END;

		StrList = RECORD
			len, max: INTEGER;	(* number of items, max number of items *)
			strings: Buf;	(* string buffer list. strings[0] = 0X -> uninitialized items appear as empty *)
			end: INTEGER;	(* next free position in string buffer list *)
			scnt: INTEGER;	(* number of strings in list, including unused entries *)
			items: POINTER TO ARRAY OF INTEGER	(* indices into string buffer list *)
		END;

		List* = RECORD
			index*: INTEGER;	(** val IN [0, n-1] **)
			len-: INTEGER;
			l: StrList
		END;

		Combo* = RECORD
			item*: String;
			len-: INTEGER;
			l: StrList
		END;

		Selection* = RECORD
			len-: INTEGER;
			sel: POINTER TO ARRAY OF SET;
			l: StrList
		END;

		Currency* = RECORD	(* number = val * 10^-scale *)
			val*: LONGINT;
			scale*: INTEGER
		END;

		Color* = RECORD
			val*: INTEGER
		END;

		TreeNode* = POINTER TO LIMITED RECORD
			nofChildren: INTEGER;
			name: String;
			parent, next, prev, firstChild: TreeNode;
			viewAsFolder, expanded: BOOLEAN;
			data: ANYPTR;
			tree: INTEGER
		END;

		Tree* = RECORD
			nofRoots, nofNodes: INTEGER;
			firstRoot, selected: TreeNode
		END;

		(** command procedure types**)

		Par* = RECORD	(** parameter for guard procedures **)
			disabled*: BOOLEAN;	(** OUT, preset to FALSE **)
			checked*: BOOLEAN;	(** OUT, preset to default **)
			undef*: BOOLEAN;	(** OUT, preset to default **)
			readOnly*: BOOLEAN;	(** OUT, preset to default **)
			label*: String	(** OUT, preset to "" **)
		END;

		GuardProc* = PROCEDURE (VAR par: Par);
		NotifierProc* = PROCEDURE (op, from, to: INTEGER);

		StringPtr = POINTER TO ARRAY [untagged] OF CHAR;
		StringTab = POINTER TO RECORD
			next: StringTab;
			name: Files.Name;
			key: POINTER TO ARRAY OF StringPtr;
			str: POINTER TO ARRAY OF StringPtr;
			data: POINTER TO ARRAY OF CHAR
		END;

		LangNotifier* = POINTER TO ABSTRACT RECORD next: LangNotifier END;
		Language* = ARRAY 3 OF CHAR;

		LangTrapCleaner = POINTER TO RECORD (Kernel.TrapCleaner) END;

		GetHook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;
		ShowHook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;
		CallHook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;
		NotifyHook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;
		LanguageHook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;

	VAR
		metricSystem*: BOOLEAN;
		showsStatus*: BOOLEAN;
		platform*: INTEGER;
		commandLinePars*: String;
		version*: INTEGER;
		appName*: ARRAY 32 OF CHAR;
		language-: Language;
		user*: ARRAY 32 OF CHAR;
		caretPeriod*: INTEGER;
		thickCaret*: BOOLEAN;

		tabList: StringTab;
		langNotifiers: LangNotifier;
		currentNotifier: LangNotifier;

		gethook: GetHook;
		showHook: ShowHook;
		callHook: CallHook;
		notifyHook: NotifyHook;
		languageHook: LanguageHook;

	PROCEDURE (h: GetHook) GetOK* (IN str, p0, p1, p2: ARRAY OF CHAR; form: SET;
															OUT res: INTEGER), NEW, ABSTRACT;
	PROCEDURE (h: GetHook) GetColor* (in: INTEGER; OUT out: INTEGER;
																OUT set: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (h: GetHook) GetIntSpec* (IN defType: Files.Type; VAR loc: Files.Locator;
														OUT name: Files.Name), NEW, ABSTRACT;
	PROCEDURE (h: GetHook) GetExtSpec* (IN defName: Files.Name; IN defType: Files.Type;
																VAR loc: Files.Locator; OUT name: Files.Name), NEW, ABSTRACT;

	PROCEDURE SetGetHook*(h: GetHook);
	BEGIN
		gethook := h
	END SetGetHook;

	PROCEDURE (h: ShowHook) ShowParamMsg* (IN str, p0, p1, p2: ARRAY OF CHAR), NEW, ABSTRACT;
	PROCEDURE (h: ShowHook) ShowParamStatus* (IN str, p0, p1, p2: ARRAY OF CHAR), NEW, ABSTRACT;

	PROCEDURE SetShowHook* (h: ShowHook);
	BEGIN
		showHook := h
	END SetShowHook;

	PROCEDURE (h: CallHook) Call* (IN proc, errorMsg: ARRAY OF CHAR; VAR res: INTEGER), NEW, ABSTRACT;

	PROCEDURE SetCallHook* (h: CallHook);
	BEGIN
		callHook := h
	END SetCallHook;

	PROCEDURE (h: NotifyHook) Notify* (id0, id1: INTEGER; opts: SET), NEW, ABSTRACT;

	PROCEDURE SetNotifyHook* (h: NotifyHook);
	BEGIN
		notifyHook := h
	END SetNotifyHook;

	PROCEDURE (h: LanguageHook) SetLanguage* (lang: Language; persistent: BOOLEAN;
																				OUT ok: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (h: LanguageHook) GetPersistentLanguage* (OUT lang: Language), NEW, ABSTRACT;

	PROCEDURE SetLanguageHook* (h: LanguageHook);
	BEGIN
		languageHook := h
	END SetLanguageHook;

	PROCEDURE ReadStringFile (subsys: Files.Name; f: Files.File; VAR tab: StringTab);
		VAR i, j, h, n, s, x, len, next, down, end: INTEGER; in, in1: Files.Reader;
			ch: CHAR; b: BYTE; p, q: StringPtr;
			
		PROCEDURE ReadInt (OUT x: INTEGER);
			VAR b: BYTE;
		BEGIN
			in.ReadByte(b); x := b MOD 256;
			in.ReadByte(b); x := x + (b MOD 256) * 100H;
			in.ReadByte(b); x := x + (b MOD 256) * 10000H;
			in.ReadByte(b); x := x + b * 1000000H
		END ReadInt;
		
		PROCEDURE ReadHead (OUT next, down, end: INTEGER);
			VAR b, t: BYTE; n: INTEGER;
		BEGIN
			in.ReadByte(b);
			REPEAT
				in.ReadByte(t);
				IF t = -14 THEN ReadInt(n)
				ELSE
					REPEAT in.ReadByte(b) UNTIL b = 0
				END
			UNTIL t # -15;
			ReadInt(n);
			ReadInt(next); next := next + in.Pos();
			ReadInt(down); down := down + in.Pos();
			ReadInt(end); end := end + in.Pos()
		END ReadHead;
	
	BEGIN
		tab := NIL;
		IF f # NIL THEN	(* read text file *)
			in := f.NewReader(NIL); in1 :=  f.NewReader(NIL);
			IF (in # NIL) & (in1 # NIL) THEN
				in.SetPos(8); ReadHead(next, down, end);	(* document view *)
				in.SetPos(down); ReadHead(next, down, end);	(* document model *)
				in.SetPos(down); ReadHead(next, down, end);	(* text view *)
				in.SetPos(down); ReadHead(next, down, end);	(* text model *)
				in.ReadByte(b); in.ReadByte(b); in.ReadByte(b);	(* versions *)
				in.ReadByte(b); in.ReadByte(b); in.ReadByte(b);
				ReadInt(x); in1.SetPos(in.Pos() + x);	(* text offset *)
				next := down;
				NEW(tab); tab.name := subsys$;
				NEW(tab.data, f.Length());
				n := 0; i := 0; s := 0; in.ReadByte(b);
				WHILE b # -1 DO
					IF next = in.Pos() THEN ReadHead(next, down, end); in.SetPos(end) END;	(* skip attributes *)
					ReadInt(len);
					IF len > 0 THEN	(* shortchar run *)
						WHILE len > 0 DO
							in1.ReadByte(b); ch := CHR(b MOD 256);
							IF ch >= " " THEN
								IF s = 0 THEN j := i; s := 1 END;	(* start of left part *)
								tab.data[j] := ch; INC(j)
							ELSIF (s = 1) & (ch = TAB) THEN
								tab.data[j] := 0X; INC(j);
								s := 2	(* start of right part *)
							ELSIF (s = 2) & (ch = CR) THEN
								tab.data[j] := 0X; INC(j);
								INC(n); i := j; s := 0	(* end of line *)
							ELSE
								s := 0	(* reset *)
							END;
							DEC(len)
						END
					ELSIF len < 0 THEN		(* longchar run *)
						WHILE len < 0 DO
							in1.ReadByte(b); x := b MOD 256; in1.ReadByte(b); ch := CHR(x + 256 * (b + 128));
							IF s = 0 THEN j := i; s := 1 END;	(* start of left part *)
							tab.data[j] := ch; INC(j);
							INC(len, 2)
						END
					ELSE	(* view *)
						ReadInt(x); ReadInt(x); in1.ReadByte(b);	(* ignore *)
					END;
					IF next = in.Pos() THEN ReadHead(next, down, end); in.SetPos(end) END;	(* skip view data *)
					in.ReadByte(b);
				END;
				IF n > 0 THEN
					NEW(tab.key, n); NEW(tab.str, n); i := 0; j := 0;
					WHILE j < n DO
						tab.key[j] := SYSTEM.VAL(StringPtr, SYSTEM.ADR(tab.data[i]));
						WHILE tab.data[i] >= " " DO INC(i) END;
						INC(i);
						tab.str[j] := SYSTEM.VAL(StringPtr, SYSTEM.ADR(tab.data[i]));
						WHILE tab.data[i] >= " " DO INC(i) END;
						INC(i); INC(j)
					END;
					(* sort keys (shellsort) *)
					h := 1; REPEAT h := h*3 + 1 UNTIL h > n;
					REPEAT h := h DIV 3; i := h;
						WHILE i < n DO p := tab.key[i]; q := tab.str[i]; j := i;
							WHILE (j >= h) & (tab.key[j-h]^ > p^) DO
								tab.key[j] := tab.key[j-h]; tab.str[j] := tab.str[j-h]; j := j-h
							END;
							tab.key[j] := p; tab.str[j] := q; INC(i)
						END
					UNTIL h = 1
				END
			END
		END
	END ReadStringFile;

	PROCEDURE MergeTabs (VAR master, extra: StringTab): StringTab;
		VAR tab: StringTab; nofKeys, datalength, di, mi, ei, ml, el, ti, i: INTEGER;
	BEGIN
		IF (extra = NIL) OR (extra.key = NIL) THEN RETURN master END;
		IF (master = NIL) OR (master.key = NIL)  THEN RETURN extra END;
		ml := LEN(master.key); el := LEN(extra.key);
		mi := 0; ei := 0; datalength := 0; nofKeys := 0;
		(* find out how big the resulting table will be *)
		WHILE (mi < ml) OR (ei < el) DO
			INC(nofKeys);
			IF (mi < ml) & (ei < el) & (master.key[mi]$ = extra.key[ei]$) THEN
				datalength := datalength + LEN(master.key[mi]$) + LEN(master.str[mi]$) + 2; INC(mi); INC(ei)
			ELSIF (ei < el) & ((mi >= ml) OR (master.key[mi]$ > extra.key[ei]$)) THEN
				datalength := datalength + LEN(extra.key[ei]$) + LEN(extra.str[ei]$) + 2; INC(ei)
			ELSE
				datalength := datalength + LEN(master.key[mi]$) + LEN(master.str[mi]$) + 2; INC(mi)
			END
		END;
		NEW(tab); tab.name := master.name;
		NEW(tab.key, nofKeys); NEW(tab.str, nofKeys); NEW(tab.data, datalength);
		mi := 0; ei := 0; di := 0; ti := 0;
		(* do the merge *)
		WHILE (mi < ml) OR (ei < el) DO
			IF  (mi < ml) & (ei < el) & (master.key[mi]$ = extra.key[ei]$)  THEN
				i := 0; tab.key[ti] := SYSTEM.VAL(StringPtr, SYSTEM.ADR(tab.data[di]));
				WHILE  master.key[mi][i] # 0X DO tab.data[di] := master.key[mi][i]; INC(di); INC(i) END;
				tab.data[di] :=0X; INC(di); i := 0;
				tab.str[ti] := SYSTEM.VAL(StringPtr, SYSTEM.ADR(tab.data[di]));
				WHILE master.str[mi][i] # 0X DO tab.data[di] := master.str[mi][i]; INC(di); INC(i) END;
				 tab.data[di] :=0X; INC(di);
				INC(mi); INC(ei)
			ELSIF (ei < el) & ((mi >= ml) OR (master.key[mi]$ > extra.key[ei]$)) THEN
				i := 0; tab.key[ti] := SYSTEM.VAL(StringPtr, SYSTEM.ADR(tab.data[di]));
				WHILE extra.key[ei][i] # 0X DO tab.data[di] := extra.key[ei][i]; INC(di); INC(i) END;
				tab.data[di] :=0X; INC(di); i := 0;
				tab.str[ti] := SYSTEM.VAL(StringPtr, SYSTEM.ADR(tab.data[di]));
				WHILE  extra.str[ei][i] # 0X DO tab.data[di] := extra.str[ei][i]; INC(di); INC(i) END;
				 tab.data[di] :=0X; INC(di);
				INC(ei)
			ELSE
				i := 0; tab.key[ti] := SYSTEM.VAL(StringPtr, SYSTEM.ADR(tab.data[di]));
				WHILE master.key[mi][i] # 0X DO  tab.data[di] := master.key[mi][i]; INC(di); INC(i) END;
				tab.data[di] :=0X; INC(di); i := 0;
				tab.str[ti] := SYSTEM.VAL(StringPtr, SYSTEM.ADR(tab.data[di]));
				WHILE master.str[mi][i] # 0X DO  tab.data[di] := master.str[mi][i]; INC(di); INC(i) END;
				 tab.data[di] :=0X; INC(di);
				INC(mi)
			END;
			INC(ti)
		END;
		RETURN tab
	END MergeTabs;

	PROCEDURE LoadStringTab (subsys: Files.Name; VAR tab: StringTab);
		VAR loc: Files.Locator; f: Files.File; name: Files.Name; ltab: StringTab;
	BEGIN
		tab := NIL;
		name := stringFile; Kernel.MakeFileName(name, "");
		loc := Files.dir.This(subsys); loc := loc.This(rsrcDir);
		IF loc # NIL THEN
			 f := Files.dir.Old(loc, name, Files.shared);
			ReadStringFile(subsys, f, tab);
			IF language # "" THEN
				loc := loc.This(language);
				IF loc # NIL THEN
					 f := Files.dir.Old(loc, name, Files.shared);
					ReadStringFile(subsys, f, ltab);
					tab := MergeTabs(ltab, tab)
				END
			END;
			IF tab # NIL THEN tab.next := tabList; tabList := tab END
		END
	END LoadStringTab;

	PROCEDURE SearchString (VAR in: ARRAY OF CHAR; OUT out: ARRAY OF CHAR);
		VAR i, j, k, len: INTEGER; ch: CHAR; subsys: Files.Name; tab: StringTab;
	BEGIN
		out := "";
		IF in[0] = "#" THEN
			i := 0; ch := in[1];
			WHILE (ch # 0X) (* & (ch # ".") *) & (ch # ":") DO subsys[i] := ch; INC(i); ch := in[i + 1] END;
			subsys[i] := 0X;
			IF ch # 0X THEN
				INC(i, 2); ch := in[i]; j := 0;
				WHILE (ch # 0X) DO in[j] := ch; INC(i); INC(j); ch := in[i] END;
				in[j] := 0X
			ELSE
				RETURN
			END;
			tab := tabList;
			WHILE (tab # NIL) & (tab.name # subsys) DO tab := tab.next END;
			IF tab = NIL THEN LoadStringTab(subsys, tab) END;
			IF tab # NIL THEN
				i := 0;
				IF tab.key = NIL THEN j := 0 ELSE j := LEN(tab.key^) END;
				WHILE i < j DO	(* binary search *)
					k := (i + j) DIV 2;
					IF tab.key[k]^ < in THEN i := k + 1 ELSE j := k END
				END;
				IF (tab.key # NIL) & (j < LEN(tab.key^)) & (tab.key[j]^ = in) THEN
					k := 0; len := LEN(out)-1;
					WHILE (k < len) & (tab.str[j][k] # 0X) DO
						out[k] := tab.str[j][k]; INC(k)
					END;
					out[k] := 0X
				END
			END
		END
	END SearchString;


	PROCEDURE Init (VAR l: StrList);
	BEGIN
		l.len := 0; l.max := 0; l.end := 0; l.scnt := 0
	END Init;

	PROCEDURE Compact (VAR l: StrList);
		VAR i, j, k: INTEGER; ibuf, jbuf: Buf; ch: CHAR;
	BEGIN
		i := 1; ibuf := l.strings; j := 1; jbuf := l.strings;
		WHILE j < l.end DO
			(* find index entry k pointing to position j *)
			k := 0; WHILE (k < l.len) & (l.items[k] # j) DO INC(k) END;
			IF k < l.len THEN	(* copy string *)
				l.items[k] := i;
				REPEAT
					ch := jbuf.s[j MOD bufLen]; INC(j);
					IF j MOD bufLen = 0 THEN jbuf := jbuf.next END;
					ibuf.s[i MOD bufLen] := ch; INC(i);
					IF i MOD bufLen = 0 THEN ibuf := ibuf.next END
				UNTIL ch = 0X
			ELSE (* skip next string *)
				REPEAT
					ch := jbuf.s[j MOD bufLen]; INC(j);
					IF j MOD bufLen = 0 THEN jbuf := jbuf.next END
				UNTIL ch = 0X
			END
		END;
		ibuf.next := NIL;	(* release superfluous buffers *)
		l.end := i; l.scnt := l.len
	END Compact;

	PROCEDURE SetLen (VAR l: StrList; len: INTEGER);
		CONST D = 32;
		VAR i, newmax: INTEGER;
			items: POINTER TO ARRAY OF INTEGER;
	BEGIN
		IF l.items = NIL THEN Init(l) END;
		IF (l.max - D < len) & (len <= l.max) THEN
			(* we do not reallocate anything *)
		ELSE
			newmax := (len + D-1) DIV D * D;
			IF newmax > 0 THEN
				IF l.strings = NIL THEN NEW(l.strings); (* l.strings[0] := 0X; *) l.end := 1 END;
				NEW(items, newmax);
				IF len < l.len THEN i := len ELSE i := l.len END;
				WHILE i > 0 DO DEC(i); items[i] := l.items[i] END;
				l.items := items
			END;
			l.max := newmax
		END;
		l.len := len;
		IF (l.scnt > 32) & (l.scnt > 2 * l.len) THEN Compact(l) END
	END SetLen;

	PROCEDURE GetItem (VAR l: StrList; index: INTEGER; VAR item: String);
		VAR i, j, k: INTEGER; b: Buf; ch: CHAR;
	BEGIN
		IF l.items = NIL THEN Init(l) END;
		IF (index >= 0) & (index < l.len) THEN
			i := l.items[index]; j := i MOD bufLen; i := i DIV bufLen;
			b := l.strings; WHILE i # 0 DO b := b.next; DEC(i) END;
			k := 0;
			REPEAT
				ch := b.s[j]; INC(j); IF j = bufLen THEN j := 0; b := b.next END;
				item[k] := ch; INC(k)
			UNTIL ch = 0X
		ELSE
			item := ""
		END
	END GetItem;

	PROCEDURE SetItem (VAR l: StrList; index: INTEGER; item: ARRAY OF CHAR);
		VAR len, i, j, k: INTEGER; b: Buf; ch: CHAR;
	BEGIN
		IF l.items = NIL THEN Init(l) END;
		IF index >= l.len THEN SetLen(l, index + 1) END;
		IF (l.scnt > 32) & (l.scnt > 2 * l.len) THEN Compact(l) END;
		len := 0; WHILE item[len] # 0X DO INC(len) END;
		IF len >= stringLen THEN len := stringLen - 1; item[len] := 0X END;	(* clip long strings *)
		l.items[index] := l.end;
		i := l.end; j := i MOD bufLen; i := i DIV bufLen;
		b := l.strings; WHILE i # 0 DO b := b.next; DEC(i) END;
		k := 0;
		REPEAT
			ch := item[k]; INC(k); INC(l.end);
			b.s[j] := ch; INC(j); IF j = bufLen THEN j := 0; NEW(b.next); b := b.next END
		UNTIL ch = 0X;
		INC(l.scnt)
	END SetItem;

	PROCEDURE SetResources (VAR l: StrList; IN key: ARRAY OF CHAR);
		VAR i, k, j, x: INTEGER; ch: CHAR; s, a: ARRAY 16 OF CHAR; h, item: ARRAY 256 OF CHAR;
	BEGIN
		IF l.items = NIL THEN Init(l) END;
		i := 0;
		REPEAT
			x := i;
			j := 0; REPEAT a[j] := CHR(x MOD 10 + ORD("0")); x := x DIV 10; INC(j) UNTIL x = 0;
			k := 0; REPEAT DEC(j); ch := a[j]; s[k] := ch; INC(k) UNTIL j = 0;
			s[k] := 0X;
			h := key + "[" + s + "]";
			SearchString(h, item);
			IF item # "" THEN SetItem(l, i, item) END;
			INC(i)
		UNTIL item = ""
	END SetResources;


	(** List **)

	PROCEDURE (VAR l: List) SetLen* (len: INTEGER), NEW;
	BEGIN
		ASSERT(len >= 0, 20);
		SetLen(l.l, len);
		l.len := l.l.len
	END SetLen;

	PROCEDURE (VAR l: List) GetItem* (index: INTEGER; OUT item: String), NEW;
	BEGIN
		GetItem(l.l, index, item);
		l.len := l.l.len
	END GetItem;

	PROCEDURE (VAR l: List) SetItem* (index: INTEGER; IN item: ARRAY OF CHAR), NEW;
	BEGIN
		ASSERT(index >= 0, 20); ASSERT(item # "", 21);
		SetItem(l.l, index, item);
		l.len := l.l.len
	END SetItem;

	PROCEDURE (VAR l: List) SetResources* (IN key: ARRAY OF CHAR), NEW;
	BEGIN
		ASSERT(key # "", 20);
		SetResources(l.l, key);
		l.len := l.l.len
	END SetResources;


	(** Selection **)

	PROCEDURE (VAR s: Selection) SetLen* (len: INTEGER), NEW;
		VAR sel: POINTER TO ARRAY OF SET; i: INTEGER;
	BEGIN
		ASSERT(len >= 0, 20);
		SetLen(s.l, len);
		len := len + (MAX(SET) - 1) DIV MAX(SET);
		IF len  = 0 THEN s.sel := NIL
		ELSIF s.sel = NIL THEN NEW(s.sel, len)
		ELSIF LEN(s.sel^) # len THEN
			NEW(sel, len);
			IF LEN(s.sel^) < len THEN len := LEN(s.sel^) END;
			i := 0; WHILE i < len DO sel[i] := s.sel[i]; INC(i) END;
			s.sel := sel
		END;
		s.len := s.l.len
	END SetLen;

	PROCEDURE (VAR s: Selection) GetItem* (index: INTEGER; OUT item: String), NEW;
	BEGIN
		GetItem(s.l, index, item);
		s.len := s.l.len
	END GetItem;

	PROCEDURE (VAR s: Selection) SetItem* (index: INTEGER; IN item: ARRAY OF CHAR), NEW;
	BEGIN
		ASSERT(index >= 0, 20); (*ASSERT(index < s.l.len, 21);*) ASSERT(item # "", 21);
		SetItem(s.l, index, item);
		IF s.l.len > s.len THEN s.SetLen(s.l.len) END
	END SetItem;

	PROCEDURE (VAR s: Selection) SetResources* (IN key: ARRAY OF CHAR), NEW;
	BEGIN
		ASSERT(key # "", 20);
		SetResources(s.l, key);
		IF s.l.len > s.len THEN s.SetLen(s.l.len) END
	END SetResources;

	PROCEDURE (VAR s: Selection) In* (index: INTEGER): BOOLEAN, NEW;
	BEGIN
		IF s.l.items = NIL THEN Init(s.l); s.len := s.l.len END;
		IF s.sel # NIL THEN RETURN (index MOD 32) IN (s.sel[index DIV 32]) ELSE RETURN FALSE END
	END In;

	PROCEDURE (VAR s: Selection) Excl* (from, to: INTEGER), NEW;
	BEGIN
		IF s.l.items = NIL THEN Init(s.l); s.len := s.l.len END;
		IF from < 0 THEN from := 0 END;
		IF to >= s.l.len THEN to := s.l.len - 1 END;
		WHILE from <= to DO EXCL(s.sel[from DIV 32], from MOD 32); INC(from) END
	END Excl;

	PROCEDURE (VAR s: Selection) Incl* (from, to: INTEGER), NEW;
	BEGIN
		IF s.l.items = NIL THEN Init(s.l); s.len := s.l.len END;
		IF from < 0 THEN from := 0 END;
		IF to >= s.l.len THEN to := s.l.len - 1 END;
		WHILE from <= to DO INCL(s.sel[from DIV 32], from MOD 32); INC(from) END
	END Incl;


	(** Combo **)

	PROCEDURE (VAR c: Combo) SetLen* (len: INTEGER), NEW;
	BEGIN
		ASSERT(len >= 0, 20);
		SetLen(c.l, len);
		c.len := c.l.len
	END SetLen;

	PROCEDURE (VAR c: Combo) GetItem* (index: INTEGER; OUT item: String), NEW;
	BEGIN
		GetItem(c.l, index, item);
		c.len := c.l.len
	END GetItem;

	PROCEDURE (VAR c: Combo) SetItem* (index: INTEGER; IN item: ARRAY OF CHAR), NEW;
	BEGIN
		ASSERT(index >= 0, 20); ASSERT(item # "", 21);
		SetItem(c.l, index, item);
		c.len := c.l.len
	END SetItem;

	PROCEDURE (VAR c: Combo) SetResources* (IN key: ARRAY OF CHAR), NEW;
	BEGIN
		ASSERT(key # "", 20);
		SetResources(c.l, key);
		c.len := c.l.len
	END SetResources;


	(* Tree and TreeNode *)

	PROCEDURE (tn: TreeNode) SetName* (name: String), NEW;
	BEGIN
		tn.name := name
	END SetName;

	PROCEDURE (tn: TreeNode) GetName* (OUT name: String), NEW;
	BEGIN
		name := tn.name
	END GetName;

	PROCEDURE (tn: TreeNode) SetData* (data: ANYPTR), NEW;
	BEGIN
		tn.data := data
	END SetData;

	PROCEDURE (tn: TreeNode) Data* (): ANYPTR, NEW;
	BEGIN
		RETURN tn.data
	END Data;

	PROCEDURE (tn: TreeNode) NofChildren* (): INTEGER, NEW;
	BEGIN
		RETURN tn.nofChildren
	END NofChildren;

	PROCEDURE (tn: TreeNode) SetExpansion* (expanded: BOOLEAN), NEW;
	BEGIN
		tn.expanded := expanded
	END SetExpansion;

	PROCEDURE (tn: TreeNode) IsExpanded* (): BOOLEAN, NEW;
	BEGIN
		RETURN tn.expanded
	END IsExpanded;

	PROCEDURE (tn: TreeNode) IsFolder* (): BOOLEAN, NEW;
	BEGIN
		IF (~tn.viewAsFolder) & (tn.firstChild = NIL) THEN
			RETURN FALSE
		ELSE
			RETURN TRUE
		END
	END IsFolder;

	PROCEDURE (tn: TreeNode) ViewAsFolder* (isFolder: BOOLEAN), NEW;
	BEGIN
		tn.viewAsFolder := isFolder
	END ViewAsFolder;

	PROCEDURE (VAR t: Tree) NofNodes* (): INTEGER, NEW;
	BEGIN
		IF t.firstRoot = NIL THEN
			RETURN 0
		ELSE
			RETURN MAX(0, t.nofNodes)
		END
	END NofNodes;

	PROCEDURE (VAR t: Tree) NofRoots* (): INTEGER, NEW;
	BEGIN
		IF t.firstRoot = NIL THEN
			RETURN 0
		ELSE
			RETURN MAX(0, t.nofRoots)
		END
	END NofRoots;

	PROCEDURE (VAR t: Tree) Parent* (node: TreeNode): TreeNode, NEW;
	BEGIN
		ASSERT(node # NIL, 20); ASSERT(node.tree = SYSTEM.ADR(t), 21);
		RETURN node.parent
	END Parent;

	PROCEDURE (VAR t: Tree) Next* (node: TreeNode): TreeNode, NEW;
	BEGIN
		ASSERT(node # NIL, 20); ASSERT(node.tree = SYSTEM.ADR(t), 21);
		RETURN node.next
	END Next;

	PROCEDURE (VAR t: Tree) Prev* (node: TreeNode): TreeNode, NEW;
	BEGIN
		ASSERT(node # NIL, 20); ASSERT(node.tree = SYSTEM.ADR(t), 21);
		RETURN node.prev
	END Prev;

	PROCEDURE (VAR t: Tree) Child* (node: TreeNode; pos: INTEGER): TreeNode, NEW;
		VAR cur: TreeNode;
	BEGIN
		ASSERT(pos >= lastPos, 20); ASSERT((node = NIL) OR (node.tree = SYSTEM.ADR(t)), 21);
		IF node = NIL THEN cur := t.firstRoot
		ELSE cur := node.firstChild END;
		IF pos = lastPos THEN
			WHILE (cur # NIL) & (cur.next # NIL) DO cur := cur.next END
		ELSE
			WHILE (cur # NIL) & (pos > 0) DO cur := cur.next; DEC(pos) END
		END;
		RETURN cur
	END Child;

	PROCEDURE (VAR t: Tree) Selected* (): TreeNode, NEW;
	BEGIN
		RETURN t.selected
	END Selected;

	PROCEDURE (VAR t: Tree) Select* (node: TreeNode), NEW;
	BEGIN
		ASSERT((node = NIL) OR (node.tree = SYSTEM.ADR(t)), 20);
		IF (node # NIL) OR (t.nofRoots = 0) THEN
			t.selected := node
		ELSE
			t.selected := t.Child(NIL, 0)
		END
	END Select;

	PROCEDURE Include (IN t: Tree; node: TreeNode);
		VAR c: TreeNode;
	BEGIN
		ASSERT(node # NIL, 20); ASSERT(node.tree = 0, 100);
		node.tree := SYSTEM.ADR(t);
		c := node.firstChild;
		WHILE c # NIL DO Include(t, c); c := c.next END
	END Include;

	PROCEDURE (VAR t: Tree) InsertAt (parent: TreeNode; pos: INTEGER; node: TreeNode), NEW;
		VAR
			cur, prev: TreeNode;
	BEGIN
		ASSERT(node # NIL, 20); ASSERT(pos >= lastPos, 21);
		ASSERT((parent = NIL) OR (parent.tree = SYSTEM.ADR(t)), 22); ASSERT(node.tree = 0, 23);
		Include(t, node);
		IF parent = NIL THEN	(* Add new root *)
			IF (t.firstRoot = NIL) OR (pos = 0) THEN
				node.next := t.firstRoot; node.prev := NIL;
				IF t.firstRoot # NIL THEN t.firstRoot.prev := node END;
				t.firstRoot := node
			ELSE
				cur := t.firstRoot;
				IF pos = lastPos THEN pos := t.nofRoots END;
				WHILE (cur # NIL) & (pos > 0) DO
					prev := cur; cur := t.Next(cur); DEC(pos)
				END;
				IF cur = NIL THEN
					prev.next := node; node.prev := prev
				ELSE
					node.next := cur; node.prev := cur.prev; cur.prev := node; prev.next := node
				END
			END;
			INC(t.nofRoots)
		ELSE	(* Add child *)
			IF pos = lastPos THEN pos := parent.nofChildren END;
			IF (parent.firstChild = NIL) OR (pos = 0) THEN
				IF parent.firstChild # NIL THEN parent.firstChild.prev := node END;
				node.prev := NIL; node.next := parent.firstChild; parent.firstChild := node
			ELSE
				cur := parent.firstChild;
				WHILE (cur # NIL) & (pos > 0) DO
					prev := cur; cur := t.Next(cur); DEC(pos)
				END;
				IF cur = NIL THEN
					prev.next := node; node.prev := prev
				ELSE
					node.next := cur; node.prev := cur.prev; cur.prev := node; prev.next := node
				END
			END;
			INC(parent.nofChildren)
		END;
		node.parent := parent;
		INC(t.nofNodes)
	END InsertAt;

	PROCEDURE (VAR t: Tree) NewChild* (parent: TreeNode; pos: INTEGER; name: String): TreeNode, NEW;
		VAR
			new: TreeNode;
	BEGIN
		NEW(new); new.tree := 0;
		new.SetName(name); new.expanded := FALSE; new.nofChildren := 0;
		new.viewAsFolder := FALSE;
		t.InsertAt(parent, pos, new);
		RETURN new
	END NewChild;

	PROCEDURE (VAR t: Tree) CountChildren (node: TreeNode): INTEGER, NEW;
		VAR tot, nofc, i: INTEGER;
	BEGIN
		tot := 0;
		IF node # NIL THEN
			nofc := node.nofChildren; tot := nofc;
			FOR i := 0 TO nofc -1 DO
				tot := tot + t.CountChildren(t.Child(node, i))
			END
		END;
		RETURN tot
	END CountChildren;

	PROCEDURE Exclude (IN t: Tree; node: TreeNode);
		VAR c: TreeNode;
	BEGIN
		ASSERT(node # NIL, 20); ASSERT(node.tree = SYSTEM.ADR(t), 100);
		IF t.Selected() = node THEN t.Select(NIL) END;
		node.tree := 0;
		c := node.firstChild;
		WHILE c # NIL DO Exclude(t, c); c := c.next END
	END Exclude;

	PROCEDURE (VAR t: Tree) Delete* (node: TreeNode): INTEGER, NEW;
		VAR
			ndel: INTEGER;
	BEGIN
		ASSERT(node # NIL, 20); ASSERT(node.tree = SYSTEM.ADR(t), 21);
		ndel := t.CountChildren(node);
		IF node.parent = NIL THEN	(* root node *)
			IF node.prev = NIL THEN
				IF node.next # NIL THEN
					t.firstRoot := node.next;
					node.next.prev := NIL
				ELSE
					t.firstRoot := NIL
				END
			ELSE
				node.prev.next := node.next;
				IF node.next # NIL THEN node.next.prev := node.prev END
			END;
			DEC(t.nofRoots)
		ELSE
			IF node.prev = NIL THEN
				IF node.next # NIL THEN
					node.parent.firstChild := node.next;
					node.next.prev := NIL
				ELSE
					node.parent.firstChild := NIL
				END
			ELSE
				node.prev.next := node.next;
				IF node.next # NIL THEN node.next.prev := node.prev END
			END;
			DEC(node.parent.nofChildren)
		END;
		node.parent := NIL; node.next := NIL; node.prev := NIL;
		Exclude(t, node);
		ndel := ndel + 1;
		t.nofNodes := t.nofNodes - ndel;
		RETURN ndel
	END Delete;

	PROCEDURE (VAR t: Tree) Move* (node, parent: TreeNode; pos: INTEGER), NEW;
		VAR ndel, nofn: INTEGER; s: TreeNode;
	BEGIN
		ASSERT(node # NIL, 20);  ASSERT(pos >= lastPos, 21);
		ASSERT(node.tree = SYSTEM.ADR(t), 22);
		nofn := t.NofNodes();
		s := t.Selected();
		ndel := t.Delete(node); t.InsertAt(parent, pos, node);
		t.nofNodes := t.nofNodes + ndel - 1;
		IF (s # NIL) & (t.Selected() # s) THEN t.Select(s) END;
		ASSERT(nofn = t.NofNodes(), 60)
	END Move;

	PROCEDURE (VAR t: Tree) DeleteAll*, NEW;
	BEGIN
		t.nofRoots := 0; t.nofNodes := 0; t.firstRoot := NIL; t.selected := NIL
	END DeleteAll;


	PROCEDURE Notify* (id0, id1: INTEGER; opts: SET);
	BEGIN
		ASSERT(notifyHook # NIL, 100);
		notifyHook.Notify(id0, id1, opts)
	END Notify;

	PROCEDURE Update* (IN x: ANYREC);
		VAR type: Kernel.Type; adr, size: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		type := Kernel.TypeOf(x);
		size := type.size;
		IF size = 0 THEN size := 1 END;
		Notify(adr, adr + size, {update, guardCheck})
	END Update;

	PROCEDURE UpdateBool* (VAR x: BOOLEAN);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(BOOLEAN), {update, guardCheck})
	END UpdateBool;

	PROCEDURE UpdateSChar* (VAR x: SHORTCHAR);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(SHORTCHAR), {update, guardCheck})
	END UpdateSChar;

	PROCEDURE UpdateChar* (VAR x: CHAR);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(CHAR), {update, guardCheck})
	END UpdateChar;

	PROCEDURE UpdateByte* (VAR x: BYTE);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(BYTE), {update, guardCheck})
	END UpdateByte;

	PROCEDURE UpdateSInt* (VAR x: SHORTINT);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(SHORTINT), {update, guardCheck})
	END UpdateSInt;

	PROCEDURE UpdateInt* (VAR x: INTEGER);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(INTEGER), {update, guardCheck})
	END UpdateInt;

	PROCEDURE UpdateLInt* (VAR x: LONGINT);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(LONGINT), {update, guardCheck})
	END UpdateLInt;

	PROCEDURE UpdateSReal* (VAR x: SHORTREAL);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(SHORTREAL), {update, guardCheck})
	END UpdateSReal;

	PROCEDURE UpdateReal* (VAR x: REAL);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(REAL), {update, guardCheck})
	END UpdateReal;

	PROCEDURE UpdateSet* (VAR x: SET);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + SIZE(SET), {update, guardCheck})
	END UpdateSet;

	PROCEDURE UpdateSString* (IN x: ARRAY OF SHORTCHAR);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + LEN(x) * SIZE(SHORTCHAR), {update, guardCheck})
	END UpdateSString;

	PROCEDURE UpdateString* (IN x: ARRAY OF CHAR);
		VAR adr: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		Notify(adr, adr + LEN(x) * SIZE(CHAR), {update, guardCheck})
	END UpdateString;

	PROCEDURE UpdateList* (IN x: ANYREC);
		VAR type: Kernel.Type; adr, size: INTEGER;
	BEGIN
		adr := SYSTEM.ADR(x);
		type := Kernel.TypeOf(x);
		size := type.size;
		IF size = 0 THEN size := 1 END;
		Notify(adr, adr + size, {listUpdate, guardCheck})
	END UpdateList;


	PROCEDURE GetOK* (IN str, p0, p1, p2: ARRAY OF CHAR; form: SET; OUT res: INTEGER);
	BEGIN
		ASSERT(((yes IN form) = (no IN form)) & ((yes IN form) # (ok IN form)), 20);
		ASSERT(gethook # NIL, 100);
		gethook.GetOK(str, p0, p1, p2, form, res)
	END GetOK;

	PROCEDURE GetIntSpec* (defType: Files.Type; VAR loc: Files.Locator; OUT name: Files.Name);
	BEGIN
		ASSERT(gethook # NIL, 100);
		gethook.GetIntSpec(defType, loc, name)
	END GetIntSpec;

	PROCEDURE GetExtSpec* (defName: Files.Name; defType: Files.Type; VAR loc: Files.Locator;
												OUT name: Files.Name);
	BEGIN
		ASSERT(gethook # NIL, 100);
		gethook.GetExtSpec(defName, defType, loc, name)
	END GetExtSpec;

	PROCEDURE GetColor* (in: INTEGER; OUT out: INTEGER; OUT set: BOOLEAN);
	BEGIN
		ASSERT(gethook # NIL, 100);
		gethook.GetColor(in, out, set)
	END GetColor;


	PROCEDURE Subst (in: ARRAY OF CHAR; IN p0, p1, p2: ARRAY OF CHAR; VAR out: ARRAY OF CHAR);
		VAR len, i, j, k: INTEGER; ch, c: CHAR;
	BEGIN
		i := 0; ch := in[i]; j := 0; len := LEN(out) - 1;
		WHILE (ch # 0X) & (j < len) DO
			IF ch = "^" THEN
				INC(i); ch := in[i];
				IF ch = "0" THEN
					k := 0; c := p0[0];
					WHILE (c # 0X) & (j < len) DO out[j] := c; INC(j); INC(k); c := p0[k] END;
					INC(i); ch := in[i]
				ELSIF ch = "1" THEN
					k := 0; c := p1[0];
					WHILE (c # 0X) & (j < len) DO out[j] := c; INC(j); INC(k); c := p1[k] END;
					INC(i); ch := in[i]
				ELSIF ch = "2" THEN
					k := 0; c := p2[0];
					WHILE (c # 0X) & (j < len) DO out[j] := c; INC(j); INC(k); c := p2[k] END;
					INC(i); ch := in[i]
				ELSE out[j] := "^"; INC(j)
				END
			ELSE out[j] := ch; INC(j); INC(i); ch := in[i]
			END
		END;
		out[j] := 0X
	END Subst;

	PROCEDURE FlushMappings*;
	BEGIN
		tabList := NIL
	END FlushMappings;

	PROCEDURE MapParamString* (in, p0, p1, p2: ARRAY OF CHAR; OUT out: ARRAY OF CHAR);
	(* use in as key in string table file, and return corresponding string in out.
		If the resource lookup fails, return in in out *)
	BEGIN
		SearchString(in, out);
		IF out # "" THEN Subst(out, p0, p1, p2, out)
		ELSE Subst(in, p0, p1, p2, out)
		END
	END MapParamString;

	PROCEDURE MapString* (in: ARRAY OF CHAR; OUT out: ARRAY OF CHAR);
		VAR len, k: INTEGER;
	BEGIN
		SearchString(in, out);
		IF out = "" THEN
			k := 0; len := LEN(out)-1;
			WHILE (k < len) & (in[k] # 0X) DO out[k] := in[k]; INC(k) END;
			out[k] := 0X
		END
	END MapString;

	PROCEDURE ShowMsg* (IN str: ARRAY OF CHAR);
	BEGIN
		ASSERT(str # "", 20);
		ASSERT(showHook # NIL, 100);
		showHook.ShowParamMsg(str, "", "", "")
	END ShowMsg;

	PROCEDURE ShowParamMsg* (IN str, p0, p1, p2: ARRAY OF CHAR);
	BEGIN
		ASSERT(str # "", 20);
		ASSERT(showHook # NIL, 100);
		showHook.ShowParamMsg(str,p0, p1, p2)
	END ShowParamMsg;

	PROCEDURE ShowStatus* (IN str: ARRAY OF CHAR);
	BEGIN
		ASSERT(showHook # NIL, 100);
		showHook.ShowParamStatus(str, "", "", "")
	END ShowStatus;

	PROCEDURE ShowParamStatus* (IN str, p0, p1, p2: ARRAY OF CHAR);
	BEGIN
		ASSERT(showHook # NIL, 100);
		showHook.ShowParamStatus(str, p0, p1, p2)
	END ShowParamStatus;


	PROCEDURE Call* (IN proc, errorMsg: ARRAY OF CHAR; OUT res: INTEGER);
	BEGIN
		ASSERT(callHook # NIL, 100);
		callHook.Call(proc, errorMsg, res)
	END Call;

	PROCEDURE Beep*;
	BEGIN
		Kernel.Beep
	END Beep;

	PROCEDURE (n: LangNotifier) Notify-(), NEW, ABSTRACT;

	PROCEDURE RegisterLangNotifier* (notifier: LangNotifier);
		VAR nl: LangNotifier;
	BEGIN
		ASSERT(notifier # NIL, 20);
		nl := langNotifiers;
		WHILE (nl # NIL) & (nl # notifier) DO nl := nl.next END;
		IF nl = NIL THEN
			notifier.next := langNotifiers; langNotifiers := notifier
		END
	END RegisterLangNotifier;

	PROCEDURE RemoveLangNotifier* (notifier: LangNotifier);
		VAR nl, prev: LangNotifier;
	BEGIN
		ASSERT(notifier # NIL, 20);
		nl := langNotifiers; prev := NIL;
		WHILE (nl # NIL) & (nl # notifier) DO prev := nl; nl := nl.next END;
		IF nl # NIL THEN
			IF prev = NIL THEN  langNotifiers := langNotifiers.next ELSE prev.next := nl.next END;
			nl.next := NIL
		END
	END RemoveLangNotifier;

	PROCEDURE Exec (a, b, c: INTEGER);
		VAR nl: LangNotifier;
	BEGIN
		nl := currentNotifier; currentNotifier := NIL;
		nl.Notify;
		currentNotifier := nl
	END Exec;

	PROCEDURE SetLanguage* (lang: Language; persistent: BOOLEAN);
		VAR nl, t: LangNotifier; ok: BOOLEAN;
	BEGIN
		ASSERT((lang = "") OR (LEN(lang$) = 2), 20);
		ASSERT(languageHook # NIL, 100);
		IF lang # language THEN
			languageHook.SetLanguage(lang, persistent, ok);
			IF ok THEN
				language := lang; FlushMappings;
				nl := langNotifiers;
				WHILE nl # NIL DO
					currentNotifier := nl;
					Kernel.Try(Exec, 0, 0, 0);
					IF currentNotifier = NIL THEN
						t := nl; nl := nl.next; RemoveLangNotifier(t)	(* Notifier trapped, remove it *)
					ELSE
						nl := nl.next
					END
				END
			END;
			currentNotifier := NIL
		END
	END SetLanguage;

	PROCEDURE ResetLanguage*;
		VAR lang: Language;
	BEGIN
		ASSERT(languageHook # NIL, 100);
		languageHook.GetPersistentLanguage(lang);
		SetLanguage(lang, nonPersistent)
	END ResetLanguage;

BEGIN
	appName := "BlackBox"; showsStatus := FALSE; caretPeriod := 500; thickCaret := FALSE; user := ""
END Dialog.
