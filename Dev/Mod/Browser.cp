MODULE DevBrowser;
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

	(* cas, bh 11 Oct 2000 *)
	(* bj 10.10.00	Change Init and SaveOptions to work with the registry instead of a file *)
	(* bj 02.10.00	Fixed Init so it reads the correct file. *)
	(* dg 29.08.00	bugfix in PutSelection *)
	(* dg 03.02.99	clientinterface and extensioninterface interfaces *)
	(* dg 09.12.98	hook interfaces (extensions of Kernel.Hook) and hook installations procedures are only 
							shown if &-Option is provided *)
	(* bh 29.1.97 codefile browser *)
	(* bh 15.10.96 TypName changed *)
	(* bh 24.7.96 AliasType corrected *)
	(* bh 10.7.96 OPM.Close *)
	(* bh 10.6.96 correction in GetQualident *)
	(* cas 24.4.96 changed option handling *)
	(* bh 16.4.96 underscores *)
	(* bh 22.2.96 COM support adapted *)
	(* bh 5.3.96 open ruler *)
	(* bh 12.12.95 anypointer, anyrecord, longchar, & largeint support *)
	(* bh 25.9.95 COM support *)

	IMPORT
		Kernel, Files, Utils, Fonts, Dates, Ports, Stores, Views, Properties, Dialog, Documents,
		TextModels, TextMappers, TextRulers, TextViews, TextControllers, StdDialog, StdFolds,
		DevCPM, DevCPT, HostRegistry;

	CONST
		width = 170 * Ports.mm;	(* view width *)
		height = 300 * Ports.mm;
		
		(* visibility *)
		internal = 0; external = 1; externalR = 2; inPar = 3; outPar = 4;

		(* object modes *)
		var = 1; varPar = 2; const = 3; field = 4; type = 5; lProc = 6; xProc = 7; cProc = 9; iProc = 10;
		mod = 11; head = 12; tProc = 13; ptrType = 31;

		(* structure forms *)
		undef = 0; bool = 2; sChar = 3; byte = 4; sInt = 5; int = 6; sReal = 7; real = 8;
		set = 9; sString = 10; nilTyp = 11; noTyp = 12; pointer = 13; procTyp = 14; comp = 15;
		char = 16; string = 17; lInt = 18;
		
		(* composite structure forms *)
		basic = 1; array = 2; dynArray = 3; record = 4;
		
		(* attribute flags (attr.adr, struct.attribute, proc.conval.setval) *)
		newAttr = 16; absAttr = 17; limAttr = 18; empAttr = 19; extAttr = 20;
		
		(* additional scanner types *)
		import = 100; module = 101; semicolon = 102; becomes = 103; comEnd = 104;

		modNotFoundKey = "#Dev:ModuleNotFound";
		noSuchItemExportedKey = "#Dev:NoSuchItemExported";
		noSuchExtItemExportedKey = "#Dev:NoSuchExtItemExported";
		noModuleNameSelectedKey = "#Dev:NoModuleNameSelected";
		noItemNameSelectedKey = "#Dev:NoItemNameSelected";

		regKey = "Dev\Browser\";
		
	TYPE
		TProcList = POINTER TO RECORD
			fld: DevCPT.Object;
			attr: SET;	(* newAttr *)
			next: TProcList
		END;
		
	VAR
		global: RECORD
			hints: BOOLEAN;	(* display low-level hints such as field offsets *)
			flatten: BOOLEAN;	(* include fields/bound procs of base types as well *)
			hideHooks: BOOLEAN;	(* hide hook procedures and interfaces *)
			clientinterface, extensioninterface: BOOLEAN;
			sectAttr, keyAttr: TextModels.Attributes;	(* formatting of section and other keywords *)
			level: SHORTINT;	(* current indentation level *)
			gap: BOOLEAN;	(* request for lazy Ln, issued with next Indent *)
			singleton: BOOLEAN;	(* selectively display one object's definition *)
			thisObj: DevCPT.Name;	(* singleton => selected object *)
			out: TextMappers.Formatter;	(* formatter used to produce definition text *)
			systemUsed: BOOLEAN;
			comUsed: BOOLEAN;
			modUsed: ARRAY 127 OF BOOLEAN;
			pos: INTEGER
		END;
		inp: Files.Reader;
		imp: ARRAY 256 OF Kernel.Name;
		num, lev, max: INTEGER;

		options*: RECORD
			hints*: BOOLEAN;	(* display low-level hints such as field offsets *)
			flatten*: BOOLEAN;	(* include fields/bound procs of base types as well *)
			formatted*: BOOLEAN;
			shints, sflatten, sformatted: BOOLEAN;	(* saved values *)
		END;
		
	PROCEDURE IsHook(typ: DevCPT.Struct): BOOLEAN;
	BEGIN
		WHILE ((typ.form = pointer) OR (typ.form = comp)) & (typ.BaseTyp # NIL) DO typ := typ.BaseTyp END;
		RETURN (DevCPT.GlbMod[typ.mno].name^ = "Kernel") & (typ.strobj.name^ = "Hook^")
	END IsHook;

	(* auxilliary *)

	PROCEDURE GetSelection (VAR text: TextModels.Model; VAR beg, end: INTEGER);
		VAR c: TextControllers.Controller;
	BEGIN
		c := TextControllers.Focus();
		IF (c # NIL) & c.HasSelection() THEN
			text := c.text; c.GetSelection(beg, end)
		ELSE
			text := NIL
		END
	END GetSelection;

	PROCEDURE GetQualIdent (VAR mod, ident: DevCPT.Name; VAR t: TextModels.Model);
		VAR s: TextMappers.Scanner; beg, end: INTEGER;
	BEGIN
		GetSelection(t, beg, end);
		IF t # NIL THEN
			s.ConnectTo(t); s.SetOpts({7});	(* acceptUnderscores *)
			s.SetPos(beg); s.Scan;
			IF (s.type = TextMappers.string) & (s.len < LEN(mod)) THEN
				mod := SHORT(s.string$); s.Scan;
				IF (s.type = TextMappers.char) & (s.char = ".") & (s.Pos() <= end) THEN
					s.Scan;
					IF (s.type = TextMappers.string) & (s.len < LEN(ident)) THEN
						ident := SHORT(s.string$)
					ELSE mod[0] := 0X; ident[0] := 0X
					END
				ELSE ident[0] := 0X
				END
			ELSE mod[0] := 0X; ident[0] := 0X
			END
		ELSE mod[0] := 0X; ident[0] := 0X
		END
	END GetQualIdent;
	
	PROCEDURE Scan (VAR s: TextMappers.Scanner);
	BEGIN
		s.Scan;
		IF s.type = TextMappers.string THEN
			IF s.string = "IMPORT" THEN s.type := import
			ELSIF s.string = "MODULE" THEN s.type := module
			END
		ELSIF s.type = TextMappers.char THEN
			IF s.char = ";" THEN s.type := semicolon
			ELSIF s.char = ":" THEN
				IF s.rider.char = "=" THEN s.rider.Read; s.type := becomes END
			ELSIF s.char = "(" THEN
				IF s.rider.char = "*" THEN
					s.rider.Read;
					REPEAT Scan(s) UNTIL (s.type = TextMappers.eot) OR (s.type = comEnd);
					Scan(s)
				END
			ELSIF s.char = "*" THEN
				IF s.rider.char = ")" THEN s.rider.Read; s.type := comEnd END
			END
		END
	END Scan;
	
	PROCEDURE CheckModName (VAR mod: DevCPT.Name; t: TextModels.Model);
		VAR s: TextMappers.Scanner;
	BEGIN
		s.ConnectTo(t); s.SetPos(0); Scan(s);
		IF s.type = module THEN
			Scan(s);
			WHILE (s.type # TextMappers.eot) & (s.type # import) DO Scan(s) END;
			WHILE (s.type # TextMappers.eot) & (s.type # semicolon)
					& ((s.type # TextMappers.string) OR (s.string # mod)) DO Scan(s) END;
			IF s.type = TextMappers.string THEN
				Scan(s);
				IF s.type = becomes THEN
					Scan(s);
					IF s.type = TextMappers.string THEN
						mod := SHORT(s.string$)
					END
				END
			END
		END
	END CheckModName;


	PROCEDURE NewRuler (): TextRulers.Ruler;
		VAR ra: TextRulers.Attributes; p: TextRulers.Prop;
	BEGIN
		NEW(p); p.valid := {TextRulers.right, TextRulers.opts};
		p.opts.val := {(*TextRulers.rightFixed*)}; p.opts.mask := p.opts.val;
		p.right := width;
		NEW(ra); ra.InitFromProp(p);
		RETURN TextRulers.dir.New(TextRulers.dir.NewStyle(ra))
	END NewRuler;

	PROCEDURE Append (VAR d: ARRAY OF CHAR; s: ARRAY OF SHORTCHAR);
		VAR l, ld, ls: INTEGER;
	BEGIN
		l := LEN(d); ld := LEN(d$); ls := LEN(s$);
		IF ld + ls >= l THEN
			s[l - ld - 1] := 0X; d := d + s;
			d[l - 2] := "."; d[l - 3] := "."; d[l - 4] := "."
		ELSE
			d := d + s
		END
	END Append;
	
	PROCEDURE IsLegal (name: POINTER TO ARRAY OF SHORTCHAR): BOOLEAN;
		VAR i: SHORTINT;
	BEGIN
		IF name^ = "" THEN RETURN FALSE END;
		IF name[0] = "@" THEN RETURN global.hints END;
		i := 0;
		WHILE name[i] # 0X DO INC(i) END;
		RETURN name[i-1] # "^"
	END IsLegal;


	(* output formatting *)

	PROCEDURE Char (ch: SHORTCHAR);
	BEGIN
		global.out.WriteChar(ch)
	END Char;

	PROCEDURE String (s: ARRAY OF SHORTCHAR);
	BEGIN
		global.out.WriteSString(s)
	END String;
	
	PROCEDURE StringConst (s: ARRAY OF SHORTCHAR; long: BOOLEAN);
		VAR i, x, y: INTEGER; quoted, first: BOOLEAN;
	BEGIN
		IF long THEN
			i := 0; REPEAT DevCPM.GetUtf8(s, y, i) UNTIL (y = 0) OR (y >= 100H);
			IF y = 0 THEN global.out.WriteString("LONG(") END
		END;
		i := 0; quoted := FALSE; first := TRUE;
		IF long THEN DevCPM.GetUtf8(s, x, i) ELSE x := ORD(s[i]); INC(i) END;
		WHILE x # 0 DO
			IF (x < ORD(" "))
					OR (x >= 7FH) & (x <= 9FH)
					OR (x >= 0D800H) & (x <= 0F8FFH)
					OR (x >= 0FFF0H) THEN	(* nonprintable character *)
				IF quoted THEN global.out.WriteChar('"') END;
				IF ~first THEN global.out.WriteString(" + ") END;
				IF x >= 0A00H THEN global.out.WriteIntForm(x, TextMappers.hexadecimal, 4, "0", FALSE)
				ELSIF x >= 0A0H THEN global.out.WriteIntForm(x, TextMappers.hexadecimal, 3, "0", FALSE)
				ELSE global.out.WriteIntForm(x, TextMappers.hexadecimal, 2, "0", FALSE)
				END;
				global.out.WriteChar("X");
				quoted := FALSE
			ELSE	(* printable character *)
				IF ~quoted THEN
					IF ~first THEN global.out.WriteString(" + ") END;
					global.out.WriteChar('"')
				END;
				global.out.WriteChar(CHR(x));
				quoted := TRUE
			END;
			IF long THEN DevCPM.GetUtf8(s, x, i) ELSE x := ORD(s[i]); INC(i) END;
			first := FALSE
		END;
		IF first THEN global.out.WriteString('""') END;
		IF quoted THEN global.out.WriteChar('"') END;
		IF long & (y = 0) THEN global.out.WriteChar(")") END
	END StringConst;
	
	PROCEDURE ProperString (s: ARRAY OF SHORTCHAR);
		VAR i: SHORTINT;
	BEGIN
		IF s # "" THEN
			i := 0; WHILE (s[i] # 0X) & (s[i] # "^") DO global.out.WriteChar(s[i]); INC(i) END
		END
	END ProperString;

	PROCEDURE Keyword (s: ARRAY OF SHORTCHAR);
		VAR a: TextModels.Attributes;
	BEGIN
		IF global.keyAttr # NIL THEN a := global.out.rider.attr; global.out.rider.SetAttr(global.keyAttr) END;
		global.out.WriteSString(s);
		IF global.keyAttr # NIL THEN global.out.rider.SetAttr(a) END
	END Keyword;

	PROCEDURE Section (VAR out: TextMappers.Formatter; s: ARRAY OF SHORTCHAR);
		VAR a: TextModels.Attributes;
	BEGIN
		IF global.sectAttr # NIL THEN a := out.rider.attr; out.rider.SetAttr(global.sectAttr) END;
		out.WriteSString(s);
		IF global.sectAttr # NIL THEN out.rider.SetAttr(a) END
	END Section;

	PROCEDURE Int (i: INTEGER);
	BEGIN
		global.out.WriteInt(i)
	END Int;
	
	PROCEDURE Hex (x, n: INTEGER);
	BEGIN
		IF n > 1 THEN Hex(x DIV 16, n - 1) END;
		x := x MOD 16;
		IF x >= 10 THEN global.out.WriteChar(SHORT(CHR(x + ORD("A") - 10)))
		ELSE global.out.WriteChar(SHORT(CHR(x + ORD("0"))))
		END
	END Hex;

	PROCEDURE Ln;
	BEGIN
		global.out.WriteLn
	END Ln;

	PROCEDURE Indent;
		VAR i: SHORTINT;
	BEGIN
		IF global.gap THEN global.gap := FALSE; Ln END;
		i := global.level; WHILE i > 0 DO global.out.WriteTab; DEC(i) END
	END Indent;

	PROCEDURE Guid (ext: DevCPT.ConstExt);
	BEGIN
		Char("{");
		Hex(ORD(ext[2]) + 256 * ORD(ext[3]), 4);
		Hex(ORD(ext[0]) + 256 * ORD(ext[1]), 4);
		Char("-");
		Hex(ORD(ext[4]) + 256 * ORD(ext[5]), 4);
		Char("-");
		Hex(ORD(ext[6]) + 256 * ORD(ext[7]), 4);
		Char("-");
		Hex(ORD(ext[8]), 2);
		Hex(ORD(ext[9]), 2);
		Char("-");
		Hex(ORD(ext[10]), 2);
		Hex(ORD(ext[11]), 2);
		Hex(ORD(ext[12]), 2);
		Hex(ORD(ext[13]), 2);
		Hex(ORD(ext[14]), 2);
		Hex(ORD(ext[15]), 2);
		Char("}")
	END Guid;
	


	(* special marks *)

	PROCEDURE Hint (label: ARRAY OF SHORTCHAR; i: INTEGER; adj: BOOLEAN);
	BEGIN
		IF global.hints THEN
			Char("["); String(label);
			IF adj & (0 <= i) & (i < 10) THEN Char(TextModels.digitspace) END;
			Int(i);
			String("] ")
		END
	END Hint;

	PROCEDURE Hint3 (label1, label2, label3: ARRAY OF SHORTCHAR; i1, i2, i3: INTEGER);
	BEGIN
		IF global.hints THEN
			Char("["); String(label1); Int(i1); String(label2); Int(i2); String(label3); Int(i3); String("] ")
		END
	END Hint3;

	PROCEDURE Vis (i: INTEGER);
	BEGIN
		IF i # external THEN Char("-") END
	END Vis;
	
	PROCEDURE ProcSysFlag (flag: SHORTINT);
	BEGIN
		IF flag # 0 THEN
			String(" [");
			IF flag = -10 THEN String("ccall")
			ELSE Int(flag)
			END;
			Char("]")
		END
	END ProcSysFlag;
	
	PROCEDURE ParSysFlag (flag: SHORTINT);
		VAR pos: INTEGER;
	BEGIN
		IF flag # 0 THEN
			String(" [");
			pos := global.out.Pos();
			IF ODD(flag) THEN String("nil") END;
(*
			IF ODD(flag DIV 2) & (flag # 18) THEN
				IF global.out.Pos() # pos THEN String(", ") END;
				String("in")
			END;
			IF ODD(flag DIV 4) & (flag # 12) THEN
				IF global.out.Pos() # pos THEN String(", ") END;
				String("out")
			END;
*)
			IF flag DIV 8 = 1 THEN
				IF global.out.Pos() # pos THEN String(", ") END;
				String("new")
			ELSIF flag DIV 8 = 2 THEN
				IF global.out.Pos() # pos THEN String(", ") END;
				String("iid")
			ELSIF flag DIV 8 # 0 THEN
				IF global.out.Pos() # pos THEN String(", ") END;
				Int(flag DIV 8 * 8)
			END;
			Char("]")
		END
	END ParSysFlag;
	
	PROCEDURE StructSysFlag (typ: DevCPT.Struct);
		VAR flag: SHORTINT;
	BEGIN
		flag := typ.sysflag;
		IF (flag # 0) & ((typ.form # pointer) OR (flag = 2)) THEN
			String(" [");
			IF flag = 1 THEN String("untagged")
			ELSIF flag = 2 THEN String("handle")
			ELSIF flag = 3 THEN String("noalign")
			ELSIF flag = 4 THEN String("align2")
			ELSIF flag = 5 THEN String("align4")
			ELSIF flag = 6 THEN String("align8")
			ELSIF flag = 7 THEN String("union")
			ELSIF flag = 10 THEN
				IF typ.ext # NIL THEN Char('"'); String(typ.ext^); Char('"')
				ELSE String("interface")
				END
			ELSIF flag = 20 THEN String("som")
			ELSE Int(flag)
			END;
			Char("]")
		END
	END StructSysFlag;
	
	PROCEDURE SysStrings (obj: DevCPT.Object);
	BEGIN
		IF global.hints & ((obj.entry # NIL) OR (obj.library # NIL)) THEN
			String(' ["');
			IF obj.library # NIL THEN String(obj.library^); String('", "') END;
			IF obj.entry # NIL THEN String(obj.entry^) END;
			String('"]')
		END
	END SysStrings;


	(* non-terminals *)

	PROCEDURE ^ Structure (typ: DevCPT.Struct);

	PROCEDURE Qualifier (mno: SHORTINT);
	BEGIN
		IF (mno > 1) OR (mno = 1) & global.singleton THEN
			global.modUsed[mno] := TRUE;
			String(DevCPT.GlbMod[mno].name^); Char(".")
		END
	END Qualifier;

	PROCEDURE LocalName (obj: DevCPT.Object);
	BEGIN
		Qualifier(0); String(obj.name^)
	END LocalName;

	PROCEDURE TypName (typ: DevCPT.Struct; force: BOOLEAN);
	BEGIN
		IF force OR IsLegal(typ.strobj.name) THEN
			IF (typ.mno > 1) OR (typ.mno = 1)
				& ((typ.form >= pointer) OR (typ.BaseTyp # DevCPT.undftyp)) & global.singleton
			THEN
				Qualifier(typ.mno)
			ELSIF typ = DevCPT.sysptrtyp THEN	(* PTR is in SYSTEM *)
				String("SYSTEM.");
				global.systemUsed := TRUE
			ELSIF
				(typ = DevCPT.guidtyp) OR (typ = DevCPT.restyp) OR (typ = DevCPT.iunktyp) OR (typ = DevCPT.punktyp)
			THEN
				String("COM.");
				global.comUsed := TRUE
			END;
			IF force THEN ProperString(typ.strobj.name^) ELSE String(typ.strobj.name^) END
		ELSE
			Structure(typ)
		END
	END TypName;

	PROCEDURE ParList (VAR par: DevCPT.Object);
	BEGIN
		IF par.mode = varPar THEN
			IF par.vis = inPar THEN Keyword("IN")
			ELSIF par.vis = outPar THEN Keyword("OUT")
			ELSE Keyword("VAR")
			END;
			ParSysFlag(par.sysflag); Char(" ")
		END;
		String(par.name^);
		WHILE (par.link # NIL) & (par.link.typ = par.typ) & (par.link.mode = par.mode)
				& (par.link.vis = par.vis) & (par.link.sysflag = par.sysflag) DO

			par := par.link; String(", "); String(par.name^)
		END;
		String(": "); TypName(par.typ, FALSE)
	END ParList;

	PROCEDURE Signature (result: DevCPT.Struct; par: DevCPT.Object);
		VAR paren, res: BOOLEAN;
	BEGIN
		res := result # DevCPT.notyp; paren := res OR (par # NIL);
		IF paren THEN String(" (") END;
		IF par # NIL THEN
			ParList(par); par := par.link;
			WHILE par # NIL DO String("; "); ParList(par); par := par.link END
		END;
		IF paren THEN Char(")") END;
		IF res THEN String(": "); TypName(result, FALSE) END
	END Signature;

	PROCEDURE TProcs (rec: DevCPT.Struct; fld: DevCPT.Object; oldProcs: TProcList; VAR newProcs: TProcList);
		VAR old: DevCPT.Object; p, elem: TProcList;
	BEGIN
		IF fld # NIL THEN
			TProcs(rec, fld.left, oldProcs, newProcs);
			IF (fld.mode = tProc) & IsLegal(fld.name) THEN
				DevCPT.FindBaseField(fld.name^, rec, old);
				IF (old = NIL) OR (fld.typ # old.typ) OR (* (rec.attribute # 0) & *) (fld.conval.setval # old.conval.setval)
				THEN
					IF global.extensioninterface OR global.clientinterface THEN
						(* do not show methods with equal name *)
						p := oldProcs;
						WHILE (p # NIL) & (p.fld.name^ # fld.name^) DO p := p.next END
					END;
					IF p = NIL THEN
						NEW(elem); elem.next := newProcs; newProcs := elem;
						elem.fld := fld;
						
						IF old = NIL THEN INCL(elem.attr, newAttr) END;
						IF absAttr IN fld.conval.setval THEN INCL(elem.attr, absAttr)
						ELSIF empAttr IN fld.conval.setval THEN INCL(elem.attr, empAttr)
						ELSIF extAttr IN fld.conval.setval THEN INCL(elem.attr, extAttr)
						END
					END
				END
			END;
			TProcs(rec, fld.right, oldProcs, newProcs)
		END
	END TProcs;
	
(*
	PROCEDURE AdjustTProcs (typ: DevCPT.Struct; fld: DevCPT.Object);
		VAR receiver, old: DevCPT.Object; base: DevCPT.Struct;
	BEGIN
		IF fld # NIL THEN
			AdjustTProcs(typ, fld.left);
			IF (fld.mode = tProc) & IsLegal(fld.name) THEN
				DevCPT.FindBaseField(fld.name^, typ, old);
				IF old # NIL THEN
					IF fld.conval.setval * {absAttr, empAttr, extAttr} = {} THEN
						old.conval.setval := old.conval.setval - {absAttr, empAttr, extAttr}
					ELSIF (extAttr IN fld.conval.setval) & (empAttr IN old.conval.setval) THEN
						(* do not list methods which only change attribute from empty to extensible *)
						old.conval.setval := old.conval.setval + {extAttr} - {empAttr}
					END;
					IF fld.typ # old.typ THEN (* do not show base methods of covariant overwritten methods *)
						old.typ := fld.typ; old.conval.setval := {}
					END;
				END
			END;
			AdjustTProcs(typ, fld.right)
		END;
		IF (typ.BaseTyp # NIL) & (typ.BaseTyp # DevCPT.iunktyp) THEN
			AdjustTProcs(typ.BaseTyp, typ.BaseTyp.link)
		END
	END AdjustTProcs;
*)

	PROCEDURE TypeboundProcs (typ: DevCPT.Struct; VAR cont: BOOLEAN; top: BOOLEAN;
												VAR list: TProcList; recAttr: INTEGER);
		VAR receiver: DevCPT.Object; new, p, q: TProcList;
	BEGIN
		IF (typ # NIL) & (typ # DevCPT.iunktyp) THEN
			TProcs(typ, typ.link, list, new);
			
			p := list;
			WHILE new # NIL DO
				q := new.next; new.next := p; p := new; new := q
			END;
			list := p;

			IF global.flatten THEN
				TypeboundProcs(typ.BaseTyp, cont, FALSE, list, recAttr);
				IF (typ.BaseTyp = NIL) OR (typ.BaseTyp = DevCPT.iunktyp) THEN
(*
					IF global.extensioninterface & (recAttr IN {absAttr, extAttr}) THEN
						IF cont THEN Char(";") END;
						Ln; Indent;
						String("(a: ANYPTR) FINALIZE-, "); Keyword("NEW"); String(", "); Keyword("EMPTY");
						cont := TRUE
					END
*)
				END
			END;
			IF top THEN (* writeList *)
				WHILE list # NIL DO
					IF ~global.clientinterface & ~global.extensioninterface (* default *)
					OR global.clientinterface & (list.fld.vis = external)
					OR global.extensioninterface & (recAttr IN {absAttr, extAttr}) & (list.attr * {absAttr, empAttr, extAttr} # {})
					THEN

						IF cont THEN Char(";") END;
						Ln;
						receiver := list.fld.link;
						Indent; Hint("entry ", list.fld.num, TRUE);
	(*
						Keyword("PROCEDURE"); String(" (");
	*)
						Char("(");
						IF receiver.mode = varPar THEN
							IF receiver.vis = inPar THEN Keyword("IN") ELSE Keyword("VAR") END;
							Char(" ")
						END;
						String(receiver.name^); String(": ");
						IF IsLegal(receiver.typ.strobj.name) & (receiver.typ.mno = 1) THEN
							String(receiver.typ.strobj.name^)
						ELSE TypName(receiver.typ, FALSE)
						END;
						String(") "); String(list.fld.name^); Vis(list.fld.vis);
						SysStrings(list.fld);
						Signature(list.fld.typ, receiver.link);
						IF newAttr IN list.attr THEN String(", "); Keyword("NEW") END;
						IF absAttr IN list.attr THEN String(", "); Keyword("ABSTRACT")
						ELSIF empAttr IN list.attr THEN String(", "); Keyword("EMPTY")
						ELSIF extAttr IN list.attr THEN String(", "); Keyword("EXTENSIBLE")
						END;
						cont := TRUE
					END;
					list := list.next
				END
			END
		END
	END TypeboundProcs;

	PROCEDURE Flds (typ: DevCPT.Struct; VAR cont: BOOLEAN);
		VAR fld: DevCPT.Object;
	BEGIN
		fld := typ.link;
		WHILE (fld # NIL) & (fld.mode = field) DO
			IF IsLegal(fld.name) THEN
				IF cont THEN Char(";") END;
				Ln;
				Indent; Hint("offset ", fld.adr, TRUE);
				IF typ.mno > 1 THEN String("(* "); TypName(typ, TRUE); String(" *) ") END;
				String(fld.name^);
				WHILE (fld.link # NIL) & (fld.link.typ = fld.typ) & (fld.link.name # NIL) DO
					Vis(fld.vis); fld := fld.link; String(", "); String(fld.name^)
				END;
				Vis(fld.vis); String(": "); TypName(fld.typ, FALSE);
				cont := TRUE
			END;
			fld := fld.link
		END
	END Flds;

	PROCEDURE Fields (typ: DevCPT.Struct; VAR cont: BOOLEAN);
	BEGIN
		IF typ # NIL THEN
			IF global.flatten THEN Fields(typ.BaseTyp, cont) END;
			Flds(typ, cont)
		END
	END Fields;

	PROCEDURE BaseTypes (typ: DevCPT.Struct);
	BEGIN
		IF typ.BaseTyp # NIL THEN
			Char("("); TypName(typ.BaseTyp, TRUE);
			IF global.flatten THEN BaseTypes(typ.BaseTyp) END;
			Char(")")
		END
	END BaseTypes;

	PROCEDURE Structure (typ: DevCPT.Struct);
		VAR cont: BOOLEAN; p: TProcList;
	BEGIN
		INC(global.level);
		CASE typ.form OF
		  pointer:
			Keyword("POINTER"); StructSysFlag(typ); Char(" ");
			Keyword("TO"); Char(" "); DEC(global.level); TypName(typ.BaseTyp, FALSE); INC(global.level)
		| procTyp:
			Keyword("PROCEDURE"); ProcSysFlag(typ.sysflag); Signature(typ.BaseTyp, typ.link)
		| comp:
			CASE typ.comp OF
			  array:
				Keyword("ARRAY"); StructSysFlag(typ); Char(" "); Int(typ.n); Char(" ");
				Keyword("OF"); Char(" "); TypName(typ.BaseTyp, FALSE)
			| dynArray:
				Keyword("ARRAY"); StructSysFlag(typ); Char(" ");
				Keyword("OF"); Char(" "); TypName(typ.BaseTyp, FALSE)
			| record:
				IF typ.attribute = absAttr THEN Keyword("ABSTRACT ")
				ELSIF typ.attribute = limAttr THEN Keyword("LIMITED ")
				ELSIF typ.attribute = extAttr THEN Keyword("EXTENSIBLE ")
				END;
				Keyword("RECORD"); StructSysFlag(typ);
				Char(" "); Hint3("tprocs ", ", size ", ", align ", typ.n, typ.size, typ.align);
				cont := FALSE;
				BaseTypes(typ); Fields(typ, cont); TypeboundProcs(typ, cont, TRUE, p, typ.attribute);
				IF cont THEN Ln; DEC(global.level); Indent; INC(global.level) ELSE Char (" ") END;
				Keyword("END")
			END
		ELSE
			IF (typ.BaseTyp # DevCPT.undftyp) THEN TypName(typ.BaseTyp, FALSE) END	(* alias structures *)
		END;
		DEC(global.level)
	END Structure;

	PROCEDURE Const (obj: DevCPT.Object);
		VAR con: DevCPT.Const; s: SET; i: SHORTINT; x, y: INTEGER; r: REAL;
	BEGIN
		Indent; LocalName(obj); SysStrings(obj); String(" = ");
		con := obj.conval;
		CASE obj.typ.form OF
		  bool:
			IF con.intval = 1 THEN String("TRUE") ELSE String("FALSE") END
		| sChar, char:
			IF con.intval >= 0A000H THEN
				Hex(con.intval, 5); Char("X")
			ELSIF con.intval >= 0100H THEN
				Hex(con.intval, 4); Char("X")
			ELSIF con.intval >= 0A0H THEN
				Hex(con.intval, 3); Char("X")
			ELSIF (con.intval >= 32) & (con.intval <= 126) THEN
				Char(22X); Char(SHORT(CHR(con.intval))); Char(22X)
			ELSE
				Hex(con.intval, 2); Char("X")
			END
		| byte, sInt, int:
			Int(con.intval)
		| lInt:
			y := SHORT(ENTIER((con.realval + con.intval) / 4294967296.0));
			r := con.realval + con.intval - y * 4294967296.0;
			IF r > MAX(INTEGER) THEN r := r - 4294967296.0 END;
			x := SHORT(ENTIER(r));
			Hex(y, 8); Hex(x, 8); Char("L")
		| set:
			Char("{"); i := 0; s := con.setval;
			WHILE i <= MAX(SET) DO
				IF i IN s THEN Int(i); EXCL(s, i);
					IF s # {} THEN String(", ") END
				END;
				INC(i)
			END;
			Char("}")
		| sReal, real:
			global.out.WriteReal(con.realval)
		| sString:
			StringConst(con.ext^, FALSE)
		| string:
			StringConst(con.ext^, TRUE)
		| nilTyp:
			Keyword("NIL")
		| comp:	(* guid *)
			Char("{"); Guid(con.ext); Char("}")
		END;
		Char(";"); Ln
	END Const;
	
	PROCEDURE AliasType (typ: DevCPT.Struct);
	BEGIN
		IF global.singleton & IsLegal(typ.strobj.name) THEN
			WHILE typ.BaseTyp # DevCPT.undftyp DO
				Char(";"); Ln; Indent;
				String(typ.strobj.name^); SysStrings(typ.strobj); String(" = ");
				IF typ.form >= pointer THEN
					Structure(typ); typ := DevCPT.int16typ
				ELSE
					typ := typ.BaseTyp;
					TypName(typ, FALSE)
				END
			END
		END
	END AliasType;

	PROCEDURE NotExtRec(typ: DevCPT.Struct): BOOLEAN;
	BEGIN
		RETURN (typ.form = comp) & ((typ.comp # record) OR ~(typ.attribute IN {absAttr, extAttr}))
			OR (typ.form = procTyp)
	END NotExtRec;

	PROCEDURE Type (obj: DevCPT.Object);
	BEGIN
		IF global.hideHooks & IsHook(obj.typ) THEN RETURN END;

		IF global.extensioninterface
(*
		& (obj.typ.strobj = obj) & ~((obj.typ.form < pointer) & (obj.typ.BaseTyp # DevCPT.undftyp))
*)
		& (NotExtRec(obj.typ)
			OR
			(obj.typ.form = pointer) & ~IsLegal(obj.typ.BaseTyp.strobj.name) &
				NotExtRec(obj.typ.BaseTyp)
		)
			
		THEN
			IF global.singleton THEN RETURN END;
			global.out.rider.SetAttr(TextModels.NewColor(global.out.rider.attr, Ports.grey50));
			IF global.pos < global.out.rider.Pos() THEN
				global.out.rider.Base().SetAttr(global.pos, global.out.rider.Pos()-1, global.out.rider.attr)
			END
		END;
		
		Indent; LocalName(obj); SysStrings(obj); String(" = ");
		IF obj.typ.strobj # obj THEN
			TypName(obj.typ, FALSE);
			AliasType(obj.typ)
		ELSIF (obj.typ.form < pointer) & (obj.typ.BaseTyp # DevCPT.undftyp) THEN
			TypName(obj.typ.BaseTyp, FALSE);
			AliasType(obj.typ.BaseTyp)
		ELSE
			Structure(obj.typ)
		END;
		Char(";"); Ln;
		global.gap := TRUE
	END Type;

	PROCEDURE PtrType (obj: DevCPT.Object);
		VAR base: DevCPT.Object;
	BEGIN
		Type(obj);
		base := obj.typ.BaseTyp.strobj;
		IF (base # NIL) & (base # DevCPT.anytyp.strobj) & (base # DevCPT.iunktyp.strobj)
				& (base.vis # internal) & (base.typ.strobj # NIL) & ~(global.hideHooks & IsHook(base.typ)) THEN
			(* show named record with pointer; avoid repetition *)

		IF global.extensioninterface
		& (base.typ.strobj = base) & ~((obj.typ.form < pointer) & (obj.typ.BaseTyp # DevCPT.undftyp))
		& (NotExtRec(base.typ)
			OR
			(base.typ.form = pointer) & ~IsLegal(base.typ.BaseTyp.strobj.name) &
				NotExtRec(base.typ.BaseTyp)
		)

		THEN
			IF global.singleton THEN
				IF global.pos < global.out.rider.Pos() THEN
					global.out.rider.Base().Delete(global.pos, global.out.rider.Pos());
					global.out.rider.SetPos(global.pos)
				END;
				RETURN
			END;
			global.out.rider.SetAttr(TextModels.NewColor(global.out.rider.attr, Ports.grey50));
			IF global.pos < global.out.rider.Pos() THEN
				global.out.rider.Base().SetAttr(global.pos, global.out.rider.Pos()-1, global.out.rider.attr)
			END
		END;

			global.gap := FALSE;
			Indent;
			String(base.name^); SysStrings(base); String(" = ");
			IF base.typ.strobj # base THEN
				TypName(base.typ, FALSE);
				AliasType(obj.typ)
			ELSIF (obj.typ.form < pointer) & (obj.typ.BaseTyp # DevCPT.undftyp) THEN
				TypName(obj.typ.BaseTyp, FALSE);
				AliasType(obj.typ.BaseTyp)
			ELSE
				Structure(base.typ)
			END;
			Char(";"); Ln;
			base.vis := internal;
			global.gap := TRUE
		END
	END PtrType;

	PROCEDURE Var (obj: DevCPT.Object);
	BEGIN
		IF global.hideHooks & IsHook(obj.typ) THEN RETURN END;
		Indent; LocalName(obj); Vis(obj.vis); SysStrings(obj); String(": ");
		TypName(obj.typ, FALSE); Char(";"); Ln
	END Var;

	PROCEDURE Proc (obj: DevCPT.Object);
		VAR ext: DevCPT.ConstExt; i, m: SHORTINT;
	BEGIN
		IF global.hideHooks & (obj.link # NIL) & (obj.link.link = NIL) & IsHook(obj.link.typ) THEN RETURN END;
		IF global.extensioninterface THEN RETURN END;
		Indent; Keyword("PROCEDURE"); (* Char(" "); *)
		IF obj.mode = cProc THEN String(" [code]")
		ELSIF obj.mode = iProc THEN String(" [callback]")
		ELSE ProcSysFlag(obj.sysflag)
		END;
		Char(" "); LocalName(obj);
		(*IF obj.mode # cProc THEN SysStrings(obj) END;*)
		SysStrings(obj);
		Signature(obj.typ, obj.link);
		IF (obj.mode = cProc) & global.hints THEN
			Ln;
			INC(global.level); Indent;
			ext := obj.conval.ext; m := ORD(ext^[0]); i := 1;
			WHILE i <= m DO
				global.out.WriteIntForm(ORD(ext^[i]), TextMappers.hexadecimal, 3, "0", TextMappers.showBase);
				IF i < m THEN String(", ") END;
				INC(i)
			END;
			DEC(global.level)
		END;
		Char(";"); Ln
	END Proc;


	(* tree traversal *)

	PROCEDURE Objects (obj: DevCPT.Object; mode: SET);
		VAR m: BYTE; a: TextModels.Attributes;
	BEGIN
		IF obj # NIL THEN
			INC(lev); INC(num); max := MAX(max, lev);
			Objects(obj.left, mode);
			m := obj.mode; IF (m = type) & (obj.typ.form = pointer) THEN m := ptrType END;
			IF (m IN mode) & (obj.vis # internal) & (obj.name # NIL) &
					(~global.singleton OR (obj.name^ = global.thisObj)) THEN
				global.pos := global.out.rider.Pos(); a := global.out.rider.attr;
				CASE m OF
				  const: Const(obj)
				| type: Type(obj)
				| ptrType: PtrType(obj)
				| var: Var(obj)
				| xProc, cProc, iProc: Proc(obj)
				END;
				global.out.rider.SetAttr(a)
			END;
			Objects(obj.right, mode);
			DEC(lev)
		END
	END Objects;


	(* definition formatting *)

	PROCEDURE PutSection (VAR out: TextMappers.Formatter; s: ARRAY OF SHORTCHAR);
		VAR buf, def: TextModels.Model; i: INTEGER;
	BEGIN
		buf := global.out.rider.Base();
		IF buf.Length() > 0 THEN
			IF ~global.singleton THEN Ln END;
			def := out.rider.Base(); out.SetPos(def.Length());
			IF s # "" THEN	(* prepend section keyword *)
				i := global.level - 1; WHILE i > 0 DO out.WriteTab; DEC(i) END;
				Section(out, s); out.WriteLn
			END;
			def.Append(buf);
			global.pos := 0;
			global.out.rider.SetPos(0)
		END;
		global.gap := FALSE
	END PutSection;

	PROCEDURE Scope (def: TextModels.Model; this: DevCPT.Name);
		VAR out: TextMappers.Formatter; i: SHORTINT; a: TextModels.Attributes;
	BEGIN
		global.gap := FALSE; global.thisObj := this; global.singleton := (this # "");
		global.systemUsed := FALSE; global.comUsed := FALSE;
		i := 1; WHILE i < LEN(global.modUsed) DO global.modUsed[i] := FALSE; INC(i) END;
		out.ConnectTo(def);
		INC(global.level);
		IF ~(global.extensioninterface & global.singleton) THEN
			IF global.extensioninterface THEN
				a := global.out.rider.attr; global.out.rider.SetAttr(TextModels.NewColor(a, Ports.grey50))
			END;
			Objects(DevCPT.GlbMod[1].right, {const});
			IF global.extensioninterface THEN global.out.rider.SetAttr(a) END;
			PutSection(out, "CONST")
		END;
		Objects(DevCPT.GlbMod[1].right, {ptrType});
		Objects(DevCPT.GlbMod[1].right, {type}); PutSection(out, "TYPE");
		IF ~global.extensioninterface THEN
			Objects(DevCPT.GlbMod[1].right, {var}); PutSection(out, "VAR")
		END;
		DEC(global.level);
		num := 0; lev := 0; max := 0;
		Objects(DevCPT.GlbMod[1].right, {xProc, cProc}); PutSection(out, "")
	END Scope;

	PROCEDURE Modules;
		VAR i, j, n: SHORTINT;
	BEGIN
		n := 0; j := 2;
		IF global.systemUsed THEN INC(n) END;
		IF global.comUsed THEN INC(n) END;
		WHILE j < DevCPT.nofGmod DO
			IF global.modUsed[j] THEN INC(n) END;
			INC(j)
		END;
		IF n > 0 THEN
			Indent; Section(global.out, "IMPORT"); INC(global.level);
			IF n < 10 THEN Char(" ") ELSE Ln; Indent END;
			i := 0; j := 2;
			IF global.systemUsed THEN
				String("SYSTEM"); INC(i);
				IF i < n THEN String(", ") END
			END;
			IF global.comUsed THEN
				String("COM"); INC(i);
				IF i < n THEN String(", ") END
			END;
			WHILE i < n DO
				IF global.modUsed[j] THEN
					String(DevCPT.GlbMod[j].name^); INC(i);
(*
					IF (DevCPT.GlbMod[j].strings # NIL) & (DevCPT.GlbMod[j].strings.ext # NIL) THEN
						String(' ["'); String(DevCPT.GlbMod[j].strings.ext^); String('"]')
					END;
*)
					IF i < n THEN
						Char(",");
						IF i MOD 10 = 0 THEN Ln; Indent ELSE Char(" ") END
					END
				END;
				INC(j)
			END;
			Char(";"); Ln; Ln
		END
	END Modules;


	(* compiler interfacing *)

	PROCEDURE OpenCompiler (mod: DevCPT.Name; VAR noerr: BOOLEAN);
		VAR null: TextModels.Model; main: DevCPT.Name;
	BEGIN
		null := TextModels.dir.New();
		DevCPM.Init(null.NewReader(NIL), null);
(*
		DevCPT.Init({}); DevCPT.Open(mod); DevCPT.SelfName := "AvoidErr154";
*)
		DevCPT.Init({}); main := "@mainMod"; DevCPT.Open(main);
		DevCPT.processor := 0;	(* accept all sym files *)
		DevCPT.Import("@notself", mod, noerr);
		IF ~DevCPM.noerr THEN noerr := FALSE END
	END OpenCompiler;

	PROCEDURE CloseCompiler;
	BEGIN
		DevCPT.Close;
		DevCPM.Close
	END CloseCompiler;


	PROCEDURE Browse (
		ident: DevCPT.Name; opts: ARRAY OF CHAR; VAR view: Views.View; VAR title: Views.Title
	);
		VAR def, buf: TextModels.Model; v: TextViews.View; h: BOOLEAN;
			p: Properties.BoundsPref; mod: DevCPT.Name; i: SHORTINT; str, str1: ARRAY 256 OF CHAR;
	BEGIN
		mod := DevCPT.GlbMod[1].name^$;
		i := 0;
		global.hints := FALSE; global.flatten := FALSE; global.hideHooks := TRUE;
		global.clientinterface := FALSE; global.extensioninterface := FALSE;
		global.keyAttr := NIL; global.sectAttr := NIL;

		IF opts[0] = '@' THEN
			IF options.hints THEN opts := opts + "+" END;
			IF options.flatten THEN opts := opts + "!" END;
			IF options.formatted THEN opts := opts + "/" END
		END;
		
		WHILE opts[i] # 0X DO
			CASE opts[i] OF
			| '+': global.hints := TRUE
			| '!': global.flatten := TRUE
			| '&': global.hideHooks := FALSE
			| '/':
				(* global.keyAttr := TextModels.NewStyle(TextModels.dir.attr, {Fonts.underline}); *)
				global.keyAttr := TextModels.NewWeight(TextModels.dir.attr, Fonts.bold);
				global.sectAttr := TextModels.NewWeight(TextModels.dir.attr, Fonts.bold)
			| 'c': global.clientinterface := TRUE
			| 'e' : global.extensioninterface := TRUE
			ELSE
			END;
			INC(i)
		END;
		IF global.clientinterface & global.extensioninterface THEN
			global.clientinterface := FALSE; global.extensioninterface := FALSE
		END;
		IF global.extensioninterface THEN global.hideHooks := FALSE END;
		def := TextModels.dir.New();
		buf := TextModels.dir.New(); global.out.ConnectTo(buf);
		IF ident # "" THEN
			h := global.hideHooks; global.hideHooks := FALSE;
			global.level := 0; Scope(def, ident);
			global.hideHooks := h;
			def.Append(buf);
			IF def.Length() > 0 THEN
				v := TextViews.dir.New(def);
				v.SetDefaults(NewRuler(), TextViews.dir.defAttr);
				p.w := Views.undefined; p.h := Views.undefined; Views.HandlePropMsg(v, p);
				title := mod$; Append(title, "."); Append(title, ident);
				view := Documents.dir.New(v, p.w, p.h)
			ELSE str := mod$; str1 := ident$; title := "";
				IF global.extensioninterface THEN
					Dialog.ShowParamMsg(noSuchExtItemExportedKey, str, str1, "")
				ELSE
					Dialog.ShowParamMsg(noSuchItemExportedKey, str, str1, "")
				END
			END
		ELSE
			global.level := 1; Scope(def, "");
			Section(global.out, "END"); Char(" "); String(mod); Char("."); Ln;
			def.Append(buf);
			Section(global.out, "DEFINITION"); Char(" "); String(mod);
			IF (DevCPT.GlbMod[1].library # NIL) THEN
				String(' ["'); String(DevCPT.GlbMod[1].library^); String('"]')
			ELSIF DevCPT.impProc # 0 THEN
				global.out.WriteTab; String("(* nonportable");
				IF DevCPT.impProc = 10 THEN String(" (i386)")
				ELSIF DevCPT.impProc = 20 THEN String(" (68k)")
				END;
				String(" *)")
			END;
			Char(";"); Ln; Ln;
			Modules;
			buf.Append(def);
			v := TextViews.dir.New(buf);
			v.SetDefaults(NewRuler(), TextViews.dir.defAttr);
			title := mod$;
			view := Documents.dir.New(v, width, height)
		END;
		global.out.ConnectTo(NIL);
		global.keyAttr := NIL; global.sectAttr := NIL
	END Browse;

	PROCEDURE ShowInterface* (opts: ARRAY OF CHAR);
		VAR noerr: BOOLEAN; mod, ident: DevCPT.Name; v: Views.View; title: Views.Title; t: TextModels.Model;
			str: ARRAY 256 OF CHAR;
	BEGIN
		GetQualIdent(mod, ident, t);
		CheckModName(mod, t);
		IF mod # "" THEN
			OpenCompiler(mod, noerr);
			IF noerr & (DevCPT.GlbMod[1] # NIL) & (DevCPT.GlbMod[1].name^ = mod) THEN
				Browse(ident, opts, v, title);
				IF v # NIL THEN
					IF global.clientinterface THEN Append(title, " (client interface)")
					ELSIF global.extensioninterface THEN Append(title, " (extension interface)")
					END;
					Views.OpenAux(v, title)
				END
			ELSE str := mod$; Dialog.ShowParamMsg(modNotFoundKey, str, "", "")
			END;
			CloseCompiler
		ELSE Dialog.ShowMsg(noModuleNameSelectedKey)
		END;
		Kernel.Cleanup
	END ShowInterface;
	
	PROCEDURE ImportSymFile* (f: Files.File; OUT s: Stores.Store);
		VAR v: Views.View; title: Views.Title; noerr: BOOLEAN;
	BEGIN
		ASSERT(f # NIL, 20);
		DevCPM.file := f;
		OpenCompiler("@file", noerr);
		IF noerr THEN
			Browse("", "", v, title);
			s := v
		END;
		CloseCompiler;
		DevCPM.file := NIL;
		Kernel.Cleanup
	END ImportSymFile;
	
	
	(* codefile browser *)

	PROCEDURE RWord (VAR x: INTEGER);
		VAR b: BYTE; y: INTEGER;
	BEGIN
		inp.ReadByte(b); y := b MOD 256;
		inp.ReadByte(b); y := y + 100H * (b MOD 256);
		inp.ReadByte(b); y := y + 10000H * (b MOD 256);
		inp.ReadByte(b); x := y + 1000000H * b
	END RWord;
	
	PROCEDURE RNum (VAR x: INTEGER);
		VAR b: BYTE; s, y: INTEGER;
	BEGIN
		s := 0; y := 0; inp.ReadByte(b);
		WHILE b < 0 DO INC(y, ASH(b + 128, s)); INC(s, 7); inp.ReadByte(b) END;
		x := ASH((b + 64) MOD 128 - 64, s) + y
	END RNum;
	
	PROCEDURE RName (VAR name: ARRAY OF SHORTCHAR);
		VAR b: BYTE; i, n: INTEGER;
	BEGIN
		i := 0; n := LEN(name) - 1; inp.ReadByte(b);
		WHILE (i < n) & (b # 0) DO name[i] := SHORT(CHR(b)); INC(i); inp.ReadByte(b) END;
		WHILE b # 0 DO inp.ReadByte(b) END;
		name[i] := 0X
	END RName;
	
	PROCEDURE RLink;
		VAR x: INTEGER;
	BEGIN
		RNum(x);
		WHILE x # 0 DO RNum(x); RNum(x) END
	END RLink;
	
	PROCEDURE RShort (p: INTEGER; OUT x: INTEGER);
		VAR b0, b1: BYTE;
	BEGIN
		inp.ReadByte(b0); inp.ReadByte(b1);
		IF p = 10 THEN x := b0 MOD 256 + b1 * 256	(* little endian *)
		ELSE x := b1 MOD 256 + b0 * 256	(* big endian *)
		END
	END RShort;

	PROCEDURE ReadHeader (file: Files.File; VAR view: Views.View; VAR title: Views.Title);
		VAR n, i, p, fp, hs, ms, ds, cs, vs, m: INTEGER; name: Kernel.Name; first: BOOLEAN;
			text: TextModels.Model; v: TextViews.View; fold: StdFolds.Fold; d: Dates.Date; t: Dates.Time;
			str: ARRAY 64 OF CHAR;
	BEGIN
		text := TextModels.dir.New(); global.out.ConnectTo(text);
		inp := file.NewReader(NIL);
		inp.SetPos(0); RWord(n);
		IF n = 6F4F4346H THEN
			RWord(p); RWord(hs); RWord(ms); RWord(ds); RWord(cs); RWord(vs);
			RNum(n); RName(name); title := name$; i := 0;
			WHILE i < n DO RName(imp[i]); INC(i) END;
			String("MODULE "); String(name); Ln;
			String("processor: ");
			IF p = 10 THEN String("80x86")
			ELSIF p = 20 THEN String("68000")
			ELSIF p = 30 THEN String("PPC")
			ELSIF p = 40 THEN String("SH3")
			ELSE Int(p)
			END;
			Ln;
			String("meta size: "); Int(ms); Ln;
			String("desc size: "); Int(ds); Ln;
			String("code size: "); Int(cs); Ln;
			String("data size: "); Int(vs); Ln;
			inp.SetPos(hs + ms + 12);
			RShort(p, d.year); RShort(p, d.month); RShort(p, d.day);
			RShort(p, t.hour); RShort(p, t.minute); RShort(p, t.second);
			String("compiled: ");
			Dates.DateToString(d, Dates.short, str); global.out.WriteString(str); String("  ");
			Dates.TimeToString(t, str); global.out.WriteString(str); Ln;
			Int(n); String(" imports:"); Ln;
			inp.SetPos(hs + ms + ds + cs);
			RLink; RLink; RLink; RLink; RLink; RLink; i := 0;
			WHILE i < n DO
				global.out.WriteTab; String(imp[i]); global.out.WriteTab;
				RNum(p);
				IF p # 0 THEN
					fold := StdFolds.dir.New(StdFolds.expanded, "", TextModels.dir.New());
					global.out.WriteView(fold); Char(" "); first := TRUE;
					WHILE p # 0 DO
						RName(name); RNum(fp);
						IF p = 2 THEN RNum(m) END;
						IF p # 1 THEN RLink END;
						IF name # "" THEN
							IF ~first THEN String(", ") END;
							first := FALSE; String(name)
						END;
						RNum(p)
					END;
					Char(" ");
					fold := StdFolds.dir.New(StdFolds.expanded, "", NIL);
					global.out.WriteView(fold);
					fold.Flip(); global.out.SetPos(text.Length())
				END;
				INC(i); Ln
			END
		ELSE String("wrong file tag: "); Hex(n, 8)
		END;
		v := TextViews.dir.New(text);
		v.SetDefaults(NewRuler(), TextViews.dir.defAttr);
		view := Documents.dir.New(v, width, height);
		global.out.ConnectTo(NIL);
		inp := NIL
	END ReadHeader;
	
	PROCEDURE ShowCodeFile*;
		VAR t: TextModels.Model; f: Files.File; v: Views.View; title: Views.Title; mod, ident: DevCPT.Name;
			name: Files.Name; loc: Files.Locator; str: ARRAY 256 OF CHAR;
	BEGIN
		GetQualIdent(mod, ident, t);
		IF mod # "" THEN
			str := mod$;
			Utils.GetSubLoc(str, Utils.OFdir, loc, name);
			f := Files.dir.Old(loc, name, Files.shared);
			IF f # NIL THEN
				ReadHeader(f, v, title);
				IF v # NIL THEN Views.OpenAux(v, title) END
			ELSE Dialog.ShowParamMsg(modNotFoundKey, str, "", "")
			END
		ELSE Dialog.ShowMsg(noModuleNameSelectedKey)
		END
	END ShowCodeFile;
	
	PROCEDURE ImportCodeFile* (f: Files.File; OUT s: Stores.Store);
		VAR v: Views.View; title: Views.Title;
	BEGIN
		ASSERT(f # NIL, 20);
		ReadHeader(f, v, title);
		s := v
	END ImportCodeFile;

	PROCEDURE SaveOptions*;
	BEGIN
		HostRegistry.WriteBool(regKey + 'hints', options.hints);
		HostRegistry.WriteBool(regKey + 'flatten', options.flatten);
		HostRegistry.WriteBool(regKey + 'formatted', options.formatted);
		options.sflatten := options.flatten;
		options.shints := options.hints;
		options.sformatted := options.formatted
	END SaveOptions;

	PROCEDURE SaveGuard*(VAR par: Dialog.Par);
	BEGIN
		par.disabled := (options.sflatten = options.flatten)
		& (options.shints = options.hints)
		& (options.sformatted = options.formatted)
	END SaveGuard;

	PROCEDURE Init*;
		VAR res: INTEGER;
	BEGIN
		HostRegistry.ReadBool(regKey + 'hints', options.hints, res);
		IF res # 0 THEN options.hints := FALSE END;
		HostRegistry.ReadBool(regKey + 'flatten', options.flatten, res);
		IF res # 0 THEN options.flatten := TRUE END;
		HostRegistry.ReadBool(regKey + 'formatted', options.formatted, res);
		IF res # 0 THEN options.formatted := FALSE END;
		Dialog.Update(options);
		options.sflatten := options.flatten;
		options.shints := options.hints;
		options.sformatted := options.formatted
	END Init;
	
BEGIN
	Init
END DevBrowser.
