MODULE DevCPL486;
(**
	project	= "BlackBox"
	organization	= "www.oberon.ch"
	contributors	= "Oberon microsystems"
	version	= "System/Rsrc/About"
	copyright	= "System/Rsrc/About"
	license	= "Docu/BB-License"
	references	= "ftp://ftp.inf.ethz.ch/pub/software/Oberon/OberonV4/Docu/OP2.Paper.ps"
	changes	= ""
	issues	= ""

**)

	IMPORT DevCPM, DevCPT, DevCPE;
	
	TYPE
		Item* = RECORD
			mode*,  tmode*, form*: BYTE;
			offset*, index*, reg*, scale*: INTEGER; (* adr = offset + index * scale *)
			typ*: DevCPT.Struct;
			obj*: DevCPT.Object
		END ;
		
(* Items:

	 mode	| offset	index		scale		reg     obj
------------------------------------------------
 1 Var	 | adr		 xreg		 scale					  obj  (ea = FP + adr + xreg * scale)
 2 VarPar| off     xreg     scale            obj  (ea = [FP + obj.adr] + off + xreg * scale)
 3 Con	 | val              (val2)           NIL
   Con   | off                               obj  (val = adr(obj) + off)
	 Con	 | id														    NIL  (for predefined reals)
 6 LProc |                                   obj
 7 XProc |                                   obj
 9 CProc |                                   obj						
10 IProc |                                   obj
13 TProc | mthno						0/1		          obj	(0 = normal / 1 = super call)
14 Ind	 | off		 xreg		 scale		Reg	      	(ea = Reg + off + xreg * scale)
15 Abs	 | adr		 xreg		 scale				    NIL  (ea = adr + xreg * scale) 
   Abs	 | off		 xreg		 scale				    obj  (ea = adr(obj) + off + xreg * scale)
   Abs   | off     len      0                obj  (for constant strings and reals)
16 Stk	 |																      	(ea = ESP)
17 Cond	| CC
18 Reg	 |				(Reg2)						 Reg
19 DInd	| off		 xreg		 scale		Reg	      	(ea = [Reg + off + xreg * scale])

	tmode	| record tag     array desc
-------------------------------------
  VarPar | [FP + obj.adr + 4]  [FP + obj.adr]
  Ind    | [Reg - 4]         [Reg + 8]
  Con    | Adr(typ.strobj)

*)

	CONST
		processor* = 10; (* for i386 *)
		NewLbl* = 0;

	TYPE
		Label* = INTEGER; (* 0: unassigned, > 0: address, < 0: - (linkadr + linktype * 2^24) *)
		
	VAR
		level*: BYTE;
		one*: DevCPT.Const;

	CONST
		(* item base modes (=object modes) *)
		Var = 1; VarPar = 2; Con = 3; LProc = 6; XProc = 7; CProc = 9; IProc = 10; TProc = 13;
	
		(* item modes for i386 (must not overlap item basemodes,	> 13) *)
		Ind = 14; Abs = 15; Stk = 16; Cond = 17; Reg = 18; DInd = 19;
	
		(* structure forms *)
		Undef = 0; Byte = 1; Bool = 2; Char8 = 3; Int8 = 4; Int16 = 5; Int32 = 6;
		Real32 = 7; Real64 = 8; Set = 9; String8 = 10; NilTyp = 11; NoTyp = 12;
		Pointer = 13; ProcTyp = 14; Comp = 15;
		Char16 = 16; String16 = 17; Int64 = 18; Guid = 23;
		
		(* composite structure forms *)
		Basic = 1; Array = 2; DynArr = 3; Record = 4;
		
		(* condition codes *)
		ccB = 2; ccAE = 3; ccBE = 6; ccA = 7; (* unsigned *)
		ccL = 12; ccGE = 13; ccLE = 14; ccG = 15; (* signed *)
		ccE = 4; ccNE = 5; ccS = 8; ccNS = 9; ccO = 0; ccNO = 1;
		ccAlways = -1; ccNever = -2; ccCall = -3;
		
		(* registers *)
		AX = 0; CX = 1; DX = 2; BX = 3; SP = 4; BP = 5; SI = 6; DI = 7; AH = 4; CH = 5; DH = 6; BH = 7;
		
		(* fixup types *)
		absolute = 100; relative = 101; copy = 102; table = 103; tableend = 104; short = 105;
		
		(* system trap numbers *)
		withTrap = -1; caseTrap = -2; funcTrap = -3; typTrap = -4;
		recTrap = -5; ranTrap = -6; inxTrap = -7; copyTrap = -8;
		

	VAR
		Size: ARRAY 32 OF INTEGER;	(* Size[typ.form] == +/- typ.size *)
		a1, a2: Item;


	PROCEDURE MakeReg* (VAR x: Item; reg: INTEGER; form: BYTE);
	BEGIN
		ASSERT((reg >= 0) & (reg < 8));
		x.mode := Reg; x.reg := reg; x.form := form
	END MakeReg;
	
	PROCEDURE MakeConst* (VAR x: Item; val: INTEGER; form: BYTE);
	BEGIN
		x.mode := Con; x.offset := val; x.form := form; x.obj := NIL;
	END MakeConst;

	PROCEDURE AllocConst* (VAR x: Item; con: DevCPT.Const; form: BYTE);
		VAR r: REAL; short: SHORTREAL; c: DevCPT.Const; i: INTEGER;
	BEGIN
		IF form IN {Real32, Real64} THEN
			r := con.realval;
			IF ABS(r) <= MAX(SHORTREAL) THEN
				short := SHORT(r);
				IF short = r THEN form := Real32	(* a shortreal can represent the exact value *)
				ELSE form := Real64	(* use a real *)
				END
			ELSE form := Real64	(* use a real *)
			END
		ELSIF form IN {String8, String16, Guid} THEN
			x.index := con.intval2	(* string length *)
		END;
		DevCPE.AllocConst(con, form, x.obj, x.offset);
		x.form := form; x.mode := Abs; x.scale := 0
	END AllocConst;

	(*******************************************************)
	
	PROCEDURE BegStat*; (* general-purpose procedure which is called before each statement *)
	BEGIN
	END BegStat;

	PROCEDURE EndStat*; (* general-purpose procedure which is called after each statement *)
	BEGIN
	END EndStat;

	(*******************************************************)
	
	PROCEDURE SetLabel* (VAR L: Label);
		VAR link, typ, disp, x: INTEGER; c: SHORTCHAR;
	BEGIN
		ASSERT(L <= 0); link := -L;
		WHILE link # 0 DO
			typ := link DIV 1000000H; link := link MOD 1000000H;
			IF typ = short THEN
				disp := DevCPE.pc - link - 1; ASSERT(disp < 128);
				DevCPE.PutByte(link, disp); link := 0
			ELSIF typ = relative THEN
				x := DevCPE.ThisWord(link); DevCPE.PutWord(link, DevCPE.pc - link - 4); link := x
			ELSE
				x := DevCPE.ThisWord(link); DevCPE.PutWord(link, DevCPE.pc + typ * 1000000H); link := x
			END
		END;		
		L := DevCPE.pc;
		a1.mode := 0; a2.mode := 0
	END SetLabel;
	

	(*******************************************************)
	
	PROCEDURE GenWord (x: INTEGER);
	BEGIN
		DevCPE.GenByte(x); DevCPE.GenByte(x DIV 256)
	END GenWord;

	PROCEDURE GenDbl (x: INTEGER);
	BEGIN
		DevCPE.GenByte(x); DevCPE.GenByte(x DIV 256); DevCPE.GenByte(x DIV 10000H); DevCPE.GenByte(x DIV 1000000H)
	END GenDbl;
	
	PROCEDURE CaseEntry* (tab, from, to: INTEGER);
		VAR a, e: INTEGER;
	BEGIN
		a := tab + 4 * from; e := tab + 4 * to;
		WHILE a <= e DO
			DevCPE.PutByte(a, DevCPE.pc);
			DevCPE.PutByte(a + 1, DevCPE.pc DIV 256);
			DevCPE.PutByte(a + 2, DevCPE.pc DIV 65536);
			INC(a, 4)
		END;
		a1.mode := 0; a2.mode := 0
	END CaseEntry;

	PROCEDURE GenLinked (VAR x: Item; type: BYTE);
		VAR link: DevCPT.LinkList;
	BEGIN
		IF x.obj = NIL THEN GenDbl(x.offset)
		ELSE
			link := DevCPE.OffsetLink(x.obj, x.offset);
			IF link # NIL THEN
				GenDbl(type * 1000000H + link.linkadr MOD 1000000H);
				link.linkadr := DevCPE.pc - 4
			ELSE GenDbl(0)
			END
		END
	END GenLinked;
	
	PROCEDURE CheckSize (form: BYTE; VAR w: INTEGER);
	BEGIN
		IF form IN {Int16, Char16} THEN DevCPE.GenByte(66H); w := 1
		ELSIF form >= Int32 THEN ASSERT(form IN {Int32, Set, NilTyp, Pointer, ProcTyp}); w := 1
		ELSE w := 0
		END
	END CheckSize;
	
	PROCEDURE CheckForm (form: BYTE; VAR mf: INTEGER);
	BEGIN
		IF form = Real32 THEN mf := 0
		ELSIF form = Real64 THEN mf := 4
		ELSIF form = Int32 THEN mf := 2
		ELSE ASSERT(form = Int16); mf := 6
		END
	END CheckForm;
	
	PROCEDURE CheckConst (VAR x: Item; VAR s: INTEGER);
	BEGIN
		IF (x.form > Int8) & (x.offset >= -128) & (x.offset < 128) & (x.obj = NIL) THEN s := 2
		ELSE s := 0
		END 
	END CheckConst;
	
	PROCEDURE GenConst (VAR x: Item; short: BOOLEAN);
	BEGIN
		IF x.obj # NIL THEN GenLinked(x, absolute)
		ELSIF x.form <= Int8 THEN DevCPE.GenByte(x.offset)
		ELSIF short & (x.offset >= -128) & (x.offset < 128) THEN DevCPE.GenByte(x.offset)
		ELSIF x.form IN {Int16, Char16} THEN GenWord(x.offset)
		ELSE GenDbl(x.offset)
		END
	END GenConst;
	
	PROCEDURE GenCExt (code: INTEGER; VAR x: Item);
		VAR disp, mod, base, scale: INTEGER;
	BEGIN
		ASSERT(x.mode IN {Reg, Ind, Abs, Stk});
		ASSERT((code MOD 8 = 0) & (code < 64));
		disp := x.offset; base := x.reg; scale := x.scale;
		IF x.mode = Reg THEN mod := 0C0H; scale := 0
		ELSIF x.mode = Stk THEN base := SP; mod := 0; disp := 0; scale := 0
		ELSIF x.mode = Abs THEN
			IF scale = 1 THEN base := x.index; mod := 80H; scale := 0
			ELSE base := BP; mod := 0
			END
		ELSIF (disp = 0) & (base # BP) THEN mod := 0
		ELSIF (disp >= -128) & (disp < 128) THEN mod := 40H
		ELSE mod := 80H
		END;
		IF scale # 0 THEN
			DevCPE.GenByte(mod + code + 4); base := base + x.index * 8;
			IF scale = 8 THEN DevCPE.GenByte(0C0H + base);
			ELSIF scale = 4 THEN DevCPE.GenByte(80H + base);
			ELSIF scale = 2 THEN DevCPE.GenByte(40H + base);
			ELSE ASSERT(scale = 1); DevCPE.GenByte(base);
			END;
		ELSE
			DevCPE.GenByte(mod + code + base);
			IF (base = SP) & (mod <= 80H) THEN DevCPE.GenByte(24H) END
		END;
		IF x.mode = Abs THEN GenLinked(x, absolute)
		ELSIF mod = 80H THEN GenDbl(disp)
		ELSIF mod = 40H THEN DevCPE.GenByte(disp)
		END
	END GenCExt;
	
	PROCEDURE GenDExt (VAR r, x: Item);
	BEGIN
		ASSERT(r.mode = Reg);
		GenCExt(r.reg * 8, x)
	END GenDExt;
	
	(*******************************************************)
	
	PROCEDURE GenMove* (VAR from, to: Item);
		VAR w: INTEGER;
	BEGIN
		ASSERT(Size[from.form] = Size[to.form]);
		IF to.mode = Reg THEN
			IF from.mode = Con THEN
				IF to.reg = AX THEN

					IF (a1.mode = Con) & (from.offset = a1.offset) & (from.obj = a1.obj) & (from.form = a1.form) THEN
						RETURN
					END;

					a1 := from; a2.mode := 0
				END;
				CheckSize(from.form, w);
				IF (from.offset = 0) & (from.obj = NIL) THEN
					DevCPE.GenByte(30H + w); DevCPE.GenByte(0C0H + 9 * to.reg) (* XOR r,r *)
				ELSE
					DevCPE.GenByte(0B0H + w * 8 + to.reg); GenConst(from, FALSE)
				END;
			ELSIF (to.reg = AX) & (from.mode = Abs) & (from.scale = 0) THEN

				IF (a1.mode = Abs) & (from.offset = a1.offset) & (from.obj = a1.obj) & (from.form = a1.form)
					OR (a2.mode = Abs) & (from.offset = a2.offset) & (from.obj = a2.obj) & (from.form = a2.form) THEN
					RETURN
				END;

				a1 := from; a2.mode := 0;
				CheckSize(from.form, w);
				DevCPE.GenByte(0A0H + w); GenLinked(from, absolute);
			ELSIF (from.mode # Reg) OR (from.reg # to.reg) THEN
				IF to.reg = AX THEN
					IF (from.mode = Ind) & (from.scale = 0) & ((from.reg = BP) OR (from.reg = BX)) THEN

						IF (a1.mode = Ind) & (from.offset = a1.offset) & (from.reg = a1.reg) & (from.form = a1.form)
							OR (a2.mode = Ind) & (from.offset = a2.offset) & (from.reg = a2.reg) & (from.form = a2.form) THEN
							RETURN
						END;

						a1 := from
					ELSE a1.mode := 0
					END;
					a2.mode := 0
				END;
				CheckSize(from.form, w);
				DevCPE.GenByte(8AH + w); GenDExt(to, from)
			END 
		ELSE
			CheckSize(from.form, w);
			IF from.mode = Con THEN
				DevCPE.GenByte(0C6H + w); GenCExt(0, to); GenConst(from, FALSE);
				a1.mode := 0; a2.mode := 0
			ELSIF (from.reg = AX) & (to.mode = Abs) & (to.scale = 0) THEN
				DevCPE.GenByte(0A2H + w); GenLinked(to, absolute);
				a2 := to
			ELSE
				DevCPE.GenByte(88H + w); GenDExt(from, to);
				IF from.reg = AX THEN
					IF (to.mode = Ind) & (to.scale = 0) & ((to.reg = BP) OR (to.reg = BX)) THEN a2 := to END
				ELSE a1.mode := 0; a2.mode := 0
				END
			END
		END
	END GenMove;
	
	PROCEDURE GenExtMove* (VAR from, to: Item);
		VAR w, op: INTEGER;
	BEGIN
		ASSERT(from.mode # Con);
		IF from.form IN {Byte, Char8, Char16} THEN op := 0B6H (* MOVZX *)
		ELSE op := 0BEH (* MOVSX *)
		END;
		IF from.form IN {Int16, Char16} THEN INC(op) END;
		DevCPE.GenByte(0FH); DevCPE.GenByte(op); GenDExt(to, from);
		IF to.reg = AX THEN a1.mode := 0; a2.mode := 0 END
	END GenExtMove;
	
	PROCEDURE GenSignExt* (VAR from, to: Item);
	BEGIN
		ASSERT(to.mode = Reg);
		IF (from.mode = Reg) & (from.reg = AX) & (to.reg = DX) THEN
			DevCPE.GenByte(99H)	(* cdq *)
		ELSE
			GenMove(from, to);	(* mov to, from *)
			DevCPE.GenByte(0C1H); GenCExt(38H, to); DevCPE.GenByte(31)	(* sar to, 31 *)
		END
	END GenSignExt;
	
	PROCEDURE GenLoadAdr* (VAR from, to: Item);
	BEGIN
		ASSERT(to.form IN {Int32, Pointer, ProcTyp});
		IF (from.mode = Abs) & (from.scale = 0) THEN
			DevCPE.GenByte(0B8H + to.reg); GenLinked(from, absolute)
		ELSIF from.mode = Stk THEN
			DevCPE.GenByte(89H); GenCExt(SP * 8, to)
		ELSIF (from.mode # Ind) OR (from.offset # 0) OR (from.scale # 0) THEN
			DevCPE.GenByte(8DH); GenDExt(to, from)
		ELSIF from.reg # to.reg THEN
			DevCPE.GenByte(89H); GenCExt(from.reg * 8, to)
		ELSE RETURN
		END;
		IF to.reg = AX THEN a1.mode := 0; a2.mode := 0 END
	END GenLoadAdr;

	PROCEDURE GenPush* (VAR src: Item);
		VAR s: INTEGER;
	BEGIN
		IF src.mode = Con THEN
			ASSERT(src.form >= Int32);
			CheckConst(src, s); DevCPE.GenByte(68H + s); GenConst(src, TRUE)
		ELSIF src.mode = Reg THEN
			ASSERT((src.form >= Int16) OR (src.reg < 4));
			DevCPE.GenByte(50H + src.reg)
		ELSE
			ASSERT(src.form >= Int32);
			DevCPE.GenByte(0FFH); GenCExt(30H, src)
		END
	END GenPush;
	
	PROCEDURE GenPop* (VAR dst: Item);
	BEGIN
		IF dst.mode = Reg THEN
			ASSERT((dst.form >= Int16) OR (dst.reg < 4));
			DevCPE.GenByte(58H + dst.reg);
			IF dst.reg = AX THEN a1.mode := 0; a2.mode := 0 END
		ELSE
			DevCPE.GenByte(08FH); GenCExt(0, dst) 
		END
	END GenPop;
	
	PROCEDURE GenConOp (op: INTEGER; VAR src, dst: Item);
		VAR w, s: INTEGER;
	BEGIN
		ASSERT(Size[src.form] = Size[dst.form]);
		CheckSize(src.form, w);
		CheckConst(src, s);
		IF (dst.mode = Reg) & (dst.reg = AX) & (s = 0) THEN
			DevCPE.GenByte(op + 4 + w); GenConst(src, FALSE)
		ELSE
			DevCPE.GenByte(80H + s + w); GenCExt(op, dst); GenConst(src, TRUE)
		END
	END GenConOp;
	
	PROCEDURE GenDirOp (op: INTEGER; VAR src, dst: Item);
		VAR w: INTEGER;
	BEGIN
		ASSERT(Size[src.form] = Size[dst.form]);
		CheckSize(src.form, w);
		IF dst.mode = Reg THEN
			DevCPE.GenByte(op + 2 + w); GenDExt(dst, src)
		ELSE
			DevCPE.GenByte(op + w); GenDExt(src, dst)
		END
	END GenDirOp;

	PROCEDURE GenAdd* (VAR src, dst: Item; ovflchk: BOOLEAN);
		VAR w: INTEGER;
	BEGIN
		ASSERT(Size[src.form] = Size[dst.form]);
		IF src.mode = Con THEN
			IF src.obj = NIL THEN
				IF src.offset = 1 THEN
					IF (dst.mode = Reg) & (dst.form >= Int32) THEN DevCPE.GenByte(40H + dst.reg) (* inc *)
					ELSE CheckSize(dst.form, w); DevCPE.GenByte(0FEH + w); GenCExt(0, dst)
					END
				ELSIF src.offset = -1 THEN
					IF (dst.mode = Reg) & (dst.form >= Int32) THEN DevCPE.GenByte(48H + dst.reg) (* dec *)
					ELSE CheckSize(dst.form, w); DevCPE.GenByte(0FEH + w); GenCExt(8, dst)
					END
				ELSIF src.offset # 0 THEN
					GenConOp(0, src, dst)
				ELSE RETURN
				END
			ELSE
				GenConOp(0, src, dst)
			END
		ELSE
			GenDirOp(0, src, dst)
		END;
		IF ovflchk THEN DevCPE.GenByte(0CEH) END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenAdd;
	
	PROCEDURE GenAddC* (VAR src, dst: Item; first, ovflchk: BOOLEAN);
		VAR op: INTEGER;
	BEGIN
		ASSERT(Size[src.form] = Size[dst.form]);
		IF first THEN op := 0 ELSE op := 10H END;
		IF src.mode = Con THEN GenConOp(op, src, dst)
		ELSE GenDirOp(op, src, dst)
		END;
		IF ovflchk THEN DevCPE.GenByte(0CEH) END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenAddC;
	
	PROCEDURE GenSub* (VAR src, dst: Item; ovflchk: BOOLEAN);
		VAR w: INTEGER;
	BEGIN
		ASSERT(Size[src.form] = Size[dst.form]);
		IF src.mode = Con THEN
			IF src.obj = NIL THEN
				IF src.offset = 1 THEN
					IF (dst.mode = Reg) & (dst.form >= Int32) THEN DevCPE.GenByte(48H + dst.reg) (* dec *)
					ELSE CheckSize(dst.form, w); DevCPE.GenByte(0FEH + w); GenCExt(8, dst)
					END
				ELSIF src.offset = -1 THEN
					IF (dst.mode = Reg) & (dst.form >= Int32) THEN DevCPE.GenByte(40H + dst.reg) (* inc *)
					ELSE CheckSize(dst.form, w); DevCPE.GenByte(0FEH + w); GenCExt(0, dst)
					END
				ELSIF src.offset # 0 THEN
					GenConOp(28H, src, dst)
				ELSE RETURN
				END
			ELSE
				GenConOp(28H, src, dst)
			END
		ELSE
			GenDirOp(28H, src, dst)
		END;
		IF ovflchk THEN DevCPE.GenByte(0CEH) END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenSub;

	PROCEDURE GenSubC* (VAR src, dst: Item; first, ovflchk: BOOLEAN);
		VAR op: INTEGER;
	BEGIN
		ASSERT(Size[src.form] = Size[dst.form]);
		IF first THEN op := 28H ELSE op := 18H END;
		IF src.mode = Con THEN GenConOp(op, src, dst)
		ELSE GenDirOp(op, src, dst)
		END;
		IF ovflchk THEN DevCPE.GenByte(0CEH) END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenSubC;

	PROCEDURE GenComp* (VAR src, dst: Item);
		VAR w: INTEGER;
	BEGIN
		IF src.mode = Con THEN
			IF (src.offset = 0) & (src.obj = NIL) & (dst.mode = Reg) THEN 
				CheckSize(dst.form, w); DevCPE.GenByte(8 + w); DevCPE.GenByte(0C0H + 9 * dst.reg) (* or r,r *)
			ELSE GenConOp(38H, src, dst)
			END
		ELSE
			GenDirOp(38H, src, dst)
		END
	END GenComp;
	
	PROCEDURE GenAnd* (VAR src, dst: Item);
	BEGIN
		IF src.mode = Con THEN
			IF (src.obj # NIL) OR (src.offset # -1) THEN GenConOp(20H, src, dst) END
		ELSE GenDirOp(20H, src, dst)
		END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenAnd;
	
	PROCEDURE GenOr* (VAR src, dst: Item);
	BEGIN
		IF src.mode = Con THEN
			IF (src.obj # NIL) OR (src.offset # 0) THEN GenConOp(8H, src, dst) END
		ELSE GenDirOp(8H, src, dst)
		END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenOr;
	
	PROCEDURE GenXor* (VAR src, dst: Item);
	BEGIN
		IF src.mode = Con THEN
			IF (src.obj # NIL) OR (src.offset # 0) THEN GenConOp(30H, src, dst) END
		ELSE GenDirOp(30H, src, dst)
		END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenXor;
	
	PROCEDURE GenTest* (VAR x, y: Item);
		VAR w: INTEGER;
	BEGIN
		ASSERT(Size[x.form] = Size[y.form]);
		CheckSize(x.form, w);
		IF x.mode = Con THEN
			IF (x.mode = Reg) & (x.reg = AX) THEN
				DevCPE.GenByte(0A8H + w); GenConst(x, FALSE)
			ELSE
				DevCPE.GenByte(0F6H + w); GenCExt(0, y); GenConst(x, FALSE)
			END
		ELSE
			DevCPE.GenByte(84H + w);
			IF y.mode = Reg THEN GenDExt(y, x) ELSE GenDExt(x, y) END
		END
	END GenTest;
	
	PROCEDURE GenNeg* (VAR dst: Item; ovflchk: BOOLEAN);
		VAR w: INTEGER;
	BEGIN
		CheckSize(dst.form, w); DevCPE.GenByte(0F6H + w); GenCExt(18H, dst);
		IF ovflchk THEN DevCPE.GenByte(0CEH) END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenNeg;
	
	PROCEDURE GenNot* (VAR dst: Item);
		VAR w: INTEGER;
	BEGIN
		CheckSize(dst.form, w); DevCPE.GenByte(0F6H + w); GenCExt(10H, dst);
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenNot;
	
	PROCEDURE GenMul* (VAR src, dst: Item; ovflchk: BOOLEAN);
		VAR w, s, val, f2, f5, f9: INTEGER;
	BEGIN
		ASSERT((dst.mode = Reg) & (Size[src.form] = Size[dst.form]));
		IF (src.mode = Con) & (src.offset = 1) THEN RETURN END;
		IF src.form <= Int8 THEN
			ASSERT(dst.reg = 0);
			DevCPE.GenByte(0F6H); GenCExt(28H, src)
		ELSIF src.mode = Con THEN
			val := src.offset;
			IF (src.obj = NIL) & (val # 0) & ~ovflchk THEN
				f2 := 0; f5 := 0; f9 := 0;
				WHILE ~ODD(val) DO val := val DIV 2; INC(f2) END;
				WHILE val MOD 9 = 0 DO val := val DIV 9; INC(f9) END;
				WHILE val MOD 5 = 0 DO val := val DIV 5; INC(f5) END;
				IF ABS(val) <= 3 THEN
					WHILE f9 > 0 DO
						DevCPE.GenByte(8DH);
						DevCPE.GenByte(dst.reg * 8 + 4);
						DevCPE.GenByte(0C0H + dst.reg * 9);
						DEC(f9)
					END;
					WHILE f5 > 0 DO
						DevCPE.GenByte(8DH);
						DevCPE.GenByte(dst.reg * 8 + 4);
						DevCPE.GenByte(80H + dst.reg * 9);
						DEC(f5)
					END;
					IF ABS(val) = 3 THEN
						DevCPE.GenByte(8DH); DevCPE.GenByte(dst.reg * 8 + 4); DevCPE.GenByte(40H + dst.reg * 9)
					END;
					IF f2 > 1 THEN DevCPE.GenByte(0C1H); DevCPE.GenByte(0E0H + dst.reg); DevCPE.GenByte(f2)
					ELSIF f2 = 1 THEN DevCPE.GenByte(1); DevCPE.GenByte(0C0H + dst.reg * 9)
					END;
					IF val < 0 THEN DevCPE.GenByte(0F7H); GenCExt(18H, dst) END;
					IF dst.reg = AX THEN a1.mode := 0; a2.mode := 0 END;
					RETURN
				END
			END;
			CheckSize(src.form, w); CheckConst(src, s);
			DevCPE.GenByte(69H + s); GenDExt(dst, dst); GenConst(src, TRUE)
		ELSE
			CheckSize(src.form, w);
			DevCPE.GenByte(0FH); DevCPE.GenByte(0AFH); GenDExt(dst, src)
		END;
		IF ovflchk THEN DevCPE.GenByte(0CEH) END;
		IF dst.reg = AX THEN a1.mode := 0; a2.mode := 0 END
	END GenMul;
	
	PROCEDURE GenDiv* (VAR src: Item; mod, pos: BOOLEAN);
		VAR w, rem: INTEGER;
	BEGIN
		ASSERT(src.mode = Reg);
		IF src.form >= Int32 THEN DevCPE.GenByte(99H) (* cdq *)
		ELSIF src.form = Int16 THEN DevCPE.GenByte(66H); DevCPE.GenByte(99H) (* cwd *)
		ELSE DevCPE.GenByte(66H); DevCPE.GenByte(98H) (* cbw *)
		END;
		CheckSize(src.form, w); DevCPE.GenByte(0F6H + w); GenCExt(38H, src); (* idiv src *)
		IF src.form > Int8 THEN rem := 2 (* edx *) ELSE rem := 4 (* ah *) END;
		IF pos THEN (* src > 0 *)
			CheckSize(src.form, w); DevCPE.GenByte(8 + w); DevCPE.GenByte(0C0H + 9 * rem); (* or rem,rem *)
			IF mod THEN
				DevCPE.GenByte(79H); DevCPE.GenByte(2);	(* jns end *)
				DevCPE.GenByte(2 + w); GenCExt(8 * rem, src); (* add rem,src *)
			ELSE
				DevCPE.GenByte(79H); DevCPE.GenByte(1);	(* jns end *)
				DevCPE.GenByte(48H);					(* dec eax *)
			END
		ELSE
			CheckSize(src.form, w); DevCPE.GenByte(30H + w); GenCExt(8 * rem, src); (* xor src,rem *)
			IF mod THEN
				DevCPE.GenByte(79H);	(* jns end *)
				IF src.form = Int16 THEN DevCPE.GenByte(9); DevCPE.GenByte(66H) ELSE DevCPE.GenByte(8) END;
				DevCPE.GenByte(8 + w); DevCPE.GenByte(0C0H + 9 * rem); (* or rem,rem *)
				DevCPE.GenByte(74H); DevCPE.GenByte(4);	(* je end *)
				DevCPE.GenByte(30H + w); GenCExt(8 * rem, src); (* xor src,rem *)
				DevCPE.GenByte(2 + w); GenCExt(8 * rem, src); (* add rem,src *)
			ELSE
				DevCPE.GenByte(79H);	(* jns end *)
				IF src.form = Int16 THEN DevCPE.GenByte(6); DevCPE.GenByte(66H) ELSE DevCPE.GenByte(5) END;
				DevCPE.GenByte(8 + w); DevCPE.GenByte(0C0H + 9 * rem); (* or rem,rem *)
				DevCPE.GenByte(74H); DevCPE.GenByte(1);	(* je end *)
				DevCPE.GenByte(48H);					(* dec eax *)
			END
(*
			CheckSize(src.form, w); DevCPE.GenByte(3AH + w); GenCExt(8 * rem, src); (* cmp rem,src *)
			IF mod THEN
				DevCPE.GenByte(72H); DevCPE.GenByte(4);	(* jb end *)
				DevCPE.GenByte(7FH); DevCPE.GenByte(2);	(* jg end *)
				DevCPE.GenByte(2 + w); GenCExt(8 * rem, src); (* add rem,src *)
			ELSE
				DevCPE.GenByte(72H); DevCPE.GenByte(3);	(* jb end *)
				DevCPE.GenByte(7FH); DevCPE.GenByte(1);	(* jg end *)
				DevCPE.GenByte(48H);					(* dec eax *)
			END
*)
		END;
		a1.mode := 0; a2.mode := 0
	END GenDiv;

	PROCEDURE GenShiftOp* (op: INTEGER; VAR cnt, dst: Item);
		VAR w: INTEGER;
	BEGIN
		CheckSize(dst.form, w);
		IF cnt.mode = Con THEN
			ASSERT(cnt.offset >= 0); ASSERT(cnt.obj = NIL);
			IF cnt.offset = 1 THEN
				IF (op = 10H) & (dst.mode = Reg) THEN (* shl r *)
					DevCPE.GenByte(w); GenDExt(dst, dst) (* add r, r *)
				ELSE
					DevCPE.GenByte(0D0H + w); GenCExt(op, dst)
				END
			ELSIF cnt.offset > 1 THEN
				DevCPE.GenByte(0C0H + w); GenCExt(op, dst); DevCPE.GenByte(cnt.offset)
			END
		ELSE
			ASSERT((cnt.mode = Reg) & (cnt.reg = CX));
			DevCPE.GenByte(0D2H + w); GenCExt(op, dst)
		END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenShiftOp;
	
	PROCEDURE GenBitOp* (op: INTEGER; VAR num, dst: Item);
	BEGIN
		DevCPE.GenByte(0FH);
		IF num.mode = Con THEN
			ASSERT(num.obj = NIL);
			DevCPE.GenByte(0BAH); GenCExt(op, dst); DevCPE.GenByte(num.offset)
		ELSE
			ASSERT((num.mode = Reg) & (num.form = Int32));
			DevCPE.GenByte(83H + op); GenDExt(num, dst)
		END;
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenBitOp;
	
	PROCEDURE GenSetCC* (cc: INTEGER; VAR dst: Item);
	BEGIN
		ASSERT((dst.form = Bool) & (cc >= 0));
		DevCPE.GenByte(0FH); DevCPE.GenByte(90H + cc); GenCExt(0, dst);
		IF (dst.mode # Reg) OR (dst.reg = AX) THEN a1.mode := 0; a2.mode := 0 END
	END GenSetCC;
	
	PROCEDURE GenFLoad* (VAR src: Item);
		VAR mf: INTEGER;
	BEGIN
		IF src.mode = Con THEN (* predefined constants *)
			DevCPE.GenByte(0D9H); DevCPE.GenByte(0E8H + src.offset)
		ELSIF src.form = Int64 THEN
			DevCPE.GenByte(0DFH); GenCExt(28H, src)
		ELSE
			CheckForm(src.form, mf);
			DevCPE.GenByte(0D9H + mf); GenCExt(0, src)
		END
	END GenFLoad;
	
	PROCEDURE GenFStore* (VAR dst: Item; pop: BOOLEAN);
		VAR mf: INTEGER;
	BEGIN
		IF dst.form = Int64 THEN ASSERT(pop);
			DevCPE.GenByte(0DFH); GenCExt(38H, dst); DevCPE.GenByte(9BH)	(* wait *)
		ELSE
			CheckForm(dst.form, mf); DevCPE.GenByte(0D9H + mf);
			IF pop THEN GenCExt(18H, dst); DevCPE.GenByte(9BH)	(* wait *)
			ELSE GenCExt(10H, dst)
			END
		END;
		a1.mode := 0; a2.mode := 0
	END GenFStore;
	
	PROCEDURE GenFDOp* (op: INTEGER; VAR src: Item);
		VAR mf: INTEGER;
	BEGIN
		IF src.mode = Reg THEN
			DevCPE.GenByte(0DEH); DevCPE.GenByte(0C1H + op)
		ELSE
			CheckForm(src.form, mf);
			DevCPE.GenByte(0D8H + mf); GenCExt(op, src)
		END
	END GenFDOp;
	
	PROCEDURE GenFMOp* (op: INTEGER);
	BEGIN
		DevCPE.GenByte(0D8H + op DIV 256);
		DevCPE.GenByte(op MOD 256);
		IF op = 07E0H THEN a1.mode := 0; a2.mode := 0 END	(* FSTSW AX *)
	END GenFMOp;
	
	PROCEDURE GenJump* (cc: INTEGER; VAR L: Label; shortjmp: BOOLEAN);
	BEGIN
		IF cc # ccNever THEN
			IF shortjmp OR (L > 0) & (DevCPE.pc + 2 - L <= 128) & (cc # ccCall) THEN
				IF cc = ccAlways THEN DevCPE.GenByte(0EBH)
				ELSE DevCPE.GenByte(70H + cc)
				END;
				IF L > 0 THEN DevCPE.GenByte(L - DevCPE.pc - 1)
				ELSE ASSERT(L = 0); L := -(DevCPE.pc + short * 1000000H); DevCPE.GenByte(0)
				END
			ELSE
				IF cc = ccAlways THEN DevCPE.GenByte(0E9H)
				ELSIF cc = ccCall THEN DevCPE.GenByte(0E8H)
				ELSE DevCPE.GenByte(0FH); DevCPE.GenByte(80H + cc)
				END;
				IF L > 0 THEN GenDbl(L - DevCPE.pc - 4)
				ELSE GenDbl(-L); L := -(DevCPE.pc - 4 + relative * 1000000H)
				END
			END
		END
	END GenJump;
	
	PROCEDURE GenExtJump* (cc: INTEGER; VAR dst: Item);
	BEGIN
		IF cc = ccAlways THEN DevCPE.GenByte(0E9H)
		ELSE DevCPE.GenByte(0FH); DevCPE.GenByte(80H + cc)
		END;
		dst.offset := 0; GenLinked(dst, relative)
	END GenExtJump;
	
	PROCEDURE GenIndJump* (VAR dst: Item);
	BEGIN
		DevCPE.GenByte(0FFH); GenCExt(20H, dst)
	END GenIndJump;
	
	PROCEDURE GenCaseJump* (VAR src: Item);
		VAR link: DevCPT.LinkList; tab: INTEGER;
	BEGIN
		ASSERT((src.form = Int32) & (src.mode = Reg));
		DevCPE.GenByte(0FFH); DevCPE.GenByte(24H); DevCPE.GenByte(85H + 8 * src.reg);
		tab := (DevCPE.pc + 7) DIV 4 * 4;
		NEW(link); link.offset := tab; link.linkadr := DevCPE.pc;
		link.next := DevCPE.CaseLinks; DevCPE.CaseLinks := link;
		GenDbl(absolute * 1000000H + tab);
		WHILE DevCPE.pc < tab DO DevCPE.GenByte(90H) END;
	END GenCaseJump;
(*	
	PROCEDURE GenCaseJump* (VAR src: Item; num: LONGINT; VAR tab: LONGINT);
		VAR link: DevCPT.LinkList; else, last: LONGINT;
	BEGIN
		ASSERT((src.form = Int32) & (src.mode = Reg));
		DevCPE.GenByte(0FFH); DevCPE.GenByte(24H); DevCPE.GenByte(85H + 8 * src.reg);
		tab := (DevCPE.pc + 7) DIV 4 * 4;
		else := tab + num * 4; last := else - 4;
		NEW(link); link.offset := tab; link.linkadr := DevCPE.pc;
		link.next := CaseLinks; CaseLinks := link;
		GenDbl(absolute * 1000000H + tab);
		WHILE DevCPE.pc < tab DO DevCPE.GenByte(90H) END;
		WHILE DevCPE.pc < last DO GenDbl(table * 1000000H + else) END;
		GenDbl(tableend * 1000000H + else)
	END GenCaseJump;
*)	
	PROCEDURE GenCaseEntry* (VAR L: Label; last: BOOLEAN);
		VAR typ: INTEGER;
	BEGIN
		IF last THEN typ := tableend * 1000000H ELSE typ := table * 1000000H END;
		IF L > 0 THEN GenDbl(L + typ) ELSE GenDbl(-L); L := -(DevCPE.pc - 4 + typ) END
	END GenCaseEntry;
	
	PROCEDURE GenCall* (VAR dst: Item);
	BEGIN
		IF dst.mode IN {LProc, XProc, IProc} THEN
			DevCPE.GenByte(0E8H);
			IF dst.obj.mnolev >= 0 THEN (* local *)
				IF dst.obj.adr > 0 THEN GenDbl(dst.obj.adr - DevCPE.pc - 4)
				ELSE GenDbl(-dst.obj.adr); dst.obj.adr := -(DevCPE.pc - 4 + relative * 1000000H)
				END
			ELSE (* imported *)
				dst.offset := 0; GenLinked(dst, relative)
			END
		ELSE DevCPE.GenByte(0FFH); GenCExt(10H, dst)
		END;
		a1.mode := 0; a2.mode := 0
	END GenCall;
	
	PROCEDURE GenAssert* (cc, no: INTEGER);
	BEGIN
		IF cc # ccAlways THEN
			IF cc >= 0 THEN
				DevCPE.GenByte(70H + cc); (* jcc end *)
				IF no < 0 THEN DevCPE.GenByte(2) ELSE DevCPE.GenByte(3) END
			END;
			IF no < 0 THEN
				DevCPE.GenByte(8DH); DevCPE.GenByte(0E0H - no)
			ELSE
				DevCPE.GenByte(8DH); DevCPE.GenByte(0F0H); DevCPE.GenByte(no)
			END
		END
	END GenAssert;
	
	PROCEDURE GenReturn* (val: INTEGER);
	BEGIN
		IF val = 0 THEN DevCPE.GenByte(0C3H)
		ELSE DevCPE.GenByte(0C2H); GenWord(val)
		END;
		a1.mode := 0; a2.mode := 0
	END GenReturn;
	
	PROCEDURE LoadStr (size: INTEGER);
	BEGIN
		IF size = 2 THEN DevCPE.GenByte(66H) END;
		IF size <= 1 THEN DevCPE.GenByte(0ACH) ELSE DevCPE.GenByte(0ADH) END (* lods *)
	END LoadStr;
	
	PROCEDURE StoreStr (size: INTEGER);
	BEGIN
		IF size = 2 THEN DevCPE.GenByte(66H) END;
		IF size <= 1 THEN DevCPE.GenByte(0AAH) ELSE DevCPE.GenByte(0ABH) END (* stos *)
	END StoreStr;
	
	PROCEDURE ScanStr (size: INTEGER; rep: BOOLEAN);
	BEGIN
		IF size = 2 THEN DevCPE.GenByte(66H) END;
		IF rep THEN DevCPE.GenByte(0F2H) END;
		IF size <= 1 THEN DevCPE.GenByte(0AEH) ELSE DevCPE.GenByte(0AFH) END (* scas *)
	END ScanStr;
	
	PROCEDURE TestNull (size: INTEGER);
	BEGIN
		IF size = 2 THEN DevCPE.GenByte(66H) END;
		IF size <= 1 THEN DevCPE.GenByte(8); DevCPE.GenByte(0C0H); (* or al,al *)
		ELSE DevCPE.GenByte(9); DevCPE.GenByte(0C0H); (* or ax,ax *)
		END
	END TestNull;
	
	PROCEDURE GenBlockMove* (wsize, len: INTEGER);	(* len = 0: len in ECX *)
		VAR w: INTEGER;
	BEGIN
		IF len = 0 THEN (* variable size move *)
			IF wsize = 4 THEN w := 1 ELSIF wsize = 2 THEN w := 1; DevCPE.GenByte(66H) ELSE w := 0 END;
			DevCPE.GenByte(0F3H); DevCPE.GenByte(0A4H + w); (* rep:movs *)
		ELSE (* fixed size move *)
			len := len * wsize;
			IF len >= 16 THEN
				DevCPE.GenByte(0B9H); GenDbl(len DIV 4); (* ld ecx,len/4 *)
				DevCPE.GenByte(0F3H); DevCPE.GenByte(0A5H); (* rep:movs long*)
				len := len MOD 4
			END;
			WHILE len >= 4 DO DevCPE.GenByte(0A5H); DEC(len, 4) END; (* movs long *);
			IF len >= 2 THEN DevCPE.GenByte(66H); DevCPE.GenByte(0A5H) END; (* movs word *);
			IF ODD(len) THEN DevCPE.GenByte(0A4H) END; (* movs byte *)
		END
	END GenBlockMove;
	
	PROCEDURE GenBlockStore* (wsize, len: INTEGER);	(* len = 0: len in ECX *)
		VAR w: INTEGER;
	BEGIN
		IF len = 0 THEN (* variable size move *)
			IF wsize = 4 THEN w := 1 ELSIF wsize = 2 THEN w := 1; DevCPE.GenByte(66H) ELSE w := 0 END;
			DevCPE.GenByte(0F3H); DevCPE.GenByte(0AAH + w); (* rep:stos *)
		ELSE (* fixed size move *)
			len := len * wsize;
			IF len >= 16 THEN
				DevCPE.GenByte(0B9H); GenDbl(len DIV 4); (* ld ecx,len/4 *)
				DevCPE.GenByte(0F3H); DevCPE.GenByte(0ABH); (* rep:stos long*)
				len := len MOD 4
			END;
			WHILE len >= 4 DO DevCPE.GenByte(0ABH); DEC(len, 4) END; (* stos long *);
			IF len >= 2 THEN DevCPE.GenByte(66H); DevCPE.GenByte(0ABH) END; (* stos word *);
			IF ODD(len) THEN DevCPE.GenByte(0ABH) END; (* stos byte *)
		END
	END GenBlockStore;
	
	PROCEDURE GenBlockComp* (wsize, len: INTEGER);	(* len = 0: len in ECX *)
		VAR w: INTEGER;
	BEGIN
		ASSERT(len >= 0);
		IF len > 0 THEN DevCPE.GenByte(0B9H); GenDbl(len) END; (* ld ecx,len *)
		IF wsize = 4 THEN w := 1 ELSIF wsize = 2 THEN w := 1; DevCPE.GenByte(66H) ELSE w := 0 END;
		DevCPE.GenByte(0F3H); DevCPE.GenByte(0A6H + w) (* repe:cmps *)
	END GenBlockComp;
	
	PROCEDURE GenStringMove* (excl: BOOLEAN; wsize, dsize, len: INTEGER);
	(*
	len = 0: len in ECX, len = -1: len undefined; wsize # dsize -> convert; size = 0: opsize = 1, incsize = 2; excl: don't move 0X
	*)
		VAR loop, end: Label;
	BEGIN
		IF len > 0 THEN DevCPE.GenByte(0B9H); GenDbl(len) END; (* ld ecx,len *)
		(* len >= 0: len IN ECX *)
		IF (dsize = 2) & (wsize < 2) THEN DevCPE.GenByte(31H); DevCPE.GenByte(0C0H) END; (* xor eax,eax *)
		loop := NewLbl; end := NewLbl;
		SetLabel(loop); LoadStr(wsize);
		IF wsize = 0 THEN DevCPE.GenByte(46H) END; (* inc esi *)
		IF len < 0 THEN (* no limit *)
			StoreStr(dsize); TestNull(wsize); GenJump(ccNE, loop, TRUE);
			IF excl THEN (* dec edi *)
				DevCPE.GenByte(4FH);
				IF dsize # 1 THEN DevCPE.GenByte(4FH) END
			END;
		ELSE	(* cx limit *)
			IF excl THEN TestNull(wsize); GenJump(ccE, end, TRUE); StoreStr(dsize)
			ELSE StoreStr(dsize); TestNull(wsize); GenJump(ccE, end, TRUE)
			END;
			DevCPE.GenByte(49H); (* dec ecx *)
			GenJump(ccNE, loop, TRUE);
			GenAssert(ccNever, copyTrap); (* trap *)
			SetLabel(end)
		END;
		a1.mode := 0; a2.mode := 0
	END GenStringMove;
	
	PROCEDURE GenStringComp* (wsize, dsize: INTEGER);
	(* wsize # dsize -> convert; size = 0: opsize = 1, incsize = 2; *)
		VAR loop, end: Label;
	BEGIN
		IF (dsize = 2) & (wsize < 2) THEN DevCPE.GenByte(31H); DevCPE.GenByte(0C0H); (* xor eax,eax *) END;
		loop := NewLbl; end := NewLbl;
		SetLabel(loop); LoadStr(wsize);
		IF wsize = 0 THEN DevCPE.GenByte(46H) END; (* inc esi *)
		ScanStr(dsize, FALSE); GenJump(ccNE, end, TRUE);
		IF dsize = 0 THEN DevCPE.GenByte(47H) END; (* inc edi *)
		TestNull(wsize); GenJump(ccNE, loop, TRUE);
		SetLabel(end);
		a1.mode := 0; a2.mode := 0
	END GenStringComp;

	PROCEDURE GenStringLength* (wsize, len: INTEGER);	(* len = 0: len in ECX, len = -1: len undefined *)
	BEGIN
		DevCPE.GenByte(31H); DevCPE.GenByte(0C0H); (* xor eax,eax *)
		IF len # 0 THEN DevCPE.GenByte(0B9H); GenDbl(len) END; (* ld ecx,len *)
		ScanStr(wsize, TRUE);
		a1.mode := 0; a2.mode := 0
	END GenStringLength;
	
	PROCEDURE GenStrStore* (size: INTEGER);
		VAR w: INTEGER;
	BEGIN
		IF size # 0 THEN
			IF size MOD 4 = 0 THEN w := 1; size := size DIV 4
			ELSIF size MOD 2 = 0 THEN w := 2; size := size DIV 2
			ELSE w := 0
			END;
			DevCPE.GenByte(0B9H); GenDbl(size); (* ld ecx,size *)
			IF w = 2 THEN DevCPE.GenByte(66H); w := 1 END
		ELSE w := 0
		END;
		DevCPE.GenByte(0F3H); DevCPE.GenByte(0AAH + w); (* rep:stos *)
		a1.mode := 0; a2.mode := 0
	END GenStrStore;

	PROCEDURE GenCode* (op: INTEGER);
	BEGIN
		DevCPE.GenByte(op);
		a1.mode := 0; a2.mode := 0
	END GenCode;


	PROCEDURE Init*(opt: SET);
	BEGIN
		DevCPE.Init(processor, opt);
		level := 0;
		NEW(one); one.realval := 1.0; one.intval := DevCPM.ConstNotAlloc;
	END Init;

	PROCEDURE Close*;
	BEGIN
		a1.obj := NIL; a1.typ := NIL; a2.obj := NIL; a2.typ := NIL; one := NIL;
		DevCPE.Close
	END Close;

BEGIN
	Size[Undef] := 0;
	Size[Byte] := 1;
	Size[Bool] := 1;
	Size[Char8] := 1;
	Size[Int8] := 1;
	Size[Int16] := 2;
	Size[Int32] := 4;
	Size[Real32] := -4;
	Size[Real64] := -8;
	Size[Set] := 4;
	Size[String8] := 0;
	Size[NilTyp] := 4;
	Size[NoTyp] := 0;
	Size[Pointer] := 4;
	Size[ProcTyp] := 4;
	Size[Comp] := 0;
	Size[Char16] := 2;
	Size[Int64] := 8;
	Size[String16] := 0
END DevCPL486.
