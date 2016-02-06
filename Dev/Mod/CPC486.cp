MODULE DevCPC486;
(* code generator for i386 *)
(**
	project	= "BlackBox"
	organization	= "www.oberon.ch"
	contributors	= "Oberon microsystems"
	version	= "System/Rsrc/About"
	copyright	= "System/Rsrc/About"
	license	= "Docu/BB-License"
	references	= "ftp://ftp.inf.ethz.ch/pub/software/Oberon/OberonV4/Docu/OP2.Paper.ps"
	changes	= "
	- 19940907, bh, ?
	- 19950925, bh, COM support
	- 19950930, bh, pointer init corrected
	- 19951206, bh, COM support changed
	- 19951209, bh, lonchar & largeint support
	- 19960116, bh, interface ptr marks in NEW
	- 19960120, bh, new kernel type in NEW
	- 19960220, bh, exception handling in interface procs
	- 19960522, bh, NEW of tagged  array of untagged record allowed
	- 19971217, bh, Java frontend extensions
	- 19980726, bh, correction in Param (receiver type)
	- 19990127, bh, fixed bug in Oberon compiler (handling of EXTENSIBLE records)
	- 19990828, bh, initialization of local variables (Enter & Param)
	- 19990831, bh, correction in Index fixed bug in Index reported by W. Braun (p.p[i].e[k])*)
	- 20000505, bh, correction in LoadLong
	- 20000911, bh, correction in Push (pushing of temporary LONGINT values)
	- 20001027, bh, correction in Param (do not push length for untagged arrays)
	- 20010208, bh, FPU control register handling (InitFpu, Call, Enter)
	- 20010208, bh, nil tests for var par & receiver (PushAdr, Param)
	- 20070123, bh, ccall support for procedure variable calls
	- 20070409, bh, OUT pointer initialization in empty procedures
	- 20091228, bh, corrections for S.VAL(LONGINT, real) in Convert & ConvMove
	"
	issues	= ""

**)

	IMPORT SYSTEM, DevCPM, DevCPT, DevCPE, DevCPL486;

	CONST
		initializeAll = FALSE;	(* initialize all local variable to zero *)
		initializeOut = FALSE;	(* initialize all OUT parameters to zero *)
		initializeDyn = FALSE;	(* initialize all open array OUT parameters to zero *)
		initializeStr = FALSE;	(* initialize rest of string value parameters to zero *)
		
		FpuControlRegister = 33EH;	(* value for fpu control register initialization *)
		
		(* structure forms *)
		Undef = 0; Byte = 1; Bool = 2; Char8 = 3; Int8 = 4; Int16 = 5; Int32 = 6;
		Real32 = 7; Real64 = 8; Set = 9; String8 = 10; NilTyp = 11; NoTyp = 12;
		Pointer = 13; ProcTyp = 14; Comp = 15;
		Char16 = 16; String16 = 17; Int64 = 18; Guid = 23;
		VString16to8 = 29; VString8 = 30; VString16 = 31;
		intSet = {Int8..Int32, Int64}; realSet = {Real32, Real64};

		(* composite structure forms *)
		Basic = 1; Array = 2; DynArr = 3; Record = 4;

		(* item base modes (=object modes) *)
		Var = 1; VarPar = 2; Con = 3; Fld = 4; Typ = 5; LProc = 6; XProc = 7; CProc = 9; IProc = 10; TProc = 13;

		(* item modes for i386 *)
		Ind = 14; Abs = 15; Stk = 16; Cond = 17; Reg = 18; DInd = 19;
		
		(* symbol values and ops *)
		times = 1; slash = 2; div = 3; mod = 4;
		and = 5; plus = 6; minus = 7; or = 8; eql = 9;
		neq = 10; lss = 11; leq = 12; gtr = 13; geq = 14;
		in = 15; is = 16; ash = 17; msk = 18; len = 19;
		conv = 20; abs = 21; cap = 22; odd = 23; not = 33;
		adr = 24; cc = 25; bit = 26; lsh = 27; rot = 28; val = 29;
		getrfn = 26; putrfn = 27;
		min = 34; max = 35; typ = 36;

		(* procedure flags (conval.setval) *)
		hasBody = 1; isRedef = 2; slNeeded = 3; imVar = 4; isGuarded = 30; isCallback = 31;

		(* attribute flags (attr.adr, struct.attribute, proc.conval.setval) *)
		newAttr = 16; absAttr = 17; limAttr = 18; empAttr = 19; extAttr = 20;
		
		false = 0; true = 1; nil = 0;

		(* registers *)
		AX = 0; CX = 1; DX = 2; BX = 3; SP = 4; BP = 5; SI = 6; DI = 7; AH = 4; CH = 5; DH = 6; BH = 7;
		stk = 31; mem = 30; con = 29; float = 28; high = 27; short = 26; deref = 25; wreg = {AX, BX, CX, DX, SI, DI};
	
		(* GenShiftOp *)
		ROL = 0; ROR = 8H; SHL = 20H; SHR = 28H; SAR = 38H;

		(* GenBitOp *)
		BT = 20H; BTS = 28H; BTR = 30H;
		
		(* GenFDOp *)
		FADD = 0; FMUL = 8H; FCOM = 10H; FCOMP = 18H; FSUB = 20H; FSUBR = 28H; FDIV = 30H; FDIVR = 38H; 
		
		(* GenFMOp *)
		FABS = 1E1H; FCHS = 1E0H; FTST = 1E4H; FSTSW = 7E0H; FUCOM = 2E9H;

		(* GenCode *)
		SAHF = 9EH; WAIT = 9BH;

		(* condition codes *)
		ccB = 2; ccAE = 3; ccBE = 6; ccA = 7; (* unsigned *)
		ccL = 12; ccGE = 13; ccLE = 14; ccG = 15; (* signed *)
		ccE = 4; ccNE = 5; ccS = 8; ccNS = 9; ccO = 0; ccNO = 1;
		ccAlways = -1; ccNever = -2; ccCall = -3;

		(* sysflag *)
		untagged = 1; callback = 2; noAlign = 3; union = 7;
		interface = 10; ccall = -10; guarded = 10; noframe = 16;
		nilBit = 1; enumBits = 8; new = 1; iid = 2;
		stackArray = 120;
		
		(* system trap numbers *)
		withTrap = -1; caseTrap = -2; funcTrap = -3; typTrap = -4;
		recTrap = -5; ranTrap = -6; inxTrap = -7; copyTrap = -8;
		
		(* module visibility of objects *)
		internal = 0; external = 1; externalR = 2; inPar = 3; outPar = 4;

		(* pointer init limits *)
		MaxPtrs = 10; MaxPush = 4;
		
		Tag0Offset = 12;
		Mth0Offset = -4;
		ArrDOffs = 8;
		numPreIntProc = 2;
		
		stackAllocLimit = 2048;

		
	VAR
		imLevel*: ARRAY 64 OF BYTE;
		intHandler*: DevCPT.Object;
		inxchk, ovflchk, ranchk, typchk, ptrinit, hints: BOOLEAN;
		WReg, BReg, AllReg: SET; FReg: INTEGER;
		ptrTab: ARRAY MaxPtrs OF INTEGER;
		stkAllocLbl: DevCPL486.Label;
		procedureUsesFpu: BOOLEAN;

	
	PROCEDURE Init* (opt: SET);
		CONST chk = 0; achk = 1; hint = 29;
	BEGIN
		inxchk := chk IN opt; ovflchk := achk IN opt; ranchk := achk IN opt; typchk := chk IN opt; ptrinit := chk IN opt;
		hints := hint IN opt;
		stkAllocLbl := DevCPL486.NewLbl
	END Init;

	PROCEDURE Reversed (cond: BYTE): BYTE;	(* reversed condition *)
	BEGIN
		IF cond = lss THEN RETURN gtr
		ELSIF cond = gtr THEN RETURN lss
		ELSIF cond = leq THEN RETURN geq
		ELSIF cond = geq THEN RETURN leq
		ELSE RETURN cond
		END
	END Reversed;
	
	PROCEDURE Inverted (cc: INTEGER): INTEGER;	(* inverted sense of condition code *)
	BEGIN
		IF ODD(cc) THEN RETURN cc-1 ELSE RETURN cc+1 END
	END Inverted;

	PROCEDURE setCC* (VAR x: DevCPL486.Item; rel: BYTE; reversed, signed: BOOLEAN);
	BEGIN
		IF reversed THEN rel := Reversed(rel) END;
		CASE rel OF
		   false: x.offset := ccNever
		| true: x.offset := ccAlways
		| eql: x.offset := ccE
		| neq: x.offset := ccNE
		| lss: IF signed THEN x.offset := ccL ELSE x.offset := ccB END
		| leq: IF signed THEN x.offset := ccLE ELSE x.offset := ccBE END
		| gtr: IF signed THEN x.offset := ccG ELSE x.offset := ccA END
		| geq: IF signed THEN x.offset := ccGE ELSE x.offset := ccAE END
		END;
		x.mode := Cond; x.form := Bool; x.reg := 0;
		IF reversed THEN x.reg := 1 END;
		IF signed THEN INC(x.reg, 2) END
	END setCC;

	PROCEDURE StackAlloc*;	(* pre: len = CX bytes; post: len = CX words *)
	BEGIN
		DevCPL486.GenJump(ccCall, stkAllocLbl, FALSE)
	END StackAlloc;
	
	PROCEDURE^ CheckAv* (reg: INTEGER);

	PROCEDURE AdjustStack (val: INTEGER);
		VAR c, sp: DevCPL486.Item;
	BEGIN
		IF val < -stackAllocLimit THEN
			CheckAv(CX);
			DevCPL486.MakeConst(c, -val, Int32); DevCPL486.MakeReg(sp, CX, Int32); DevCPL486.GenMove(c, sp);
			StackAlloc
		ELSIF val # 0 THEN
			DevCPL486.MakeConst(c, val, Int32); DevCPL486.MakeReg(sp, SP, Int32); DevCPL486.GenAdd(c, sp, FALSE)
		END
	END AdjustStack;
	
	PROCEDURE DecStack (form: INTEGER);
	BEGIN
		IF form IN {Real64, Int64} THEN AdjustStack(-8) ELSE AdjustStack(-4) END
	END DecStack;
	
	PROCEDURE IncStack (form: INTEGER);
	BEGIN
		IF form IN {Real64, Int64} THEN AdjustStack(8) ELSE AdjustStack(4) END
	END IncStack;
	
	(*-----------------register handling------------------*)
	
	PROCEDURE SetReg* (reg: SET);
	BEGIN
		AllReg := reg; WReg := reg; BReg := reg * {0..3} + SYSTEM.LSH(reg * {0..3}, 4); FReg := 8
	END SetReg;
	
	PROCEDURE CheckReg*;
		VAR reg: SET;
	BEGIN
		reg := AllReg - WReg;
		IF reg # {} THEN
			DevCPM.err(-777); (* register not released *)
			IF AX IN reg THEN DevCPM.LogWStr(" AX") END;
			IF BX IN reg THEN DevCPM.LogWStr(" BX") END;
			IF CX IN reg THEN DevCPM.LogWStr(" CX") END;
			IF DX IN reg THEN DevCPM.LogWStr(" DX") END;
			IF SI IN reg THEN DevCPM.LogWStr(" SI") END;
			IF DI IN reg THEN DevCPM.LogWStr(" DI") END;
			WReg := AllReg; BReg := AllReg * {0..3} + SYSTEM.LSH(AllReg * {0..3}, 4)
		END;
		IF FReg < 8 THEN DevCPM.err(-778); FReg := 8	 (* float register not released *)
		ELSIF FReg > 8 THEN DevCPM.err(-779); FReg := 8
		END
	END CheckReg;
	
	PROCEDURE CheckAv* (reg: INTEGER);
	BEGIN
		ASSERT(reg IN WReg)
	END CheckAv; 
	
	PROCEDURE GetReg (VAR x: DevCPL486.Item; f: BYTE; hint, stop: SET);
		VAR n: INTEGER; s, s1: SET;
	BEGIN
		CASE f OF
		| Byte, Bool, Char8, Int8:
			s := BReg * {0..3} - stop;
			IF (high IN stop) OR (high IN hint) & (s - hint  # {}) THEN n := 0;
				IF s = {} THEN DevCPM.err(215); WReg := wreg; BReg := {0..7}; s := {0..7} END;
				IF s - hint # {} THEN s := s - hint END;
				WHILE ~(n IN s) DO INC(n) END
			ELSE
				s := BReg - (stop * {0..3}) - SYSTEM.LSH(stop * {0..3}, 4); n := 0;
				IF s = {} THEN DevCPM.err(215); WReg := wreg; BReg := {0..7}; s := {0..7} END;
				s1 := s - (hint * {0..3}) - SYSTEM.LSH(hint * {0..3}, 4);
				IF s1 # {} THEN s := s1 END;
				WHILE ~(n IN s) & ~(n + 4 IN s) DO INC(n) END;
				IF ~(n IN s) THEN n := n + 4 END
			END;
			EXCL(BReg, n); EXCL(WReg, n MOD 4)
		| Int16, Int32, Set, String8, NilTyp, Pointer, ProcTyp, Comp, Char16, String16: 
			s := WReg - stop;
			IF high IN stop THEN s := s * {0..3} END;
			IF s = {} THEN DevCPM.err(215); WReg := wreg; BReg := {0..7}; s := wreg END;
			s1 := s - hint;
			IF high IN hint THEN s1 := s1 * {0..3} END;
			IF s1 # {} THEN s := s1 END;
			IF 0 IN s THEN n := 0
			ELSIF 2 IN s THEN n := 2
			ELSIF 6 IN s THEN n := 6
			ELSIF 7 IN s THEN n := 7
			ELSIF 1 IN s THEN n := 1
			ELSE n := 3
			END;
			EXCL(WReg, n);
			IF n < 4 THEN EXCL(BReg, n); EXCL(BReg, n + 4) END
		| Real32, Real64:
			IF (FReg = 0) OR (float IN stop) THEN DevCPM.err(216); FReg := 99 END;
			DEC(FReg); n := 0
		END;
		DevCPL486.MakeReg(x, n, f);
	END GetReg;
	
	PROCEDURE FreeReg (n, f: INTEGER);
	BEGIN
		IF f <= Int8 THEN
			INCL(BReg, n);
			IF (n + 4) MOD 8 IN BReg THEN INCL(WReg, n MOD 4) END
		ELSIF f IN realSet THEN
			INC(FReg)
		ELSIF n IN AllReg THEN
			INCL(WReg, n);
			IF n < 4 THEN INCL(BReg, n); INCL(BReg, n + 4) END
		END
	END FreeReg;
	
	PROCEDURE FreeWReg (n: INTEGER);
	BEGIN
		IF n IN AllReg THEN
			INCL(WReg, n);
			IF n < 4 THEN INCL(BReg, n); INCL(BReg, n + 4) END
		END
	END FreeWReg;
	
	PROCEDURE Free* (VAR x: DevCPL486.Item);
	BEGIN
		CASE x.mode OF
		| Var, VarPar, Abs: IF x.scale # 0 THEN FreeWReg(x.index) END
		| Ind: FreeWReg(x.reg);
			IF x.scale # 0 THEN FreeWReg(x.index) END
		| Reg: FreeReg(x.reg, x.form);
			IF x.form = Int64 THEN FreeWReg(x.index) END
		ELSE
		END
	END Free;
	
	PROCEDURE FreeHi (VAR x: DevCPL486.Item);	(* free hi byte of word reg *)
	BEGIN
		IF x.mode = Reg THEN
			IF x.form = Int64 THEN FreeWReg(x.index)
			ELSIF x.reg < 4 THEN INCL(BReg, x.reg + 4)
			END
		END
	END FreeHi;

	PROCEDURE Fits* (VAR x: DevCPL486.Item; stop: SET): BOOLEAN;	(* x.mode = Reg *)
	BEGIN
		IF (short IN stop) & (x.form <= Int8) THEN RETURN FALSE END;
		IF x.form <= Int8 THEN RETURN ~(x.reg MOD 4 IN stop) & ((x.reg < 4) OR ~(high IN stop))
		ELSIF x.form IN realSet THEN RETURN ~(float IN stop)
		ELSIF x.form = Int64 THEN RETURN ~(x.reg IN stop) & ~(x.index IN stop)
		ELSE RETURN ~(x.reg IN stop) & ((x.reg < 4) OR ~(high IN stop))
		END
	END Fits;
	
	PROCEDURE Pop* (VAR r: DevCPL486.Item; f: BYTE; hint, stop: SET);
		VAR rh: DevCPL486.Item;
	BEGIN
		IF f = Int64 THEN
			GetReg(r, Int32, hint, stop); DevCPL486.GenPop(r);
			GetReg(rh, Int32, hint, stop); DevCPL486.GenPop(rh);
			r.form := Int64; r.index := rh.reg
		ELSE
			IF f < Int16 THEN INCL(stop, high) END;
			GetReg(r, f, hint, stop); DevCPL486.GenPop(r)
		END
	END Pop;
	
	PROCEDURE^ LoadLong (VAR x: DevCPL486.Item; hint, stop: SET);
	
	PROCEDURE Load* (VAR x: DevCPL486.Item; hint, stop: SET);	(* = Assert(x, hint, stop + {mem, stk}) *)
		VAR r: DevCPL486.Item; f: BYTE;
	BEGIN
		f := x.typ.form;
		IF x.mode = Con THEN
			IF (short IN stop) & (x.form IN {Int8, Int16, Bool, Char8, Char16}) THEN f := Int32; x.form := Int32 END;
			IF con IN stop THEN
				IF f = Int64 THEN LoadLong(x, hint, stop)
				ELSE
					GetReg(r, f, hint, stop); DevCPL486.GenMove(x, r);
					x.mode := Reg; x.reg := r.reg; x.form := f
				END
			END
		ELSIF x.mode = Stk THEN
			IF f IN realSet THEN
				GetReg(r, f, hint, stop); DevCPL486.GenFLoad(x); IncStack(x.form)
			ELSE
				Pop(r, f, hint, stop)
			END;
			x.mode := Reg; x.reg := r.reg; x.index := r.index; x.form := f
		ELSIF (short IN stop) & (x.form IN {Int8, Int16, Bool, Char8, Char16}) THEN
			Free(x); GetReg(r, Int32, hint, stop); DevCPL486.GenExtMove(x, r);
			x.mode := Reg; x.reg := r.reg; x.form := Int32
		ELSIF (x.mode # Reg) OR ~Fits(x, stop) THEN
			IF f = Int64 THEN LoadLong(x, hint, stop)
			ELSE
				Free(x); GetReg(r, f, hint, stop);
				IF f IN realSet THEN DevCPL486.GenFLoad(x) ELSE DevCPL486.GenMove(x, r) END;
				x.mode := Reg; x.reg := r.reg; x.form := f
			END
		END
	END Load;
	
	PROCEDURE Push* (VAR x: DevCPL486.Item);
		VAR y: DevCPL486.Item;
	BEGIN
		IF x.form IN realSet THEN
			Load(x, {}, {}); DecStack(x.form);
			Free(x); x.mode := Stk;
			IF x.typ = DevCPT.intrealtyp THEN x.form := Int64 END;
			DevCPL486.GenFStore(x, TRUE)
		ELSIF x.form = Int64 THEN
			Free(x); x.form := Int32; y := x;
			IF x.mode = Reg THEN y.reg := x.index ELSE INC(y.offset, 4) END;
			DevCPL486.GenPush(y); DevCPL486.GenPush(x);
			x.mode := Stk; x.form := Int64
		ELSE
			IF x.form < Int16 THEN Load(x, {}, {high})
			ELSIF x.form = Int16 THEN Load(x, {}, {})
			END;
			Free(x); DevCPL486.GenPush(x); x.mode := Stk
		END
	END Push;
	
	PROCEDURE Assert* (VAR x: DevCPL486.Item; hint, stop: SET);
		VAR r: DevCPL486.Item;
	BEGIN
		IF (short IN stop) & (x.form IN {Int8, Int16, Bool, Char8, Char16}) & (x.mode # Con) THEN
			IF (wreg - stop = {}) & ~(stk IN stop) THEN Load(x, {}, {short}); Push(x)
			ELSE Load(x, hint, stop);
			END
		ELSE
			CASE x.mode OF
			| Var, VarPar: IF ~(mem IN stop) THEN RETURN END
			| Con: IF ~(con IN stop) THEN RETURN END
			| Ind: IF ~(mem IN stop) & ~(x.reg IN stop) & ((x.scale = 0) OR ~(x.index IN stop)) THEN RETURN END
			| Abs: IF ~(mem IN stop) & ((x.scale = 0) OR ~(x.index IN stop)) THEN RETURN END
			| Stk: IF ~(stk IN stop) THEN RETURN END
			| Reg: IF Fits(x, stop) THEN RETURN END
			ELSE RETURN
			END;
			IF ((float IN stop) OR ~(x.typ.form IN realSet) & (wreg - stop = {})) & ~(stk IN stop) THEN Push(x)
			ELSE Load(x, hint, stop)
			END
		END
	END Assert;
	
	(*------------------------------------------------*)

	PROCEDURE LoadR (VAR x: DevCPL486.Item);
	BEGIN
		IF x.mode # Reg THEN
			Free(x); DevCPL486.GenFLoad(x);
			IF x.mode = Stk THEN IncStack(x.form) END;
			GetReg(x, Real32, {}, {})
		END
	END LoadR;

	PROCEDURE PushR (VAR x: DevCPL486.Item);
	BEGIN
		IF x.mode # Reg THEN LoadR(x) END;
		DecStack(x.form);
		Free(x); x.mode := Stk; DevCPL486.GenFStore(x, TRUE)
	END PushR;
	
	PROCEDURE LoadW (VAR x: DevCPL486.Item; hint, stop: SET);
		VAR r: DevCPL486.Item;
	BEGIN
		IF x.mode = Stk THEN
			Pop(x, x.form, hint, stop)
		ELSE
			Free(x); GetReg(r, x.form, hint, stop);
			DevCPL486.GenMove(x, r);
			x.mode := Reg; x.reg := r.reg
		END
	END LoadW;

	PROCEDURE LoadL (VAR x: DevCPL486.Item; hint, stop: SET);
		VAR r: DevCPL486.Item;
	BEGIN
		IF x.mode = Stk THEN
			Pop(x, x.form, hint, stop);
			IF (x.form < Int32) OR (x.form = Char16) THEN
				r := x; x.form := Int32; DevCPL486.GenExtMove(r, x)
			END
		ELSE
			Free(x);
			IF (x.form < Int32) OR (x.form = Char16) THEN GetReg(r, Int32, hint, stop) ELSE GetReg(r, x.form, hint, stop) END;
			IF x.mode = Con THEN x.form := r.form END;
			IF x.form # r.form THEN DevCPL486.GenExtMove(x, r) ELSE DevCPL486.GenMove(x, r) END;
			x.mode := Reg; x.reg := r.reg; x.form := r.form
		END
	END LoadL;
	
	PROCEDURE LoadLong (VAR x: DevCPL486.Item; hint, stop: SET);
		VAR r, rh, c: DevCPL486.Item; offs: INTEGER;
	BEGIN
		IF x.form = Int64 THEN
			IF  x.mode = Stk THEN
				Pop(x, x.form, hint, stop)
			ELSIF x.mode = Reg THEN
				FreeReg(x.reg, Int32); GetReg(r, Int32, hint, stop);
				FreeReg(x.index, Int32); GetReg(rh, Int32, hint, stop);
				x.form := Int32; DevCPL486.GenMove(x, r);
				x.reg := x.index; DevCPL486.GenMove(x, rh);
				x.reg := r.reg; x.index := rh.reg
			ELSE
				GetReg(rh, Int32, hint, stop + {AX});
				Free(x);
				GetReg(r, Int32, hint, stop); 
				x.form := Int32; offs := x.offset;
				IF x.mode = Con THEN x.offset := x.scale ELSE INC(x.offset, 4) END;
				DevCPL486.GenMove(x, rh);
				x.offset := offs;
				DevCPL486.GenMove(x, r);
				x.mode := Reg; x.reg := r.reg; x.index := rh.reg
			END
		ELSE
			LoadL(x, hint, stop); GetReg(rh, Int32, hint, stop); DevCPL486.GenSignExt(x, rh);
			x.index := rh.reg
		END;
		x.form := Int64
	END LoadLong;
	
	(*------------------------------------------------*)
	
	PROCEDURE CopyReg* (VAR x, y: DevCPL486.Item; hint, stop: SET);
	BEGIN
		ASSERT(x.mode = Reg);
		GetReg(y, x.form, hint, stop);
		DevCPL486.GenMove(x, y)
	END CopyReg;

	PROCEDURE GetAdr* (VAR x: DevCPL486.Item; hint, stop: SET);
		VAR r: DevCPL486.Item;
	BEGIN
		IF x.mode = DInd THEN
			x.mode := Ind
		ELSIF (x.mode = Ind) & (x.offset = 0) & (x.scale = 0) & (x.reg IN wreg) THEN
			x.mode := Reg
		ELSE
			Free(x); GetReg(r, Pointer, hint, stop);
			IF x.mode = Con THEN DevCPL486.GenMove(x, r) ELSE DevCPL486.GenLoadAdr(x, r) END;
			x.mode := Reg; x.reg := r.reg; x.form := Pointer
		END;
		x.form := Pointer; x.typ := DevCPT.anyptrtyp;
		Assert(x, hint, stop)
	END GetAdr;
	
	PROCEDURE PushAdr (VAR x: DevCPL486.Item; niltest: BOOLEAN);
		VAR r, v: DevCPL486.Item;
	BEGIN
		IF (x.mode = Abs) & (x.scale = 0) THEN x.mode := Con; x.form := Pointer
		ELSIF niltest THEN
			GetAdr(x, {}, {mem, stk});
			DevCPL486.MakeReg(r, AX, Int32);
			v.mode := Ind; v.form := Int32; v.offset := 0; v.scale := 0; v.reg := x.reg;
			DevCPL486.GenTest(r, v)
		ELSIF x.mode = DInd THEN x.mode := Ind; x.form := Pointer
		ELSE GetAdr(x, {}, {})
		END;
		Free(x); DevCPL486.GenPush(x)
	END PushAdr;

	PROCEDURE LevelBase (VAR a: DevCPL486.Item; lev: INTEGER; hint, stop: SET);
		VAR n: BYTE;
	BEGIN
		a.mode := Ind; a.scale := 0; a.form := Int32; a.typ := DevCPT.int32typ;
		IF lev = DevCPL486.level THEN a.reg := BP
		ELSE
			a.reg := BX; n := SHORT(SHORT(imLevel[DevCPL486.level] - imLevel[lev]));
			WHILE n > 0 DO
				a.offset := -4; LoadL(a, hint, stop); a.mode := Ind; DEC(n)
			END
		END
	END LevelBase;
	
	PROCEDURE LenDesc (VAR x, len: DevCPL486.Item; typ: DevCPT.Struct); (* set len to LEN(x, -typ.n) *)
	BEGIN
		IF x.tmode = VarPar THEN
			LevelBase(len, x.obj.mnolev, {}, {}); len.offset := x.obj.adr;
		ELSE ASSERT((x.tmode = Ind) & (x.mode = Ind));
			len := x; len.offset := ArrDOffs; len.scale := 0; len.form := Int32
		END;
		INC(len.offset, typ.n * 4 + 4);
		IF typ.sysflag = stackArray THEN len.offset := -4 END
	END LenDesc;
	
	PROCEDURE Tag* (VAR x, tag: DevCPL486.Item);
		VAR typ: DevCPT.Struct;
	BEGIN
		typ := x.typ;
		IF typ.form = Pointer THEN typ := typ.BaseTyp END;
		IF (x.typ # DevCPT.sysptrtyp) & (typ.attribute = 0) & ~(DevCPM.oberon IN DevCPM.options) THEN	(* final type *)
			DevCPL486.MakeConst(tag, 0, Pointer); tag.obj := DevCPE.TypeObj(typ)
		ELSIF x.typ.form = Pointer THEN
			ASSERT(x.mode = Reg);
			tag.mode := Ind; tag.reg := x.reg; tag.offset := -4;
			IF x.typ.sysflag = interface THEN tag.offset := 0 END
		ELSIF x.tmode = VarPar THEN
			LevelBase(tag, x.obj.mnolev, {}, {}); tag.offset := x.obj.adr + 4;
			Free(tag)	(* ??? *)
		ELSIF x.tmode = Ind THEN
			ASSERT(x.mode = Ind);
			tag := x; tag.offset := -4
		ELSE
			DevCPL486.MakeConst(tag, 0, Pointer); tag.obj := DevCPE.TypeObj(x.typ)
		END;
		tag.scale := 0; tag.form := Pointer; tag.typ := DevCPT.sysptrtyp
	END Tag;
	
	PROCEDURE NumOfIntProc (typ: DevCPT.Struct): INTEGER;
	BEGIN
		WHILE (typ # NIL) & (typ.sysflag # interface) DO typ := typ.BaseTyp END;
		IF typ # NIL THEN RETURN typ.n
		ELSE RETURN 0
		END
	END NumOfIntProc;
	
	PROCEDURE ContainsIPtrs* (typ: DevCPT.Struct): BOOLEAN;
		VAR fld: DevCPT.Object;
	BEGIN
		WHILE typ.comp IN {DynArr, Array} DO typ := typ.BaseTyp END;
		IF (typ.form = Pointer) & (typ.sysflag = interface) THEN RETURN TRUE
		ELSIF (typ.comp = Record) & (typ.sysflag # union) THEN
			REPEAT
				fld := typ.link;
				WHILE (fld # NIL) & (fld.mode = Fld) DO
					IF (fld.sysflag = interface) & (fld.name^ = DevCPM.HdUtPtrName) 
						OR ContainsIPtrs(fld.typ) THEN RETURN TRUE END;
					fld := fld.link
				END;
				typ := typ.BaseTyp
			UNTIL typ = NIL
		END;
		RETURN FALSE
	END ContainsIPtrs;
	
	PROCEDURE GuidFromString* (str: DevCPT.ConstExt; VAR x: DevCPL486.Item);
		VAR cv: DevCPT.Const;
	BEGIN
		IF ~DevCPM.ValidGuid(str^) THEN DevCPM.err(165) END;
		cv := DevCPT.NewConst();
		cv.intval := DevCPM.ConstNotAlloc; cv.intval2 := 16; cv.ext := str;
		DevCPL486.AllocConst(x, cv, Guid); x.typ := DevCPT.guidtyp
	END GuidFromString;
	
	PROCEDURE IPAddRef* (VAR x: DevCPL486.Item; offset: INTEGER; nilTest: BOOLEAN);
		VAR r, p, c: DevCPL486.Item; lbl: DevCPL486.Label;
	BEGIN
		ASSERT(x.mode IN {Reg, Ind, Abs});
		ASSERT({AX, CX, DX} - WReg = {});
		IF hints THEN
			IF nilTest THEN DevCPM.err(-701) ELSE DevCPM.err(-700) END
		END;
		IF x.mode # Reg THEN 
			GetReg(r, Pointer, {}, {});
			p := x; INC(p.offset, offset); p.form := Pointer; DevCPL486.GenMove(p, r);
		ELSE r := x
		END;
		IF nilTest THEN
			DevCPL486.MakeConst(c, 0, Pointer); DevCPL486.GenComp(c, r);
			lbl := DevCPL486.NewLbl; DevCPL486.GenJump(ccE, lbl, TRUE)
		END;
		DevCPL486.GenPush(r); p := r;
		IF x.mode # Reg THEN Free(r) END;
		GetReg(r, Pointer, {}, {});
		p.mode := Ind; p.offset := 0; p.scale := 0; p.form := Pointer; DevCPL486.GenMove(p, r);
		p.offset := 4; p.reg := r.reg; Free(r); DevCPL486.GenCall(p);
		IF nilTest THEN DevCPL486.SetLabel(lbl) END;
	END IPAddRef;
	
	PROCEDURE IPRelease* (VAR x: DevCPL486.Item; offset: INTEGER; nilTest, nilSet: BOOLEAN);
		VAR r, p, c: DevCPL486.Item; lbl: DevCPL486.Label;
	BEGIN
		ASSERT(x.mode IN {Ind, Abs});
		ASSERT({AX, CX, DX} - WReg = {});
		IF hints THEN
			IF nilTest THEN DevCPM.err(-703) ELSE DevCPM.err(-702) END
		END;
		GetReg(r, Pointer, {}, {});
		p := x; INC(p.offset, offset); p.form := Pointer; DevCPL486.GenMove(p, r);
		DevCPL486.MakeConst(c, 0, Pointer);
		IF nilTest THEN
			DevCPL486.GenComp(c, r);
			lbl := DevCPL486.NewLbl; DevCPL486.GenJump(ccE, lbl, TRUE)
		END;
		IF nilSet THEN DevCPL486.GenMove(c, p) END;
		DevCPL486.GenPush(r);
		p.mode := Ind; p.reg := r.reg; p.offset := 0; p.scale := 0; DevCPL486.GenMove(p, r);
		p.offset := 8; Free(r); DevCPL486.GenCall(p);
		IF nilTest THEN DevCPL486.SetLabel(lbl) END;
	END IPRelease;
	
	PROCEDURE Prepare* (VAR x: DevCPL486.Item; hint, stop: SET);
		VAR n, i, lev: INTEGER; len, y: DevCPL486.Item; typ: DevCPT.Struct;
	BEGIN
		IF (x.mode IN {Var, VarPar, Ind, Abs}) & (x.scale # 0) THEN
			DevCPL486.MakeReg(y, x.index, Int32); typ := x.typ;
			WHILE typ.comp = DynArr DO (* complete dynamic array iterations *)
				LenDesc(x, len, typ); DevCPL486.GenMul(len, y, FALSE); typ := typ.BaseTyp;
				IF x.tmode = VarPar THEN Free(len) END;	(* ??? *)
			END;
			n := x.scale; i := 0;
			WHILE (n MOD 2 = 0) & (i < 3) DO n := n DIV 2; INC(i) END;
			IF n > 1 THEN (* assure scale factor in {1, 2, 4, 8} *)
				DevCPL486.MakeConst(len, n, Int32); DevCPL486.GenMul(len, y, FALSE); x.scale := x.scale DIV n 
			END
		END;
		CASE x.mode OF
		   Var, VarPar:
				lev := x.obj.mnolev;
				IF lev <= 0 THEN
					x.mode := Abs
				ELSE
					LevelBase(y, lev, hint, stop);
					IF x.mode # VarPar THEN
						x.mode := Ind
					ELSIF (deref IN hint) & (x.offset = 0) & (x.scale = 0) THEN
						x.mode := DInd; x.offset := x.obj.adr
					ELSE
						y.offset := x.obj.adr; Load(y, hint, stop); x.mode := Ind
					END;
					x.reg := y.reg
				END;
				x.form := x.typ.form
		| LProc, XProc, IProc:
				x.mode := Con; x.offset := 0; x.form := ProcTyp
		| TProc, CProc:
				x.form := ProcTyp
		| Ind, Abs, Stk, Reg:
				IF ~(x.typ.form IN {String8, String16}) THEN x.form := x.typ.form END
		END
	END Prepare;
	
	PROCEDURE Field* (VAR x: DevCPL486.Item; field: DevCPT.Object);
	BEGIN
		INC(x.offset, field.adr); x.tmode := Con
	END Field;
	
	PROCEDURE DeRef* (VAR x: DevCPL486.Item);
		VAR btyp: DevCPT.Struct;
	BEGIN
		x.mode := Ind; x.tmode := Ind; x.scale := 0;
		btyp := x.typ.BaseTyp;
		IF btyp.untagged OR (btyp.sysflag = stackArray) THEN x.offset := 0
		ELSIF btyp.comp = DynArr THEN x.offset := ArrDOffs + btyp.size
		ELSIF btyp.comp = Array THEN x.offset := ArrDOffs + 4 
		ELSE x.offset := 0
		END
	END DeRef;
	
	PROCEDURE Index* (VAR x, y: DevCPL486.Item; hint, stop: SET);	(* x[y] *)
		VAR idx, len: DevCPL486.Item; btyp: DevCPT.Struct; elsize: INTEGER;
	BEGIN
		btyp := x.typ.BaseTyp; elsize := btyp.size;
		IF elsize = 0 THEN Free(y)
		ELSIF x.typ.comp = Array THEN
			len.mode := Con; len.obj := NIL;
			IF y.mode = Con THEN
				INC(x.offset, y.offset * elsize)
			ELSE
				Load(y, hint, stop + {mem, stk, short});
				IF inxchk THEN
					DevCPL486.MakeConst(len, x.typ.n, Int32);
					DevCPL486.GenComp(len, y); DevCPL486.GenAssert(ccB, inxTrap)
				END;
				IF x.scale = 0 THEN x.index := y.reg
				ELSE
					IF x.scale MOD elsize # 0 THEN
						IF (x.scale MOD 4 = 0) & (elsize MOD 4 = 0) THEN elsize := 4
						ELSIF (x.scale MOD 2 = 0) & (elsize MOD 2 = 0) THEN elsize := 2
						ELSE elsize := 1
						END;
						DevCPL486.MakeConst(len, btyp.size DIV elsize, Int32);
						DevCPL486.GenMul(len, y, FALSE)
					END;
					DevCPL486.MakeConst(len, x.scale DIV elsize, Int32);
					DevCPL486.MakeReg(idx, x.index, Int32);
					DevCPL486.GenMul(len, idx, FALSE); DevCPL486.GenAdd(y, idx, FALSE); Free(y)
				END;
				x.scale := elsize
			END;
			x.tmode := Con
		ELSE (* x.typ.comp = DynArr *)
			IF (btyp.comp = DynArr) & x.typ.untagged THEN DevCPM.err(137) END;
			LenDesc(x, len, x.typ);
			IF x.scale # 0 THEN
				DevCPL486.MakeReg(idx, x.index, Int32); 
				DevCPL486.GenMul(len, idx, FALSE)
			END;
			IF (y.mode # Con) OR (y.offset # 0) THEN
				IF (y.mode # Con) OR (btyp.comp = DynArr) & (x.scale = 0) THEN
					Load(y, hint, stop + {mem, stk, con, short})
				ELSE y.form := Int32
				END;
				IF inxchk & ~x.typ.untagged THEN
					DevCPL486.GenComp(y, len); DevCPL486.GenAssert(ccA, inxTrap)
				END;
				IF (y.mode = Con) & (btyp.comp # DynArr) THEN
					INC(x.offset, y.offset * elsize)
				ELSIF x.scale = 0 THEN
					WHILE btyp.comp = DynArr DO btyp := btyp.BaseTyp END;
					x.index := y.reg; x.scale := btyp.size
				ELSE
					DevCPL486.GenAdd(y, idx, FALSE); Free(y)
				END
			END;
			IF x.tmode = VarPar THEN Free(len) END;	(* ??? *)
			IF x.typ.BaseTyp.comp # DynArr THEN x.tmode := Con END
		END
	END Index;
	
	PROCEDURE TypTest* (VAR x: DevCPL486.Item; testtyp: DevCPT.Struct; guard, equal: BOOLEAN);
		VAR tag, tdes, r: DevCPL486.Item; typ: DevCPT.Struct;
	BEGIN
		typ := x.typ;
		IF typ.form = Pointer THEN testtyp := testtyp.BaseTyp; typ := typ.BaseTyp END;
		IF ~guard & typ.untagged THEN DevCPM.err(139)
		ELSIF ~guard OR typchk & ~typ.untagged THEN
			IF testtyp.untagged THEN DevCPM.err(139)
			ELSE
				IF (x.typ.form = Pointer) & (x.mode # Reg) THEN
					GetReg(r, Pointer, {}, {}); DevCPL486.GenMove(x, r); Free(r); r.typ := x.typ; Tag(r, tag)
				ELSE Tag(x, tag)
				END;
				IF ~guard THEN Free(x) END;
				IF ~equal THEN
					GetReg(r, Pointer, {}, {}); DevCPL486.GenMove(tag, r); Free(r);
					tag.mode := Ind; tag.reg := r.reg; tag.scale := 0; tag.offset := Tag0Offset + 4 * testtyp.extlev
				END;
				DevCPL486.MakeConst(tdes, 0, Pointer); tdes.obj := DevCPE.TypeObj(testtyp);
				DevCPL486.GenComp(tdes, tag);
				IF guard THEN
					IF equal THEN DevCPL486.GenAssert(ccE, recTrap) ELSE DevCPL486.GenAssert(ccE, typTrap) END
				ELSE setCC(x, eql, FALSE, FALSE)
				END
			END
		END
	END TypTest;
	
	PROCEDURE ShortTypTest* (VAR x: DevCPL486.Item; testtyp: DevCPT.Struct);
		VAR tag, tdes: DevCPL486.Item;
	BEGIN
		(* tag must be in AX ! *)
		IF testtyp.form = Pointer THEN testtyp := testtyp.BaseTyp END;
		IF testtyp.untagged THEN DevCPM.err(139)
		ELSE
			tag.mode := Ind; tag.reg := AX; tag.scale := 0; tag.offset := Tag0Offset + 4 * testtyp.extlev; tag.form := Pointer;
			DevCPL486.MakeConst(tdes, 0, Pointer); tdes.obj := DevCPE.TypeObj(testtyp);
			DevCPL486.GenComp(tdes, tag);
			setCC(x, eql, FALSE, FALSE)
		END
	END ShortTypTest;

	PROCEDURE Check (VAR x: DevCPL486.Item; min, max: INTEGER);
		VAR c: DevCPL486.Item;
	BEGIN
		ASSERT((x.mode # Reg) OR (max > 255) OR (max = 31) OR (x.reg < 4));
		IF ranchk & (x.mode # Con) THEN
			DevCPL486.MakeConst(c, max, x.form); DevCPL486.GenComp(c, x);
			IF min # 0 THEN
				DevCPL486.GenAssert(ccLE, ranTrap);
				c.offset := min; DevCPL486.GenComp(c, x);
				DevCPL486.GenAssert(ccGE, ranTrap)
			ELSIF max # 0 THEN
				DevCPL486.GenAssert(ccBE, ranTrap)
			ELSE
				DevCPL486.GenAssert(ccNS, ranTrap)
			END
		END
	END Check;

	PROCEDURE Floor (VAR x: DevCPL486.Item; useSt1: BOOLEAN);
		VAR c: DevCPL486.Item; local: DevCPL486.Label;
	BEGIN
		IF useSt1 THEN DevCPL486.GenFMOp(5D1H);	(* FST ST1 *)
		ELSE DevCPL486.GenFMOp(1C0H);	(* FLD ST0 *)
		END;
		DevCPL486.GenFMOp(1FCH);	(* FRNDINT *)
		DevCPL486.GenFMOp(0D1H);	(* FCOM *)
		CheckAv(AX);
		DevCPL486.GenFMOp(FSTSW);
		DevCPL486.GenFMOp(5D9H);	(* FSTP ST1 *)
		(* DevCPL486.GenCode(WAIT); *) DevCPL486.GenCode(SAHF);
		local := DevCPL486.NewLbl; DevCPL486.GenJump(ccBE, local, TRUE);
		DevCPL486.AllocConst(c, DevCPL486.one, Real32);
		DevCPL486.GenFDOp(FSUB, c);
		DevCPL486.SetLabel(local);
	END Floor;
	
	PROCEDURE Entier(VAR x: DevCPL486.Item; typ: DevCPT.Struct; hint, stop: SET);
	BEGIN
		IF typ # DevCPT.intrealtyp THEN Floor(x, FALSE) END;
		DevCPL486.GenFStore(x, TRUE);
		IF (x.mode = Stk) & (stk IN stop) THEN Pop(x, x.form, hint, stop) END
	END Entier;

	PROCEDURE ConvMove (VAR x, y: DevCPL486.Item; sysval: BOOLEAN; hint, stop: SET);	(* x := y *)
		(* scalar values only, y.mode # Con, all kinds of conversions, x.mode = Undef => convert y only *)
		VAR f, m: BYTE; s: INTEGER; z: DevCPL486.Item;
	BEGIN
		f := x.form; m := x.mode; ASSERT(m IN {Undef, Reg, Abs, Ind, Stk});
		IF y.form IN {Real32, Real64} THEN
			IF f IN {Real32, Real64} THEN
				IF m = Undef THEN
					IF (y.form = Real64) & (f = Real32) THEN
						IF y.mode # Reg THEN LoadR(y) END;
						Free(y); DecStack(Real32); y.mode := Stk; y.form := Real32; DevCPL486.GenFStore(y, TRUE)
					END
				ELSE
					IF y.mode # Reg THEN LoadR(y) END;
					IF m = Stk THEN DecStack(f) END;
					IF m # Reg THEN Free(y); DevCPL486.GenFStore(x, TRUE) END;
				END
			ELSE (* x not real *)
				IF sysval THEN
					IF y.mode = Reg THEN Free(y);
						IF (m # Stk) & (m # Undef) & (m # Reg) & (f >= Int32) THEN
							x.form := y.form; DevCPL486.GenFStore(x, TRUE); x.form := f
						ELSIF y.form = Real64 THEN	(* S.VAL(LONGINT, real) *)
							ASSERT((m = Undef) & (f = Int64));
							DecStack(y.form); y.mode := Stk; DevCPL486.GenFStore(y, TRUE); y.form := Int64;
							Pop(y, y.form, hint, stop)
						ELSE
							ASSERT(y.form # Real64);
							DecStack(y.form); y.mode := Stk; DevCPL486.GenFStore(y, TRUE); y.form := Int32;
							IF m # Stk THEN
								Pop(y, y.form, hint, stop);
								IF f < Int16 THEN ASSERT(y.reg < 4) END;
								y.form := f;
								IF m # Undef THEN Free(y); DevCPL486.GenMove(y, x) END
							END
						END
					ELSE (* y.mode # Reg *)
						y.form := f;
						IF m # Undef THEN LoadW(y, hint, stop); Free(y);
							IF m = Stk THEN DevCPL486.GenPush(y) ELSE DevCPL486.GenMove(y, x) END
						END
					END
				ELSE (* not sysval *)
					IF y.mode # Reg THEN LoadR(y) END;
					Free(y);
					IF (m # Stk) & (m # Undef) & (m # Reg) & (f >= Int16) & (f # Char16) THEN
						Entier(x, y.typ, hint, stop);
					ELSE
						DecStack(f); y.mode := Stk;
						IF (f < Int16) OR (f = Char16) THEN y.form := Int32 ELSE y.form := f END;
						IF m = Stk THEN Entier(y, y.typ, {}, {})
						ELSIF m = Undef THEN Entier(y, y.typ, hint, stop)
						ELSE Entier(y, y.typ, hint, stop + {stk})
						END;
						IF f = Int8 THEN Check(y, -128, 127); FreeHi(y)
						ELSIF f = Char8 THEN Check(y, 0, 255); FreeHi(y)
						ELSIF f = Char16 THEN Check(y, 0, 65536); FreeHi(y)
						END;
						y.form := f;
						IF (m # Undef) & (m # Stk) THEN
							IF f = Int64 THEN
								Free(y); y.form := Int32; z := x; z.form := Int32; DevCPL486.GenMove(y, z);
								IF z.mode = Reg THEN z.reg := z.index ELSE INC(z.offset, 4) END;
								y.reg := y.index; DevCPL486.GenMove(y, z);
							ELSE
								Free(y); DevCPL486.GenMove(y, x);
							END
						END
					END
				END
			END
		ELSE (* y not real *)
			IF sysval THEN
				IF (y.form < Int16) & (f >= Int16) OR (y.form IN {Int16, Char16}) & (f >= Int32) & (f < Char16) THEN LoadL(y, hint, stop) END;
				IF (y.form >= Int16) & (f < Int16) THEN FreeHi(y) END
			ELSE
				CASE y.form OF
				| Byte, Bool:
						IF f = Int64 THEN LoadLong(y, hint, stop)
						ELSIF f >= Int16 THEN LoadL(y, hint, stop)
						END
				| Char8:
						IF f = Int8 THEN Check(y, 0, 0)
						ELSIF f = Int64 THEN LoadLong(y, hint, stop)
						ELSIF f >= Int16 THEN LoadL(y, hint, stop)
						END
				| Char16:
						IF f = Char8 THEN Check(y, 0, 255); FreeHi(y)
						ELSIF f = Int8 THEN Check(y, -128, 127); FreeHi(y)
						ELSIF f = Int16 THEN Check(y, 0, 0)
						ELSIF f = Char16 THEN (* ok *)
						ELSIF f = Int64 THEN LoadLong(y, hint, stop)
						ELSIF f >= Int32 THEN LoadL(y, hint, stop)
						END
				| Int8:
						IF f = Char8 THEN Check(y, 0, 0)
						ELSIF f = Int64 THEN LoadLong(y, hint, stop)
						ELSIF f >= Int16 THEN LoadL(y, hint, stop)
						END
				| Int16:
						IF f = Char8 THEN Check(y, 0, 255); FreeHi(y)
						ELSIF f = Char16 THEN Check(y, 0, 0)
						ELSIF f = Int8 THEN Check(y, -128, 127); FreeHi(y)
						ELSIF f = Int64 THEN LoadLong(y, hint, stop)
						ELSIF (f = Int32) OR (f = Set) THEN LoadL(y, hint, stop)
						END
				| Int32, Set, Pointer, ProcTyp:
						IF f = Char8 THEN Check(y, 0, 255); FreeHi(y)
						ELSIF f = Char16 THEN Check(y, 0, 65536)
						ELSIF f = Int8 THEN Check(y, -128, 127); FreeHi(y)
						ELSIF f = Int16 THEN Check(y, -32768, 32767)
						ELSIF f = Int64 THEN LoadLong(y, hint, stop)
						END
				| Int64:
						IF f IN {Bool..Int32, Char16} THEN
							(* make range checks !!! *)
							FreeHi(y)
						END
				END
			END;
			IF f IN {Real32, Real64} THEN
				IF sysval THEN
					IF (m # Undef) & (m # Reg) THEN
						IF y.mode # Reg THEN LoadW(y, hint, stop) END;
						Free(y);
						IF m = Stk THEN DevCPL486.GenPush(y)
						ELSE x.form := Int32; DevCPL486.GenMove(y, x); x.form := f
						END
					ELSE
						IF y.mode = Reg THEN Push(y) END;
						y.form := f;
						IF m = Reg THEN LoadR(y) END
					END
				ELSE (* not sysval *) (* int -> float *)
					IF y.mode = Reg THEN Push(y) END;
					IF m = Stk THEN
						Free(y); DevCPL486.GenFLoad(y); s := -4;
						IF f = Real64 THEN DEC(s, 4) END;
						IF y.mode = Stk THEN
							IF y.form = Int64 THEN INC(s, 8) ELSE INC(s, 4) END
						END;
						IF s # 0 THEN AdjustStack(s) END;
						GetReg(y, Real32, {}, {});
						Free(y); DevCPL486.GenFStore(x, TRUE)
					ELSIF m = Reg THEN
						LoadR(y)
					ELSIF m # Undef THEN
						LoadR(y); Free(y); DevCPL486.GenFStore(x, TRUE) 
					END
				END
			ELSE
				y.form := f;
				IF m = Stk THEN
					IF ((f < Int32) OR (f = Char16)) & (y.mode # Reg) THEN LoadW(y, hint, stop) END;
					Push(y)
				ELSIF m # Undef THEN
					IF f = Int64 THEN
						IF y.mode # Reg THEN LoadLong(y, hint, stop) END;
						Free(y); y.form := Int32; z := x; z.form := Int32; DevCPL486.GenMove(y, z);
						IF z.mode = Reg THEN ASSERT(z.reg # y.index); z.reg := z.index ELSE INC(z.offset, 4) END;
						y.reg := y.index; DevCPL486.GenMove(y, z);
					ELSE
						IF y.mode # Reg THEN LoadW(y, hint, stop) END;
						Free(y); DevCPL486.GenMove(y, x)
					END
				END
			END
		END	
	END ConvMove;

	PROCEDURE Convert* (VAR x: DevCPL486.Item; f: BYTE; size: INTEGER; hint, stop: SET);	(* size >= 0: sysval *)
		VAR y: DevCPL486.Item;
	BEGIN
		ASSERT(x.mode # Con);
		IF (size >= 0)
			& ((size # x.typ.size) & ((size > 4) OR (x.typ.size > 4))
				OR (f IN {Comp, Real64}) & (x.mode IN {Reg, Stk})
				OR (f  = Int64) & (x.mode = Stk)) THEN DevCPM.err(220) END;
		y.mode := Undef; y.form := f; ConvMove(y, x, size >= 0, hint, stop)
	END Convert;

	PROCEDURE LoadCond* (VAR x, y: DevCPL486.Item; F, T: DevCPL486.Label; hint, stop: SET);
		VAR end, T1: DevCPL486.Label; c, r: DevCPL486.Item;
	BEGIN
		IF mem IN stop THEN GetReg(x, Bool, hint, stop) END;
		IF (F = DevCPL486.NewLbl) & (T = DevCPL486.NewLbl) THEN (* no label used *)
			DevCPL486.GenSetCC(y.offset, x)
		ELSE
			end := DevCPL486.NewLbl; T1 := DevCPL486.NewLbl;
			DevCPL486.GenJump(y.offset, T1, TRUE);	(* T1 to enable short jump *)
			DevCPL486.SetLabel(F);
			DevCPL486.MakeConst(c, 0, Bool); DevCPL486.GenMove(c, x);
			DevCPL486.GenJump(ccAlways, end, TRUE);
			DevCPL486.SetLabel(T); DevCPL486.SetLabel(T1); 
			DevCPL486.MakeConst(c, 1, Bool); DevCPL486.GenMove(c, x);
			DevCPL486.SetLabel(end)
		END;
		IF x.mode # Reg THEN Free(x) END
	END LoadCond;
	
	PROCEDURE IntDOp* (VAR x, y: DevCPL486.Item; subcl: BYTE; rev: BOOLEAN);
		VAR local: DevCPL486.Label;
	BEGIN
		ASSERT((x.mode = Reg) OR (y.mode = Reg) OR (y.mode = Con));
		CASE subcl OF
		| eql..geq:
				DevCPL486.GenComp(y, x); Free(x);
				setCC(x, subcl, rev, x.typ.form IN {Int8..Int32})
		| times: 
				IF x.form = Set THEN DevCPL486.GenAnd(y, x) ELSE DevCPL486.GenMul(y, x, ovflchk) END
		| slash:
				DevCPL486.GenXor(y, x)
		| plus:
				IF x.form = Set THEN DevCPL486.GenOr(y, x) ELSE DevCPL486.GenAdd(y, x, ovflchk) END
		| minus, msk:
				IF (x.form = Set) OR (subcl = msk) THEN (* and not *)
					IF rev THEN DevCPL486.GenNot(x); DevCPL486.GenAnd(y, x)								(* y and not x *)
					ELSIF y.mode = Con THEN y.offset := -1 - y.offset; DevCPL486.GenAnd(y, x)	(* x and y' *)
					ELSIF y.mode = Reg THEN DevCPL486.GenNot(y); DevCPL486.GenAnd(y, x)			(* x and not y *)
					ELSE DevCPL486.GenNot(x); DevCPL486.GenOr(y, x); DevCPL486.GenNot(x)					(* not (not x or y) *)
					END
				ELSE	(* minus *)
					IF rev THEN	(* y - x *)
						IF (y.mode = Con) & (y.offset = -1) THEN DevCPL486.GenNot(x)
						ELSE DevCPL486.GenNeg(x, ovflchk); DevCPL486.GenAdd(y, x, ovflchk)	(* ??? *)
						END
					ELSE	(* x - y *)
						DevCPL486.GenSub(y, x, ovflchk)
					END
				END
		| min, max:
				local := DevCPL486.NewLbl;
				DevCPL486.GenComp(y, x);
				IF subcl = min THEN 
					IF x.typ.form IN {Char8, Char16} THEN DevCPL486.GenJump(ccBE, local, TRUE)
					ELSE DevCPL486.GenJump(ccLE, local, TRUE)
					END
				ELSE
					IF x.typ.form IN {Char8, Char16} THEN DevCPL486.GenJump(ccAE, local, TRUE)
					ELSE DevCPL486.GenJump(ccGE, local, TRUE)
					END
				END;
				DevCPL486.GenMove(y, x);
				DevCPL486.SetLabel(local)
		END;
		Free(y);
		IF x.mode # Reg THEN Free(x) END
	END IntDOp;
	
	PROCEDURE LargeInc* (VAR x, y: DevCPL486.Item; dec: BOOLEAN);	(* INC(x, y) or DEC(x, y) *)
	BEGIN
		ASSERT(x.form = Int64);
		IF ~(y.mode IN {Reg, Con}) THEN LoadLong(y, {}, {}) END;
		Free(x); Free(y); x.form := Int32; y.form := Int32;
		IF dec THEN DevCPL486.GenSubC(y, x, TRUE, FALSE) ELSE DevCPL486.GenAddC(y, x, TRUE, FALSE) END;
		INC(x.offset, 4);
		IF y.mode = Reg THEN y.reg := y.index ELSE y.offset := y.scale END;
		IF dec THEN DevCPL486.GenSubC(y, x, FALSE, ovflchk) ELSE DevCPL486.GenAddC(y, x, FALSE, ovflchk) END;
	END LargeInc;
	
	PROCEDURE FloatDOp* (VAR x, y: DevCPL486.Item; subcl: BYTE; rev: BOOLEAN);
		VAR local: DevCPL486.Label; a, b: DevCPL486.Item;
	BEGIN
		ASSERT(x.mode = Reg);
		IF y.form = Int64 THEN LoadR(y) END;
		IF y.mode = Reg THEN rev := ~rev END;
		CASE subcl OF
		| eql..geq: DevCPL486.GenFDOp(FCOMP, y)
		| times: DevCPL486.GenFDOp(FMUL, y)
		| slash: IF rev THEN DevCPL486.GenFDOp(FDIVR, y) ELSE DevCPL486.GenFDOp(FDIV, y) END
		| plus: DevCPL486.GenFDOp(FADD, y)
		| minus: IF rev THEN DevCPL486.GenFDOp(FSUBR, y) ELSE DevCPL486.GenFDOp(FSUB, y) END
		| min, max:
			IF y.mode = Reg THEN
				DevCPL486.GenFMOp(0D1H);	(* FCOM ST1 *)
				CheckAv(AX); DevCPL486.GenFMOp(FSTSW); (* DevCPL486.GenCode(WAIT); *) DevCPL486.GenCode(SAHF);
				local := DevCPL486.NewLbl;
				IF subcl = min THEN DevCPL486.GenJump(ccAE, local, TRUE) ELSE DevCPL486.GenJump(ccBE, local, TRUE) END;
				DevCPL486.GenFMOp(5D1H);	(* FST ST1 *)
				DevCPL486.SetLabel(local);
				DevCPL486.GenFMOp(5D8H)	(* FSTP ST0 *)
			ELSE
				DevCPL486.GenFDOp(FCOM, y);
				CheckAv(AX); DevCPL486.GenFMOp(FSTSW); (* DevCPL486.GenCode(WAIT); *) DevCPL486.GenCode(SAHF);
				local := DevCPL486.NewLbl;
				IF subcl = min THEN DevCPL486.GenJump(ccBE, local, TRUE) ELSE DevCPL486.GenJump(ccAE, local, TRUE) END;
				DevCPL486.GenFMOp(5D8H);	(* FSTP ST0 *)
				DevCPL486.GenFLoad(y);
				DevCPL486.SetLabel(local)
			END
		(* largeint support *)
		| div:
			IF rev THEN DevCPL486.GenFDOp(FDIVR, y) ELSE DevCPL486.GenFDOp(FDIV, y) END;
			Floor(y, FALSE)
		| mod:
			IF y.mode # Reg THEN LoadR(y); rev := ~rev END;
			IF rev THEN DevCPL486.GenFMOp(1C9H); (* FXCH ST1 *) END;
			DevCPL486.GenFMOp(1F8H);	(* FPREM *)
			DevCPL486.GenFMOp(1E4H);	(* FTST *)
			CheckAv(AX);
			DevCPL486.GenFMOp(FSTSW);
			DevCPL486.MakeReg(a, AX, Int32); GetReg(b, Int32, {}, {AX});
			DevCPL486.GenMove(a, b);
			DevCPL486.GenFMOp(0D1H);	(* FCOM *)
			DevCPL486.GenFMOp(FSTSW);
			DevCPL486.GenXor(b, a); Free(b);
			(* DevCPL486.GenCode(WAIT); *) DevCPL486.GenCode(SAHF);
			local := DevCPL486.NewLbl; DevCPL486.GenJump(ccBE, local, TRUE);
			DevCPL486.GenFMOp(0C1H);	(* FADD ST1 *)
			DevCPL486.SetLabel(local);
			DevCPL486.GenFMOp(5D9H);	(* FSTP ST1 *)
		| ash:
			IF y.mode # Reg THEN LoadR(y); rev := ~rev END;
			IF rev THEN DevCPL486.GenFMOp(1C9H); (* FXCH ST1 *) END;
			DevCPL486.GenFMOp(1FDH);	(* FSCALE *)
			Floor(y, TRUE)
		END;
		IF y.mode = Stk THEN IncStack(y.form) END;
		Free(y);
		IF (subcl >= eql) & (subcl <= geq) THEN
			Free(x); CheckAv(AX);
			DevCPL486.GenFMOp(FSTSW);
			(* DevCPL486.GenCode(WAIT); *) DevCPL486.GenCode(SAHF);
			setCC(x, subcl, rev, FALSE)
		END
	END FloatDOp;
	
	PROCEDURE IntMOp* (VAR x: DevCPL486.Item; subcl: BYTE);
		VAR L: DevCPL486.Label; c: DevCPL486.Item;
	BEGIN
		CASE subcl OF
		| minus:
				IF x.form = Set THEN DevCPL486.GenNot(x) ELSE DevCPL486.GenNeg(x, ovflchk) END
		| abs:
				L := DevCPL486.NewLbl; DevCPL486.MakeConst(c, 0, x.form);
				DevCPL486.GenComp(c, x);
				DevCPL486.GenJump(ccNS, L, TRUE);
				DevCPL486.GenNeg(x, ovflchk);
				DevCPL486.SetLabel(L)
		| cap:
				DevCPL486.MakeConst(c, -1 - 20H, x.form);
				DevCPL486.GenAnd(c, x)
		| not:
				DevCPL486.MakeConst(c, 1, x.form);
				DevCPL486.GenXor(c, x)
		END;
		IF x.mode # Reg THEN Free(x) END
	END IntMOp;
	
	PROCEDURE FloatMOp* (VAR x: DevCPL486.Item; subcl: BYTE);
	BEGIN
		ASSERT(x.mode = Reg);
		IF subcl = minus THEN DevCPL486.GenFMOp(FCHS)
		ELSE ASSERT(subcl = abs); DevCPL486.GenFMOp(FABS)
		END
	END FloatMOp;

	PROCEDURE MakeSet* (VAR x: DevCPL486.Item; range, neg: BOOLEAN; hint, stop: SET);
		(* range neg	result
				F	F		{x}
				F	T		-{x}
				T	F		{x..31}
				T	T		-{0..x}	*)
		VAR c, r: DevCPL486.Item; val: INTEGER;
	BEGIN
		IF x.mode = Con THEN
			IF range THEN
				IF neg THEN val := -2 ELSE val := -1 END;
				x.offset := SYSTEM.LSH(val, x.offset)
			ELSE
				val := 1; x.offset := SYSTEM.LSH(val, x.offset);
				IF neg THEN x.offset := -1 - x.offset END
			END
		ELSE
			Check(x, 0, 31);
			IF neg THEN val := -2
			ELSIF range THEN val := -1
			ELSE val := 1
			END;
			DevCPL486.MakeConst(c, val, Set); GetReg(r, Set, hint, stop); DevCPL486.GenMove(c, r);
			IF range THEN DevCPL486.GenShiftOp(SHL, x, r) ELSE DevCPL486.GenShiftOp(ROL, x, r) END;
			Free(x); x.reg := r.reg
		END;
		x.typ := DevCPT.settyp; x.form := Set
	END MakeSet;
	
	PROCEDURE MakeCond* (VAR x: DevCPL486.Item);
		VAR c: DevCPL486.Item;
	BEGIN
		IF x.mode = Con THEN
			setCC(x, SHORT(SHORT(x.offset)), FALSE, FALSE)
		ELSE
			DevCPL486.MakeConst(c, 0, x.form);
			DevCPL486.GenComp(c, x); Free(x);
			setCC(x, neq, FALSE, FALSE)
		END
	END MakeCond;
	
	PROCEDURE Not* (VAR x: DevCPL486.Item);
		VAR a: INTEGER;
	BEGIN
		x.offset := Inverted(x.offset); (* invert cc *)
	END Not;
	
	PROCEDURE Odd* (VAR x: DevCPL486.Item);
		VAR c: DevCPL486.Item;
	BEGIN
		IF x.mode = Stk THEN Pop(x, x.form, {}, {}) END;
		Free(x); DevCPL486.MakeConst(c, 1, x.form);
		IF x.mode = Reg THEN
			IF x.form IN {Int16, Int64} THEN x.form := Int32; c.form := Int32 END;
			DevCPL486.GenAnd(c, x)
		ELSE
			c.form := Int8; x.form := Int8; DevCPL486.GenTest(c, x)
		END;
		setCC(x, neq, FALSE, FALSE)
	END Odd;
	
	PROCEDURE In* (VAR x, y: DevCPL486.Item);
	BEGIN
		IF y.form = Set THEN Check(x, 0, 31) END;
		DevCPL486.GenBitOp(BT, x, y); Free(x); Free(y);
		setCC(x, lss, FALSE, FALSE); (* carry set *)
	END In;
	
	PROCEDURE Shift* (VAR x, y: DevCPL486.Item; subcl: BYTE);	(* ASH, LSH, ROT *)
		VAR L1, L2: DevCPL486.Label; c: DevCPL486.Item; opl, opr: INTEGER;
	BEGIN
		IF subcl = ash THEN opl := SHL; opr := SAR
		ELSIF subcl = lsh THEN opl := SHL; opr := SHR
		ELSE opl := ROL; opr := ROR
		END;
		IF y.mode = Con THEN
			IF y.offset > 0 THEN
				DevCPL486.GenShiftOp(opl, y, x)
			ELSIF y.offset < 0 THEN
				y.offset := -y.offset;
				DevCPL486.GenShiftOp(opr, y, x)
			END
		ELSE
			ASSERT(y.mode = Reg);
			Check(y, -31, 31);
			L1 := DevCPL486.NewLbl; L2 := DevCPL486.NewLbl; 
			DevCPL486.MakeConst(c, 0, y.form); DevCPL486.GenComp(c, y);
			DevCPL486.GenJump(ccNS, L1, TRUE);
			DevCPL486.GenNeg(y, FALSE);
			DevCPL486.GenShiftOp(opr, y, x);
			DevCPL486.GenJump(ccAlways, L2, TRUE);
			DevCPL486.SetLabel(L1);
			DevCPL486.GenShiftOp(opl, y, x);
			DevCPL486.SetLabel(L2);
			Free(y)
		END;
		IF x.mode # Reg THEN Free(x) END
	END Shift;

	PROCEDURE DivMod* (VAR x, y: DevCPL486.Item; mod: BOOLEAN);
		VAR s: SET; r: DevCPL486.Item; pos: BOOLEAN;
	BEGIN
		ASSERT((x.mode = Reg) & (x.reg = AX)); pos := FALSE;
		IF y.mode = Con THEN pos := (y.offset > 0) & (y.obj = NIL); Load(y, {}, {AX, DX, con}) END;
		DevCPL486.GenDiv(y, mod, pos); Free(y);
		IF mod THEN
			r := x; GetReg(x, x.form, {}, wreg - {AX, DX}); Free(r) (* ax -> dx; al -> ah *)	(* ??? *)
		END
	END DivMod;

	PROCEDURE Mem* (VAR x: DevCPL486.Item; offset: INTEGER; typ: DevCPT.Struct);	(* x := Mem[x+offset] *)
	BEGIN
		IF x.mode = Con THEN x.mode := Abs; x.obj := NIL; INC(x.offset, offset)
		ELSE ASSERT(x.mode = Reg); x.mode := Ind; x.offset := offset
		END;
		x.scale := 0; x.typ := typ; x.form := typ.form
	END Mem;
	
	PROCEDURE SysMove* (VAR len: DevCPL486.Item);	(* implementation of SYSTEM.MOVE *)
	BEGIN
		IF len.mode = Con THEN
			IF len.offset > 0 THEN DevCPL486.GenBlockMove(1, len.offset) END
		ELSE
			Load(len, {}, wreg - {CX} + {short, mem, stk}); DevCPL486.GenBlockMove(1, 0); Free(len)
		END;
		FreeWReg(SI); FreeWReg(DI)
	END SysMove;
	
	PROCEDURE Len* (VAR x, y: DevCPL486.Item);
		VAR typ: DevCPT.Struct; dim: INTEGER;
	BEGIN
		dim := y.offset; typ := x.typ;
		IF typ.untagged THEN DevCPM.err(136) END;
		WHILE dim > 0 DO typ := typ.BaseTyp; DEC(dim) END;
		LenDesc(x, x, typ);
	END Len;
	
	PROCEDURE StringWSize (VAR x: DevCPL486.Item): INTEGER;
	BEGIN
		CASE x.form OF
		| String8, VString8: RETURN 1
		| String16, VString16: RETURN 2
		| VString16to8: RETURN 0
		| Comp: RETURN x.typ.BaseTyp.size
		END
	END StringWSize;

	PROCEDURE CmpString* (VAR x, y: DevCPL486.Item; rel: BYTE; rev: BOOLEAN);
		VAR sw, dw: INTEGER;
	BEGIN
		CheckAv(CX);
		IF (x.typ = DevCPT.guidtyp) OR (y.typ = DevCPT.guidtyp) THEN
			DevCPL486.GenBlockComp(4, 4)
		ELSIF x.form = String8 THEN DevCPL486.GenBlockComp(1, x.index)
		ELSIF y.form = String8 THEN DevCPL486.GenBlockComp(1, y.index)
		ELSIF x.form = String16 THEN DevCPL486.GenBlockComp(2, x.index)
		ELSIF y.form = String16 THEN DevCPL486.GenBlockComp(2, y.index)
		ELSE DevCPL486.GenStringComp(StringWSize(y), StringWSize(x))
		END;
		FreeWReg(SI); FreeWReg(DI); setCC(x, rel, ~rev, FALSE);
	END CmpString;

	PROCEDURE VarParDynArr (ftyp: DevCPT.Struct; VAR y: DevCPL486.Item);
		VAR len, z: DevCPL486.Item; atyp: DevCPT.Struct;
	BEGIN
		atyp := y.typ;
		WHILE ftyp.comp = DynArr DO
			IF ftyp.BaseTyp = DevCPT.bytetyp THEN
				IF atyp.comp = DynArr THEN
					IF atyp.untagged THEN DevCPM.err(137) END;
					LenDesc(y, len, atyp);
					IF y.tmode = VarPar THEN Free(len) END;	(* ??? *)
					GetReg(z, Int32, {}, {}); DevCPL486.GenMove(len, z);
					len.mode := Reg; len.reg := z.reg; atyp := atyp.BaseTyp;
					WHILE atyp.comp = DynArr DO
						LenDesc(y, z, atyp); DevCPL486.GenMul(z, len, FALSE);
						IF y.tmode = VarPar THEN Free(z) END;	(* ??? *)
						atyp := atyp.BaseTyp
					END;
					DevCPL486.MakeConst(z, atyp.size, Int32); DevCPL486.GenMul(z, len, FALSE);
					Free(len)
				ELSE
					DevCPL486.MakeConst(len, atyp.size, Int32)
				END
			ELSE
				IF atyp.comp = DynArr THEN LenDesc(y, len, atyp);
					IF atyp.untagged THEN DevCPM.err(137) END;
					IF y.tmode = VarPar THEN Free(len) END;	(* ??? *)
				ELSE DevCPL486.MakeConst(len, atyp.n, Int32)
				END
			END;
			DevCPL486.GenPush(len);
			ftyp := ftyp.BaseTyp; atyp := atyp.BaseTyp
		END
	END VarParDynArr;

	PROCEDURE Assign* (VAR x, y: DevCPL486.Item); (* x := y *)
	BEGIN
		IF y.mode = Con THEN
			IF y.form IN {Real32, Real64} THEN
				DevCPL486.GenFLoad(y); GetReg(y, Real32, {}, {});
				IF x.mode # Reg THEN Free(y); DevCPL486.GenFStore(x, TRUE) END	(* ??? move const *)
			ELSIF x.form = Int64 THEN
				ASSERT(x.mode IN {Ind, Abs});
				y.form := Int32; x.form := Int32; DevCPL486.GenMove(y, x);
				y.offset := y.scale; INC(x.offset, 4); DevCPL486.GenMove(y, x);
				DEC(x.offset, 4); x.form := Int64
			ELSE
				DevCPL486.GenMove(y, x)
			END
		ELSE
			IF y.form IN {Comp, String8, String16, VString8, VString16} THEN	(* convert to pointer *)
				ASSERT(x.form = Pointer);
				GetAdr(y, {}, {}); y.typ := x.typ; y.form := Pointer
			END;
			IF ~(x.form IN realSet) OR ~(y.form IN intSet) THEN Assert(y, {}, {stk}) END;
			ConvMove(x, y, FALSE, {}, {})
		END;
		Free(x)
	END Assign;
	
	PROCEDURE ArrayLen* (VAR x, len: DevCPL486.Item; hint, stop: SET);
		VAR c: DevCPL486.Item;
	BEGIN
		IF x.typ.comp = Array THEN DevCPL486.MakeConst(c, x.typ.n, Int32); GetReg(len, Int32, hint, stop); DevCPL486.GenMove(c, len)
		ELSIF ~x.typ.untagged THEN LenDesc(x, c, x.typ); GetReg(len, Int32, hint, stop); DevCPL486.GenMove(c, len)
		ELSE len.mode := Con
		END;
		len.typ := DevCPT.int32typ
	END ArrayLen;

(*
		src		dest	zero
sx	= sy	x b		y b
SHORT(lx)	= sy	x b+	x w	y b
SHORT(lx)	= SHORT(ly)	x b+	x w	y b+

lx	= ly	x w		y w
LONG(sx)	= ly	x b		y w	*
LONG(SHORT(lx))	= ly	x b+	x w*	y w	*

sx	:= sy	y b		x b
sx	:= SHORT(ly)	y b+	y w	x b

lx	:= ly	y w		x w
lx	:= LONG(sy)	y b		x w	*
lx	:= LONG(SHORT(ly))	y b+	y w*	x w	*
*)
	
	PROCEDURE AddCopy* (VAR x, y: DevCPL486.Item; last: BOOLEAN); (* x := .. + y + .. *)
	BEGIN
		IF (x.typ.comp = DynArr) & x.typ.untagged THEN
			DevCPL486.GenStringMove(~last, StringWSize(y), StringWSize(x), -1)
		ELSE
			DevCPL486.GenStringMove(~last, StringWSize(y), StringWSize(x), 0)
		END;
		FreeWReg(SI); FreeWReg(DI)
	END AddCopy;
	
	PROCEDURE Copy* (VAR x, y: DevCPL486.Item; short: BOOLEAN); (* x := y *)
		VAR sx, sy, sy2, sy4: INTEGER; c, r: DevCPL486.Item;
	BEGIN
		sx := x.typ.size; CheckAv(CX);
		IF y.form IN {String8, String16} THEN
			sy := y.index * y.typ.BaseTyp.size;
			IF x.typ.comp = Array THEN	(* adjust size for optimal performance *)
				sy2 := sy + sy MOD 2; sy4 := sy2 + sy2 MOD 4;
				IF sy4 <= sx THEN sy := sy4
				ELSIF sy2 <= sx THEN sy := sy2
				ELSIF sy > sx THEN DevCPM.err(114); sy := 1
				END
			ELSIF inxchk & ~x.typ.untagged THEN	(* check array length *)
				Free(x); LenDesc(x, c, x.typ);
				DevCPL486.MakeConst(y, y.index, Int32);
				DevCPL486.GenComp(y, c); DevCPL486.GenAssert(ccAE, copyTrap);
				Free(c)
			END;
			DevCPL486.GenBlockMove(1, sy)
		ELSIF x.typ.comp = DynArr THEN
			IF x.typ.untagged THEN
				DevCPL486.GenStringMove(FALSE, StringWSize(y), StringWSize(x), -1)
			ELSE
				Free(x); LenDesc(x, c, x.typ); DevCPL486.MakeReg(r, CX, Int32); DevCPL486.GenMove(c, r); Free(c);
				DevCPL486.GenStringMove(FALSE, StringWSize(y), StringWSize(x), 0)
			END
		ELSIF y.form IN {VString16to8, VString8, VString16} THEN
			DevCPL486.GenStringMove(FALSE, StringWSize(y), StringWSize(x), x.typ.n);
			ASSERT(y.mode # Stk)
		ELSIF short THEN	(* COPY *)
			sy := y.typ.size;
			IF (y.typ.comp # DynArr) & (sy < sx) THEN sx := sy END;
			DevCPL486.GenStringMove(FALSE, StringWSize(y), StringWSize(x), x.typ.n);
			IF y.mode = Stk THEN AdjustStack(sy) END
		ELSE	(* := *)
			IF sx > 0 THEN DevCPL486.GenBlockMove(1, sx) END;
			IF y.mode = Stk THEN AdjustStack(sy) END
		END;
		FreeWReg(SI); FreeWReg(DI)
	END Copy;
	
	PROCEDURE StrLen* (VAR x: DevCPL486.Item; typ: DevCPT.Struct; incl0x: BOOLEAN);
		VAR c: DevCPL486.Item;
	BEGIN
		CheckAv(AX); CheckAv(CX);
		DevCPL486.GenStringLength(typ.BaseTyp.size, -1);
		Free(x); GetReg(x, Int32, {}, wreg - {CX});
		DevCPL486.GenNot(x);
		IF ~incl0x THEN DevCPL486.MakeConst(c, 1, Int32); DevCPL486.GenSub(c, x, FALSE) END;
		FreeWReg(DI)
	END StrLen;

	PROCEDURE MulDim* (VAR y, z: DevCPL486.Item; VAR fact: INTEGER; dimtyp: DevCPT.Struct);	(* z := z * y *)
		VAR c: DevCPL486.Item;
	BEGIN
		IF y.mode = Con THEN
			IF y.offset <= MAX(INTEGER) DIV fact THEN fact := fact * y.offset
			ELSE fact := 1; DevCPM.err(214)
			END
		ELSE
			IF ranchk OR inxchk THEN
				DevCPL486.MakeConst(c, 0, Int32); DevCPL486.GenComp(c, y); DevCPL486.GenAssert(ccG, ranTrap)
			END;
			DevCPL486.GenPush(y);
			IF z.mode = Con THEN z := y
			ELSE DevCPL486.GenMul(y, z, ovflchk OR inxchk); Free(y)
			END
		END
	END MulDim;
	
	PROCEDURE SetDim* (VAR x, y: DevCPL486.Item; dimtyp: DevCPT.Struct); (* set LEN(x^, -dimtyp.n) *)
		(* y const or on stack *) 
		VAR z: DevCPL486.Item; end: DevCPL486.Label;
	BEGIN
		ASSERT((x.mode = Reg) & (x.form = Pointer));
		z.mode := Ind; z.reg := x.reg; z.offset := ArrDOffs + 4 + dimtyp.n * 4; z.scale := 0; z.form := Int32;
		IF y.mode = Con THEN y.form := Int32
		ELSE Pop(y, Int32, {}, {})
		END;
		end := DevCPL486.NewLbl; DevCPL486.GenJump(ccE, end, TRUE);	(* flags set in New *)
		DevCPL486.GenMove(y, z);
		DevCPL486.SetLabel(end);
		IF y.mode = Reg THEN Free(y) END
	END SetDim;

	PROCEDURE SysNew* (VAR x: DevCPL486.Item);
	BEGIN
		DevCPM.err(141)
	END SysNew;

	PROCEDURE New* (VAR x, nofel: DevCPL486.Item; fact: INTEGER);
		(* x.typ.BaseTyp.comp IN {Record, Array, DynArr} *)
		VAR p, tag, c: DevCPL486.Item; nofdim, dlen, n: INTEGER; typ, eltyp: DevCPT.Struct; lbl: DevCPL486.Label;
	BEGIN
		typ := x.typ.BaseTyp;
		IF typ.untagged THEN DevCPM.err(138) END;
		IF typ.comp = Record THEN	(* call to Kernel.NewRec(tag: Tag): ADDRESS *)
			DevCPL486.MakeConst(tag, 0, Pointer); tag.obj := DevCPE.TypeObj(typ);
			IF ContainsIPtrs(typ) THEN INC(tag.offset) END;
			DevCPL486.GenPush(tag);
			p.mode := XProc; p.obj := DevCPE.KNewRec;
		ELSE eltyp := typ.BaseTyp;
			IF typ.comp = Array THEN
				nofdim := 0; nofel.mode := Con; nofel.form := Int32; fact := typ.n
			ELSE (* DynArr *)
				nofdim := typ.n+1;
				WHILE eltyp.comp = DynArr DO eltyp := eltyp.BaseTyp END
			END ;
			WHILE eltyp.comp = Array DO fact := fact * eltyp.n; eltyp := eltyp.BaseTyp END;
			IF eltyp.comp = Record THEN
				IF eltyp.untagged THEN DevCPM.err(138) END;
				DevCPL486.MakeConst(tag, 0, Pointer); tag.obj := DevCPE.TypeObj(eltyp);
				IF ContainsIPtrs(eltyp) THEN INC(tag.offset) END;
			ELSIF eltyp.form = Pointer THEN
				IF ~eltyp.untagged THEN
					DevCPL486.MakeConst(tag, 0, Pointer)	(* special TDesc in Kernel for ARRAY OF pointer *)
				ELSIF eltyp.sysflag = interface THEN
					DevCPL486.MakeConst(tag, -1, Pointer)	(* special TDesc in Kernel for ARRAY OF interface pointer *)
				ELSE
					DevCPL486.MakeConst(tag, 12, Pointer)
				END
			ELSE	(* eltyp is pointerless basic type *)
				CASE eltyp.form OF
				| Undef, Byte, Char8: n := 1;
				| Int16: n := 2;
				| Int8: n := 3;
				| Int32: n := 4;
				| Bool: n := 5;
				| Set: n := 6;
				| Real32: n := 7;
				| Real64: n := 8;
				| Char16: n := 9;
				| Int64: n := 10;
				| ProcTyp: n := 11;
				END;
				DevCPL486.MakeConst(tag, n, Pointer)
(*
				DevCPL486.MakeConst(tag, eltyp.size, Pointer)
*)
			END;
			IF nofel.mode = Con THEN nofel.offset := fact; nofel.obj := NIL
			ELSE DevCPL486.MakeConst(p, fact, Int32); DevCPL486.GenMul(p, nofel, ovflchk OR inxchk)
			END;
			DevCPL486.MakeConst(p, nofdim, Int32); DevCPL486.GenPush(p);
			DevCPL486.GenPush(nofel); Free(nofel); DevCPL486.GenPush(tag);
			p.mode := XProc; p.obj := DevCPE.KNewArr;
		END;
		DevCPL486.GenCall(p); GetReg(x, Pointer, {}, wreg - {AX});
		IF typ.comp = DynArr THEN	(* set flags for nil test *)
			DevCPL486.MakeConst(c, 0, Pointer); DevCPL486.GenComp(c, x)
		ELSIF typ.comp = Record THEN
			n := NumOfIntProc(typ);
			IF n > 0 THEN	(* interface method table pointer setup *)
				DevCPL486.MakeConst(c, 0, Pointer); DevCPL486.GenComp(c, x);
				lbl := DevCPL486.NewLbl; DevCPL486.GenJump(ccE, lbl, TRUE);
				tag.offset := - 4 * (n + numPreIntProc);
				p.mode := Ind; p.reg := AX; p.offset := 0; p.scale := 0; p.form := Pointer;
				DevCPL486.GenMove(tag, p);
				IF nofel.mode # Con THEN	(* unk pointer setup *)
					p.offset := 8;
					DevCPL486.GenMove(nofel, p);
					Free(nofel)
				END;
				DevCPL486.SetLabel(lbl);
			END
		END
	END New;

	PROCEDURE Param* (fp: DevCPT.Object; rec, niltest: BOOLEAN; VAR ap, tag: DevCPL486.Item);	(* returns tag if rec *)
		VAR f: BYTE; s, ss: INTEGER; par, a, c: DevCPL486.Item; recTyp: DevCPT.Struct;
	BEGIN
		par.mode := Stk; par.typ := fp.typ; par.form := par.typ.form;
		IF ODD(fp.sysflag DIV nilBit) THEN niltest := FALSE END;
		IF ap.typ = DevCPT.niltyp THEN
			IF ((par.typ.comp = Record) OR (par.typ.comp = DynArr)) & ~par.typ.untagged THEN
				DevCPM.err(142)
			END;
			DevCPL486.GenPush(ap)
		ELSIF par.typ.comp = DynArr THEN
			IF ap.form IN {String8, String16} THEN
				IF ~par.typ.untagged THEN
					DevCPL486.MakeConst(c, ap.index (* * ap.typ.BaseTyp.size *), Int32); DevCPL486.GenPush(c)
				END;
				ap.mode := Con; DevCPL486.GenPush(ap);
			ELSIF ap.form IN {VString8, VString16} THEN
				DevCPL486.MakeReg(a, DX, Pointer); DevCPL486.GenLoadAdr(ap, a);
				IF ~par.typ.untagged THEN
					DevCPL486.MakeReg(c, DI, Pointer); DevCPL486.GenMove(a, c);
					Free(ap); StrLen(c, ap.typ, TRUE);
					DevCPL486.GenPush(c); Free(c)
				END;
				DevCPL486.GenPush(a)
			ELSE
				IF ~par.typ.untagged THEN
					IF ap.typ.comp = DynArr THEN niltest := FALSE END;	(* ap dereferenced for length descriptor *)
					VarParDynArr(par.typ, ap)
				END;
				PushAdr(ap, niltest)
			END
		ELSIF fp.mode = VarPar THEN
			recTyp := ap.typ;
			IF recTyp.form = Pointer THEN recTyp := recTyp.BaseTyp END;
			IF (par.typ.comp = Record) & (~fp.typ.untagged) THEN
				Tag(ap, tag);
				IF rec & (tag.mode # Con) THEN
					GetReg(c, Pointer, {}, {}); DevCPL486.GenMove(tag, c); tag := c
				END;
				DevCPL486.GenPush(tag);
				IF tag.mode # Con THEN niltest := FALSE END;
				PushAdr(ap, niltest);
				IF rec THEN Free(tag) END
			ELSE PushAdr(ap, niltest)
			END;
			tag.typ := recTyp
		ELSIF par.form = Comp THEN
			s := par.typ.size;
			IF initializeStr & (ap.form IN {String8, String16, VString8, VString16, VString16to8}) THEN
				s := (s + 3) DIV 4 * 4; AdjustStack(-s);
				IF ap.form IN {String8, String16} THEN
					IF ap.index > 1 THEN	(* nonempty string *)
						ss := (ap.index * ap.typ.BaseTyp.size + 3) DIV 4 * 4;
						DevCPL486.MakeReg(c, SI, Pointer); DevCPL486.GenLoadAdr(ap, c); Free(ap);
						DevCPL486.MakeReg(c, DI, Pointer); DevCPL486.GenLoadAdr(par, c);
						DevCPL486.GenBlockMove(1, ss);
					ELSE
						ss := 0;
						DevCPL486.MakeReg(c, DI, Pointer); DevCPL486.GenLoadAdr(par, c)
					END;
					IF s > ss THEN
						DevCPL486.MakeReg(a, AX, Int32); DevCPL486.MakeConst(c, 0, Int32); DevCPL486.GenMove(c, a);
						DevCPL486.GenBlockStore(1, s - ss)
					END;
				ELSE
					DevCPL486.MakeReg(c, SI, Pointer); DevCPL486.GenLoadAdr(ap, c); Free(ap);
					DevCPL486.MakeReg(c, DI, Pointer); DevCPL486.GenLoadAdr(par, c);
					DevCPL486.GenStringMove(TRUE, StringWSize(ap), StringWSize(par), par.typ.n);
					DevCPL486.MakeReg(a, AX, Int32); DevCPL486.MakeConst(c, 0, Int32); DevCPL486.GenMove(c, a);
					DevCPL486.GenBlockStore(StringWSize(par), 0)
				END
			ELSE
				IF (ap.form IN {String8, String16}) & (ap.index = 1) THEN	(* empty string *)
					AdjustStack((4 - s) DIV 4 * 4);
					DevCPL486.MakeConst(c, 0, Int32); DevCPL486.GenPush(c)
				ELSE
					AdjustStack((-s) DIV 4 * 4);
					DevCPL486.MakeReg(c, SI, Pointer); DevCPL486.GenLoadAdr(ap, c); Free(ap);
					DevCPL486.MakeReg(c, DI, Pointer); DevCPL486.GenLoadAdr(par, c);
					IF ap.form IN {String8, String16} THEN
						DevCPL486.GenBlockMove(1, (ap.index * ap.typ.BaseTyp.size + 3) DIV 4 * 4)
					ELSIF ap.form IN {VString8, VString16, VString16to8} THEN
						DevCPL486.GenStringMove(FALSE, StringWSize(ap), StringWSize(par), par.typ.n)
					ELSE
						DevCPL486.GenBlockMove(1, (s + 3) DIV 4 * 4)
					END
				END
			END
		ELSIF ap.mode = Con THEN
			IF ap.form IN {Real32, Real64} THEN	(* ??? push const *)
				DevCPL486.GenFLoad(ap); DecStack(par.typ.form); DevCPL486.GenFStore(par, TRUE)
			ELSE
				ap.form := Int32;
				IF par.form = Int64 THEN DevCPL486.MakeConst(c, ap.scale, Int32); DevCPL486.GenPush(c) END;
				DevCPL486.GenPush(ap)
			END
		ELSIF ap.typ.form = Pointer THEN
			recTyp := ap.typ.BaseTyp;
			IF rec THEN
				Load(ap, {}, {}); Tag(ap, tag);
				IF tag.mode = Con THEN	(* explicit nil test needed *)
					DevCPL486.MakeReg(a, AX, Int32);
					c.mode := Ind; c.form := Int32; c.offset := 0; c.scale := 0; c.reg := ap.reg;
					DevCPL486.GenTest(a, c)
				END
			END;
			DevCPL486.GenPush(ap); Free(ap);
			tag.typ := recTyp
		ELSIF ap.form IN {Comp, String8, String16, VString8, VString16} THEN	(* convert to pointer *)
			ASSERT(par.form = Pointer);
			PushAdr(ap, FALSE)
		ELSE
			ConvMove(par, ap, FALSE, {}, {high});
		END
	END Param;
	
	PROCEDURE Result* (proc: DevCPT.Object; VAR res: DevCPL486.Item);
		VAR r: DevCPL486.Item;
	BEGIN
		DevCPL486.MakeReg(r, AX, proc.typ.form);	(* don't allocate AX ! *)
		IF res.mode = Con THEN
			IF r.form IN {Real32, Real64} THEN DevCPL486.GenFLoad(res);
			ELSIF r.form = Int64 THEN
				r.form := Int32; res.form := Int32; DevCPL486.GenMove(res, r);
				r.reg := DX; res.offset := res.scale; DevCPL486.GenMove(res, r)
			ELSE DevCPL486.GenMove(res, r);
			END
		ELSIF res.form IN {Comp, String8, String16, VString8, VString16} THEN	(* convert to pointer *)
			ASSERT(r.form = Pointer);
			GetAdr(res, {}, wreg - {AX})
		ELSE
			r.index := DX;	(* for int64 *)
			ConvMove(r, res, FALSE, wreg - {AX} + {high}, {});
		END;
		Free(res)
	END Result;
	
	PROCEDURE InitFpu;
		VAR x: DevCPL486.Item;
	BEGIN
		DevCPL486.MakeConst(x, FpuControlRegister, Int32); DevCPL486.GenPush(x);
		DevCPL486.GenFMOp(12CH); DevCPL486.GenCode(24H);	(* FLDCW 0(SP) *)
		DevCPL486.MakeReg(x, CX, Int32); DevCPL486.GenPop(x);	(* reset stack *)
	END InitFpu;
	
	PROCEDURE PrepCall* (proc: DevCPT.Object);
		VAR lev: BYTE; r: DevCPL486.Item;
	BEGIN
		lev := proc.mnolev;
		IF (slNeeded IN proc.conval.setval) & (imLevel[lev] > 0) & (imLevel[DevCPL486.level] > imLevel[lev]) THEN
			DevCPL486.MakeReg(r, BX, Pointer); DevCPL486.GenPush(r)
		END
	END PrepCall;
	
	PROCEDURE Call* (VAR x, tag: DevCPL486.Item);	(* TProc: tag.typ = actual receiver type *)
		VAR i, n: INTEGER; r, y: DevCPL486.Item; typ: DevCPT.Struct; lev: BYTE; saved: BOOLEAN; p: DevCPT.Object;
	BEGIN
		IF x.mode IN {LProc, XProc, IProc} THEN
			lev := x.obj.mnolev; saved := FALSE;
			IF (slNeeded IN x.obj.conval.setval) & (imLevel[lev] > 0) THEN	(* pass static link *)
				n := imLevel[DevCPL486.level] - imLevel[lev];
				IF n > 0 THEN
					saved := TRUE;
					y.mode := Ind; y.scale := 0; y.form := Pointer; y.reg := BX; y.offset := -4;
					DevCPL486.MakeReg(r, BX, Pointer);
					WHILE n > 0 DO DevCPL486.GenMove(y, r); DEC(n) END
				END
			END;
			DevCPL486.GenCall(x);
			IF x.obj.sysflag = ccall THEN	(* remove parameters *)
				p := x.obj.link; n := 0;
				WHILE p # NIL DO
					IF p.mode = VarPar THEN INC(n, 4)
					ELSE INC(n, (p.typ.size + 3) DIV 4 * 4)
					END;
					p := p.link
				END;
				AdjustStack(n)
			END;
			IF saved THEN DevCPL486.GenPop(r) END;
		ELSIF x.mode = TProc THEN
			IF x.scale = 1 THEN (* super *)
				DevCPL486.MakeConst(tag, 0, Pointer); tag.obj := DevCPE.TypeObj(tag.typ.BaseTyp)
			ELSIF x.scale = 2 THEN (* static call *)
				DevCPL486.MakeConst(tag, 0, Pointer); typ := x.obj.link.typ;
				IF typ.form = Pointer THEN typ := typ.BaseTyp END;
				tag.obj := DevCPE.TypeObj(typ)
			ELSIF x.scale = 3 THEN (* interface method call *)
				DevCPM.err(200)
			END;
			IF tag.mode = Con THEN
				y.mode := Abs; y.offset := tag.offset; y.obj := tag.obj; y.scale := 0
			ELSIF (x.obj.conval.setval * {absAttr, empAttr, extAttr} = {}) & ~(DevCPM.oberon IN DevCPM.options) THEN	(* final method *)
				y.mode := Abs; y.offset := 0; y.obj := DevCPE.TypeObj(tag.typ); y.scale := 0;
				IF tag.mode = Ind THEN	(* nil test *)
					DevCPL486.MakeReg(r, AX, Int32); tag.offset := 0; DevCPL486.GenTest(r, tag)
				END
			ELSE
				IF tag.mode = Reg THEN y.reg := tag.reg
				ELSE GetReg(y, Pointer, {}, {}); DevCPL486.GenMove(tag, y)
				END;
				y.mode := Ind; y.offset := 0; y.scale := 0
			END;
			IF (tag.typ.sysflag = interface) & (y.mode = Ind) THEN y.offset := 4 * x.offset
			ELSIF tag.typ.untagged THEN DevCPM.err(140)
			ELSE
				IF x.obj.link.typ.sysflag = interface THEN	(* correct method number *)
					x.offset := numPreIntProc + NumOfIntProc(tag.typ) - 1 - x.offset
				END;
				INC(y.offset, Mth0Offset - 4 * x.offset)
			END;
			DevCPL486.GenCall(y); Free(y)
		ELSIF x.mode = CProc THEN
			IF x.obj.link # NIL THEN	(* tag = first param *)
				IF x.obj.link.mode = VarPar THEN
					GetAdr(tag, {}, wreg - {AX} + {stk, mem, con}); Free(tag)
				ELSE 
					(* Load(tag, {}, wreg - {AX} + {con}); Free(tag) *)
					Result(x.obj.link, tag)	(* use result load for first parameter *)
				END
			END;
			i := 1; n := ORD(x.obj.conval.ext^[0]);
			WHILE i <= n DO DevCPL486.GenCode(ORD(x.obj.conval.ext^[i])); INC(i) END
		ELSE	(* proc var *)
			DevCPL486.GenCall(x); Free(x);
			IF x.typ.sysflag = ccall THEN	(* remove parameters *)
				p := x.typ.link; n := 0;
				WHILE p # NIL DO
					IF p.mode = VarPar THEN INC(n, 4)
					ELSE INC(n, (p.typ.size + 3) DIV 4 * 4)
					END;
					p := p.link
				END;
				AdjustStack(n)
			END;
			x.typ := x.typ.BaseTyp
		END;
		IF procedureUsesFpu & (x.mode = XProc) & (x.obj.mnolev < 0) & (x.obj.mnolev > -128)
				& ((x.obj.library # NIL) OR (DevCPT.GlbMod[-x.obj.mnolev].library # NIL)) THEN	(* restore fpu *)
			InitFpu
		END;
		CheckReg;
		IF x.typ.form = Int64 THEN
			GetReg(x, Int32, {}, wreg - {AX}); GetReg(y, Int32, {}, wreg - {DX});
			x.index := y.reg; x.form := Int64
		ELSIF x.typ.form # NoTyp THEN GetReg(x, x.typ.form, {}, wreg - {AX} + {high})
		END
	END Call;
	
	PROCEDURE CopyDynArray* (adr: INTEGER; typ: DevCPT.Struct);	(* needs CX, SI, DI *)
		VAR len, ptr, c, sp, src, dst: DevCPL486.Item; bt: DevCPT.Struct;
	BEGIN
		IF typ.untagged THEN DevCPM.err(-137) END;
		ptr.mode := Ind; ptr.reg := BP; ptr.offset := adr+4; ptr.scale := 0; ptr.form := Pointer;
		DevCPL486.MakeReg(len, CX, Int32); DevCPL486.MakeReg(sp, SP, Int32);
		DevCPL486.MakeReg(src, SI, Int32); DevCPL486.MakeReg(dst, DI, Int32);
		DevCPL486.GenMove(ptr, len); bt := typ.BaseTyp;
		WHILE bt.comp = DynArr DO
			INC(ptr.offset, 4); DevCPL486.GenMul(ptr, len, FALSE); bt := bt.BaseTyp
		END;
		ptr.offset := adr; DevCPL486.GenMove(ptr, src);
		DevCPL486.MakeConst(c, bt.size, Int32); DevCPL486.GenMul(c, len, FALSE);
		(* CX = length in bytes *)
		StackAlloc; 
		(* CX = length in 32bit words *)
		DevCPL486.GenMove(sp, dst); DevCPL486.GenMove(dst, ptr);
		DevCPL486.GenBlockMove(4, 0)  (* 32bit moves *)
	END CopyDynArray;
	
	PROCEDURE Sort (VAR tab: ARRAY OF INTEGER; VAR n: INTEGER);
		VAR i, j, x: INTEGER;
	BEGIN
		(* align *)
		i := 1;
		WHILE i < n DO
			x := tab[i]; j := i-1;
			WHILE (j >= 0) & (tab[j] < x) DO tab[j+1] := tab[j]; DEC(j) END;
			tab[j+1] := x; INC(i)
		END;
		(* eliminate equals *)
		i := 1; j := 1;
		WHILE i < n DO
			IF tab[i] # tab[i-1] THEN tab[j] := tab[i]; INC(j) END;
			INC(i)
		END;
		n := j
	END Sort;
	
	PROCEDURE FindPtrs (typ: DevCPT.Struct; adr: INTEGER; VAR num: INTEGER);
		VAR fld: DevCPT.Object; btyp: DevCPT.Struct; i, n: INTEGER;
	BEGIN
		IF typ.form IN {Pointer, ProcTyp} THEN
			IF num < MaxPtrs THEN ptrTab[num] := adr DIV 4 * 4 END;
			INC(num);
			IF adr MOD 4 # 0 THEN
				IF num < MaxPtrs THEN ptrTab[num] := adr DIV 4 * 4 + 4 END;
				INC(num)
			END
		ELSIF typ.comp = Record THEN
			btyp := typ.BaseTyp;
			IF btyp # NIL THEN FindPtrs(btyp, adr, num) END ;
			fld := typ.link;
			WHILE (fld # NIL) & (fld.mode = Fld) DO
				IF (fld.name^ = DevCPM.HdPtrName) OR
					(fld.name^ = DevCPM.HdUtPtrName) OR
					(fld.name^ = DevCPM.HdProcName) THEN
					FindPtrs(DevCPT.sysptrtyp, fld.adr + adr, num)
				ELSE FindPtrs(fld.typ, fld.adr + adr, num)
				END;
				fld := fld.link
			END
		ELSIF typ.comp = Array THEN
			btyp := typ.BaseTyp; n := typ.n;
			WHILE btyp.comp = Array DO n := btyp.n * n; btyp := btyp.BaseTyp END ;
			IF (btyp.form = Pointer) OR (btyp.comp = Record) THEN
				i := num; FindPtrs(btyp, adr, num);
				IF num # i THEN i := 1;
					WHILE (i < n) & (num <= MaxPtrs) DO
						INC(adr, btyp.size); FindPtrs(btyp, adr, num); INC(i)
					END
				END
			END
		END
	END FindPtrs;

	PROCEDURE InitOutPar (par: DevCPT.Object; VAR zreg: DevCPL486.Item);
		VAR x, y, c, len: DevCPL486.Item; lbl: DevCPL486.Label; size, s: INTEGER; bt: DevCPT.Struct;
	BEGIN
		x.mode := Ind; x.reg := BP; x.scale := 0; x.form := Pointer; x.offset := par.adr;
		DevCPL486.MakeReg(y, DI, Int32);
		IF par.typ.comp # DynArr THEN
			DevCPL486.GenMove(x, y);
			lbl := DevCPL486.NewLbl;
			IF ODD(par.sysflag DIV nilBit) THEN
				DevCPL486.GenComp(zreg, y);
				DevCPL486.GenJump(ccE, lbl, TRUE)
			END;
			size := par.typ.size;
			IF size <= 16 THEN
				x.mode := Ind; x.reg := DI; x.form := Int32; x.offset := 0;
				WHILE size > 0 DO
					IF size = 1 THEN x.form := Int8; s := 1
					ELSIF size = 2 THEN x.form := Int16; s := 2
					ELSE x.form := Int32; s := 4
					END;
					zreg.form := x.form; DevCPL486.GenMove(zreg, x); INC(x.offset, s); DEC(size, s)
				END;
				zreg.form := Int32
			ELSE
				DevCPL486.GenBlockStore(1, size)
			END;
			DevCPL486.SetLabel(lbl)
		ELSIF initializeDyn & ~par.typ.untagged THEN	(* untagged open arrays not initialized !!! *)
			DevCPL486.GenMove(x, y);
			DevCPL486.MakeReg(len, CX, Int32);
			INC(x.offset, 4); DevCPL486.GenMove(x, len); (* first len *)
			bt := par.typ.BaseTyp;
			WHILE bt.comp = DynArr DO
				INC(x.offset, 4); DevCPL486.GenMul(x, len, FALSE); bt := bt.BaseTyp
			END;
			size := bt.size;
			IF size MOD 4 = 0 THEN size := size DIV 4; s := 4
			ELSIF size MOD 2 = 0 THEN size := size DIV 2; s := 2
			ELSE s := 1
			END;
			DevCPL486.MakeConst(c, size, Int32); DevCPL486.GenMul(c, len, FALSE);
			DevCPL486.GenBlockStore(s, 0)
		END
	END InitOutPar;

	PROCEDURE AllocAndInitAll (proc: DevCPT.Object; adr, size: INTEGER; VAR nofptrs: INTEGER);
		VAR x, y, z, zero: DevCPL486.Item; par: DevCPT.Object; op: INTEGER;
	BEGIN
		op := 0; par := proc.link;
		WHILE par # NIL DO	(* count out parameters [with COM pointers] *)
			IF (par.mode = VarPar) & (par.vis = outPar) & (initializeOut OR ContainsIPtrs(par.typ)) THEN INC(op) END;
			par := par.link
		END;
		DevCPL486.MakeConst(zero, 0, Int32);
		IF (op = 0) & (size <= 8) THEN	(* use PUSH 0 *)
			WHILE size > 0 DO DevCPL486.GenPush(zero); DEC(size, 4) END
		ELSE
			DevCPL486.MakeReg(z, AX, Int32); DevCPL486.GenMove(zero, z);
			IF size <= 32 THEN	(* use PUSH reg *)
				WHILE size > 0 DO DevCPL486.GenPush(z); DEC(size, 4) END
			ELSE	(* use string store *)
				AdjustStack(-size);
				DevCPL486.MakeReg(x, SP, Int32); DevCPL486.MakeReg(y, DI, Int32); DevCPL486.GenMove(x, y);
				DevCPL486.GenBlockStore(1, size)
			END;
			IF op > 0 THEN
				par := proc.link;
				WHILE par # NIL DO	(* init out parameters [with COM pointers] *)
					IF (par.mode = VarPar) & (par.vis = outPar) & (initializeOut OR ContainsIPtrs(par.typ)) THEN InitOutPar(par, z) END;
					par := par.link
				END
			END
		END
	END AllocAndInitAll;
	
	PROCEDURE AllocAndInitPtrs1 (proc: DevCPT.Object; adr, size: INTEGER; VAR nofptrs: INTEGER);	(* needs AX *)
		VAR i, base, a, gaps: INTEGER; x, z: DevCPL486.Item; obj: DevCPT.Object;
	BEGIN
		IF ptrinit & (proc.scope # NIL) THEN
			nofptrs := 0; obj := proc.scope.scope;	(* local variables *)
			WHILE (obj # NIL) & (nofptrs <= MaxPtrs) DO
				FindPtrs(obj.typ, obj.adr, nofptrs);
				obj := obj.link
			END;
			IF (nofptrs > 0) & (nofptrs <= MaxPtrs) THEN
				base := proc.conval.intval2;
				Sort(ptrTab, nofptrs); i := 0; a := size + base; gaps := 0;
				WHILE i < nofptrs DO
					DEC(a, 4);
					IF a # ptrTab[i] THEN a := ptrTab[i]; INC(gaps) END;
					INC(i)
				END;
				IF a # base THEN INC(gaps) END;
				IF (gaps <= (nofptrs + 1) DIV 2) & (size < stackAllocLimit) THEN
					DevCPL486.MakeConst(z, 0, Pointer);
					IF (nofptrs > 4) THEN x := z; DevCPL486.MakeReg(z, AX, Int32); DevCPL486.GenMove(x, z) END;
					i := 0; a := size + base;
					WHILE i < nofptrs DO
						DEC(a, 4);
						IF a # ptrTab[i] THEN AdjustStack(ptrTab[i] - a); a := ptrTab[i] END;
						DevCPL486.GenPush(z); INC(i)
					END;
					IF a # base THEN AdjustStack(base - a) END
				ELSE
					AdjustStack(-size);
					DevCPL486.MakeConst(x, 0, Pointer); DevCPL486.MakeReg(z, AX, Int32); DevCPL486.GenMove(x, z);
					x.mode := Ind; x.reg := BP; x.scale := 0; x.form := Pointer; i := 0; 
					WHILE i < nofptrs DO
						x.offset := ptrTab[i]; DevCPL486.GenMove(z, x); INC(i)
					END
				END
			ELSE
				AdjustStack(-size)
			END
		ELSE
			nofptrs := 0;
			AdjustStack(-size)
		END
	END AllocAndInitPtrs1;

	PROCEDURE InitPtrs2 (proc: DevCPT.Object; adr, size, nofptrs: INTEGER);	(* needs AX, CX, DI *)
		VAR x, y, z, zero: DevCPL486.Item; obj: DevCPT.Object; zeroed: BOOLEAN; i: INTEGER; lbl: DevCPL486.Label;
	BEGIN
		IF ptrinit THEN
			zeroed := FALSE; DevCPL486.MakeConst(zero, 0, Pointer);
			IF nofptrs > MaxPtrs THEN
				DevCPL486.MakeReg(z, AX, Int32); DevCPL486.GenMove(zero, z); zeroed := TRUE;
				x.mode := Ind; x.reg := BP; x.scale := 0; x.form := Pointer; x.offset := adr;
				DevCPL486.MakeReg(y, DI, Int32); DevCPL486.GenLoadAdr(x, y);
				DevCPL486.GenStrStore(size)
			END;
			obj := proc.link;	(* parameters *)
			WHILE obj # NIL DO
				IF (obj.mode = VarPar) & (obj.vis = outPar) THEN
					nofptrs := 0;
					IF obj.typ.comp = DynArr THEN	(* currently not initialized *)
					ELSE FindPtrs(obj.typ, 0, nofptrs)
					END;
					IF nofptrs > 0 THEN
						IF ~zeroed THEN
							DevCPL486.MakeReg(z, AX, Int32); DevCPL486.GenMove(zero, z); zeroed := TRUE
						END;
						x.mode := Ind; x.reg := BP; x.scale := 0; x.form := Pointer; x.offset := obj.adr;
						DevCPL486.MakeReg(y, DI, Int32); DevCPL486.GenMove(x, y);
						IF ODD(obj.sysflag DIV nilBit) THEN
							DevCPL486.GenComp(zero, y);
							lbl := DevCPL486.NewLbl; DevCPL486.GenJump(ccE, lbl, TRUE)
						END;
						IF nofptrs > MaxPtrs THEN
							DevCPL486.GenStrStore(obj.typ.size)
						ELSE
							Sort(ptrTab, nofptrs);
							x.reg := DI; i := 0;
							WHILE i < nofptrs DO
								x.offset := ptrTab[i]; DevCPL486.GenMove(z, x); INC(i)
							END
						END;
						IF ODD(obj.sysflag DIV nilBit) THEN DevCPL486.SetLabel(lbl) END
					END
				END;
				obj := obj.link
			END
		END
	END InitPtrs2;
	
	PROCEDURE NeedOutPtrInit (proc: DevCPT.Object): BOOLEAN;
		VAR obj: DevCPT.Object; nofptrs: INTEGER;
	BEGIN
		IF ptrinit THEN
			obj := proc.link;
			WHILE obj # NIL DO
				IF (obj.mode = VarPar) & (obj.vis = outPar) THEN
					nofptrs := 0;
					IF obj.typ.comp = DynArr THEN	(* currently not initialized *)
					ELSE FindPtrs(obj.typ, 0, nofptrs)
					END;
					IF nofptrs > 0 THEN RETURN TRUE END
				END;
				obj := obj.link
			END
		END;
		RETURN FALSE
	END NeedOutPtrInit;
	
	PROCEDURE Enter* (proc: DevCPT.Object; empty, useFpu: BOOLEAN);
		VAR sp, fp, r, r1: DevCPL486.Item; par: DevCPT.Object; adr, size, np: INTEGER;
	BEGIN
		procedureUsesFpu := useFpu;
		SetReg({AX, CX, DX, SI, DI});
		DevCPL486.MakeReg(fp, BP, Pointer); DevCPL486.MakeReg(sp, SP, Pointer);
		IF proc # NIL THEN (* enter proc *)
			DevCPL486.SetLabel(proc.adr);
			IF (~empty OR NeedOutPtrInit(proc)) & (proc.sysflag # noframe) THEN
				DevCPL486.GenPush(fp);
				DevCPL486.GenMove(sp, fp);
				adr := proc.conval.intval2; size := -adr;
				IF size < 0 THEN DevCPM.err(214); size := 256 END;
				IF isGuarded IN proc.conval.setval THEN
					DevCPL486.MakeReg(r, BX, Pointer); DevCPL486.GenPush(r);
					DevCPL486.MakeReg(r, DI, Pointer); DevCPL486.GenPush(r);
					DevCPL486.MakeReg(r, SI, Pointer); DevCPL486.GenPush(r);
					r1.mode := Con; r1.form := Int32; r1.offset := proc.conval.intval - 8; r1.obj := NIL;
					DevCPL486.GenPush(r1);
					intHandler.used := TRUE;
					r1.mode := Con; r1.form := Int32; r1.offset := 0; r1.obj := intHandler;
					DevCPL486.GenPush(r1);
					r1.mode := Abs; r1.form := Int32; r1.offset := 0; r1.scale := 0; r1.obj := NIL;
					DevCPL486.GenCode(64H); DevCPL486.GenPush(r1);
					DevCPL486.GenCode(64H); DevCPL486.GenMove(sp, r1);
					DEC(size, 24)
				ELSE
					IF imVar IN proc.conval.setval THEN	(* set down pointer *)
						DevCPL486.MakeReg(r, BX, Pointer); DevCPL486.GenPush(r); DEC(size, 4)
					END;
					IF isCallback IN proc.conval.setval THEN
						DevCPL486.MakeReg(r, DI, Pointer); DevCPL486.GenPush(r);
						DevCPL486.MakeReg(r, SI, Pointer); DevCPL486.GenPush(r); DEC(size, 8)
					END
				END;
				ASSERT(size >= 0);
				IF initializeAll THEN
					AllocAndInitAll(proc, adr, size, np)
				ELSE
					AllocAndInitPtrs1(proc, adr, size, np);	(* needs AX *)
					InitPtrs2(proc, adr, size, np);	(* needs AX, CX, DI *)
				END;
				par := proc.link;	(* parameters *)
				WHILE par # NIL DO
					IF (par.mode = Var) & (par.typ.comp = DynArr) THEN 
						CopyDynArray(par.adr, par.typ)
					END;
					par := par.link
				END;
				IF imVar IN proc.conval.setval THEN
					DevCPL486.MakeReg(r, BX, Pointer); DevCPL486.GenMove(fp, r)
				END
			END
		ELSIF ~empty THEN (* enter module *)
			DevCPL486.GenPush(fp);
			DevCPL486.GenMove(sp, fp);
			DevCPL486.MakeReg(r, DI, Int32); DevCPL486.GenPush(r);
			DevCPL486.MakeReg(r, SI, Int32); DevCPL486.GenPush(r)
		END;
		IF useFpu THEN InitFpu END
	END Enter;
	
	PROCEDURE Exit* (proc: DevCPT.Object; empty: BOOLEAN);
		VAR sp, fp, r, x: DevCPL486.Item; mode: SHORTINT; size: INTEGER;
	BEGIN
		DevCPL486.MakeReg(sp, SP, Pointer); DevCPL486.MakeReg(fp, BP, Pointer);
		IF proc # NIL THEN (* exit proc *)
			IF proc.sysflag # noframe THEN
				IF ~empty OR NeedOutPtrInit(proc) THEN
					IF isGuarded IN proc.conval.setval THEN	(* remove exception frame *)
						x.mode := Ind; x.reg := BP; x.offset := -24; x.scale := 0; x.form := Int32;
						DevCPL486.MakeReg(r, CX, Int32); DevCPL486.GenMove(x, r);
						x.mode := Abs; x.offset := 0; x.scale := 0; x.form := Int32; x.obj := NIL;
						DevCPL486.GenCode(64H); DevCPL486.GenMove(r, x);
						size := 12
					ELSE
						size := 0;
						IF imVar IN proc.conval.setval THEN INC(size, 4) END;
						IF isCallback IN proc.conval.setval THEN INC(size, 8) END
					END;
					IF size > 0 THEN
						x.mode := Ind; x.reg := BP; x.offset := -size; x.scale := 0; x.form := Int32;
						DevCPL486.GenLoadAdr(x, sp);
						IF size > 4 THEN
							DevCPL486.MakeReg(r, SI, Int32); DevCPL486.GenPop(r);
							DevCPL486.MakeReg(r, DI, Int32); DevCPL486.GenPop(r)
						END;
						IF size # 8 THEN
							DevCPL486.MakeReg(r, BX, Int32); DevCPL486.GenPop(r)
						END
					ELSE
						DevCPL486.GenMove(fp, sp)
					END;
					DevCPL486.GenPop(fp)
				END;
				IF proc.sysflag = ccall THEN DevCPL486.GenReturn(0)
				ELSE DevCPL486.GenReturn(proc.conval.intval - 8)
				END
			END
		ELSE (* exit module *)
			IF ~empty THEN
				DevCPL486.MakeReg(r, SI, Int32); DevCPL486.GenPop(r);
				DevCPL486.MakeReg(r, DI, Int32); DevCPL486.GenPop(r);
				DevCPL486.GenMove(fp, sp); DevCPL486.GenPop(fp)
			END;
			DevCPL486.GenReturn(0)
		END
	END Exit;
	
	PROCEDURE InstallStackAlloc*;
		VAR name: ARRAY 32 OF SHORTCHAR; ax, cx, sp, c, x: DevCPL486.Item; l1, l2: DevCPL486.Label;
	BEGIN
		IF stkAllocLbl # DevCPL486.NewLbl THEN
			DevCPL486.SetLabel(stkAllocLbl);
			DevCPL486.MakeReg(ax, AX, Int32);
			DevCPL486.MakeReg(cx, CX, Int32);
			DevCPL486.MakeReg(sp, SP, Int32);
			DevCPL486.GenPush(ax);
			DevCPL486.MakeConst(c, -5, Int32); DevCPL486.GenAdd(c, cx, FALSE);
			l1 := DevCPL486.NewLbl; DevCPL486.GenJump(ccNS, l1, TRUE);
			DevCPL486.MakeConst(c, 0, Int32); DevCPL486.GenMove(c, cx);
			DevCPL486.SetLabel(l1);
			DevCPL486.MakeConst(c, -4, Int32); DevCPL486.GenAnd(c, cx);
			DevCPL486.GenMove(cx, ax);
			DevCPL486.MakeConst(c, 4095, Int32); DevCPL486.GenAnd(c, ax);
			DevCPL486.GenSub(ax, sp, FALSE);
			DevCPL486.GenMove(cx, ax);
			DevCPL486.MakeConst(c, 12, Int32); DevCPL486.GenShiftOp(SHR, c, ax);
			l2 := DevCPL486.NewLbl; DevCPL486.GenJump(ccE, l2, TRUE);
			l1 := DevCPL486.NewLbl; DevCPL486.SetLabel(l1);
			DevCPL486.MakeConst(c, 0, Int32); DevCPL486.GenPush(c);
			DevCPL486.MakeConst(c, 4092, Int32); DevCPL486.GenSub(c, sp, FALSE);
			DevCPL486.MakeConst(c, -1, Int32); DevCPL486.GenAdd(c, ax, FALSE);
			DevCPL486.GenJump(ccNE, l1, TRUE);
			DevCPL486.SetLabel(l2);
			DevCPL486.MakeConst(c, 8, Int32); DevCPL486.GenAdd(c, cx, FALSE);
			x.mode := Ind; x.form := Int32; x.offset := -4; x.index := CX; x.reg := SP; x.scale := 1;
			DevCPL486.GenMove(x, ax);
			DevCPL486.GenPush(ax);
			DevCPL486.GenMove(x, ax);
			DevCPL486.MakeConst(c, 2, Int32); DevCPL486.GenShiftOp(SHR, c, cx);
			DevCPL486.GenReturn(0);
			name := "$StackAlloc"; DevCPE.OutRefName(name);
		END
	END InstallStackAlloc;

	PROCEDURE Trap* (n: INTEGER);
	BEGIN
		DevCPL486.GenAssert(ccNever, n)
	END Trap;
	
	PROCEDURE Jump* (VAR L: DevCPL486.Label);
	BEGIN
		DevCPL486.GenJump(ccAlways, L, FALSE)
	END Jump;

	PROCEDURE JumpT* (VAR x: DevCPL486.Item; VAR L: DevCPL486.Label);
	BEGIN
		DevCPL486.GenJump(x.offset, L, FALSE);
	END JumpT; 
	
	PROCEDURE JumpF* (VAR x: DevCPL486.Item; VAR L: DevCPL486.Label);
	BEGIN
		DevCPL486.GenJump(Inverted(x.offset), L, FALSE);
	END JumpF;
	
	PROCEDURE CaseTableJump* (VAR x: DevCPL486.Item; low, high: INTEGER; VAR else: DevCPL486.Label);
		VAR c: DevCPL486.Item; n: INTEGER;
	BEGIN
		n := high - low + 1;
		DevCPL486.MakeConst(c, low, Int32); DevCPL486.GenSub(c, x, FALSE);
		DevCPL486.MakeConst(c, n, Int32); DevCPL486.GenComp(c, x);
		DevCPL486.GenJump(ccAE, else, FALSE);
		DevCPL486.GenCaseJump(x)
	END CaseTableJump;
	
	PROCEDURE CaseJump* (VAR x: DevCPL486.Item; low, high: INTEGER; VAR this, else: DevCPL486.Label; tree, first: BOOLEAN);
		VAR c: DevCPL486.Item;
	BEGIN
		IF high = low THEN
			DevCPL486.MakeConst(c, low, Int32); DevCPL486.GenComp(c, x);
			IF tree THEN DevCPL486.GenJump(ccG, else, FALSE) END;
			DevCPL486.GenJump(ccE, this, FALSE)
		ELSIF first THEN
			DevCPL486.MakeConst(c, low, Int32); DevCPL486.GenComp(c, x);
			DevCPL486.GenJump(ccL, else, FALSE);
			DevCPL486.MakeConst(c, high, Int32); DevCPL486.GenComp(c, x);
			DevCPL486.GenJump(ccLE, this, FALSE);
		ELSE
			DevCPL486.MakeConst(c, high, Int32); DevCPL486.GenComp(c, x);
			DevCPL486.GenJump(ccG, else, FALSE);
			DevCPL486.MakeConst(c, low, Int32); DevCPL486.GenComp(c, x);
			DevCPL486.GenJump(ccGE, this, FALSE);
		END
	END CaseJump;

BEGIN
	imLevel[0] := 0
END DevCPC486.
