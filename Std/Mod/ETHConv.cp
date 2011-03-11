MODULE StdETHConv;
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

	IMPORT
		Fonts, Files, Stores, Ports, Views,
		TextModels, TextRulers, TextViews,
		Stamps := StdStamps, Clocks := StdClocks, StdFolds;

	CONST
		V2Tag = -4095; (* 01 F0 *)
		V4Tag = 496; (* F0 01 *)

	TYPE
		FontDesc = RECORD
			typeface: Fonts.Typeface;
			size: INTEGER;
			style: SET;
			weight: INTEGER
		END;

	VAR default: Fonts.Font;

	PROCEDURE Split (name: ARRAY OF CHAR; VAR d: FontDesc);
		VAR i: INTEGER; ch: CHAR;
	BEGIN
		i := 0; ch := name[0];
		WHILE (ch < "0") OR (ch >"9") DO
			d.typeface[i] := ch; INC(i); ch := name[i]
		END;
		d.typeface[i] := 0X;
		d.size := 0;
		WHILE ("0" <= ch) & (ch <= "9") DO
			d.size := d.size * 10 + (ORD(ch) - 30H); INC(i); ch := name[i]
		END;
		CASE ch OF
		  "b": d.style := {}; d.weight := Fonts.bold
		| "i": d.style := {Fonts.italic}; d.weight := Fonts.normal
		| "j": d.style := {Fonts.italic}; d.weight := Fonts.bold
		| "m": d.style := {}; d.weight := Fonts.bold
		ELSE d.style := {}; d.weight := Fonts.normal	(* unknown style *)
		END
	END Split;

	PROCEDURE ThisFont (name: ARRAY OF CHAR): Fonts.Font; 
		VAR d: FontDesc;
	BEGIN
		Split(name, d);
		IF d.typeface = "Syntax" THEN d.typeface := default.typeface END;
		IF d.size = 10 THEN d.size := default.size
		ELSE d.size := (d.size - 2) * Ports.point
		END;
		RETURN Fonts.dir.This(d.typeface, d.size, d.style, d.weight)
	END ThisFont;

	PROCEDURE ThisChar (ch: CHAR): CHAR;
	BEGIN
		CASE ORD(ch) OF
		  80H: ch := 0C4X | 81H: ch := 0D6X | 82H: ch := 0DCX
		| 83H: ch := 0E4X | 84H: ch := 0F6X | 85H: ch := 0FCX
		| 86H: ch := 0E2X | 87H: ch := 0EAX | 88H: ch := 0EEX | 89H: ch := 0F4X | 8AH: ch := 0FBX
		| 8BH: ch := 0E0X | 8CH: ch := 0E8X | 8DH: ch := 0ECX | 8EH: ch := 0F2X | 8FH: ch := 0F9X
		| 90H: ch := 0E9X
		| 91H: ch := 0EBX | 92H: ch := 0EFX
		| 93H: ch := 0E7X
		| 94H: ch := 0E1X
		| 95H: ch := 0F1X
		| 9BH: ch := TextModels.hyphen
		| 9FH: ch := TextModels.nbspace
		| 0ABH: ch := 0DFX
		ELSE
			ch := 0BFX	(* use inverted question mark for unknown character codes *)
		END;
		RETURN ch
	END ThisChar;
	
	PROCEDURE ^ LoadTextBlock (r: Stores.Reader; t: TextModels.Model);
	
	PROCEDURE StdFold (VAR r: Stores.Reader): Views.View;
		CONST colLeft = 0; colRight = 1; expRight = 2; expLeft = 3;
		VAR k: BYTE; state: BOOLEAN; hidden: TextModels.Model; fold: StdFolds.Fold;
	BEGIN
		r.ReadByte(k);
		CASE k MOD 4 OF
			| colLeft: state := StdFolds.collapsed
			| colRight: state := StdFolds.collapsed
			| expRight: state := StdFolds.expanded
			| expLeft: state := StdFolds.expanded
		END;
		IF (k MOD 4 IN {colLeft, expLeft}) & (k < 4) THEN
			hidden := TextModels.dir.New(); LoadTextBlock(r, hidden);
		ELSE hidden := NIL;
		END;
		fold := StdFolds.dir.New(state, "", hidden);
		RETURN fold;
	END StdFold;
	
	PROCEDURE LoadTextBlock (r: Stores.Reader; t: TextModels.Model);
		VAR r0: Stores.Reader; wr: TextModels.Writer;
			org, len: INTEGER; en, ano, i, n: BYTE; col, voff, ch: CHAR; tag: INTEGER;
			fname: ARRAY 32 OF CHAR;
			attr: ARRAY 32 OF TextModels.Attributes;
			mod, proc: ARRAY 32 OF ARRAY 32 OF CHAR;

		PROCEDURE ReadNum (VAR n: INTEGER);
			VAR s: BYTE; ch: CHAR; y: INTEGER;
		BEGIN
			s := 0; y := 0; r.ReadXChar(ch);
			WHILE ch >= 80X DO
				INC(y, ASH(ORD(ch)-128, s)); INC(s, 7); r.ReadXChar(ch)
			END;
			n := ASH((ORD(ch) + 64) MOD 128 - 64, s) + y
		END ReadNum;

		PROCEDURE ReadSet (VAR s: SET);
			VAR x: INTEGER;
		BEGIN
			ReadNum(x); s := BITS(x)
		END ReadSet;

		PROCEDURE Elem (VAR r: Stores.Reader; span: INTEGER);
			VAR v: Views.View; end, ew, eh, n, indent: INTEGER; eno, version: BYTE;
				p: TextRulers.Prop; opts: SET;
		BEGIN
			r.ReadInt(ew); r.ReadInt(eh); r.ReadByte(eno);
			IF eno > en THEN en := eno; r.ReadXString(mod[eno]); r.ReadXString(proc[eno]) END;
			end := r.Pos() + span;
			IF (mod[eno] = "ParcElems") OR (mod[eno] = "StyleElems") THEN
				r.ReadByte(version);
				NEW(p);
				p.valid := {TextRulers.first .. TextRulers.tabs};
				ReadNum(indent); ReadNum(p.left);
				p.first := p.left + indent;
				ReadNum(n); p.right := p.left + n;
				ReadNum(p.lead);
				ReadNum(p.grid);
				ReadNum(p.dsc); p.asc := p.grid - p.dsc;
				ReadSet(opts); p.opts.val := {};
				IF ~(0 IN opts) THEN p.grid := 1 END;
				IF 1 IN opts THEN INCL(p.opts.val, TextRulers.leftAdjust) END;
				IF 2 IN opts THEN INCL(p.opts.val, TextRulers.rightAdjust) END;
				IF 3 IN opts THEN INCL(p.opts.val, TextRulers.pageBreak) END;
				INCL(p.opts.val, TextRulers.rightFixed);
				p.opts.mask := {TextRulers.leftAdjust .. TextRulers.pageBreak, TextRulers.rightFixed};
				ReadNum(n); p.tabs.len := n;
				i := 0; WHILE i < p.tabs.len DO ReadNum(p.tabs.tab[i].stop); INC(i) END;
				v := TextRulers.dir.NewFromProp(p);
				wr.WriteView(v, ew, eh)
			ELSIF mod[eno] = "StampElems" THEN
				v := Stamps.New();
				wr.WriteView(v, ew, eh)
			ELSIF mod[eno] = "ClockElems" THEN
				v := Clocks.New();
				wr.WriteView(v, ew, eh)
			ELSIF mod[eno] = "FoldElems" THEN
				v := StdFold(r);
				wr.WriteView(v, ew, eh);
			END;
			r.SetPos(end)
		END Elem;

	BEGIN
		(* skip inner text tags (legacy from V2) *)
		r.ReadXInt(tag);
		IF tag # V2Tag THEN r.SetPos(r.Pos()-2) END;
		(* load text block *)
		org := r.Pos(); r.ReadInt(len); INC(org, len - 2);
		r0.ConnectTo(r.rider.Base()); r0.SetPos(org);
		wr := t.NewWriter(NIL); wr.SetPos(0);
		n := 0; en := 0; r.ReadByte(ano);
		WHILE ano # 0 DO
			IF ano > n THEN
				n := ano; r.ReadXString(fname);
				attr[n] := TextModels.NewFont(wr.attr, ThisFont(fname))
			END;
			r.ReadXChar(col); r.ReadXChar(voff); r.ReadInt(len);
			wr.SetAttr(attr[ano]);
			IF len > 0 THEN
				WHILE len # 0 DO
					r0.ReadXChar(ch);
					IF ch >= 80X THEN ch := ThisChar(ch) END;
					IF (ch >= " ") OR (ch = TextModels.tab) OR (ch = TextModels.line) THEN
						wr.WriteChar(ch)
					END;
					DEC(len)
				END
			ELSE
				Elem(r, -len); r0.ReadXChar(ch)
			END;
			r.ReadByte(ano)
		END;
		r.ReadInt(len);
		r.SetPos(r.Pos() + len);
	END LoadTextBlock;

	PROCEDURE ImportOberon* (f: Files.File): TextModels.Model;
		VAR r: Stores.Reader; t: TextModels.Model; tag: INTEGER;
	BEGIN
		r.ConnectTo(f); r.SetPos(0);
		r.ReadXInt(tag);
		IF tag = ORD("o") + 256 * ORD("B") THEN
			(* ignore file header of Oberon for Windows and DOSOberon files *)
			r.SetPos(34); r.ReadXInt(tag)
		END;
		ASSERT((tag = V2Tag) OR (tag = V4Tag), 100);
		t := TextModels.dir.New();
		LoadTextBlock(r, t);
		RETURN t;
	END ImportOberon;
	

	PROCEDURE ImportETHDoc* (f: Files.File; OUT s: Stores.Store);
		VAR t: TextModels.Model;
	BEGIN
		ASSERT(f # NIL, 20);
		t := ImportOberon(f);
		IF t # NIL THEN s := TextViews.dir.New(t) END
	END ImportETHDoc;

BEGIN
	default := Fonts.dir.Default()
END StdETHConv.
