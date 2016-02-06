MODULE DevCPB;
(**
	project	= "BlackBox"
	organization	= "www.oberon.ch"
	contributors	= "Oberon microsystems, Robert Campbell"
	version	= "System/Rsrc/About"
	copyright	= "System/Rsrc/About"
	license	= "Docu/BB-License"
	references	= "ftp://ftp.inf.ethz.ch/pub/software/Oberon/OberonV4/Docu/OP2.Paper.ps"
	changes	= "
	- 19940616, bh, ?
	- 19950925, bh, COM support
	- 19951206, bh, COM functions
	- 19951208, bh, longchar & largeint support
	- 19951212, bh, alias structures
	- 19951226, bh, largeint via longreal
	- 19960221, bh, [new] flag handling
	- 19960327, bh, NEW(ptrInterface, ptrInterface)
	- 19960328, bh, CheckAssign corrected
	- 19960509, bh, assignment compatibility for guid constants
	- 19960703, bh, Convert corrected for largeint constants
	- 19960905, bh, CheckAssign: string to open array assign allowed
	- 19960905, bh, CheckAssign: ptr to array of T compatible to ptr to array of T
	- 19960905, bh, Param: ptr to array of T compatible to VAR ptr to array of T
	- 19960905, bh, Deref, Index, Field, TypTest: expressions allowed
	- 19960910, bh, AssignString added for optimal string assignment
	- 19990308, bh, fixed compiler bug (ORD({1}) in case statement lead to problem in Loader.Fixup
	- 19990825, bh, open arrays allowed in SYSTEM.VAL (StdPar0, StdPar1)
	- 19990825, bh, string expressions allowed for IN parameters (Param, CheckBuffering)
	- 19990902, bh, check for obsolete oberon types (CheckOldType, StPar0)
	- 19991122, bh, CheckLeaf in Param
	- 20000316, bh, bug in Convert fixed (string compare)
	- 20001027, bh, StrDeref added in GetMaxLength for untagged arrays
	- 20010313, bh, Contravariant use of OUT parameters prohibited
	- 20010601, bh, parameter test for MIN & MAX in StFct corrected
	- 20020207, bh, test for type expressions in Param
	- 20070123, bh, NewString changed (Unicode support)
	- 20070123, bh, CheckAssign changed to check procedure type sysflags
	- 20070307, bh, added char to string conversion in LEN handling
	- 20070300, bh, x.obj := NIL in ConstOp
	- 20080202, bh, Real constant operations corrected (proposed by Robert Campbell)
	"
	issues	= ""

**)

	(* RC 6.3.89 / 24.5.93 / 17.2.94 *)

	IMPORT DevCPT, DevCPM;

	CONST
		(* symbol values or ops *)
		times = 1; slash = 2; div = 3; mod = 4;
		and = 5; plus = 6; minus = 7; or = 8; eql = 9;
		neq = 10; lss = 11; leq = 12; gtr = 13; geq = 14;
		in = 15; is = 16; ash = 17; msk = 18; len = 19;
		conv = 20; abs = 21; cap = 22; odd = 23; not = 33;
		(*SYSTEM*)
		adr = 24; cc = 25; bit = 26; lsh = 27; rot = 28; val = 29;
		min = 34; max = 35; typfn = 36; size = 37;
		
		(* object modes *)
		Var = 1; VarPar = 2; Con = 3; Fld = 4; Typ = 5; LProc = 6; XProc = 7;
		SProc = 8; CProc = 9; IProc = 10; Mod = 11; Head = 12; TProc = 13;

		(* Structure forms *)
		Undef = 0; Byte = 1; Bool = 2; Char8 = 3; Int8 = 4; Int16 = 5; Int32 = 6;
		Real32 = 7; Real64 = 8; Set = 9; String8 = 10; NilTyp = 11; NoTyp = 12;
		Pointer = 13; ProcTyp = 14; Comp = 15;
		Char16 = 16; String16 = 17; Int64 = 18;
		intSet = {Int8..Int32, Int64}; realSet = {Real32, Real64}; charSet = {Char8, Char16};

		(* composite structure forms *)
		Basic = 1; Array = 2; DynArr = 3; Record = 4;

		(* nodes classes *)
		Nvar = 0; Nvarpar = 1; Nfield = 2; Nderef = 3; Nindex = 4; Nguard = 5; Neguard = 6;
		Nconst = 7; Ntype = 8; Nproc = 9; Nupto = 10; Nmop = 11; Ndop = 12; Ncall = 13;
		Ninittd = 14; Nif = 15; Ncaselse = 16; Ncasedo = 17; Nenter = 18; Nassign = 19;
		Nifelse = 20; Ncase = 21; Nwhile = 22; Nrepeat = 23; Nloop = 24; Nexit = 25;
		Nreturn = 26; Nwith = 27; Ntrap = 28;

		(*function number*)
		assign = 0;
		haltfn = 0; newfn = 1; absfn = 2; capfn = 3; ordfn = 4;
		entierfn = 5; oddfn = 6; minfn = 7; maxfn = 8; chrfn = 9;
		shortfn = 10; longfn = 11; sizefn = 12; incfn = 13; decfn = 14;
		inclfn = 15; exclfn = 16; lenfn = 17; copyfn = 18; ashfn = 19; assertfn = 32;
		lchrfn = 33; lentierfcn = 34; bitsfn = 37; bytesfn = 38;
		
		(*SYSTEM function number*)
		adrfn = 20; ccfn = 21; lshfn = 22; rotfn = 23;
		getfn = 24; putfn = 25; getrfn = 26; putrfn = 27;
		bitfn = 28; valfn = 29; sysnewfn = 30; movefn = 31;
		thisrecfn = 45; thisarrfn = 46;

		(* COM function number *)
		validfn = 40; iidfn = 41; queryfn = 42;
		
		(* module visibility of objects *)
		internal = 0; external = 1; externalR = 2; inPar = 3; outPar = 4;

		(* procedure flags (conval.setval) *)
		hasBody = 1; isRedef = 2; slNeeded = 3; imVar = 4;
		
		(* attribute flags (attr.adr, struct.attribute, proc.conval.setval)*)
		newAttr = 16; absAttr = 17; limAttr = 18; empAttr = 19; extAttr = 20;
		
		(* case statement flags (conval.setval) *)
		useTable = 1; useTree = 2;
		
		(* sysflags *)
		nilBit = 1; inBit = 2; outBit = 4; newBit = 8; iidBit = 16; interface = 10; jint = -11; jstr = -13;

		AssertTrap = 0;	(* default trap number *)

		covarOut = FALSE;
		
		
	VAR
		typSize*: PROCEDURE(typ: DevCPT.Struct);
		zero, one, two, dummy, quot: DevCPT.Const;

	PROCEDURE err(n: SHORTINT);
	BEGIN DevCPM.err(n)
	END err;
	
	PROCEDURE NewLeaf*(obj: DevCPT.Object): DevCPT.Node;
		VAR node: DevCPT.Node; typ: DevCPT.Struct;
	BEGIN
		typ := obj.typ;
		CASE obj.mode OF
		  Var:
				node := DevCPT.NewNode(Nvar); node.readonly := (obj.vis = externalR) & (obj.mnolev < 0)
		| VarPar:
				node := DevCPT.NewNode(Nvarpar); node.readonly := obj.vis = inPar;
		| Con:
				node := DevCPT.NewNode(Nconst); node.conval := DevCPT.NewConst();
				node.conval^ := obj.conval^	(* string is not copied, only its ref *)
		| Typ:
				node := DevCPT.NewNode(Ntype)
		| LProc..IProc, TProc:
				node := DevCPT.NewNode(Nproc)
		ELSE err(127); node := DevCPT.NewNode(Nvar); typ := DevCPT.notyp
		END ;
		node.obj := obj; node.typ := typ;
		RETURN node
	END NewLeaf;
	
	PROCEDURE Construct*(class: BYTE; VAR x: DevCPT.Node;  y: DevCPT.Node);
		VAR node: DevCPT.Node;
	BEGIN
		node := DevCPT.NewNode(class); node.typ := DevCPT.notyp;
		node.left := x; node.right := y; x := node
	END Construct;
	
	PROCEDURE Link*(VAR x, last: DevCPT.Node; y: DevCPT.Node);
	BEGIN
		IF x = NIL THEN x := y ELSE last.link := y END ;
		WHILE y.link # NIL DO y := y.link END ;
		last := y
	END Link;
	
	PROCEDURE BoolToInt(b: BOOLEAN): INTEGER;
	BEGIN
		IF b THEN RETURN 1 ELSE RETURN 0 END
	END BoolToInt;
	
	PROCEDURE IntToBool(i: INTEGER): BOOLEAN;
	BEGIN
		IF i = 0 THEN RETURN FALSE ELSE RETURN TRUE END
	END IntToBool;
	
	PROCEDURE NewBoolConst*(boolval: BOOLEAN): DevCPT.Node;
		VAR x: DevCPT.Node;
	BEGIN
		x := DevCPT.NewNode(Nconst); x.typ := DevCPT.booltyp;
		x.conval := DevCPT.NewConst(); x.conval.intval := BoolToInt(boolval); RETURN x
	END NewBoolConst;
	
	PROCEDURE OptIf*(VAR x: DevCPT.Node);	(* x.link = NIL *)
		VAR if, pred: DevCPT.Node;
	BEGIN
		if := x.left;
		WHILE if.left.class = Nconst DO
			IF IntToBool(if.left.conval.intval) THEN x := if.right; RETURN
			ELSIF if.link = NIL THEN x := x.right; RETURN
			ELSE if := if.link; x.left := if
			END
		END ;
		pred := if; if := if.link;
		WHILE if # NIL DO
			IF if.left.class = Nconst THEN
				IF IntToBool(if.left.conval.intval) THEN
					pred.link := NIL; x.right := if.right; RETURN
				ELSE if := if.link; pred.link := if
				END
			ELSE pred := if; if := if.link
			END
		END
	END OptIf;

	PROCEDURE Nil*(): DevCPT.Node;
		VAR x: DevCPT.Node;
	BEGIN
		x := DevCPT.NewNode(Nconst); x.typ := DevCPT.niltyp;
		x.conval := DevCPT.NewConst(); x.conval.intval := 0; RETURN x
	END Nil;

	PROCEDURE EmptySet*(): DevCPT.Node;
		VAR x: DevCPT.Node;
	BEGIN
		x := DevCPT.NewNode(Nconst); x.typ := DevCPT.settyp;
		x.conval := DevCPT.NewConst(); x.conval.setval := {}; RETURN x
	END EmptySet;
	
	PROCEDURE MarkAsUsed (node: DevCPT.Node);
		VAR c: BYTE;
	BEGIN
		c := node.class;
		WHILE (c = Nfield) OR (c = Nindex) OR (c = Nguard) OR (c = Neguard) DO node := node.left; c := node.class END;
		IF (c = Nvar) & (node.obj.mnolev > 0) THEN node.obj.used := TRUE END
	END MarkAsUsed;
	
	
	PROCEDURE GetTempVar* (name: ARRAY OF SHORTCHAR; typ: DevCPT.Struct; VAR obj: DevCPT.Object);
		VAR n: DevCPT.Name; o: DevCPT.Object;
	BEGIN
		n := "@@  "; DevCPT.Insert(n, obj); obj.name^ := name$;	(* avoid err 1 *)
		obj.mode := Var; obj.typ := typ;
		o := DevCPT.topScope.scope;
		IF o = NIL THEN DevCPT.topScope.scope := obj
		ELSE
			WHILE o.link # NIL DO o := o.link END;
			o.link := obj
		END
	END GetTempVar;


	(* ---------- constant operations ---------- *)
	
	PROCEDURE Log (x: DevCPT.Node): INTEGER;
		VAR val, exp: INTEGER;
	BEGIN
		exp := 0;
		IF x.typ.form = Int64 THEN
			RETURN -1
		ELSE
			val := x.conval.intval;
			IF val > 0 THEN
				WHILE ~ODD(val) DO val := val DIV 2; INC(exp) END
			END;
			IF val # 1 THEN exp := -1 END
		END;
		RETURN exp
	END Log;

	PROCEDURE Floor (x: REAL): REAL;
		VAR y: REAL;
	BEGIN
		IF ABS(x) > 9007199254740992.0 (* 2^53 *) THEN RETURN x
		ELSIF (x >= MAX(INTEGER) + 1.0) OR (x < MIN(INTEGER)) THEN
			y := Floor(x / (MAX(INTEGER) + 1.0)) * (MAX(INTEGER) + 1.0);
			RETURN SHORT(ENTIER(x - y)) + y
		ELSE RETURN SHORT(ENTIER(x))
		END
	END Floor;

	PROCEDURE SetToInt (s: SET): INTEGER;
		VAR x, i: INTEGER;
	BEGIN
		i := 31; x := 0;
		IF 31 IN s THEN x := -1 END;
		WHILE i > 0 DO
			x := x * 2; DEC(i);
			IF i IN s THEN INC(x) END
		END;
		RETURN x
	END SetToInt;

	PROCEDURE IntToSet (x: INTEGER): SET;
		VAR i: INTEGER; s: SET;
	BEGIN
		i := 0; s := {};
		WHILE i < 32 DO
			IF ODD(x) THEN INCL(s, i) END;
			x := x DIV 2; INC(i)
		END;
		RETURN s
	END IntToSet;

	PROCEDURE GetConstType (x: DevCPT.Const; form: INTEGER; errno: SHORTINT; VAR typ: DevCPT.Struct);
		CONST MAXL = 9223372036854775808.0; (* 2^63 *)
	BEGIN
		IF (form IN intSet + charSet) & (x.realval + x.intval >= MIN(INTEGER))
				& (x.realval + x.intval <= MAX(INTEGER)) THEN
			x.intval := SHORT(ENTIER(x.realval + x.intval)); x.realval := 0
		END;
		IF form IN intSet THEN
			IF x.realval = 0 THEN typ := DevCPT.int32typ
			ELSIF (x.intval >= -MAXL - x.realval) & (x.intval < MAXL - x.realval) THEN typ := DevCPT.int64typ
			ELSE err(errno); x.intval := 1; x.realval := 0; typ := DevCPT.int32typ
			END
		ELSIF form IN realSet THEN	(* SR *)
			typ := DevCPT.real64typ
		ELSIF form IN charSet THEN
			IF x.intval <= 255 THEN typ := DevCPT.char8typ
			ELSE typ := DevCPT.char16typ
			END
		ELSE typ := DevCPT.undftyp
		END
	END GetConstType;
	
	PROCEDURE CheckConstType (x: DevCPT.Const; form: INTEGER; errno: SHORTINT);
		VAR type: DevCPT.Struct;
	BEGIN
		GetConstType(x, form, errno, type);
		IF  ~DevCPT.Includes(form, type.form)
		& ((form # Int8) OR (x.realval # 0) OR (x.intval < -128) OR (x.intval > 127))
		& ((form # Int16) OR (x.realval # 0) OR (x.intval < -32768) OR (x.intval > 32767)) 
		& ((form # Real32) OR (ABS(x.realval) > DevCPM.MaxReal32) & (ABS(x.realval) # DevCPM.InfReal)) THEN
			err(errno); x.intval := 1; x.realval := 0
		END
(*
		IF (form IN intSet + charSet) & (x.realval + x.intval >= MIN(INTEGER))
				& (x.realval + x.intval <= MAX(INTEGER)) THEN
			x.intval := SHORT(ENTIER(x.realval + x.intval)); x.realval := 0
		END;
		IF (form = Int64) & ((x.intval < -MAXL - x.realval) OR (x.intval >= MAXL - x.realval))
		OR (form = Int32) & (x.realval # 0)
		OR (form = Int16) & ((x.realval # 0) OR (x.intval < -32768) OR (x.intval > 32767))
		OR (form = Int8) & ((x.realval # 0) OR (x.intval < -128) OR (x.intval > 127))
		OR (form = Char16) & ((x.realval # 0) OR (x.intval < 0) OR (x.intval > 65535))
		OR (form = Char8) & ((x.realval # 0) OR (x.intval < 0) OR (x.intval > 255))
		OR (form = Real32) & (ABS(x.realval) > DevCPM.MaxReal32) & (ABS(x.realval) # DevCPM.InfReal) THEN
			err(errno); x.intval := 1; x.realval := 0
		END
*)
	END CheckConstType;
	
	PROCEDURE ConvConst (x: DevCPT.Const; from, to: INTEGER);
		VAR sr: SHORTREAL;
	BEGIN
		IF from = Set THEN
			x.intval := SetToInt(x.setval); x.realval := 0; x.setval := {};
		ELSIF from IN intSet + charSet THEN
			IF to = Set THEN CheckConstType(x, Int32, 203); x.setval := IntToSet(x.intval)
			ELSIF to IN intSet THEN CheckConstType(x, to, 203)
			ELSIF to IN realSet THEN x.realval := x.realval + x.intval; x.intval := DevCPM.ConstNotAlloc
			ELSE (*to IN charSet*) CheckConstType(x, to, 220)
			END
		ELSIF from IN realSet THEN
			IF to IN realSet THEN CheckConstType(x, to, 203);
				IF to = Real32 THEN sr := SHORT(x.realval); x.realval := sr END	(* reduce precision *)
			ELSE x.realval := Floor(x.realval); x.intval := 0; CheckConstType(x, to, 203)
			END
		END
	END ConvConst;
	
	PROCEDURE Prepare (x: DevCPT.Const);
		VAR r: REAL;
	BEGIN
		x.realval := x.realval + x.intval DIV 32768 * 32768;
		x.intval := x.intval MOD 32768;
		r := Floor(x.realval / 4096) * 4096;
		x.intval := x.intval + SHORT(ENTIER(x.realval - r));
		x.realval := r
		(* ABS(x.intval) < 2^15  &  ABS(x.realval) MOD 2^12 = 0 *)
	END Prepare;
	
	PROCEDURE AddConst (x, y, z: DevCPT.Const; VAR type: DevCPT.Struct);	(* z := x + y *)
	BEGIN
		IF type.form IN intSet THEN
			Prepare(x); Prepare(y);
			z.intval := x.intval + y.intval; z.realval := x.realval + y.realval
		ELSIF type.form IN realSet THEN
			IF (ABS(x.realval) = DevCPM.InfReal) & (x.realval = - y.realval) THEN err(212)
			ELSE z.realval := x.realval + y.realval
			END
		ELSE HALT(100)
		END;
		GetConstType(z, type.form, 206, type)
	END AddConst;
	
	PROCEDURE NegateConst (y, z: DevCPT.Const; VAR type: DevCPT.Struct);	(* z := - y *)
	BEGIN
		IF type.form IN intSet THEN Prepare(y); z.intval :=  -y.intval; z.realval := -y.realval
		ELSIF type.form IN realSet THEN z.realval := -y.realval
		ELSE HALT(100)
		END;
		GetConstType(z, type.form, 207, type)
	END NegateConst;
	
	PROCEDURE SubConst (x, y, z: DevCPT.Const; VAR type: DevCPT.Struct);	(* z := x - y *)
	BEGIN
		IF type.form IN intSet THEN
			Prepare(x); Prepare(y);
			z.intval := x.intval - y.intval; z.realval := x.realval - y.realval
		ELSIF type.form IN realSet THEN
			IF (ABS(x.realval) = DevCPM.InfReal) & (x.realval =  y.realval) THEN err(212)
			ELSE z.realval := x.realval - y.realval
			END
		ELSE HALT(100)
		END;
		GetConstType(z, type.form, 207, type)
	END SubConst;
	
	PROCEDURE MulConst (x, y, z: DevCPT.Const; VAR type: DevCPT.Struct);	(* z := x * y *)
	BEGIN
		IF type.form IN intSet THEN
			Prepare(x); Prepare(y);
			z.realval := x.realval * y.realval + x.intval * y.realval + x.realval * y.intval;
			z.intval := x.intval * y.intval
		ELSIF type.form IN realSet THEN
			IF (ABS(x.realval) = DevCPM.InfReal) & ( y.realval = 0.0) THEN err(212)
			ELSIF (ABS(y.realval) = DevCPM.InfReal) & (x.realval = 0.0) THEN err(212)
			ELSE z.realval := x.realval * y.realval
			END
		ELSE HALT(100)
		END;
		GetConstType(z, type.form, 204, type)
	END MulConst;
	
	PROCEDURE DivConst (x, y, z: DevCPT.Const; VAR type: DevCPT.Struct);	(* z := x / y *)
	BEGIN
		IF type.form IN realSet THEN
			IF (x.realval = 0.0) & (y.realval = 0.0) THEN err(212)
			ELSIF (ABS(x.realval) =  DevCPM.InfReal) & (ABS(y.realval) =  DevCPM.InfReal) THEN err(212)
			ELSE z.realval := x.realval / y.realval
			END
		ELSE HALT(100)
		END;
		GetConstType(z, type.form, 204, type)
	END DivConst;
	
	PROCEDURE DivModConst (x, y: DevCPT.Const; div: BOOLEAN; VAR type: DevCPT.Struct);
	(* x := x DIV y | x MOD y *)
	BEGIN
		IF type.form IN intSet THEN
			IF y.realval + y.intval # 0 THEN
				Prepare(x); Prepare(y);
				quot.realval := Floor((x.realval + x.intval) / (y.realval + y.intval));
				quot.intval := 0; Prepare(quot);
				x.realval := x.realval - quot.realval * y.realval - quot.realval * y.intval - quot.intval * y.realval;
				x.intval := x.intval - quot.intval * y.intval;
				IF y.realval + y.intval > 0 THEN
					WHILE x.realval + x.intval > 0 DO SubConst(x, y, x, type); INC(quot.intval) END;
					WHILE x.realval + x.intval < 0 DO AddConst(x, y, x, type); DEC(quot.intval) END
				ELSE
					WHILE x.realval + x.intval < 0 DO SubConst(x, y, x, type); INC(quot.intval) END;
					WHILE x.realval + x.intval > 0 DO AddConst(x, y, x, type); DEC(quot.intval) END
				END;
				IF div THEN x.realval := quot.realval; x.intval := quot.intval END;
				GetConstType(x, type.form, 204, type)
			ELSE err(205)
			END
		ELSE HALT(100)
		END
	END DivModConst;
	
	PROCEDURE EqualConst (x, y: DevCPT.Const; form: INTEGER): BOOLEAN;	(* x = y *)
		VAR res: BOOLEAN;
	BEGIN
		CASE form OF
		| Undef: res := TRUE
		| Bool, Byte, Char8..Int32, Char16: res := x.intval = y.intval
		| Int64: Prepare(x); Prepare(y); res := (x.realval - y.realval) + (x.intval - y.intval) = 0
		| Real32, Real64: res := x.realval = y.realval
		| Set: res := x.setval = y.setval
		| String8, String16, Comp (* guid *): res := x.ext^ = y.ext^
		| NilTyp, Pointer, ProcTyp: res := x.intval = y.intval
		END;
		RETURN res
	END EqualConst;
	
	PROCEDURE LessConst (x, y: DevCPT.Const; form: INTEGER): BOOLEAN;	(* x < y *)
		VAR res: BOOLEAN;
	BEGIN
		CASE form OF
		| Undef: res := TRUE
		| Byte, Char8..Int32, Char16: res := x.intval < y.intval
		| Int64: Prepare(x); Prepare(y); res := (x.realval - y.realval) + (x.intval - y.intval) < 0
		| Real32, Real64: res := x.realval < y.realval
		| String8, String16: res := x.ext^ < y.ext^
		| Bool, Set, NilTyp, Pointer, ProcTyp, Comp: err(108)
		END;
		RETURN res
	END LessConst;
	
	PROCEDURE IsNegConst (x: DevCPT.Const; form: INTEGER): BOOLEAN;	(* x < 0  OR x = (-0.0) *)
		VAR res: BOOLEAN;
	BEGIN
		CASE form OF
		| Int8..Int32: res := x.intval < 0
		| Int64: Prepare(x); res := x.realval + x.intval < 0
		| Real32, Real64: res := (x.realval <= 0.) & (1. / x.realval <= 0.)
		END;
		RETURN res
	END IsNegConst;


	PROCEDURE NewIntConst*(intval: INTEGER): DevCPT.Node;
		VAR x: DevCPT.Node;
	BEGIN
		x := DevCPT.NewNode(Nconst); x.conval := DevCPT.NewConst();
		x.conval.intval := intval; x.conval.realval := 0; x.typ := DevCPT.int32typ; RETURN x
	END NewIntConst;
	
	PROCEDURE NewLargeIntConst* (intval: INTEGER; realval: REAL): DevCPT.Node;
		VAR x: DevCPT.Node;
	BEGIN
		x := DevCPT.NewNode(Nconst); x.conval := DevCPT.NewConst();
		x.conval.intval := intval; x.conval.realval := realval; x.typ := DevCPT.int64typ; RETURN x
	END NewLargeIntConst;
	
	PROCEDURE NewRealConst*(realval: REAL; typ: DevCPT.Struct): DevCPT.Node;
		VAR x: DevCPT.Node;
	BEGIN
		x := DevCPT.NewNode(Nconst); x.conval := DevCPT.NewConst();
		x.conval.realval := realval; x.conval.intval := DevCPM.ConstNotAlloc;
		IF typ = NIL THEN typ := DevCPT.real64typ END;
		x.typ := typ;
		RETURN x
	END NewRealConst;
	
	PROCEDURE NewString*(str: DevCPT.String; lstr: POINTER TO ARRAY OF CHAR; len: INTEGER): DevCPT.Node;
		VAR i, j, c: INTEGER; x: DevCPT.Node; ext: DevCPT.ConstExt;
	BEGIN
		x := DevCPT.NewNode(Nconst); x.conval := DevCPT.NewConst();
		IF lstr # NIL THEN
			x.typ := DevCPT.string16typ;
			NEW(ext, 3 * len); i := 0; j := 0;
			REPEAT c := ORD(lstr[i]); INC(i); DevCPM.PutUtf8(ext^, c, j) UNTIL c = 0;
			x.conval.ext := ext
		ELSE
			x.typ := DevCPT.string8typ; x.conval.ext := str
		END;
		x.conval.intval := DevCPM.ConstNotAlloc; x.conval.intval2 := len;
		RETURN x
	END NewString;
	
	PROCEDURE CharToString8(n: DevCPT.Node);
		VAR ch: SHORTCHAR;
	BEGIN
		n.typ := DevCPT.string8typ; ch := SHORT(CHR(n.conval.intval)); NEW(n.conval.ext, 2);
		IF ch = 0X THEN n.conval.intval2 := 1 ELSE n.conval.intval2 := 2; n.conval.ext[1] := 0X END ;
		n.conval.ext[0] := ch; n.conval.intval := DevCPM.ConstNotAlloc; n.obj := NIL
	END CharToString8;
	
	PROCEDURE CharToString16 (n: DevCPT.Node);
		VAR ch, ch1: SHORTCHAR; i: INTEGER;
	BEGIN
		n.typ := DevCPT.string16typ; NEW(n.conval.ext, 4);
		IF n.conval.intval = 0 THEN
			n.conval.ext[0] := 0X; n.conval.intval2 := 1
		ELSE
			i := 0; DevCPM.PutUtf8(n.conval.ext^, n.conval.intval, i);
			n.conval.ext[i] := 0X; n.conval.intval2 := 2
		END;
		n.conval.intval := DevCPM.ConstNotAlloc; n.obj := NIL
	END CharToString16;
	
	PROCEDURE String8ToString16 (n: DevCPT.Node);
		VAR i, j, x: INTEGER; ext, new: DevCPT.ConstExt;
	BEGIN
		n.typ := DevCPT.string16typ; ext := n.conval.ext;
		NEW(new, 2 * n.conval.intval2); i := 0; j := 0; 
		REPEAT x := ORD(ext[i]); INC(i); DevCPM.PutUtf8(new^, x, j) UNTIL x = 0;
		n.conval.ext := new; n.obj := NIL
	END String8ToString16;
	
	PROCEDURE String16ToString8 (n: DevCPT.Node);
		VAR i, j, x: INTEGER; ext, new: DevCPT.ConstExt;
	BEGIN
		n.typ := DevCPT.string8typ; ext := n.conval.ext;
		NEW(new, n.conval.intval2); i := 0; j := 0;
		REPEAT DevCPM.GetUtf8(ext^, x, i); new[j] := SHORT(CHR(x MOD 256)); INC(j) UNTIL x = 0;
		n.conval.ext := new; n.obj := NIL
	END String16ToString8;
	
	PROCEDURE StringToGuid (VAR n: DevCPT.Node);
	BEGIN
		ASSERT((n.class = Nconst) & (n.typ.form = String8));
		IF ~DevCPM.ValidGuid(n.conval.ext^) THEN err(165) END;
		n.typ := DevCPT.guidtyp
	END StringToGuid;
	
	PROCEDURE CheckString (n: DevCPT.Node; typ: DevCPT.Struct; e: SHORTINT);
		VAR ntyp: DevCPT.Struct;
	BEGIN
		ntyp := n.typ;
		IF (typ = DevCPT.guidtyp) & (n.class = Nconst) & (ntyp.form = String8) THEN StringToGuid(n)
		ELSIF (typ.comp IN {Array, DynArr}) & (typ.BaseTyp.form = Char8) OR (typ.form = String8) THEN
			IF (n.class = Nconst) & (ntyp.form = Char8) THEN CharToString8(n)
			ELSIF (ntyp.comp IN {Array, DynArr}) & (ntyp.BaseTyp.form = Char8) OR (ntyp.form = String8) THEN (* ok *)
			ELSE err(e)
			END
		ELSIF (typ.comp IN {Array, DynArr}) & (typ.BaseTyp.form = Char16) OR (typ.form = String16) THEN
			IF (n.class = Nconst) & (ntyp.form IN charSet) THEN CharToString16(n)
			ELSIF (n.class = Nconst) & (ntyp.form = String8) THEN String8ToString16(n)
			ELSIF (ntyp.comp IN {Array, DynArr}) & (ntyp.BaseTyp.form = Char16) OR (ntyp.form = String16) THEN
				(* ok *)
			ELSE err(e)
			END
		ELSE err(e)
		END
	END CheckString;
	
	
	PROCEDURE BindNodes(class: BYTE; typ: DevCPT.Struct; VAR x: DevCPT.Node; y: DevCPT.Node);
		VAR node: DevCPT.Node;
	BEGIN
		node := DevCPT.NewNode(class); node.typ := typ;
		node.left := x; node.right := y; x := node
	END BindNodes;

	PROCEDURE NotVar(x: DevCPT.Node): BOOLEAN;
	BEGIN
		RETURN (x.class >= Nconst) & ((x.class # Nmop) OR (x.subcl # val) OR (x.left.class >= Nconst))
			OR (x.typ.form IN {String8, String16})
	END NotVar;


	PROCEDURE Convert(VAR x: DevCPT.Node; typ: DevCPT.Struct);
		VAR node: DevCPT.Node; f, g: SHORTINT; k: INTEGER; r: REAL;
	BEGIN f := x.typ.form; g := typ.form;
		IF x.class = Nconst THEN
			IF g = String8 THEN
				IF f = String16 THEN String16ToString8(x)
				ELSIF f IN charSet THEN CharToString8(x)
				ELSE typ := DevCPT.undftyp
				END
			ELSIF g = String16 THEN
				IF f = String8 THEN String8ToString16(x)
				ELSIF f IN charSet THEN CharToString16(x)
				ELSE typ := DevCPT.undftyp
				END
			ELSE ConvConst(x.conval, f, g)
			END;
			x.obj := NIL
		ELSIF (x.class = Nmop) & (x.subcl = conv) & (DevCPT.Includes(f, x.left.typ.form) OR DevCPT.Includes(f, g))
		THEN
			(* don't create new node *)
			IF x.left.typ.form = typ.form THEN (* and suppress existing node *) x := x.left END
		ELSE
			IF (x.class = Ndop) & (x.typ.form IN {String8, String16}) THEN	(* propagate to leaf nodes *)
				Convert(x.left, typ); Convert(x.right, typ)
			ELSE
				node := DevCPT.NewNode(Nmop); node.subcl := conv; node.left := x; x := node;
			END
		END;
		x.typ := typ
	END Convert;

	PROCEDURE Promote (VAR left, right: DevCPT.Node; op: INTEGER);	(* check expression compatibility *)
		VAR f, g: INTEGER; new: DevCPT.Struct;
	BEGIN
		f := left.typ.form; g := right.typ.form; new := left.typ;
		IF f IN intSet + realSet THEN
			IF g IN intSet + realSet THEN
				IF (f = Real32) & (right.class = Nconst) & (g IN realSet) & (left.class # Nconst)
					(* & ((ABS(right.conval.realval) <= DevCPM.MaxReal32)
							OR (ABS(right.conval.realval) = DevCPM.InfReal)) *)
				OR (g = Real32) & (left.class = Nconst) & (f IN realSet) & (right.class # Nconst)
					(* & ((ABS(left.conval.realval) <= DevCPM.MaxReal32)
							OR (ABS(left.conval.realval) = DevCPM.InfReal)) *) THEN
						new := DevCPT.real32typ	(* SR *)
				ELSIF (f = Real64) OR (g = Real64) THEN new := DevCPT.real64typ
				ELSIF (f = Real32) OR (g = Real32) THEN new := DevCPT.real32typ	(* SR *)
				ELSIF op = slash THEN new := DevCPT.real64typ
				ELSIF (f = Int64) OR (g = Int64) THEN new := DevCPT.int64typ
				ELSE new := DevCPT.int32typ
				END
			ELSE err(100)
			END
		ELSIF (left.typ = DevCPT.guidtyp) OR (right.typ = DevCPT.guidtyp) THEN
			IF f = String8 THEN StringToGuid(left) END;
			IF g = String8 THEN StringToGuid(right) END;
			IF left.typ # right.typ THEN err(100) END;
			f := Comp
		ELSIF f IN charSet + {String8, String16} THEN
			IF g IN charSet + {String8, String16} THEN
				IF (f = String16) OR (g = String16) OR (f = Char16) & (g = String8) OR (f = String8) & (g = Char16) THEN
					new := DevCPT.string16typ
				ELSIF (f = Char16) OR (g = Char16) THEN new := DevCPT.char16typ
				ELSIF (f = String8) OR (g = String8) THEN new := DevCPT.string8typ
				ELSIF op = plus THEN
					IF (f = Char16) OR (g = Char16) THEN new := DevCPT.string16typ
					ELSE new := DevCPT.string8typ
					END
				END;
				IF (new.form IN {String8, String16})
					& ((f IN charSet) & (left.class # Nconst) OR (g IN charSet) & (right.class # Nconst))
				THEN
					err(100)
				END
			ELSE err(100)
			END
		ELSIF (f IN {NilTyp, Pointer, ProcTyp}) & (g IN {NilTyp, Pointer, ProcTyp}) THEN
			IF ~DevCPT.SameType(left.typ, right.typ) & (f # NilTyp) & (g # NilTyp)
				& ~((f = Pointer) & (g = Pointer)
					& (DevCPT.Extends(left.typ, right.typ) OR DevCPT.Extends(right.typ, left.typ))) THEN err(100) END
		ELSIF f # g THEN err(100)
		END;
		IF ~(f IN {NilTyp, Pointer, ProcTyp, Comp}) THEN
			IF g # new.form THEN Convert(right, new) END;
			IF f # new.form THEN Convert(left, new) END
		END
	END Promote;

	PROCEDURE CheckParameters* (fp, ap: DevCPT.Object; checkNames: BOOLEAN); (* checks par list match *)
		VAR ft, at: DevCPT.Struct;
	BEGIN
		WHILE fp # NIL DO
			IF ap # NIL THEN
				ft := fp.typ; at := ap.typ;
				IF fp.ptyp # NIL THEN ft := fp.ptyp END;	(* get original formal type *)
				IF ap.ptyp # NIL THEN at := ap.ptyp END;	(* get original formal type *)
				IF ~DevCPT.EqualType(ft, at)
					OR (fp.mode # ap.mode) OR (fp.sysflag # ap.sysflag) OR (fp.vis # ap.vis)
					OR checkNames & (fp.name^ # ap.name^) THEN err(115) END ;
				ap := ap.link
			ELSE err(116)
			END;
			fp := fp.link
		END;
		IF ap # NIL THEN err(116) END
	END CheckParameters;

	PROCEDURE CheckNewParamPair* (newPar, iidPar: DevCPT.Node);
		VAR ityp, ntyp: DevCPT.Struct;
	BEGIN
		ntyp := newPar.typ.BaseTyp;
		IF (newPar.class = Nvarpar) & ODD(newPar.obj.sysflag DIV newBit) THEN
			IF (iidPar.class = Nvarpar) & ODD(iidPar.obj.sysflag DIV iidBit) & (iidPar.obj.mnolev = newPar.obj.mnolev)
			THEN (* ok *)
			ELSE err(168)
			END
		ELSIF ntyp.extlev = 0 THEN	(* ok *)
		ELSIF (iidPar.class = Nconst) & (iidPar.obj # NIL) & (iidPar.obj.mode = Typ) THEN
			IF ~DevCPT.Extends(iidPar.obj.typ, ntyp) THEN err(168) END
		ELSE err(168)
		END
	END CheckNewParamPair;

	
	PROCEDURE DeRef*(VAR x: DevCPT.Node);
		VAR strobj, bstrobj: DevCPT.Object; typ, btyp: DevCPT.Struct;
	BEGIN
		typ := x.typ;
		IF (x.class = Nconst) OR (x.class = Ntype) OR (x.class = Nproc) THEN err(78)
		ELSIF typ.form = Pointer THEN
			btyp := typ.BaseTyp; strobj := typ.strobj; bstrobj := btyp.strobj;
			IF (strobj # NIL) & (strobj.name # DevCPT.null) & (bstrobj # NIL) & (bstrobj.name # DevCPT.null) THEN
				btyp.pbused := TRUE
			END ;
			BindNodes(Nderef, btyp, x, NIL); x.subcl := 0
		ELSE err(84)
		END
	END DeRef;

	PROCEDURE StrDeref*(VAR x: DevCPT.Node);
		VAR typ, btyp: DevCPT.Struct;
	BEGIN
		typ := x.typ;
		IF (x.class = Nconst) OR (x.class = Ntype) OR (x.class = Nproc) THEN err(78)
		ELSIF ((typ.comp IN {Array, DynArr}) & (typ.BaseTyp.form IN charSet)) OR (typ.sysflag = jstr) THEN
			IF (typ.BaseTyp # NIL) & (typ.BaseTyp.form = Char8) THEN btyp := DevCPT.string8typ
			ELSE btyp := DevCPT.string16typ
			END;
			BindNodes(Nderef, btyp, x, NIL); x.subcl := 1
		ELSE err(90)
		END
	END StrDeref;

	PROCEDURE Index*(VAR x: DevCPT.Node; y: DevCPT.Node);
		VAR f: SHORTINT; typ: DevCPT.Struct;
	BEGIN
		f := y.typ.form;
		IF (x.class = Nconst) OR (x.class = Ntype) OR (x.class = Nproc) THEN err(79)
		ELSIF ~(f IN intSet) OR (y.class IN {Nproc, Ntype}) THEN err(80); y.typ := DevCPT.int32typ END ;
		IF f = Int64 THEN Convert(y, DevCPT.int32typ) END;
		IF x.typ.comp = Array THEN typ := x.typ.BaseTyp;
			IF (y.class = Nconst) & ((y.conval.intval < 0) OR (y.conval.intval >= x.typ.n)) THEN err(81) END
		ELSIF x.typ.comp = DynArr THEN typ := x.typ.BaseTyp;
			IF (y.class = Nconst) & (y.conval.intval < 0) THEN err(81) END
		ELSE err(82); typ := DevCPT.undftyp
		END ;
		BindNodes(Nindex, typ, x, y); x.readonly := x.left.readonly
	END Index;
	
	PROCEDURE Field*(VAR x: DevCPT.Node; y: DevCPT.Object);
	BEGIN (*x.typ.comp = Record*)
		IF (x.class = Nconst) OR (x.class = Ntype) OR (x.class = Nproc) THEN err(77) END ;
		IF (y # NIL) & (y.mode IN {Fld, TProc}) THEN
			BindNodes(Nfield, y.typ, x, NIL); x.obj := y;
			x.readonly := x.left.readonly OR ((y.vis = externalR) & (y.mnolev < 0))
		ELSE err(83); x.typ := DevCPT.undftyp
		END
	END Field;
	
	PROCEDURE TypTest*(VAR x: DevCPT.Node; obj: DevCPT.Object; guard: BOOLEAN);

		PROCEDURE GTT(t0, t1: DevCPT.Struct);
			VAR node: DevCPT.Node;
		BEGIN
			IF (t0 # NIL) & DevCPT.SameType(t0, t1) & (guard OR (x.class # Nguard)) THEN
				IF ~guard THEN x := NewBoolConst(TRUE) END
			ELSIF (t0 = NIL) OR DevCPT.Extends(t1, t0) OR (t0.sysflag = jint) OR (t1.sysflag = jint)
					OR (t1.comp = DynArr) & (DevCPM.java IN DevCPM.options) THEN
				IF guard THEN BindNodes(Nguard, NIL, x, NIL); x.readonly := x.left.readonly
				ELSE node := DevCPT.NewNode(Nmop); node.subcl := is; node.left := x; node.obj := obj; x := node
				END
			ELSE err(85)
			END
		END GTT;

	BEGIN
		IF (x.class = Nconst) OR (x.class = Ntype) OR (x.class = Nproc) THEN err(112)
		ELSIF x.typ.form = Pointer THEN
			IF x.typ = DevCPT.sysptrtyp THEN
				IF obj.typ.form = Pointer THEN GTT(NIL, obj.typ.BaseTyp)
				ELSE err(86)
				END
			ELSIF x.typ.BaseTyp.comp # Record THEN err(85)
			ELSIF obj.typ.form = Pointer THEN GTT(x.typ.BaseTyp, obj.typ.BaseTyp)
			ELSE err(86)
			END
		ELSIF (x.typ.comp = Record) & (x.class = Nvarpar) & (x.obj.vis # outPar) & (obj.typ.comp = Record) THEN
			GTT(x.typ, obj.typ)
		ELSE err(87)
		END ;
		IF guard THEN x.typ := obj.typ ELSE x.typ := DevCPT.booltyp END
	END TypTest;
	
	PROCEDURE In*(VAR x: DevCPT.Node; y: DevCPT.Node);
		VAR f: SHORTINT; k: INTEGER;
	BEGIN f := x.typ.form;
		IF (x.class = Ntype) OR (x.class = Nproc) OR (y.class = Ntype) OR (y.class = Nproc) THEN err(126)
		ELSIF (f IN intSet) & (y.typ.form = Set) THEN
			IF f = Int64 THEN Convert(x, DevCPT.int32typ) END;
			IF x.class = Nconst THEN
				k := x.conval.intval;
				IF (k < 0) OR (k > DevCPM.MaxSet) THEN err(202)
				ELSIF y.class = Nconst THEN x.conval.intval := BoolToInt(k IN y.conval.setval); x.obj := NIL
				ELSE BindNodes(Ndop, DevCPT.booltyp, x, y); x.subcl := in
				END
			ELSE BindNodes(Ndop, DevCPT.booltyp, x, y); x.subcl := in
			END
		ELSE err(92)
		END ;
		x.typ := DevCPT.booltyp
	END In;

	PROCEDURE MOp*(op: BYTE; VAR x: DevCPT.Node);
		VAR f: SHORTINT; typ: DevCPT.Struct; z: DevCPT.Node;
		
		PROCEDURE NewOp(op: BYTE; typ: DevCPT.Struct; z: DevCPT.Node): DevCPT.Node;
			VAR node: DevCPT.Node;
		BEGIN
			node := DevCPT.NewNode(Nmop); node.subcl := op; node.typ := typ;
			node.left := z; RETURN node
		END NewOp;

	BEGIN z := x;
		IF ((z.class = Ntype) OR (z.class = Nproc)) & (op # adr) & (op # typfn) & (op # size) THEN err(126)	(* !!! *)
		ELSE
			typ := z.typ; f := typ.form;
			CASE op OF
			| not:
				IF f = Bool THEN
					IF z.class = Nconst THEN
						z.conval.intval := BoolToInt(~IntToBool(z.conval.intval)); z.obj := NIL
					ELSE z := NewOp(op, typ, z)
					END
				ELSE err(98)
				END
			| plus:
				IF ~(f IN intSet + realSet) THEN err(96) END
			| minus:
				IF f IN intSet + realSet + {Set} THEN
					IF z.class = Nconst THEN
						IF f = Set THEN z.conval.setval := -z.conval.setval
						ELSE NegateConst(z.conval, z.conval, z.typ)
						END;
						z.obj := NIL
					ELSE
						IF f < Int32 THEN Convert(z, DevCPT.int32typ) END;
						z := NewOp(op, z.typ, z)
					END
				ELSE err(97)
				END
			| abs:
				IF f IN intSet + realSet THEN
					IF z.class = Nconst THEN
						IF IsNegConst(z.conval, f) THEN NegateConst(z.conval, z.conval, z.typ) END;
						z.obj := NIL
					ELSE
						IF f < Int32 THEN Convert(z, DevCPT.int32typ) END;
						z := NewOp(op, z.typ, z)
					END
				ELSE err(111)
				END
			| cap:
				IF f IN charSet THEN
					IF z.class = Nconst THEN
						IF ODD(z.conval.intval DIV 32) THEN DEC(z.conval.intval, 32) END;
						z.obj := NIL
					ELSE z := NewOp(op, typ, z)
					END
				ELSE err(111); z.typ := DevCPT.char8typ
				END
			| odd:
				IF f IN intSet THEN
					IF z.class = Nconst THEN
						DivModConst(z.conval, two, FALSE, z.typ);	(* z MOD 2 *)
						z.obj := NIL
					ELSE z := NewOp(op, typ, z)
					END
				ELSE err(111)
				END ;
				z.typ := DevCPT.booltyp
			| adr: (*ADR*)
				IF z.class = Nproc THEN
					IF z.obj.mnolev > 0 THEN err(73)
					ELSIF z.obj.mode = LProc THEN z.obj.mode := XProc
					END;
					z := NewOp(op, typ, z)
				ELSIF z.class = Ntype THEN
					IF z.obj.typ.untagged THEN err(111) END;
					z := NewOp(op, typ, z)
				ELSIF (z.class < Nconst) OR (z.class = Nconst) & (f IN {String8, String16}) THEN
					z := NewOp(op, typ, z)
				ELSE err(127)
				END ;
				z.typ := DevCPT.int32typ
			| typfn, size: (*TYP, SIZE*)
				z := NewOp(op, typ, z);
				z.typ := DevCPT.int32typ
			| cc: (*SYSTEM.CC*)
				IF (f IN intSet) & (z.class = Nconst) THEN
					IF (0 <= z.conval.intval) & (z.conval.intval <= DevCPM.MaxCC) & (z.conval.realval = 0) THEN
						z := NewOp(op, typ, z)
					ELSE err(219)
					END
				ELSE err(69)
				END;
				z.typ := DevCPT.booltyp
			END
		END;
		x := z
	END MOp;
	
	PROCEDURE ConstOp(op: SHORTINT; x, y: DevCPT.Node);
		VAR f: SHORTINT; i, j: INTEGER; xval, yval: DevCPT.Const; ext: DevCPT.ConstExt; t: DevCPT.Struct;
	BEGIN
		f := x.typ.form;
		IF f = y.typ.form THEN
			xval := x.conval; yval := y.conval;
			CASE op OF
			| times:
				IF f IN intSet + realSet THEN MulConst(xval, yval, xval, x.typ)
				ELSIF f = Set THEN xval.setval := xval.setval * yval.setval
				ELSIF f # Undef THEN err(101)
				END
			| slash:
				IF f IN realSet THEN DivConst(xval, yval, xval, x.typ)
				ELSIF f = Set THEN xval.setval := xval.setval / yval.setval
				ELSIF f # Undef THEN err(102)
				END
			| div:
				IF f IN intSet THEN DivModConst(xval, yval, TRUE, x.typ)
				ELSIF f # Undef THEN err(103)
				END
			| mod:
				IF f IN intSet THEN DivModConst(xval, yval, FALSE, x.typ)
				ELSIF f # Undef THEN err(104)
				END
			| and:
				IF f = Bool THEN xval.intval := BoolToInt(IntToBool(xval.intval) & IntToBool(yval.intval))
				ELSE err(94)
				END
			| plus:
				IF f IN intSet + realSet THEN AddConst(xval, yval, xval, x.typ)
				ELSIF f = Set THEN xval.setval := xval.setval + yval.setval
				ELSIF (f IN {String8, String16}) & (xval.ext # NIL) & (yval.ext # NIL) THEN
					NEW(ext, LEN(xval.ext^) + LEN(yval.ext^));
					i := 0; WHILE xval.ext[i] # 0X DO ext[i] := xval.ext[i]; INC(i) END;
					j := 0; WHILE yval.ext[j] # 0X DO ext[i] := yval.ext[j]; INC(i); INC(j) END;
					ext[i] := 0X; xval.ext := ext; INC(xval.intval2, yval.intval2 - 1)
				ELSIF f # Undef THEN err(105)
				END
			| minus:
				IF f IN intSet + realSet THEN SubConst(xval, yval, xval, x.typ)
				ELSIF f = Set THEN xval.setval := xval.setval - yval.setval
				ELSIF f # Undef THEN err(106)
				END
			| min:
				IF f IN intSet + realSet THEN
					IF LessConst(yval, xval, f) THEN xval^ := yval^ END
				ELSIF f # Undef THEN err(111)
				END
			| max:
				IF f IN intSet + realSet THEN
					IF LessConst(xval, yval, f) THEN xval^ := yval^ END
				ELSIF f # Undef THEN err(111)
				END
			| or:
				IF f = Bool THEN xval.intval := BoolToInt(IntToBool(xval.intval) OR IntToBool(yval.intval))
				ELSE err(95)
				END
			| eql: xval.intval := BoolToInt(EqualConst(xval, yval, f)); x.typ := DevCPT.booltyp
			| neq: xval.intval := BoolToInt(~EqualConst(xval, yval, f)); x.typ := DevCPT.booltyp
			| lss: xval.intval := BoolToInt(LessConst(xval, yval, f)); x.typ := DevCPT.booltyp
			| leq: xval.intval := BoolToInt(~LessConst(yval, xval, f)); x.typ := DevCPT.booltyp
			| gtr: xval.intval := BoolToInt(LessConst(yval, xval, f)); x.typ := DevCPT.booltyp
			| geq: xval.intval := BoolToInt(~LessConst(xval, yval, f)); x.typ := DevCPT.booltyp
			END
		ELSE err(100)
		END;
		x.obj := NIL
	END ConstOp;
	
	PROCEDURE Op*(op: BYTE; VAR x: DevCPT.Node; y: DevCPT.Node);
		VAR f, g: SHORTINT; t, z: DevCPT.Node; typ: DevCPT.Struct; do: BOOLEAN; val: INTEGER;

		PROCEDURE NewOp(op: BYTE; typ: DevCPT.Struct; VAR x: DevCPT.Node; y: DevCPT.Node);
			VAR node: DevCPT.Node;
		BEGIN
			node := DevCPT.NewNode(Ndop); node.subcl := op; node.typ := typ;
			node.left := x; node.right := y; x := node
		END NewOp;

	BEGIN z := x;
		IF (z.class = Ntype) OR (z.class = Nproc) OR (y.class = Ntype) OR (y.class = Nproc) THEN err(126)
		ELSE
			Promote(z, y, op);
			IF (z.class = Nconst) & (y.class = Nconst) THEN ConstOp(op, z, y)
			ELSE
				typ := z.typ; f := typ.form; g := y.typ.form;
				CASE op OF
				| times:
					do := TRUE;
					IF f IN intSet THEN
						IF z.class = Nconst THEN
							IF EqualConst(z.conval, one, f) THEN do := FALSE; z := y
							ELSIF EqualConst(z.conval, zero, f) THEN do := FALSE
							ELSE val := Log(z);
								IF val >= 0 THEN
									t := y; y := z; z := t;
									op := ash; y.typ := DevCPT.int32typ; y.conval.intval := val; y.obj := NIL
								END
							END
						ELSIF y.class = Nconst THEN
							IF EqualConst(y.conval, one, f) THEN do := FALSE
							ELSIF EqualConst(y.conval, zero, f) THEN do := FALSE; z := y
							ELSE val := Log(y);
								IF val >= 0 THEN
									op := ash; y.typ := DevCPT.int32typ; y.conval.intval := val; y.obj := NIL
								END
							END
						END
					ELSIF ~(f IN {Undef, Real32..Set}) THEN err(105); typ := DevCPT.undftyp
					END ;
					IF do THEN NewOp(op, typ, z, y) END;
				| slash:
					IF f IN realSet THEN (* OK *)
					ELSIF (f # Set) & (f # Undef) THEN err(102); typ := DevCPT.undftyp
					END ;
					NewOp(op, typ, z, y)
				| div:
					do := TRUE;
					IF f IN intSet THEN
						IF y.class = Nconst THEN
							IF EqualConst(y.conval, zero, f) THEN err(205)
							ELSIF EqualConst(y.conval, one, f) THEN do := FALSE
							ELSE val := Log(y);
								IF val >= 0 THEN
									op := ash; y.typ := DevCPT.int32typ; y.conval.intval := -val; y.obj := NIL
								END
							END
						END
					ELSIF f # Undef THEN err(103); typ := DevCPT.undftyp
					END ;
					IF do THEN NewOp(op, typ, z, y) END;
				| mod:
					IF f IN intSet THEN
						IF y.class = Nconst THEN
							IF EqualConst(y.conval, zero, f) THEN err(205)
							ELSE val := Log(y);
								IF val >= 0 THEN
									op := msk; y.conval.intval := ASH(-1, val); y.obj := NIL
								END
							END
						END
					ELSIF f # Undef THEN err(104); typ := DevCPT.undftyp
					END ;
					NewOp(op, typ, z, y);
				| and:
					IF f = Bool THEN
						IF z.class = Nconst THEN
							IF IntToBool(z.conval.intval) THEN z := y END
						ELSIF (y.class = Nconst) & IntToBool(y.conval.intval) THEN (* optimize z & TRUE -> z *)
						ELSE NewOp(op, typ, z, y)
						END
					ELSIF f # Undef THEN err(94); z.typ := DevCPT.undftyp
					END
				| plus:
					IF ~(f IN {Undef, Int8..Set, Int64, String8, String16}) THEN err(105); typ := DevCPT.undftyp END;
					do := TRUE;
					IF f IN intSet THEN
						IF (z.class = Nconst) & EqualConst(z.conval, zero, f) THEN do := FALSE; z := y END ;
						IF (y.class = Nconst) & EqualConst(y.conval, zero, f) THEN do := FALSE END
					ELSIF f IN {String8, String16} THEN
						IF (z.class = Nconst) & (z.conval.intval2 = 1) THEN do := FALSE; z := y END ;
						IF (y.class = Nconst) & (y.conval.intval2 = 1) THEN do := FALSE END;
						IF do THEN
							IF z.class = Ndop THEN
								t := z; WHILE t.right.class = Ndop DO t := t.right END;
								IF (t.right.class = Nconst) & (y.class = Nconst) THEN
									ConstOp(op, t.right, y); do := FALSE
								ELSIF (t.right.class = Nconst) & (y.class = Ndop) & (y.left.class = Nconst) THEN
									ConstOp(op, t.right, y.left); y.left := t.right; t.right := y; do := FALSE
								ELSE
									NewOp(op, typ, t.right, y); do := FALSE
								END
							ELSE
								IF (z.class = Nconst) & (y.class = Ndop) & (y.left.class = Nconst) THEN
									ConstOp(op, z, y.left); y.left := z; z := y; do := FALSE
								END
							END
						END
					END ;
					IF do THEN NewOp(op, typ, z, y) END;
				| minus:
					IF ~(f IN {Undef, Int8..Set, Int64}) THEN err(106); typ := DevCPT.undftyp END;
					IF ~(f IN intSet) OR (y.class # Nconst) OR ~EqualConst(y.conval, zero, f) THEN NewOp(op, typ, z, y)
					END;
				| min, max:
					IF ~(f IN {Undef} + intSet + realSet + charSet) THEN err(111); typ := DevCPT.undftyp END;
					NewOp(op, typ, z, y);
				| or:
					IF f = Bool THEN
						IF z.class = Nconst THEN
							IF ~IntToBool(z.conval.intval) THEN z := y END
						ELSIF (y.class = Nconst) & ~IntToBool(y.conval.intval) THEN (* optimize z OR FALSE -> z *)
						ELSE NewOp(op, typ, z, y)
						END
					ELSIF f # Undef THEN err(95); z.typ := DevCPT.undftyp
					END
				| eql, neq, lss, leq, gtr, geq:
					IF f IN {String8, String16} THEN
						IF (f = String16) & (z.class = Nmop) & (z.subcl = conv) & (y.class = Nmop) & (y.subcl = conv) THEN
							z := z.left; y := y.left	(* remove LONG on both sides *)
						ELSIF (z.class = Nconst) & (z.conval.intval2 = 1) & (y.class = Nderef) THEN (* y$ = "" -> y[0] = 0X *)
							y := y.left; Index(y, NewIntConst(0)); z.typ := y.typ; z.conval.intval := 0
						ELSIF (y.class = Nconst) & (y.conval.intval2 = 1) & (z.class = Nderef) THEN (* z$ = "" -> z[0] = 0X *)
							z := z.left; Index(z, NewIntConst(0)); y.typ := z.typ; y.conval.intval := 0
						END;
						typ := DevCPT.booltyp
					ELSIF (f IN {Undef, Char8..Real64, Char16, Int64})
							OR (op <= neq) & ((f IN {Bool, Set, NilTyp, Pointer, ProcTyp}) OR (typ = DevCPT.guidtyp)) THEN
						typ := DevCPT.booltyp
					ELSE err(107); typ := DevCPT.undftyp
					END;
					NewOp(op, typ, z, y)
				END
			END
		END;
		x := z
	END Op;

	PROCEDURE SetRange*(VAR x: DevCPT.Node; y: DevCPT.Node);
		VAR k, l: INTEGER;
	BEGIN
		IF (x.class = Ntype) OR (x.class = Nproc) OR (y.class = Ntype) OR (y.class = Nproc) THEN err(126)	
		ELSIF (x.typ.form IN intSet) & (y.typ.form IN intSet) THEN
			IF x.typ.form = Int64 THEN Convert(x, DevCPT.int32typ) END;
			IF y.typ.form = Int64 THEN Convert(y, DevCPT.int32typ) END;
			IF x.class = Nconst THEN
				k := x.conval.intval;
				IF (0 > k) OR (k > DevCPM.MaxSet) OR (x.conval.realval # 0) THEN err(202) END
			END ;
			IF y.class = Nconst THEN
				l := y.conval.intval;
				IF (0 > l) OR (l > DevCPM.MaxSet) OR (y.conval.realval # 0) THEN err(202) END
			END ;
			IF (x.class = Nconst) & (y.class = Nconst) THEN
				IF k <= l THEN
					x.conval.setval := {k..l}
				ELSE err(201); x.conval.setval := {l..k}
				END ;
				x.obj := NIL
			ELSE BindNodes(Nupto, DevCPT.settyp, x, y)
			END
		ELSE err(93)
		END ;
		x.typ := DevCPT.settyp
	END SetRange;

	PROCEDURE SetElem*(VAR x: DevCPT.Node);
		VAR k: INTEGER;
	BEGIN
		IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126) END;
		IF x.typ.form IN intSet THEN
			IF x.typ.form = Int64 THEN Convert(x, DevCPT.int32typ) END;
			IF x.class = Nconst THEN
				k := x.conval.intval;
				IF (0 <= k) & (k <= DevCPM.MaxSet) & (x.conval.realval = 0) THEN x.conval.setval := {k}
				ELSE err(202)
				END ;
				x.obj := NIL
			ELSE BindNodes(Nmop, DevCPT.settyp, x, NIL); x.subcl := bit
			END ;
		ELSE err(93)
		END;
		x.typ := DevCPT.settyp
	END SetElem;
	
	PROCEDURE CheckAssign* (x: DevCPT.Struct; VAR ynode: DevCPT.Node);
	(* x := y, checks assignment compatibility *)
		VAR f, g: SHORTINT; y, b: DevCPT.Struct;
	BEGIN
		y := ynode.typ; f := x.form; g := y.form;
		IF (ynode.class = Ntype) OR (ynode.class = Nproc) & (f # ProcTyp) THEN err(126) END ;
		CASE f OF
		| Undef, String8, String16, Byte:
		| Bool, Set:
			IF g # f THEN err(113) END
		| Int8, Int16, Int32, Int64, Real32, Real64:	(* SR *)
			IF (g IN intSet) OR (g IN realSet) & (f IN realSet) THEN
				IF ynode.class = Nconst THEN Convert(ynode, x)
				ELSIF ~DevCPT.Includes(f, g) THEN err(113)
				END
			ELSE err(113)
			END
(*			
			IF ~(g IN intSet + realSet) OR ~DevCPT.Includes(f, g) & (~(g IN intSet) OR (ynode.class # Nconst)) THEN
				err(113)
			ELSIF ynode.class = Nconst THEN Convert(ynode, x)
			END
*)
		| Char8, Char16:
			IF ~(g IN charSet) OR ~DevCPT.Includes(f, g) THEN err(113)
			ELSIF ynode.class = Nconst THEN Convert(ynode, x)
			END
		| Pointer:
			b := x.BaseTyp;
			IF DevCPT.Extends(y, x)
				OR (g = NilTyp)
				OR (g = Pointer)
					& ((x = DevCPT.sysptrtyp) OR (DevCPM.java IN DevCPM.options) & (x = DevCPT.anyptrtyp))
			THEN (* ok *)
			ELSIF (b.comp = DynArr) & b.untagged THEN	(* pointer to untagged open array *)
				IF ynode.class = Nconst THEN CheckString(ynode, b, 113)
				ELSIF ~(y.comp IN {Array, DynArr}) OR ~DevCPT.EqualType(b.BaseTyp, y.BaseTyp) THEN err(113)
				END
			ELSIF b.untagged & (ynode.class = Nmop) & (ynode.subcl = adr) THEN	(* p := ADR(r) *)
				IF (b.comp = DynArr) & (ynode.left.class = Nconst) THEN CheckString(ynode.left, b, 113)
				ELSIF ~DevCPT.Extends(ynode.left.typ, b) THEN err(113)
				END
			ELSIF (b.sysflag = jstr) & ((g = String16) OR (ynode.class = Nconst) & (g IN {Char8, Char16, String8}))
			THEN
				IF g # String16 THEN Convert(ynode, DevCPT.string16typ) END
			ELSE err(113)
			END
		| ProcTyp:
			IF DevCPT.EqualType(x, y) OR (g = NilTyp) THEN (* ok *)
			ELSIF (ynode.class = Nproc) & (ynode.obj.mode IN {XProc, IProc, LProc}) THEN
				IF ynode.obj.mode = LProc THEN
					IF ynode.obj.mnolev = 0 THEN ynode.obj.mode := XProc ELSE err(73) END
				END;
				IF (x.sysflag = 0) & (ynode.obj.sysflag >= 0) OR (x.sysflag = ynode.obj.sysflag) THEN
					IF DevCPT.EqualType(x.BaseTyp, ynode.obj.typ) THEN CheckParameters(x.link, ynode.obj.link, FALSE)
					ELSE err(117)
					END
				ELSE err(113)
				END
			ELSE err(113)
			END
		| NoTyp, NilTyp: err(113)
		| Comp:
			x.pvused := TRUE;	(* idfp of y guarantees assignment compatibility with x *)
			IF x.comp = Record THEN
				IF ~DevCPT.EqualType(x, y) OR (x.attribute # 0) THEN err(113) END
			ELSIF g IN {Char8, Char16, String8, String16} THEN
				IF (x.BaseTyp.form = Char16) & (g = String8) THEN Convert(ynode, DevCPT.string16typ)
				ELSE CheckString(ynode, x, 113);
				END;
				IF (x # DevCPT.guidtyp) & (x.comp = Array) & (ynode.class = Nconst) & (ynode.conval.intval2 > x.n) THEN
					err(114)
				END
			ELSIF (x.comp = Array) & DevCPT.EqualType(x, y) THEN (* ok *)
			ELSE err(113)
			END
		END
	END CheckAssign;
	
	PROCEDURE AssignString (VAR x: DevCPT.Node; str: DevCPT.Node);	(* x := str or x[0] := 0X *)
	BEGIN
		ASSERT((str.class = Nconst) & (str.typ.form IN {String8, String16}));
		IF (x.typ.comp IN {Array, DynArr}) & (str.conval.intval2 = 1) THEN	(* x := "" -> x[0] := 0X *)
			Index(x, NewIntConst(0));
			str.typ := x.typ; str.conval.intval := 0;
		END;
		BindNodes(Nassign, DevCPT.notyp, x, str); x.subcl := assign
	END AssignString;
	
	PROCEDURE CheckLeaf(x: DevCPT.Node; dynArrToo: BOOLEAN);
	BEGIN
		IF (x.class = Nmop) & (x.subcl = val) THEN x := x.left END ;
		IF x.class = Nguard THEN x := x.left END ;	(* skip last (and unique) guard *)
		IF (x.class = Nvar) & (dynArrToo OR (x.typ.comp # DynArr)) THEN x.obj.leaf := FALSE END
	END CheckLeaf;
	
	PROCEDURE CheckOldType (x: DevCPT.Node);
	BEGIN
		IF ~(DevCPM.oberon IN DevCPM.options)
			& ((x.typ = DevCPT.lreal64typ) OR (x.typ = DevCPT.lint64typ) OR (x.typ = DevCPT.lchar16typ)) THEN
			err(198)
		END
	END CheckOldType;
	
	PROCEDURE StPar0*(VAR par0: DevCPT.Node; fctno: SHORTINT);	(* par0: first param of standard proc *)
		VAR f: SHORTINT; typ: DevCPT.Struct; x, t: DevCPT.Node;
	BEGIN x := par0; f := x.typ.form;
		CASE fctno OF
		  haltfn: (*HALT*)
				IF (f IN intSet - {Int64}) & (x.class = Nconst) THEN
					IF (DevCPM.MinHaltNr <= x.conval.intval) & (x.conval.intval <= DevCPM.MaxHaltNr) THEN
						BindNodes(Ntrap, DevCPT.notyp, x, x)
					ELSE err(218)
					END
				ELSIF (DevCPM.java IN DevCPM.options)
					& ((x.class = Ntype) OR (x.class = Nvar))
					& (x.typ.form = Pointer)
				THEN
					BindNodes(Ntrap, DevCPT.notyp, x, x)
				ELSE err(69)
				END ;
				x.typ := DevCPT.notyp
		| newfn: (*NEW*)
				typ := DevCPT.notyp;
				IF NotVar(x) THEN err(112)
				ELSIF f = Pointer THEN
					IF DevCPM.NEWusingAdr THEN CheckLeaf(x, TRUE) END ;
					IF x.readonly THEN err(76)
					ELSIF (x.typ.BaseTyp.attribute = absAttr)
						OR (x.typ.BaseTyp.attribute = limAttr) & (x.typ.BaseTyp.mno # 0) THEN err(193)
					ELSIF (x.obj # NIL) & ODD(x.obj.sysflag DIV newBit) THEN err(167)
					END ;
					MarkAsUsed(x);
					f := x.typ.BaseTyp.comp;
					IF f IN {Record, DynArr, Array} THEN
						IF f = DynArr THEN typ := x.typ.BaseTyp END ;
						BindNodes(Nassign, DevCPT.notyp, x, NIL); x.subcl := newfn
					ELSE err(111)
					END
				ELSE err(111)
				END ;
				x.typ := typ
		| absfn: (*ABS*)
				MOp(abs, x)
		| capfn: (*CAP*)
				MOp(cap, x)
		| ordfn: (*ORD*) 
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f = Char8 THEN Convert(x, DevCPT.int16typ)
				ELSIF f = Char16 THEN Convert(x, DevCPT.int32typ)
				ELSIF f = Set THEN Convert(x, DevCPT.int32typ)
				ELSE err(111)
				END
		| bitsfn: (*BITS*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN {Int8, Int16, Int32} THEN Convert(x, DevCPT.settyp)
				ELSE err(111)
				END
		| entierfn: (*ENTIER*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN realSet THEN Convert(x, DevCPT.int64typ)
				ELSE err(111)
				END ;
				x.typ := DevCPT.int64typ
		| lentierfcn: (* LENTIER *)
				IF ~(DevCPM.oberon IN DevCPM.options) THEN err(199) END;
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN realSet THEN Convert(x, DevCPT.int64typ)
				ELSE err(111)
				END ;
				x.typ := DevCPT.int64typ
		| oddfn: (*ODD*)
				MOp(odd, x)
		| minfn: (*MIN*)
				IF x.class = Ntype THEN
					CheckOldType(x);
					CASE f OF
					  Bool:  x := NewBoolConst(FALSE)
					| Char8:  x := NewIntConst(0); x.typ := DevCPT.char8typ
					| Char16:  x := NewIntConst(0); x.typ := DevCPT.char8typ
					| Int8:  x := NewIntConst(-128)
					| Int16:   x := NewIntConst(-32768)
					| Int32:  x := NewIntConst(-2147483648)
					| Int64:  x := NewLargeIntConst(0, -9223372036854775808.0E0)	(* -2^63 *)
					| Set:   x := NewIntConst(0) (*; x.typ := DevCPT.int16typ *)
					| Real32:  x := NewRealConst(DevCPM.MinReal32, DevCPT.real64typ)
					| Real64: x := NewRealConst(DevCPM.MinReal64, DevCPT.real64typ)
					ELSE err(111)
					END;
					x.hint := 1
				ELSIF ~(f IN intSet + realSet + charSet) THEN err(111)
				END
		| maxfn: (*MAX*)
				IF x.class = Ntype THEN
					CheckOldType(x);
					CASE f OF
					  Bool:  x := NewBoolConst(TRUE)
					| Char8:  x := NewIntConst(0FFH); x.typ := DevCPT.char8typ
					| Char16:  x := NewIntConst(0FFFFH); x.typ := DevCPT.char16typ
					| Int8:  x := NewIntConst(127)
					| Int16:   x := NewIntConst(32767)
					| Int32:  x := NewIntConst(2147483647)
					| Int64:  x := NewLargeIntConst(-1, 9223372036854775808.0E0)	(* 2^63 - 1 *)
					| Set:   x := NewIntConst(31) (*; x.typ := DevCPT.int16typ *)
					| Real32:  x := NewRealConst(DevCPM.MaxReal32, DevCPT.real64typ)
					| Real64: x := NewRealConst(DevCPM.MaxReal64, DevCPT.real64typ)
					ELSE err(111)
					END;
					x.hint := 1
				ELSIF ~(f IN intSet + realSet + charSet) THEN err(111)
				END
		| chrfn: (*CHR*) 
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN {Undef, Int8..Int32, Int64} THEN Convert(x, DevCPT.char16typ)
				ELSE err(111); x.typ := DevCPT.char16typ
				END
		| lchrfn: (* LCHR *)
				IF ~(DevCPM.oberon IN DevCPM.options) THEN err(199) END;
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN {Undef, Int8..Int32, Int64} THEN Convert(x, DevCPT.char16typ)
				ELSE err(111); x.typ := DevCPT.char16typ
				END
		| shortfn: (*SHORT*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSE
					IF (x.typ.comp IN {Array, DynArr}) & (x.typ.BaseTyp.form IN charSet) THEN StrDeref(x); f := x.typ.form
					END;
					IF f = Int16 THEN Convert(x, DevCPT.int8typ)
					ELSIF f = Int32 THEN Convert(x, DevCPT.int16typ)
					ELSIF f = Int64 THEN Convert(x, DevCPT.int32typ)
					ELSIF f = Real64 THEN Convert(x, DevCPT.real32typ)
					ELSIF f = Char16 THEN Convert(x, DevCPT.char8typ)
					ELSIF f = String16 THEN Convert(x, DevCPT.string8typ)
					ELSE err(111)
					END
				END
		| longfn: (*LONG*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSE
					IF (x.typ.comp IN {Array, DynArr}) & (x.typ.BaseTyp.form IN charSet) THEN StrDeref(x); f := x.typ.form
					END;
					IF f = Int8 THEN Convert(x, DevCPT.int16typ)
					ELSIF f = Int16 THEN Convert(x, DevCPT.int32typ)
					ELSIF f = Int32 THEN Convert(x, DevCPT.int64typ)
					ELSIF f = Real32 THEN Convert(x, DevCPT.real64typ)
					ELSIF f = Char8 THEN Convert(x, DevCPT.char16typ)
					ELSIF f = String8 THEN Convert(x, DevCPT.string16typ)
					ELSE err(111)
					END
				END
		| incfn, decfn: (*INC, DEC*) 
				IF NotVar(x) THEN err(112)
				ELSIF ~(f IN intSet) THEN err(111)
				ELSIF x.readonly THEN err(76)
				END;
				MarkAsUsed(x)
		| inclfn, exclfn: (*INCL, EXCL*)
				IF NotVar(x) THEN err(112)
				ELSIF f # Set THEN err(111); x.typ := DevCPT.settyp
				ELSIF x.readonly THEN err(76)
				END;
				MarkAsUsed(x)
		| lenfn: (*LEN*)
				IF (* (x.class = Ntype) OR *) (x.class = Nproc) THEN err(126)	(* !!! *)
				(* ELSIF x.typ.sysflag = jstr THEN StrDeref(x) *)
				ELSE
					IF x.typ.form = Pointer THEN DeRef(x) END;
					IF x.class = Nconst THEN
						IF x.typ.form = Char8 THEN CharToString8(x)
						ELSIF x.typ.form = Char16 THEN CharToString16(x)
						END
					END;
					IF ~(x.typ.comp IN {DynArr, Array}) & ~(x.typ.form IN {String8, String16}) THEN err(131) END
				END
		| copyfn: (*COPY*)
				IF ~(DevCPM.oberon IN DevCPM.options) THEN err(199) END;
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126) END
		| ashfn: (*ASH*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN intSet THEN
					IF f < Int32 THEN Convert(x, DevCPT.int32typ) END
				ELSE err(111); x.typ := DevCPT.int32typ
				END
		| adrfn: (*ADR*)
				IF x.class = Ntype THEN CheckOldType(x) END;
				CheckLeaf(x, FALSE); MOp(adr, x)
		| typfn: (*TYP*)
				CheckLeaf(x, FALSE);
				IF x.class = Ntype THEN
					CheckOldType(x);
					IF x.typ.form = Pointer THEN x := NewLeaf(x.typ.BaseTyp.strobj) END;
					IF x.typ.comp # Record THEN err(111) END;
					MOp(adr, x)
				ELSE
					IF x.typ.form = Pointer THEN DeRef(x) END;
					IF x.typ.comp # Record THEN err(111) END;
					MOp(typfn, x)
				END
		| sizefn: (*SIZE*)
				IF x.class # Ntype THEN err(110); x := NewIntConst(1)
				ELSIF (f IN {Byte..Set, Pointer, ProcTyp, Char16, Int64}) OR (x.typ.comp IN {Array, Record}) THEN
					CheckOldType(x); x.typ.pvused := TRUE;
					IF typSize # NIL THEN
						typSize(x.typ); x := NewIntConst(x.typ.size)
					ELSE
						MOp(size, x)
					END
				ELSE err(111); x := NewIntConst(1)
				END
		| thisrecfn, (*THISRECORD*)
		  thisarrfn: (*THISARRAY*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN {Int8, Int16} THEN Convert(x, DevCPT.int32typ)
				ELSIF f # Int32 THEN err(111)
				END
		| ccfn: (*SYSTEM.CC*)
				MOp(cc, x)
		| lshfn, rotfn: (*SYSTEM.LSH, SYSTEM.ROT*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF ~(f IN intSet + charSet + {Byte, Set}) THEN err(111)
				END
		| getfn, putfn, bitfn, movefn: (*SYSTEM.GET, SYSTEM.PUT, SYSTEM.BIT, SYSTEM.MOVE*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF (x.class = Nconst) & (f IN {Int8, Int16}) THEN Convert(x, DevCPT.int32typ)
				ELSIF ~(f IN {Int32, Pointer}) THEN err(111); x.typ := DevCPT.int32typ
				END
		| getrfn, putrfn: (*SYSTEM.GETREG, SYSTEM.PUTREG*)
				IF (f IN intSet) & (x.class = Nconst) THEN
					IF (x.conval.intval < DevCPM.MinRegNr) OR (x.conval.intval > DevCPM.MaxRegNr) THEN err(220)
					END
				ELSE err(69)
				END
		| valfn: (*SYSTEM.VAL*)
				IF x.class # Ntype THEN err(110)
				ELSIF (f IN {Undef, String8, String16, NoTyp, NilTyp}) (* OR (x.typ.comp = DynArr) *) THEN err(111)
				ELSE CheckOldType(x)
				END
		| assertfn: (*ASSERT*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126); x := NewBoolConst(FALSE)
				ELSIF f # Bool THEN err(120); x := NewBoolConst(FALSE)
				ELSE MOp(not, x)
				END
		| validfn: (* VALID *)
				IF (x.class = Nvarpar) & ODD(x.obj.sysflag DIV nilBit) THEN
					MOp(adr, x); x.typ := DevCPT.sysptrtyp; Op(neq, x, Nil())
				ELSE err(111)
				END;
				x.typ := DevCPT.booltyp
		| iidfn: (* COM.IID *)
				IF (x.class = Nconst) & (f = String8) THEN StringToGuid(x)
				ELSE
					typ := x.typ;
					IF typ.form = Pointer THEN typ := typ.BaseTyp END;
					IF (typ.sysflag = interface) & (typ.ext # NIL) & (typ.strobj # NIL) THEN
						IF x.obj # typ.strobj THEN x := NewLeaf(typ.strobj) END
					ELSE err(111)
					END;
					x.class := Nconst; x.typ := DevCPT.guidtyp
				END
		| queryfn: (* COM.QUERY *)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f # Pointer THEN err(111)
				END
		END ;
		par0 := x
	END StPar0;

	PROCEDURE StPar1*(VAR par0: DevCPT.Node; x: DevCPT.Node; fctno: BYTE);
	(* x: second parameter of standard proc *)
		VAR f, n, L, i: INTEGER; typ, tp1: DevCPT.Struct; p, t: DevCPT.Node;
		
		PROCEDURE NewOp(class, subcl: BYTE; left, right: DevCPT.Node): DevCPT.Node;
			VAR node: DevCPT.Node;
		BEGIN
			node := DevCPT.NewNode(class); node.subcl := subcl;
			node.left := left; node.right := right; RETURN node
		END NewOp;
		
	BEGIN p := par0; f := x.typ.form;
		CASE fctno OF
		  incfn, decfn: (*INC DEC*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126); p.typ := DevCPT.notyp
				ELSE
					IF f # p.typ.form THEN
						IF f IN intSet THEN Convert(x, p.typ)
						ELSE err(111)
						END
					END ;
					p := NewOp(Nassign, fctno, p, x);
					p.typ := DevCPT.notyp
				END
		| inclfn, exclfn: (*INCL, EXCL*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN intSet THEN
					IF f = Int64 THEN Convert(x, DevCPT.int32typ) END;
					IF (x.class = Nconst) & ((0 > x.conval.intval) OR (x.conval.intval > DevCPM.MaxSet)) THEN err(202)
					END ;
					p := NewOp(Nassign, fctno, p, x)
				ELSE err(111)
				END ;
				p.typ := DevCPT.notyp
		| lenfn: (*LEN*)
				IF ~(f IN intSet) OR (x.class # Nconst) THEN err(69)
				ELSE
					IF f = Int64 THEN Convert(x, DevCPT.int32typ) END;
					L := SHORT(x.conval.intval); typ := p.typ;
					WHILE (L > 0) & (typ.comp IN {DynArr, Array}) DO typ := typ.BaseTyp; DEC(L) END ;
					IF (L # 0) OR ~(typ.comp IN {DynArr, Array}) THEN err(132)
					ELSE x.obj := NIL;
						IF typ.comp = DynArr THEN
							WHILE p.class = Nindex DO
								p := p.left; INC(x.conval.intval) (* possible side effect ignored *)
							END;
							p := NewOp(Ndop, len, p, x); p.typ := DevCPT.int32typ
						ELSE p := x; p.conval.intval := typ.n; p.typ := DevCPT.int32typ
						END
					END
				END
		| copyfn: (*COPY*)
				IF NotVar(x) THEN err(112)
				ELSIF x.readonly THEN err(76)
				ELSE
					CheckString(p, x.typ, 111); t := x; x := p; p := t;
					IF (x.class = Nconst) & (x.typ.form IN {String8, String16}) THEN AssignString(p, x)
					ELSE p := NewOp(Nassign, copyfn, p, x)
					END
				END ;
				p.typ := DevCPT.notyp; MarkAsUsed(x)
		| ashfn: (*ASH*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN intSet THEN
					IF (x.class = Nconst) & ((x.conval.intval > 64) OR (x.conval.intval < -64)) THEN err(208)
					ELSIF (p.class = Nconst) & (x.class = Nconst) THEN
						n := x.conval.intval;
						IF n > 0 THEN
							WHILE n > 0 DO MulConst(p.conval, two, p.conval, p.typ); DEC(n) END
						ELSE
							WHILE n < 0 DO DivModConst(p.conval, two, TRUE, p.typ); INC(n) END
						END;
						p.obj := NIL
					ELSE
						IF f = Int64 THEN Convert(x, DevCPT.int32typ) END;
						typ := p.typ; p := NewOp(Ndop, ash, p, x); p.typ := typ
					END
				ELSE err(111)
				END
		| minfn: (*MIN*)
				IF p.class # Ntype THEN Op(min, p, x) ELSE err(64) END
		| maxfn: (*MAX*)
				IF p.class # Ntype THEN Op(max, p, x) ELSE err(64) END
		| newfn: (*NEW(p, x...)*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF p.typ.comp = DynArr THEN
					IF f IN intSet THEN
						IF f = Int64 THEN Convert(x, DevCPT.int32typ) END;
						IF (x.class = Nconst) & (x.conval.intval <= 0)
							& (~(DevCPM.java IN DevCPM.options) OR (x.conval.intval < 0))THEN err(63) END
					ELSE err(111)
					END ;
					p.right := x; p.typ := p.typ.BaseTyp
				ELSIF (p.left # NIL) & (p.left.typ.form = Pointer) THEN
					typ := p.left.typ;
					WHILE (typ # DevCPT.undftyp) & (typ.BaseTyp # NIL) DO typ := typ.BaseTyp END;
					IF typ.sysflag = interface THEN
						typ := x.typ;
						WHILE (typ # DevCPT.undftyp) & (typ.BaseTyp # NIL) DO typ := typ.BaseTyp END;
						IF (f = Pointer) & (typ.sysflag = interface) THEN
							p.right := x
						ELSE err(169)
						END
					ELSE err(64)
					END
				ELSE err(111)
				END
		| thisrecfn, (*THISRECORD*)
		  thisarrfn: (*THISARRAY*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN {Int8, Int16, Int32} THEN
					IF f < Int32 THEN Convert(x, DevCPT.int32typ) END;
					p := NewOp(Ndop, fctno, p, x); p.typ := DevCPT.undftyp
				ELSE err(111)
				END
		| lshfn, rotfn: (*SYSTEM.LSH, SYSTEM.ROT*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF ~(f IN intSet) THEN err(111)
				ELSE
					IF fctno = lshfn THEN p := NewOp(Ndop, lsh, p, x) ELSE p := NewOp(Ndop, rot, p, x) END ;
					p.typ := p.left.typ
				END
		| getfn, putfn, getrfn, putrfn: (*SYSTEM.GET, SYSTEM.PUT, SYSTEM.GETREG, SYSTEM.PUTREG*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN {Undef..Set, NilTyp, Pointer, ProcTyp, Char16, Int64} THEN
					IF (fctno = getfn) OR (fctno = getrfn) THEN
						IF NotVar(x) THEN err(112) END ;
						t := x; x := p; p := t
					END ;
					p := NewOp(Nassign, fctno, p, x)
				ELSE err(111)
				END ;
				p.typ := DevCPT.notyp
		| bitfn: (*SYSTEM.BIT*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF f IN intSet THEN
					p := NewOp(Ndop, bit, p, x)
				ELSE err(111)
				END ;
				p.typ := DevCPT.booltyp
		| valfn: (*SYSTEM.VAL*)	(* type is changed without considering the byte ordering on the target machine *)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF x.typ.comp = DynArr THEN
					IF x.typ.untagged & ((p.typ.comp # DynArr) OR p.typ.untagged) THEN	(* ok *)
					ELSIF (p.typ.comp = DynArr) & (x.typ.n = p.typ.n) THEN
						typ := x.typ;
						WHILE typ.comp = DynArr DO typ := typ.BaseTyp END;
						tp1 := p.typ;
						WHILE tp1.comp = DynArr DO tp1 := tp1.BaseTyp END;
						IF typ.size # tp1.size THEN err(115) END
					ELSE err(115)
					END
				ELSIF p.typ.comp = DynArr THEN err(115)
				ELSIF (x.class = Nconst) & (f = String8) & (p.typ.form = Int32) & (x.conval.intval2 <= 5) THEN
					i := 0; n := 0;
					WHILE i < x.conval.intval2 - 1 DO n := 256 * n + ORD(x.conval.ext[i]); INC(i) END;
					x := NewIntConst(n)
				ELSIF (f IN {Undef, NoTyp, NilTyp}) OR (f IN {String8, String16}) & ~(DevCPM.java IN DevCPM.options) THEN err(111)
				END ;
				IF (x.class = Nconst) & (x.typ = p.typ) THEN	(* ok *)
				ELSIF (x.class >= Nconst) OR ((f IN realSet) # (p.typ.form IN realSet))
						OR (DevCPM.options * {DevCPM.java, DevCPM.allSysVal} # {}) THEN
					t := DevCPT.NewNode(Nmop); t.subcl := val; t.left := x; x := t
				ELSE x.readonly := FALSE
				END ;
				x.typ := p.typ; p := x
		| movefn: (*SYSTEM.MOVE*)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF (x.class = Nconst) & (f IN {Int8, Int16}) THEN Convert(x, DevCPT.int32typ)
				ELSIF ~(f IN {Int32, Pointer}) THEN err(111); x.typ := DevCPT.int32typ
				END ;
				p.link := x
		| assertfn: (*ASSERT*)
				IF (f IN intSet - {Int64}) & (x.class = Nconst) THEN
					IF (DevCPM.MinHaltNr <= x.conval.intval) & (x.conval.intval <= DevCPM.MaxHaltNr) THEN
						BindNodes(Ntrap, DevCPT.notyp, x, x);
						Construct(Nif, p, x); Construct(Nifelse, p, NIL); OptIf(p);
					ELSE err(218)
					END
				ELSIF
					(DevCPM.java IN DevCPM.options) & ((x.class = Ntype) OR (x.class = Nvar)) & (x.typ.form = Pointer)
				THEN
					BindNodes(Ntrap, DevCPT.notyp, x, x);
					Construct(Nif, p, x); Construct(Nifelse, p, NIL); OptIf(p);
				ELSE err(69)
				END;
				IF p = NIL THEN	(* ASSERT(TRUE) *)
				ELSIF p.class = Ntrap THEN err(99)
				ELSE p.subcl := assertfn
				END
		| queryfn: (* COM.QUERY *)
				IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
				ELSIF x.typ # DevCPT.guidtyp THEN err(111); x.typ := DevCPT.guidtyp
				END;
				p.link := x
		ELSE err(64)
		END ;
		par0 := p
	END StPar1;

	PROCEDURE StParN*(VAR par0: DevCPT.Node; x: DevCPT.Node; fctno, n: SHORTINT);
	(* x: n+1-th param of standard proc *)
		VAR node: DevCPT.Node; f: SHORTINT; p: DevCPT.Node; typ: DevCPT.Struct;
	BEGIN p := par0; f := x.typ.form;
		IF fctno = newfn THEN (*NEW(p, ..., x...*)
			IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
			ELSIF p.typ.comp # DynArr THEN err(64)
			ELSIF f IN intSet THEN
				IF f = Int64 THEN Convert(x, DevCPT.int32typ) END;
				IF (x.class = Nconst) & (x.conval.intval <= 0) THEN err(63) END;
				node := p.right; WHILE node.link # NIL DO node := node.link END;
				node.link := x; p.typ := p.typ.BaseTyp
			ELSE err(111)
			END
		ELSIF (fctno = movefn) & (n = 2) THEN (*SYSTEM.MOVE*)
			IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
			ELSIF f IN intSet THEN
				node := DevCPT.NewNode(Nassign); node.subcl := movefn; node.right := p;
				node.left := p.link; p.link := x; p := node
			ELSE err(111)
			END ;
			p.typ := DevCPT.notyp
		ELSIF (fctno = queryfn) & (n = 2) THEN (* COM.QUERY *)
			IF (x.class = Ntype) OR (x.class = Nproc) THEN err(126)
			ELSIF (x.class < Nconst) & (f = Pointer) & (x.typ.sysflag = interface) THEN
				IF ~DevCPT.Extends(p.typ, x.typ) THEN err(164) END;
				IF x.readonly THEN err(76) END;
				CheckNewParamPair(x, p.link);
				MarkAsUsed(x);
				node := DevCPT.NewNode(Ndop); node.subcl := queryfn;
				node.left := p; node.right := p.link; p.link := NIL; node.right.link := x; p := node
			ELSE err(111)
			END;
			p.typ := DevCPT.booltyp
		ELSE err(64)
		END ;
		par0 := p
	END StParN;

	PROCEDURE StFct*(VAR par0: DevCPT.Node; fctno: BYTE; parno: SHORTINT);
		VAR dim: SHORTINT; x, p: DevCPT.Node;
	BEGIN p := par0;
		IF fctno <= ashfn THEN
			IF (fctno = newfn) & (p.typ # DevCPT.notyp) THEN
				IF p.typ.comp = DynArr THEN err(65) END ;
				p.typ := DevCPT.notyp
			ELSIF (fctno = minfn) OR (fctno = maxfn) THEN
				IF (parno < 1) OR (parno = 1) & (p.hint # 1) THEN err(65) END;
				p.hint := 0
			ELSIF fctno <= sizefn THEN (* 1 param *)
				IF parno < 1 THEN err(65) END
			ELSE (* more than 1 param *)
				IF ((fctno = incfn) OR (fctno = decfn)) & (parno = 1) THEN (*INC, DEC*)
					BindNodes(Nassign, DevCPT.notyp, p, NewIntConst(1)); p.subcl := fctno; p.right.typ := p.left.typ
				ELSIF (fctno = lenfn) & (parno = 1) THEN (*LEN*)
					IF p.typ.form IN {String8, String16} THEN
						IF p.class = Nconst THEN p := NewIntConst(p.conval.intval2 - 1)
						ELSIF (p.class = Ndop) & (p.subcl = plus) THEN	(* propagate to leaf nodes *)
							StFct(p.left, lenfn, 1); StFct(p.right, lenfn, 1); p.typ := DevCPT.int32typ
						ELSE
							WHILE (p.class = Nmop) & (p.subcl = conv) DO p := p.left END;
							IF DevCPM.errors = 0 THEN ASSERT(p.class = Nderef) END;
							BindNodes(Ndop, DevCPT.int32typ, p, NewIntConst(0)); p.subcl := len
						END
					ELSIF p.typ.comp = DynArr THEN dim := 0;
						WHILE p.class = Nindex DO p := p.left; INC(dim) END ;	(* possible side effect ignored *)
						BindNodes(Ndop, DevCPT.int32typ, p, NewIntConst(dim)); p.subcl := len
					ELSE
						p := NewIntConst(p.typ.n)
					END
				ELSIF parno < 2 THEN err(65)
				END
			END
		ELSIF fctno = assertfn THEN
			IF parno = 1 THEN x := NIL;
				BindNodes(Ntrap, DevCPT.notyp, x, NewIntConst(AssertTrap));
				Construct(Nif, p, x); Construct(Nifelse, p, NIL); OptIf(p);
				IF p = NIL THEN	(* ASSERT(TRUE) *)
				ELSIF p.class = Ntrap THEN err(99)
				ELSE p.subcl := assertfn
				END
			ELSIF parno < 1 THEN err(65)
			END
		ELSIF (fctno >= lchrfn) & (fctno <= bytesfn) THEN
			IF parno < 1 THEN err(65) END
		ELSIF fctno < validfn THEN (*SYSTEM*)
			IF (parno < 1) OR
				(fctno > ccfn) & (parno < 2) OR
				(fctno = movefn) & (parno < 3) THEN err(65)
			END
		ELSIF (fctno = thisrecfn) OR (fctno = thisarrfn) THEN
			IF parno < 2 THEN err(65) END
		ELSE (* COM *)
			IF fctno = queryfn THEN
				IF parno < 3 THEN err(65) END
			ELSE
				IF parno < 1 THEN err(65) END
			END
		END ;
		par0 := p
	END StFct;
	
	PROCEDURE DynArrParCheck (ftyp: DevCPT.Struct; VAR ap: DevCPT.Node; fvarpar: BOOLEAN);
	(* check array compatibility *)
		VAR atyp: DevCPT.Struct;
	BEGIN (* ftyp.comp = DynArr *)
		atyp := ap.typ;
		IF atyp.form IN {Char8, Char16, String8, String16} THEN
			IF ~fvarpar & (ftyp.BaseTyp.form = Char16) & (atyp.form = String8) THEN Convert(ap, DevCPT.string16typ)
			ELSE CheckString(ap, ftyp, 67)
			END
		ELSE		
			WHILE (ftyp.comp = DynArr) & ((atyp.comp IN {Array, DynArr}) OR (atyp.form IN {String8, String16})) DO
				ftyp := ftyp.BaseTyp; atyp := atyp.BaseTyp
			END;
			IF ftyp.comp = DynArr THEN err(67)
			ELSIF ~fvarpar & (ftyp.form = Pointer) & DevCPT.Extends(atyp, ftyp) THEN (* ok *)
			ELSIF ~DevCPT.EqualType(ftyp, atyp) THEN err(66)
			END
		END
	END DynArrParCheck;

	PROCEDURE PrepCall*(VAR x: DevCPT.Node; VAR fpar: DevCPT.Object);
	BEGIN
		IF (x.obj # NIL) & (x.obj.mode IN {LProc, XProc, TProc, CProc}) THEN
			fpar := x.obj.link;
			IF x.obj.mode = TProc THEN
				IF fpar.typ.form = Pointer THEN
					IF x.left.class = Nderef THEN x.left := x.left.left (*undo DeRef*) ELSE err(71) END
				END;
				fpar := fpar.link
			END
		ELSIF (x.class # Ntype) & (x.typ # NIL) & (x.typ.form = ProcTyp) THEN
			fpar := x.typ.link
		ELSE err(121); fpar := NIL; x.typ := DevCPT.undftyp
		END
	END PrepCall;

	PROCEDURE Param* (VAR ap: DevCPT.Node; fp: DevCPT.Object);	(* checks parameter compatibilty *)
		VAR at, ft: DevCPT.Struct;
	BEGIN
		at := ap.typ; ft := fp.typ;
		IF fp.ptyp # NIL THEN ft := fp.ptyp END;	(* get original formal type *)
		IF ft.form # Undef THEN
			IF (ap.class = Ntype) OR (ap.class = Nproc) & (ft.form # ProcTyp) THEN err(126) END;
			IF fp.mode = VarPar THEN
				IF ODD(fp.sysflag DIV nilBit) & (at = DevCPT.niltyp) THEN (* ok *)
				ELSIF (ft.comp = Record) & ~ft.untagged & (ap.class = Ndop) & (ap.subcl = thisrecfn) THEN (* ok *)
				ELSIF (ft.comp = DynArr) & ~ft.untagged & (ft.n = 0) & (ap.class = Ndop) & (ap.subcl = thisarrfn) THEN
					(* ok *)
				ELSE
					IF fp.vis = inPar THEN
						IF (ft = DevCPT.guidtyp) & (ap.class = Nconst) & (at.form = String8) THEN
							StringToGuid(ap); at := ap.typ
(*
						ELSIF ((at.form IN charSet + {String8, String16}) OR (at = DevCPT.guidtyp))
								& ((ap.class = Nderef) OR (ap.class = Nconst)) THEN (* ok *)
						ELSIF NotVar(ap) THEN err(122)
*)
						END;
						IF ~NotVar(ap) THEN CheckLeaf(ap, FALSE) END
					ELSE
						IF NotVar(ap) THEN err(122)
						ELSIF ap.readonly THEN err(76)
						ELSIF (ap.obj # NIL) & ODD(ap.obj.sysflag DIV newBit) & ~ODD(fp.sysflag DIV newBit) THEN		
							err(167)
						ELSE MarkAsUsed(ap); CheckLeaf(ap, FALSE)
						END
					END;
					IF ft.comp = DynArr THEN DynArrParCheck(ft, ap, fp.vis # inPar)
					ELSIF ODD(fp.sysflag DIV newBit) THEN
						IF ~DevCPT.Extends(at, ft) THEN err(123) END
					ELSIF (ft = DevCPT.sysptrtyp) & (at.form = Pointer) THEN (* ok *)
					ELSIF (fp.vis # outPar) & (ft.comp = Record) & DevCPT.Extends(at, ft) THEN (* ok *)
					ELSIF covarOut & (fp.vis = outPar) & (ft.form = Pointer) & DevCPT.Extends(ft, at) THEN (* ok *)
					ELSIF fp.vis = inPar THEN CheckAssign(ft, ap)
					ELSIF ~DevCPT.EqualType(ft, at) THEN err(123)
					END
				END
			ELSIF ft.comp = DynArr THEN DynArrParCheck(ft, ap, FALSE)
			ELSE CheckAssign(ft, ap)
			END
		END
	END Param;
	
	PROCEDURE StaticLink*(dlev: BYTE; var: BOOLEAN);
		VAR scope: DevCPT.Object;
	BEGIN
		scope := DevCPT.topScope;
		WHILE dlev > 0 DO DEC(dlev);
			INCL(scope.link.conval.setval, slNeeded);
			scope := scope.left
		END;
		IF var THEN INCL(scope.link.conval.setval, imVar) END	(* !!! *)
	END StaticLink;

	PROCEDURE Call*(VAR x: DevCPT.Node; apar: DevCPT.Node; fp: DevCPT.Object);
		VAR typ: DevCPT.Struct; p: DevCPT.Node; lev: BYTE;
	BEGIN
		IF x.class = Nproc THEN typ := x.typ;
			lev := x.obj.mnolev;
			IF lev > 0 THEN StaticLink(SHORT(SHORT(DevCPT.topScope.mnolev-lev)), FALSE) END ;	(* !!! *)
			IF x.obj.mode = IProc THEN err(121) END
		ELSIF (x.class = Nfield) & (x.obj.mode = TProc) THEN typ := x.typ;
			x.class := Nproc; p := x.left; x.left := NIL; p.link := apar; apar := p; fp := x.obj.link
		ELSE typ := x.typ.BaseTyp
		END ;
		BindNodes(Ncall, typ, x, apar); x.obj := fp
	END Call;

	PROCEDURE Enter*(VAR procdec: DevCPT.Node; stat: DevCPT.Node; proc: DevCPT.Object);
		VAR x: DevCPT.Node;
	BEGIN
		x := DevCPT.NewNode(Nenter); x.typ := DevCPT.notyp; x.obj := proc;
		x.left := procdec; x.right := stat; procdec := x
	END Enter;
	
	PROCEDURE Return*(VAR x: DevCPT.Node; proc: DevCPT.Object);
		VAR node: DevCPT.Node;
	BEGIN
		IF proc = NIL THEN (* return from module *)
			IF x # NIL THEN err(124) END
		ELSE
			IF x # NIL THEN CheckAssign(proc.typ, x)
			ELSIF proc.typ # DevCPT.notyp THEN err(124)
			END
		END ;
		node := DevCPT.NewNode(Nreturn); node.typ := DevCPT.notyp; node.obj := proc; node.left := x; x := node
	END Return;

	PROCEDURE Assign*(VAR x: DevCPT.Node; y: DevCPT.Node);
		VAR z: DevCPT.Node;
	BEGIN
		IF (x.class >= Nconst) OR (x.typ.form IN {String8, String16}) THEN err(56) END ;
		CheckAssign(x.typ, y);
		IF x.readonly THEN err(76)
		ELSIF (x.obj # NIL) & ODD(x.obj.sysflag DIV newBit) THEN err(167)
		END ;
		MarkAsUsed(x);
		IF (y.class = Nconst) & (y.typ.form IN {String8, String16}) & (x.typ.form # Pointer) THEN AssignString(x, y)
		ELSE BindNodes(Nassign, DevCPT.notyp, x, y); x.subcl := assign
		END
	END Assign;
	
	PROCEDURE Inittd*(VAR inittd, last: DevCPT.Node; typ: DevCPT.Struct);
		VAR node: DevCPT.Node;
	BEGIN
		node := DevCPT.NewNode(Ninittd); node.typ := typ;
		node.conval := DevCPT.NewConst(); node.conval.intval := typ.txtpos;
		IF inittd = NIL THEN inittd := node ELSE last.link := node END ;
		last := node
	END Inittd;
	
	(* handling of temporary variables for string operations *)
	
	PROCEDURE Overlap (left, right: DevCPT.Node): BOOLEAN;
	BEGIN
		IF right.class = Nconst THEN
			RETURN FALSE
		ELSIF (right.class = Ndop) & (right.subcl = plus) THEN
			RETURN Overlap(left, right.left) OR Overlap(left, right.right)
		ELSE
			WHILE right.class = Nmop DO right := right.left END;
			IF right.class = Nderef THEN right := right.left END;
			IF left.typ.BaseTyp # right.typ.BaseTyp THEN RETURN FALSE END;
			LOOP
				IF left.class = Nvarpar THEN
					WHILE (right.class = Nindex) OR (right.class = Nfield) OR (right.class = Nguard) DO
						right := right.left
					END;
					RETURN (right.class # Nvar) OR (right.obj.mnolev < left.obj.mnolev)
				ELSIF right.class = Nvarpar THEN
					WHILE (left.class = Nindex) OR (left.class = Nfield) OR (left.class = Nguard) DO left := left.left END;
					RETURN (left.class # Nvar) OR (left.obj.mnolev < right.obj.mnolev)
				ELSIF (left.class = Nvar) & (right.class = Nvar) THEN
					RETURN left.obj = right.obj
				ELSIF (left.class = Nderef) & (right.class = Nderef) THEN
					RETURN TRUE
				ELSIF (left.class = Nindex) & (right.class = Nindex) THEN
					IF (left.right.class = Nconst) & (right.right.class = Nconst)
						& (left.right.conval.intval # right.right.conval.intval) THEN RETURN FALSE END;
					left := left.left; right := right.left
				ELSIF (left.class = Nfield) & (right.class = Nfield) THEN
					IF left.obj # right.obj THEN RETURN FALSE END;
					left := left.left; right := right.left;
					WHILE left.class = Nguard DO left := left.left END;
					WHILE right.class = Nguard DO right := right.left END
				ELSE
					RETURN FALSE
				END
			END
		END
	END Overlap;

	PROCEDURE GetStaticLength (n: DevCPT.Node; OUT length: INTEGER);
		VAR x: INTEGER;
	BEGIN
		IF n.class = Nconst THEN
			length := n.conval.intval2 - 1
		ELSIF (n.class = Ndop) & (n.subcl = plus) THEN
			GetStaticLength(n.left, length); GetStaticLength(n.right, x);
			IF (length >= 0) & (x >= 0) THEN length := length + x ELSE length := -1 END
		ELSE
			WHILE (n.class = Nmop) & (n.subcl = conv) DO n := n.left END;
			IF (n.class = Nderef) & (n.subcl = 1) THEN n := n.left END;
			IF n.typ.comp = Array THEN
				length := n.typ.n - 1
			ELSIF n.typ.comp = DynArr THEN
				length := -1
			ELSE	(* error case *)
				length := 4
			END
		END
	END GetStaticLength;

	PROCEDURE GetMaxLength (n: DevCPT.Node; VAR stat, last: DevCPT.Node; OUT length: DevCPT.Node);
		VAR x: DevCPT.Node; d: INTEGER; obj: DevCPT.Object;
	BEGIN
		IF n.class = Nconst THEN
			length := NewIntConst(n.conval.intval2 - 1)
		ELSIF (n.class = Ndop) & (n.subcl = plus) THEN
			GetMaxLength(n.left, stat, last, length); GetMaxLength(n.right, stat, last, x);
			IF (length.class = Nconst) & (x.class = Nconst) THEN ConstOp(plus, length, x)
			ELSE BindNodes(Ndop, length.typ, length, x); length.subcl := plus
			END
		ELSE
			WHILE (n.class = Nmop) & (n.subcl = conv) DO n := n.left END;
			IF (n.class = Nderef) & (n.subcl = 1) THEN n := n.left END;
			IF n.typ.comp = Array THEN
				length := NewIntConst(n.typ.n - 1)
			ELSIF n.typ.comp = DynArr THEN
				d := 0;
				WHILE n.class = Nindex DO n := n.left; INC(d) END;
				ASSERT((n.class = Nderef) OR (n.class = Nvar) OR (n.class = Nvarpar));
				IF (n.class = Nderef) & (n.left.class # Nvar) & (n.left.class # Nvarpar) THEN
					GetTempVar("@tmp", n.left.typ, obj);
					x := NewLeaf(obj); Assign(x, n.left); Link(stat, last, x);
					n.left := NewLeaf(obj);	(* tree is manipulated here *)
					n := NewLeaf(obj); DeRef(n)
				END;
				IF n.typ.untagged & (n.typ.comp = DynArr) & (n.typ.BaseTyp.form IN {Char8, Char16}) THEN
					StrDeref(n);
					BindNodes(Ndop, DevCPT.int32typ, n, NewIntConst(d)); n.subcl := len;
					BindNodes(Ndop, DevCPT.int32typ, n, NewIntConst(1)); n.subcl := plus
				ELSE
					BindNodes(Ndop, DevCPT.int32typ, n, NewIntConst(d)); n.subcl := len;
				END;
				length := n
			ELSE	(* error case *)
				length := NewIntConst(4)
			END
		END
	END GetMaxLength;

	PROCEDURE CheckBuffering* (
		VAR n: DevCPT.Node; left: DevCPT.Node; par: DevCPT.Object; VAR stat, last: DevCPT.Node
	);
		VAR length, x: DevCPT.Node; obj: DevCPT.Object; typ: DevCPT.Struct; len, xlen: INTEGER;
	BEGIN
		IF (n.typ.form IN {String8, String16}) & ~(DevCPM.java IN DevCPM.options)
			& ((n.class = Ndop) & (n.subcl = plus) & ((left = NIL) OR Overlap(left, n.right))
				OR (n.class = Nmop) & (n.subcl = conv) & (left = NIL)
				OR (par # NIL) & (par.vis = inPar) & (par.typ.comp = Array)) THEN
			IF (par # NIL) & (par.typ.comp = Array) THEN
				len := par.typ.n - 1
			ELSE
				IF left # NIL THEN GetStaticLength(left, len) ELSE len := -1 END;
				GetStaticLength(n, xlen);
				IF (len = -1) OR (xlen # -1) & (xlen < len) THEN len := xlen END
			END;
			IF len # -1 THEN
				typ := DevCPT.NewStr(Comp, Array); typ.n := len + 1; typ.BaseTyp := n.typ.BaseTyp;
				GetTempVar("@str", typ, obj);
				x := NewLeaf(obj); Assign(x, n); Link(stat, last, x);
				n := NewLeaf(obj)
			ELSE
				IF left # NIL THEN GetMaxLength(left, stat, last, length)
				ELSE GetMaxLength(n, stat, last, length)
				END;
				typ := DevCPT.NewStr(Pointer, Basic);
				typ.BaseTyp := DevCPT.NewStr(Comp, DynArr); typ.BaseTyp.BaseTyp := n.typ.BaseTyp;
				GetTempVar("@ptr", typ, obj);
				x := NewLeaf(obj); Construct(Nassign, x, length); x.subcl := newfn; Link(stat, last, x);
				x := NewLeaf(obj); DeRef(x); Assign(x, n); Link(stat, last, x);
				n := NewLeaf(obj); DeRef(n)
			END;
			StrDeref(n)
		ELSIF (n.typ.form = Pointer) & (n.typ.sysflag = interface) & (left = NIL)
				& ((par # NIL) OR (n.class = Ncall))
				& ((n.class # Nvar) OR (n.obj.mnolev <= 0)) THEN
			GetTempVar("@cip", DevCPT.punktyp, obj);
			x := NewLeaf(obj); Assign(x, n); Link(stat, last, x);
			n := NewLeaf(obj)
		END
	END CheckBuffering;
	
	PROCEDURE CheckVarParBuffering* (VAR n: DevCPT.Node; VAR stat, last: DevCPT.Node);
		VAR x: DevCPT.Node; obj: DevCPT.Object;
	BEGIN
		IF (n.class # Nvar) OR (n.obj.mnolev <= 0) THEN
			GetTempVar("@ptr", n.typ, obj);
			x := NewLeaf(obj); Assign(x, n); Link(stat, last, x);
			n := NewLeaf(obj)
		END
	END CheckVarParBuffering;

	
	(* case optimization *)

	PROCEDURE Evaluate (n: DevCPT.Node; VAR min, max, num, dist: INTEGER; VAR head: DevCPT.Node);
		VAR a: INTEGER;
	BEGIN
		IF n.left # NIL THEN
			a := MIN(INTEGER); Evaluate(n.left, min, a, num, dist, head);
			IF n.conval.intval - a > dist THEN dist := n.conval.intval - a; head := n END
		ELSIF n.conval.intval < min THEN
			min := n.conval.intval
		END;
		IF n.right # NIL THEN
			a := MAX(INTEGER); Evaluate(n.right, a, max, num, dist, head);
			IF a - n.conval.intval2 > dist THEN dist := a - n.conval.intval2; head := n END
		ELSIF n.conval.intval2 > max THEN
			max := n.conval.intval2
		END;
		INC(num);
		IF n.conval.intval < n.conval.intval2 THEN
			INC(num);
			IF n.conval.intval2 - n.conval.intval > dist THEN dist := n.conval.intval2 - n.conval.intval; head := n END
		END
	END Evaluate;
	
	PROCEDURE Rebuild (VAR root: DevCPT.Node; head: DevCPT.Node);
		VAR n: DevCPT.Node;
	BEGIN
		IF root # head THEN
			IF head.conval.intval2 < root.conval.intval THEN
				Rebuild(root.left, head);
				root.left := head.right; head.right := root; root := head
			ELSE
				Rebuild(root.right, head);
				root.right := head.left; head.left := root; root := head
			END
		END
	END Rebuild;
	
	PROCEDURE OptimizeCase* (VAR n: DevCPT.Node);
		VAR min, max, num, dist, limit: INTEGER; head: DevCPT.Node;
	BEGIN
		IF n # NIL THEN
			min := MAX(INTEGER); max := MIN(INTEGER); num := 0; dist := 0; head := n;
			Evaluate(n, min, max, num, dist, head);
			limit := 6 * num;
			IF limit < 100 THEN limit := 100 END;
			IF (num > 4) & ((min > MAX(INTEGER) - limit) OR (max < min + limit)) THEN
				INCL(n.conval.setval, useTable)
			ELSE
				IF num > 4 THEN Rebuild(n, head) END;
				INCL(n.conval.setval, useTree);
				OptimizeCase(n.left);
				OptimizeCase(n.right)
			END
		END
	END OptimizeCase;
(*	
	PROCEDURE ShowTree (n: DevCPT.Node; opts: SET);
	BEGIN
		IF n # NIL THEN
			IF opts = {} THEN opts := n.conval.setval END;
			IF useTable IN opts THEN
				IF n.left # NIL THEN ShowTree(n.left, opts); DevCPM.LogW(",") END;
				DevCPM.LogWNum(n.conval.intval, 1);
				IF n.conval.intval2 > n.conval.intval THEN DevCPM.LogW("-"); DevCPM.LogWNum(n.conval.intval2, 1)
				END;
				IF n.right # NIL THEN DevCPM.LogW(","); ShowTree(n.right, opts) END
			ELSIF useTree IN opts THEN
				DevCPM.LogW("("); ShowTree(n.left, {}); DevCPM.LogW("|"); DevCPM.LogWNum(n.conval.intval, 1);
				IF n.conval.intval2 > n.conval.intval THEN DevCPM.LogW("-"); DevCPM.LogWNum(n.conval.intval2, 1)
				END;
				DevCPM.LogW("|"); ShowTree(n.right, {}); DevCPM.LogW(")")
			ELSE
				ShowTree(n.left, opts); DevCPM.LogW(" "); DevCPM.LogWNum(n.conval.intval, 1);
				IF n.conval.intval2 > n.conval.intval THEN DevCPM.LogW("-"); DevCPM.LogWNum(n.conval.intval2, 1)
				END;
				DevCPM.LogW(" "); ShowTree(n.right, opts)
			END
		END
	END ShowTree;
*)
BEGIN
	zero := DevCPT.NewConst(); zero.intval := 0; zero.realval := 0;
	one := DevCPT.NewConst(); one.intval := 1; one.realval := 0;
	two := DevCPT.NewConst(); two.intval := 2; two.realval := 0;
	dummy := DevCPT.NewConst();
	quot := DevCPT.NewConst()
END DevCPB.
