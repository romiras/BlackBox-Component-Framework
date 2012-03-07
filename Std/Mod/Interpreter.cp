MODULE StdInterpreter;
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
	
	IMPORT Kernel, Meta, Strings, Views, Dialog;

	TYPE
		IntValue = POINTER TO RECORD (Meta.Value)
			int: INTEGER;
		END;
		StrValue = POINTER TO RECORD (Meta.Value)
			str: Dialog.String;
		END;
		CallHook = POINTER TO RECORD (Dialog.CallHook) END;

		
	PROCEDURE (hook: CallHook) Call (IN proc, errorMsg: ARRAY OF CHAR; VAR res: INTEGER);
		TYPE Ident = ARRAY 32 OF CHAR;
		CONST
			modNotFound = 10; procNotFound = 11; identExpected = 12; unknownIdent = 13;
			depositExpected = 14; noDepositExpected = 15; syntaxError = 16;
			lparenExpected = 17; rparenExpected = 18; containerExpected = 19; quoteExpected = 20;
			fileNotFound = 21; noController = 22; noDialog = 23; cannotUnload = 24; commaExpected = 25;
			incompParList = 26;
		CONST
			 ident = 0; dot = 1; semicolon = 2; eot = 3; lparen = 4; rparen = 5; quote = 6; comma = 7; int = 8;
		VAR
			i, type: INTEGER; ch: CHAR; id: Ident; x: INTEGER;
			par: ARRAY 100 OF POINTER TO Meta.Value; numPar: INTEGER;
			
		PROCEDURE Concat (a, b: ARRAY OF CHAR; VAR c: ARRAY OF CHAR);
			VAR i, j: INTEGER; ch: CHAR;
		BEGIN
			IF a = " " THEN Dialog.MapString("#System:CommandError", c) ELSE c := a$ END;
			i := 0; WHILE c[i] # 0X DO INC(i) END;
			c[i] := " "; INC(i);
			j := 0; ch := b[0]; WHILE ch # 0X DO c[i] := ch; INC(i); INC(j); ch := b[j] END;
			c[i] := 0X
		END Concat;

		PROCEDURE Error (n: INTEGER; msg, par0, par1: ARRAY OF CHAR);
			VAR e, f: ARRAY 256 OF CHAR;
		BEGIN
			IF res = 0 THEN
				res := n;
				IF errorMsg # "" THEN
					Dialog.MapString(errorMsg, e);
					Dialog.MapParamString(msg, par0, par1, "", f);
					Concat(e, f, f);
					Dialog.ShowMsg(f)
				END
			END
		END Error;
		
		PROCEDURE Init (VAR s: ARRAY OF CHAR);
			VAR i: INTEGER;
		BEGIN
			i := 0; WHILE i < LEN(s) DO s[i] := 0X; INC(i) END
		END Init;
		
		PROCEDURE ShowLoaderResult (IN mod: ARRAY OF CHAR);
			VAR res: INTEGER; importing, imported, object: ARRAY 256 OF CHAR;
		BEGIN
			Kernel.GetLoaderResult(res, importing, imported, object);
			CASE res OF
			| Kernel.fileNotFound:
				Error(Kernel.fileNotFound, "#System:CodeFileNotFound", imported, "")
			| Kernel.syntaxError:
				Error(Kernel.syntaxError, "#System:CorruptedCodeFileFor", imported, "")
			| Kernel.objNotFound:
				Error(Kernel.objNotFound, "#System:ObjNotFoundImpFrom", imported, importing)
			| Kernel.illegalFPrint:
				Error(Kernel.illegalFPrint, "#System:ObjInconsImpFrom", imported, importing)
			| Kernel.cyclicImport:
				Error(Kernel.cyclicImport, "#System:CyclicImpFrom", imported, importing)
			| Kernel.noMem:
				Error(Kernel.noMem, "#System:NotEnoughMemoryFor", imported, "")
			ELSE
				Error(res, "#System:CannotLoadModule", mod, "")
			END
		END ShowLoaderResult;

		PROCEDURE CallProc (IN mod, proc: ARRAY OF CHAR);
			VAR i, t: Meta.Item; ok: BOOLEAN;
		BEGIN
			ok := FALSE;
			Meta.Lookup(mod, i);
			IF i.obj = Meta.modObj THEN
				i.Lookup(proc, i);
				IF i.obj = Meta.procObj THEN
					i.GetReturnType(t);
					IF (t.typ = 0) & (i.NumParam() = numPar) THEN
						i.ParamCallVal(par, t, ok)
					ELSE ok := FALSE
					END;
					IF ~ok THEN
						Error(incompParList, "#System:IncompatibleParList", mod, proc)
					END
				ELSE
					Error(Kernel.commNotFound, "#System:CommandNotFoundIn", proc, mod)
				END
			ELSE
				ShowLoaderResult(mod)
			END
		END CallProc;

		PROCEDURE GetCh;
		BEGIN
			IF i < LEN(proc) THEN ch := proc[i]; INC(i) ELSE ch := 0X END
		END GetCh;

		PROCEDURE Scan;
			VAR j: INTEGER; num: ARRAY 32 OF CHAR; r: INTEGER;
		BEGIN
			IF res = 0 THEN
				WHILE (ch # 0X) & (ch <= " ") DO GetCh END;
				IF ch = 0X THEN
					type := eot
				ELSIF ch = "." THEN
					type := dot; GetCh
				ELSIF ch = ";" THEN
					type := semicolon; GetCh
				ELSIF ch = "(" THEN
					type := lparen; GetCh
				ELSIF ch = ")" THEN
					type := rparen; GetCh
				ELSIF ch = "'" THEN
					type := quote; GetCh
				ELSIF ch = "," THEN
					type := comma; GetCh
				ELSIF (ch >= "0") & (ch <= "9") OR (ch = "-") THEN
					type := int; j := 0;
					REPEAT num[j] := ch; INC(j); GetCh UNTIL (ch < "0") OR (ch > "9") & (ch < "A") OR (ch > "H");
					num[j] := 0X; Strings.StringToInt(num, x, r)
				ELSIF (ch >= "a") & (ch <= "z") OR (ch >= "A") & (ch <= "Z") OR
						(ch >= 0C0X) & (ch # "×") & (ch # "÷") & (ch <= 0FFX) OR (ch = "_") THEN
					type := ident;
					id[0] := ch; j := 1; GetCh;
					WHILE (ch # 0X) & (i < LEN(proc)) &
								((ch >= "a") & (ch <= "z") OR (ch >= "A") & (ch <= "Z") OR
								(ch >= 0C0X) & (ch # "×") & (ch # "÷") & (ch <= 0FFX) OR
								(ch = "_") OR (ch >= "0") & (ch <= "9")) DO
						id[j] := ch; INC(j); GetCh
					END;
					id[j] := 0X
				ELSE Error(syntaxError, "#System:SyntaxError", "", "")
				END
			END
		END Scan;
		
		PROCEDURE String (VAR s: ARRAY OF CHAR);
			VAR j: INTEGER;
		BEGIN
			IF type = quote THEN
				j := 0;
				WHILE (ch # 0X) & (ch # "'") & (j < LEN(s) - 1) DO s[j] := ch; INC(j); GetCh END; s[j] := 0X;
				IF ch = "'" THEN
					GetCh; Scan
				ELSE Error(quoteExpected, "#System:QuoteExpected", "", "")
				END
			ELSE Error(quoteExpected, "#System:QuoteExpected", "", "")
			END
		END String;

		PROCEDURE ParamList ();
			VAR iv: IntValue; sv: StrValue;
		BEGIN
			numPar := 0;
			IF type = lparen THEN Scan;
				WHILE (numPar < LEN(par)) & (type # rparen) & (res = 0) DO
					IF type = quote THEN
						NEW(sv);
						String(sv.str);
						par[numPar] := sv;
						INC(numPar)
					ELSIF type = int THEN
						NEW(iv);
						iv.int := x; Scan;
						par[numPar] := iv;
						INC(numPar)
					ELSE Error(syntaxError, "#System:SyntaxError", "", "")
					END;
					IF type = comma THEN Scan
					ELSIF type # rparen THEN Error(rparenExpected, "#System:RParenExpected", "", "")
					END
				END;
				Scan
			END
		END ParamList;

		PROCEDURE Command;
			VAR left, right: Ident;
		BEGIN
			(* protect from parasitic anchors on stack *)
			Init(left); Init(right);
			left := id; Scan;
			IF type = dot THEN	(* Oberon command *)
				Scan;
				IF type = ident THEN
					right := id; Scan; ParamList();
					CallProc(left, right)
				ELSE Error(identExpected, "#System:IdentExpected", "", "")
				END
			ELSE Error(unknownIdent, "#System:UnknownIdent", id, "")
			END
		END Command;

	BEGIN
		(* protect from parasitic anchors on stack *)
		i := 0; type := 0; Init(id); x := 0;
		Views.ClearQueue;
		res := 0; i := 0; GetCh;
		Scan;
		IF type = ident THEN
			Command; WHILE (type = semicolon) & (res = 0) DO Scan; Command END;
			IF type # eot THEN Error(syntaxError, "#System:SyntaxError", "", "") END
		ELSE Error(syntaxError, "#System:SyntaxError", "", "")
		END;
		IF (res = 0) & (Views.Available() > 0) THEN
			Error(noDepositExpected, "#System:NoDepositExpected", "", "")
		END;
		Views.ClearQueue
	END Call;
	
	PROCEDURE Init;
		VAR hook: CallHook;
	BEGIN
		NEW(hook); Dialog.SetCallHook(hook)
	END Init;

BEGIN
	Init
END StdInterpreter.
