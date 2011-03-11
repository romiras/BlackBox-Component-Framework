MODULE Controls;
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
		Kernel, Dates, Dialog, Meta, Services, Stores, Views, Properties, 
		Strings, Fonts, Ports, Controllers, Windows, StdCFrames;

	CONST
		(** elements of Property.valid **)
		opt0* = 0; opt1* = 1; opt2* = 2; opt3* = 3; opt4* = 4;
		link* = 5; label* = 6; guard* = 7; notifier* = 8; level* = 9;

		default* = opt0; cancel* = opt1;
		left* = opt0; right* = opt1; multiLine* = opt2; password* = opt3;
		sorted* = opt0;
		haslines* = opt1; hasbuttons* = opt2; atroot* = opt3; foldericons* = opt4;

		minVersion = 0; maxBaseVersion = 4;
		pbVersion = 0; cbVersion = 0; rbVersion = 0; fldVersion = 0; 
		dfldVersion = 0; tfldVersion = 0; cfldVersion = 0;
		lbxVersion = 0; sbxVersion = 0; cbxVersion = 0; capVersion = 1; grpVersion = 0; 
		tfVersion = 0; 

		rdel = 07X; ldel = 08X;  tab = 09X; ltab = 0AX; lineChar = 0DX; esc = 01BX;
		arrowLeft = 1CX; arrowRight = 1DX; arrowUp = 1EX; arrowDown = 1FX;

		update = 2;	(* notify options *)
		listUpdate = 3;
		guardCheck = 4;
		flushCaches = 5;	(* re-map labels for flushed string resources, after a language change *)
		
		maxAdr = 8;

	TYPE
		Prop* = POINTER TO RECORD (Properties.Property)
			opt*: ARRAY 5 OF BOOLEAN;
			link*: Dialog.String;
			label*: Dialog.String;
			guard*: Dialog.String;
			notifier*: Dialog.String;
			level*: INTEGER
		END;
		
		Directory* = POINTER TO ABSTRACT RECORD END;

		Control* = POINTER TO ABSTRACT RECORD (Views.View)
			item-: Meta.Item;
			disabled-, undef-, readOnly-, customFont-: BOOLEAN;
			font-: Fonts.Font;
			label-: Dialog.String;
			prop-: Prop;
			adr: ARRAY maxAdr OF INTEGER;
			num: INTEGER;
			stamp: INTEGER;
			shortcut: CHAR;
			guardErr, notifyErr: BOOLEAN
		END;

		DefaultsPref* = RECORD (Properties.Preference)
			disabled*: BOOLEAN;	(** OUT, preset to ~c.item.Valid() *)
			undef*: BOOLEAN;	(** OUT, preset to FALSE *)
			readOnly*: BOOLEAN	(** OUT, preset to c.item.vis = readOnly *)
		END;

		PropPref* = RECORD (Properties.Preference)
			valid*: SET	(** OUT, preset to {link, label, guard, notifier, customFont} *)
		END;

		PushButton = POINTER TO RECORD (Control) END;

		CheckBox = POINTER TO RECORD (Control) END;

		RadioButton = POINTER TO RECORD (Control) END;

		Field = POINTER TO RECORD (Control)
			maxLen: INTEGER
		END;

		UpDownField = POINTER TO RECORD (Control)
			min, max, inc: INTEGER
		END;

		DateField = POINTER TO RECORD (Control) 
			selection: INTEGER	(* 0: no selection, 1..n-1: this part selected, -1: part n selected *)
		END;

		TimeField = POINTER TO RECORD (Control) 
			selection: INTEGER
		END;

		ColorField = POINTER TO RECORD (Control) END;

		ListBox = POINTER TO RECORD (Control) END;

		SelectionBox = POINTER TO RECORD (Control) END;

		ComboBox = POINTER TO RECORD (Control) END;

		Caption = POINTER TO RECORD (Control) END;

		Group = POINTER TO RECORD (Control) END;

		TreeControl = POINTER TO RECORD (Control) END;

		StdDirectory = POINTER TO RECORD (Directory) END;

		Op = POINTER TO RECORD (Stores.Operation)
			ctrl: Control;
			prop: Prop
		END;

		FontOp = POINTER TO RECORD (Stores.Operation)
			ctrl: Control;
			font: Fonts.Font;
			custom: BOOLEAN
		END;

		NotifyMsg = RECORD (Views.NotifyMsg)
			frame: Views.Frame;
			op, from, to: INTEGER
		END;

		UpdateCachesMsg = RECORD (Views.UpdateCachesMsg) END;

		SelectPtr = POINTER TO Dialog.Selection;
		
		ProcValue = RECORD (Meta.Value) p*: PROCEDURE END;
		SelectValue = RECORD (Meta.Value) p*: SelectPtr END;
		GuardProcVal = RECORD (Meta.Value) p*: Dialog.GuardProc END;
		NotifyProcValOld = RECORD (Meta.Value) p*: PROCEDURE (op, from, to: INTEGER) END;
		GuardProcPVal = RECORD (Meta.Value) p*: PROCEDURE(n: INTEGER; VAR p: Dialog.Par) END;
		NotifyProcPVal = RECORD (Meta.Value) p*: PROCEDURE(n, op, f, t: INTEGER) END;

		Param = RECORD from, to, i: INTEGER; n: Dialog.String END;
		
		TVParam = RECORD l: INTEGER; e: BOOLEAN; nodeIn, nodeOut: Dialog.TreeNode END; 

		Action = POINTER TO RECORD (Services.Action) 
			w: Windows.Window;
			resolution, cnt: INTEGER
		END;

		TrapCleaner = POINTER TO RECORD (Kernel.TrapCleaner) END;

	VAR
		dir-, stdDir-: Directory;
		par-: Control;
		stamp: INTEGER;
		action: Action;
		cleaner:  TrapCleaner;
		cleanerInstalled: INTEGER;


	(** Cleaner **)

	PROCEDURE (c: TrapCleaner) Cleanup;
	BEGIN
		par := NIL;
		cleanerInstalled := 0
	END Cleanup;


	PROCEDURE (c: Control) Update- (f: Views.Frame; op, from, to: INTEGER), NEW, EMPTY;
	PROCEDURE (c: Control) UpdateList- (f: Views.Frame), NEW, EMPTY;
	PROCEDURE (c: Control) CheckLink- (VAR ok: BOOLEAN), NEW, EMPTY;
	PROCEDURE (c: Control) HandlePropMsg2- (VAR p: Views.PropMessage), NEW, EMPTY;
	PROCEDURE (c: Control) HandleViewMsg2- (f: Views.Frame; VAR msg: Views.Message), NEW, EMPTY;
	PROCEDURE (c: Control) HandleCtrlMsg2- (f: Views.Frame; VAR msg: Views.CtrlMessage;
																	VAR focus: Views.View), NEW, EMPTY;
	PROCEDURE (c: Control) Externalize2- (VAR wr: Stores.Writer), NEW, EMPTY;
	PROCEDURE (c: Control) Internalize2- (VAR rd: Stores.Reader), NEW, EMPTY;


	(* auxiliary procedures *)

	PROCEDURE IsShortcut (ch: CHAR; c: Control): BOOLEAN;
	BEGIN
		IF (ch >= "a") & (ch <= "z") OR (ch >= 0E0X) THEN ch := CAP(ch) END;
		RETURN ch = c.shortcut
	END IsShortcut;

	PROCEDURE ExtractShortcut (c: Control);
		VAR label: Dialog.String; i: INTEGER; ch, sCh: CHAR;
	BEGIN
		Dialog.MapString(c.label, label);
		i := 0; ch := label[0]; sCh := "&";
		WHILE sCh = "&" DO
			WHILE (ch # 0X) & (ch # "&") DO INC(i); ch := label[i] END;
			IF ch = 0X THEN sCh := 0X
			ELSE INC(i); sCh := label[i]; INC(i); ch := label[i]
			END
		END;
		IF (sCh >= "a") & (sCh <= "z") OR (sCh >= 0E0X) THEN sCh := CAP(sCh) END;
		c.shortcut := sCh
	END ExtractShortcut;

	PROCEDURE GetGuardProc (name: ARRAY OF CHAR; VAR i: Meta.Item; VAR err: BOOLEAN;
												VAR par: BOOLEAN; VAR n: INTEGER);
		VAR j, k, e: INTEGER; num: ARRAY 32 OF CHAR;
	BEGIN
		j := 0;
		WHILE (name[j] # 0X) & (name[j] # "(") DO INC(j) END;
		IF name[j] = "(" THEN
			INC(j); k := 0;
			WHILE (name[j] # 0X) & (name[j] # ")") DO num[k] := name[j]; INC(j); INC(k) END;
			IF (name[j] = ")") & (name[j+1] = 0X) THEN
				num[k] := 0X; Strings.StringToInt(num, n, e);
				IF e = 0 THEN
					name[j - k - 1] := 0X;
					Meta.LookupPath(name, i); par := TRUE
				ELSE
					IF ~err THEN
						Dialog.ShowParamMsg("#System:SyntaxErrorIn", name, "", "");
						err := TRUE
					END;
					Meta.Lookup("", i);
					RETURN
				END
			ELSE
				IF ~err THEN
					Dialog.ShowParamMsg("#System:SyntaxErrorIn", name, "", "");
					err := TRUE
				END;
				Meta.Lookup("", i);
				RETURN
			END
		ELSE
			Meta.LookupPath(name, i); par := FALSE
		END;
		IF (i.obj = Meta.procObj) OR (i.obj = Meta.varObj) & (i.typ = Meta.procTyp) THEN (*ok *)
		ELSE
			IF ~err THEN
				IF i.obj = Meta.undef THEN
					Dialog.ShowParamMsg("#System:NotFound", name, "", "")
				ELSE
					Dialog.ShowParamMsg("#System:HasWrongType", name, "", "")
				END;
				err := TRUE
			END;
			Meta.Lookup("", i)
		END
	END GetGuardProc;
	
	PROCEDURE CallGuard (c: Control);
		VAR ok, up: BOOLEAN; n: INTEGER; dpar: Dialog.Par; p: Control;
			v: GuardProcVal; vp: GuardProcPVal; i: Meta.Item; pref: DefaultsPref;
	BEGIN
		Controllers.SetCurrentPath(Controllers.targetPath);
		pref.disabled := ~c.item.Valid();
		pref.undef := FALSE;
		pref.readOnly := c.item.vis = Meta.readOnly;
		Views.HandlePropMsg(c, pref);
		c.disabled := pref.disabled;
		c.undef := pref.undef;
		c.readOnly := pref.readOnly;
		c.label := c.prop.label$;
		IF ~c.disabled & (c.prop.guard # "") & ~c.guardErr THEN
			IF cleanerInstalled = 0 THEN Kernel.PushTrapCleaner(cleaner) END;
			INC(cleanerInstalled);
			p := par; par := c;
			dpar.disabled := FALSE; dpar.undef := FALSE;
			dpar.readOnly := c.readOnly;
			dpar.checked := FALSE; dpar.label := c.label$;
			GetGuardProc(c.prop.guard, i, c.guardErr, up, n);
			IF i.obj # Meta.undef THEN
				IF up THEN	(* call with numeric parameter *)
					i.GetVal(vp, ok);
					IF ok THEN vp.p(n, dpar) END
				ELSE
					i.GetVal(v, ok);
					IF ok THEN v.p(dpar) END
				END;
				IF ok THEN
					c.disabled := dpar.disabled;
					c.undef := dpar.undef;
					IF dpar.readOnly THEN c.readOnly := TRUE END;
					IF dpar.label # c.label THEN c.label := dpar.label END
				ELSIF ~c.guardErr THEN
					Dialog.ShowParamMsg("#System:HasWrongType", c.prop.guard, "", "");
					c.guardErr := TRUE
				END
			END;
			par := p;
			DEC(cleanerInstalled);
			IF cleanerInstalled = 0 THEN Kernel.PopTrapCleaner(cleaner) END
		END;
		ExtractShortcut(c);
		Controllers.ResetCurrentPath()
	END CallGuard;

	PROCEDURE CallNotifier (c: Control; op, from, to: INTEGER);
		VAR ok, up: BOOLEAN; n: INTEGER; vold: NotifyProcValOld; vp: NotifyProcPVal;
			i: Meta.Item; p: Control;
	BEGIN
		IF c.prop.notifier # "" THEN
			IF cleanerInstalled = 0 THEN Kernel.PushTrapCleaner(cleaner) END;
			INC(cleanerInstalled);
			p := par; par := c;
			IF c.prop.notifier[0] = "!" THEN
				IF op = Dialog.pressed THEN
					c.prop.notifier[0] := " ";
					Dialog.ShowStatus(c.prop.notifier);
					c.prop.notifier[0] := "!"
				ELSIF op = Dialog.released THEN
					Dialog.ShowStatus("")
				END
			ELSE
				GetGuardProc(c.prop.notifier, i, c.notifyErr, up, n);
				IF i.obj # Meta.undef THEN
					IF up THEN	(* call with numeric parameter *)
						i.GetVal(vp, ok);
						IF ok THEN vp.p(n, op, from, to) END
					ELSE
						i.GetVal(vold, ok);
						IF ok THEN vold.p(op, from, to) END
					END;
					IF ~ok & ~c.notifyErr THEN
						Dialog.ShowParamMsg("#System:HasWrongType", c.prop.notifier, "", "");
						c.notifyErr := TRUE
					END
				END
			END;
			par := p;
			DEC(cleanerInstalled);
			IF cleanerInstalled = 0 THEN Kernel.PopTrapCleaner(cleaner) END
		END
	END CallNotifier;

	PROCEDURE DCHint (modifiers: SET): INTEGER;
	BEGIN
		IF Controllers.doubleClick IN modifiers THEN RETURN 1
		ELSE RETURN 0
		END
	END DCHint;

	PROCEDURE Notify* (c: Control; f: Views.Frame; op, from, to: INTEGER);
		VAR msg: NotifyMsg;
	BEGIN
		IF ~c.readOnly & ~ c.disabled THEN
			CallNotifier(c, op, from, to);
			IF op >= Dialog.changed THEN
				msg.id0 := c.item.adr; msg.id1 := msg.id0 + c.item.Size(); msg.frame := f;
				msg.op := op; msg.from := from; msg.to := to;
				msg.opts := {update, guardCheck};
				Views.Omnicast(msg)
			END
		END
	END Notify;

	PROCEDURE NotifyFlushCaches*;
		VAR msg: NotifyMsg;
	BEGIN
		msg.opts := {flushCaches}; msg.id0 := 0; msg.id1 := 0;
		Views.Omnicast(msg)
	END NotifyFlushCaches;
	
	PROCEDURE GetName (VAR path, name: ARRAY OF CHAR; VAR i: INTEGER);
		VAR j: INTEGER; ch: CHAR;
	BEGIN
		j := 0; ch := path[i];
		WHILE (j < LEN(name) - 1) & ((ch >= "0") & (ch <= "9") OR (CAP(ch) >= "A") & (CAP(ch) <= "Z")
												OR (ch >= 0C0X) & (ch # "�") & (ch # "�") & (ch <= 0FFX) OR (ch = "_")) DO
			name[j] := ch; INC(i); INC(j); ch := path[i]
		END;
		IF (ch = 0X) OR (ch = ".") OR (ch = "[") OR (ch = "^") THEN name[j] := 0X
		ELSE name[0] := 0X
		END
	END GetName;

	PROCEDURE LookupPath (path: ARRAY OF CHAR; VAR i: Meta.Item;
												VAR adr: ARRAY OF INTEGER; VAR num: INTEGER);
		VAR j, n: INTEGER; name: Meta.Name; ch: CHAR;
	BEGIN
		path[LEN(path) - 1] := 0X; j := 0; num := 0;
		GetName(path, name, j); Meta.Lookup(name, i);
		IF (i.obj = Meta.modObj) & (path[j] = ".") THEN
			INC(j); GetName(path, name, j);
			i.Lookup(name, i); ch := path[j]; INC(j);
			WHILE i.obj = Meta.varObj DO
				adr[num] := i.adr;
				IF num < LEN(adr) - 1 THEN INC(num) END;
				IF ch = 0X THEN RETURN 
				ELSIF i.typ = Meta.ptrTyp THEN
					IF ch = "^" THEN ch := path[j]; INC(j) END;
					i.Deref(i)
				ELSIF (i.typ = Meta.recTyp) & (ch = ".") THEN
					GetName(path, name, j); i.Lookup(name, i);
					ch := path[j]; INC(j)
				ELSIF (i.typ = Meta.arrTyp) & (ch = "[") THEN
					ch := path[j]; INC(j); n := 0;
					WHILE (ch >= "0") & (ch <= "9") DO n := 10 * n + ORD(ch) - ORD("0"); ch := path[j]; INC(j) END;
					IF ch = "]" THEN ch := path[j]; INC(j); i.Index(n, i) ELSE Meta.Lookup("", i) END
				ELSE Meta.Lookup("", i)
				END
			END
		ELSE
			Meta.LookupPath(path, i); num := 0;
			IF i.obj = Meta.varObj THEN adr[0] := i.adr; num := 1
			ELSIF i.obj # Meta.procObj THEN Meta.Lookup("", i)
			END
		END
	END LookupPath;

	PROCEDURE Sort (VAR adr: ARRAY OF INTEGER; num: INTEGER);
		VAR i, j, p: INTEGER;
	BEGIN
		i := 1;
		WHILE i < num DO
			p := adr[i]; j := i;
			WHILE (j >= 1) & (adr[j - 1] > p) DO adr[j] := adr[j - 1]; DEC(j) END;
			adr[j] := p; INC(i)
		END
	END Sort;

	PROCEDURE GetTypeName (IN item: Meta.Item; OUT name: Meta.Name);
		VAR mod: Meta.Name;
	BEGIN
		IF (item.typ = Meta.recTyp) THEN
			item.GetTypeName(mod, name);
			IF (mod = "Dialog") OR (mod = "Dates") THEN (* ok *)
			ELSE name := ""
			END
		ELSE name := ""
		END
	END GetTypeName;

	PROCEDURE OpenLink* (c: Control; p: Prop);
		VAR ok: BOOLEAN;
	BEGIN
		ASSERT(c # NIL, 20); ASSERT(p # NIL, 21);
		c.num := 0;
		c.prop := Properties.CopyOf(p)(Prop);
		IF c.font = NIL THEN
			IF c.customFont THEN c.font := StdCFrames.defaultLightFont
			ELSE c.font := StdCFrames.defaultFont
			END
		END;
		c.guardErr := FALSE; c.notifyErr := FALSE;
		LookupPath(p.link, c.item, c.adr, c.num);
		IF c.item.obj = Meta.varObj THEN
			Sort(c.adr, c.num);
			ok := TRUE; c.CheckLink(ok);
			IF ~ok THEN
				Meta.Lookup("", c.item);
				Dialog.ShowParamMsg("#System:HasWrongType", p.link, "", "")
			END
		ELSE
			Meta.Lookup("", c.item); c.num := 0
		END;
		CallGuard(c);
		c.stamp := stamp
	END OpenLink;


	(** Prop **)

	PROCEDURE (p: Prop) IntersectWith* (q: Properties.Property; OUT equal: BOOLEAN);
		VAR valid: SET;
	BEGIN
		WITH q: Prop DO
			valid := p.valid * q.valid; equal := TRUE;
			IF p.link # q.link THEN EXCL(valid, link) END;
			IF p.label # q.label THEN EXCL(valid, label) END;
			IF p.guard # q.guard THEN EXCL(valid, guard) END;
			IF p.notifier # q.notifier THEN EXCL(valid, notifier) END;
			IF p.level # q.level THEN EXCL(valid, level) END;
			IF p.opt[0] # q.opt[0] THEN EXCL(valid, opt0) END;
			IF p.opt[1] # q.opt[1] THEN EXCL(valid, opt1) END;
			IF p.opt[2] # q.opt[2] THEN EXCL(valid, opt2) END;
			IF p.opt[3] # q.opt[3] THEN EXCL(valid, opt3) END;
			IF p.opt[4] # q.opt[4] THEN EXCL(valid, opt4) END;
			IF p.valid # valid THEN p.valid := valid; equal := FALSE END
		END
	END IntersectWith;


	(* Control *)

	PROCEDURE (c: Control) CopyFromSimpleView2- (source: Control), NEW, EMPTY;

	PROCEDURE (c: Control) CopyFromSimpleView- (source: Views.View);
	BEGIN
		WITH source: Control DO
			c.item := source.item;
			c.adr := source.adr;
			c.num := source.num;
			c.disabled := source.disabled;
			c.undef := source.undef;
			c.readOnly := source.readOnly;
			c.shortcut := source.shortcut;
			c.customFont := source.customFont;
			c.font := source.font;
			c.label := source.label$;
			c.prop := Properties.CopyOf(source.prop)(Prop);
			c.CopyFromSimpleView2(source)
		END
	END CopyFromSimpleView;

	PROCEDURE (c: Control) Internalize- (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER; x, def, canc, sort: BOOLEAN;
	BEGIN
		c.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxBaseVersion, thisVersion);
		IF rd.cancelled THEN RETURN END;
		NEW(c.prop);
		IF thisVersion >= 3 THEN
			rd.ReadString(c.prop.link);
			rd.ReadString(c.prop.label);
			rd.ReadString(c.prop.guard);
			rd.ReadString(c.prop.notifier);
			rd.ReadInt(c.prop.level);
			rd.ReadBool(c.customFont);
			rd.ReadBool(c.prop.opt[0]);
			rd.ReadBool(c.prop.opt[1]);
			rd.ReadBool(c.prop.opt[2]);
			rd.ReadBool(c.prop.opt[3]);
			rd.ReadBool(c.prop.opt[4]);
			IF c.customFont & (thisVersion = 4) THEN
				Views.ReadFont(rd, c.font)
			END
		ELSE
			rd.ReadXString(c.prop.link);
			rd.ReadXString(c.prop.label);
			rd.ReadXString(c.prop.guard);
			c.prop.notifier := "";
			c.prop.opt[2] := FALSE;
			c.prop.opt[3] := FALSE;
			c.prop.opt[4] := FALSE;
			sort := FALSE;
			IF thisVersion = 2 THEN
				rd.ReadXString(c.prop.notifier);
				rd.ReadBool(sort);
				rd.ReadBool(c.prop.opt[multiLine])
			ELSIF thisVersion = 1 THEN
				rd.ReadXString(c.prop.notifier);
				rd.ReadBool(sort)
			END;
			rd.ReadBool(x);	(* free, was sed for prop.element *)
			rd.ReadBool(def); 
			rd.ReadBool(canc);
			rd.ReadXInt(c.prop.level);
			rd.ReadBool(c.customFont);
			c.prop.opt[default] := def OR sort OR (c IS Field);
			c.prop.opt[cancel] := canc
		END;
		c.Internalize2(rd);
		OpenLink(c, c.prop)
	END Internalize;

	PROCEDURE (c: Control) Externalize- (VAR wr: Stores.Writer);
	BEGIN
		c.Externalize^(wr);
		wr.WriteVersion(maxBaseVersion);
		wr.WriteString(c.prop.link);
		wr.WriteString(c.prop.label);
		wr.WriteString(c.prop.guard);
		wr.WriteString(c.prop.notifier);
		wr.WriteInt(c.prop.level);
		wr.WriteBool(c.customFont);
		wr.WriteBool(c.prop.opt[0]);
		wr.WriteBool(c.prop.opt[1]);
		wr.WriteBool(c.prop.opt[2]);
		wr.WriteBool(c.prop.opt[3]);
		wr.WriteBool(c.prop.opt[4]);
		IF c.customFont THEN Views.WriteFont(wr, c.font) END;
		c.Externalize2(wr)
	END Externalize;

	PROCEDURE (c: Control) HandleViewMsg- (f: Views.Frame; VAR msg: Views.Message);
		VAR disabled, undef, readOnly, done, allDone: BOOLEAN; i: INTEGER; lbl: Dialog.String;
	BEGIN
		WITH msg: Views.NotifyMsg DO
			done := FALSE; allDone := FALSE;
			IF guardCheck IN msg.opts THEN
				(* should call c.Update for each frame but Views.Update only once *)
				WITH f: StdCFrames.Caption DO lbl := f.label$
				| f: StdCFrames.PushButton DO lbl := f.label$
				| f: StdCFrames.RadioButton DO lbl := f.label$
				| f: StdCFrames.CheckBox DO lbl := f.label$
				| f: StdCFrames.Group DO lbl := f.label$
				ELSE lbl := c.label$
				END;
				WITH f: StdCFrames.Frame DO
					disabled := f.disabled; undef := f.undef; readOnly := f.readOnly
				ELSE
					disabled := c.disabled; undef := c.undef; readOnly := c.readOnly
				END;
				CallGuard(c);
				IF (c.disabled # disabled) OR (c.undef # undef)
				OR (c.readOnly # readOnly) OR (c.label # lbl) THEN
					WITH f: StdCFrames.Frame DO
						IF f.noRedraw THEN
							f.disabled := c.disabled;
							f.undef := c.undef;
							f.readOnly := c.readOnly;
							c.Update(f, 0, 0, 0); done := TRUE
						ELSE Views.Update(c, Views.rebuildFrames); allDone := TRUE
						END
					ELSE Views.Update(c, Views.keepFrames); done := TRUE
					END
				END
			END;
			IF flushCaches IN msg.opts THEN
				Views.Update(c, Views.rebuildFrames)
			END;
			i := 0; WHILE (i < c.num) & (c.adr[i] < msg.id0) DO INC(i) END;
			IF (i < c.num) & (c.adr[i] < msg.id1) & ~allDone THEN
				IF (update IN msg.opts) & ~done THEN
					WITH msg: NotifyMsg DO
						IF msg.frame # f THEN	(* don't update origin frame *)
							c.Update(f, msg.op, msg.from, msg.to)
						END
					ELSE
						c.Update(f, 0, 0, 0)
					END
				END;
				IF listUpdate IN msg.opts THEN
					c.UpdateList(f)
				END
			END
		| msg: Views.UpdateCachesMsg DO
			IF c.stamp # stamp THEN
				OpenLink(c, c.prop);
				IF msg IS UpdateCachesMsg THEN
					Views.Update(c, Views.rebuildFrames)
				END
			END
		ELSE
		END;
		c.HandleViewMsg2(f, msg)
	END HandleViewMsg;

	PROCEDURE (c: Control) HandleCtrlMsg* (f: Views.Frame; VAR msg: Controllers.Message;
																							VAR focus: Views.View);
		VAR sp: Properties.SizeProp; p: Control; dcOk: BOOLEAN;
	BEGIN
		IF cleanerInstalled = 0 THEN Kernel.PushTrapCleaner(cleaner) END;
		INC(cleanerInstalled);
		p := par; par := c;
		WITH msg: Properties.PollPickMsg DO
			msg.dest := f
		| msg: Properties.PickMsg DO
			NEW(sp); sp.known := {Properties.width, Properties.height}; sp.valid := sp.known;
			c.context.GetSize(sp.width, sp.height);
			Properties.Insert(msg.prop, sp)
		| msg: Controllers.TrackMsg DO
			IF ~c.disabled THEN
				dcOk := TRUE;
				IF f IS StdCFrames.Frame THEN dcOk := f(StdCFrames.Frame).DblClickOk(msg.x, msg.y) END;
				IF (DCHint(msg.modifiers) = 1)  & dcOk THEN
					(* double click *)
					Notify(c, f, Dialog.pressed, 1, 0)
				ELSE
					Notify(c, f, Dialog.pressed, 0, 0)
				END
			END
		ELSE
		END;
		c.HandleCtrlMsg2(f, msg, focus);
		WITH msg: Controllers.TrackMsg DO
			IF ~c.disabled THEN
				Notify(c, f, Dialog.released, 0, 0)
			END
		ELSE
		END;
		par := p;
		DEC(cleanerInstalled);
		IF cleanerInstalled = 0 THEN Kernel.PopTrapCleaner(cleaner) END
	END HandleCtrlMsg;

	PROCEDURE (c: Control) HandlePropMsg- (VAR msg: Properties.Message);
		VAR fpref: Properties.FocusPref; stp: Properties.StdProp;
			cp: Prop; ppref: PropPref; op: Op; valid: SET; p: Properties.Property;
			fop: FontOp; face: Fonts.Typeface; size, weight: INTEGER; style: SET;
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF (msg.char = lineChar) OR (msg.char = esc) THEN msg.accepts := FALSE END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) THEN
				fpref.hotFocus := FALSE; fpref.setFocus := FALSE; fpref.atLocation := FALSE;
				Views.HandlePropMsg(c, fpref);
				IF fpref.setFocus THEN msg.getFocus := TRUE END
			END
		| msg: Properties.PollMsg DO
			ppref.valid := {link, label, notifier, guard};
			Views.HandlePropMsg(c, ppref);
			cp := Properties.CopyOf(c.prop)(Prop);
			cp.valid := ppref.valid; cp.known := cp.valid; cp.readOnly := {};
			Properties.Insert(msg.prop, cp);
			NEW(stp);
			stp.valid := {Properties.typeface..Properties.weight};
			stp.known := stp.valid;
			IF c.customFont THEN stp.typeface := c.font.typeface$
			ELSE stp.typeface := Fonts.default
			END;
			stp.size := c.font.size; stp.style.val := c.font.style; stp.weight := c.font.weight;
			stp.style.mask := {Fonts.italic, Fonts.strikeout, Fonts.underline};
			Properties.Insert(msg.prop, stp)
		| msg: Properties.SetMsg DO
			p := msg.prop; op := NIL; fop := NIL;
			WHILE (p # NIL) & (op = NIL) DO
				WITH p: Prop DO
					ppref.valid := {link, label, notifier, guard};
					Views.HandlePropMsg(c, ppref);
					valid := p.valid * ppref.valid;
					IF valid # {} THEN
						NEW(op); 
						op.ctrl := c; 
						op.prop := Properties.CopyOf(p)(Prop); op.prop.valid := valid
					END
				| p: Properties.StdProp DO
					valid := p.valid * {Properties.typeface..Properties.weight};
					IF valid # {} THEN
						NEW(fop); fop.ctrl := c;
						face := c.font.typeface$; size := c.font.size; style := c.font.style; weight := c.font.weight;
						IF Properties.typeface IN p.valid THEN face := p.typeface$;
							IF face = Fonts.default THEN face := StdCFrames.defaultFont.typeface END
						END;
						IF Properties.size IN p.valid THEN size := p.size END;
						IF Properties.style IN p.valid THEN
							style := (p.style.val * p.style.mask) + (style - p.style.mask)
						END;
						IF Properties.weight IN p.valid THEN weight := p.weight END;
						fop.custom := TRUE;
						fop.font := Fonts.dir.This(face, size, style, weight);
						IF (fop.font.typeface = StdCFrames.defaultFont.typeface)
						& (fop.font.size = StdCFrames.defaultFont.size)
						& (fop.font.style = StdCFrames.defaultFont.style)
						& (fop.font.weight = StdCFrames.defaultFont.weight) THEN
							fop.custom := FALSE;
							fop.font := StdCFrames.defaultFont
						END
					END
				ELSE
				END;
				p := p.next
			END;
			IF op # NIL THEN Views.Do(c, "#System:SetProp", op) END;
			IF fop # NIL THEN Views.Do(c, "#System:SetProp", fop) END
		| msg: Properties.TypePref DO
			IF Services.Is(c, msg.type) THEN msg.view := c END
		ELSE
		END;
		c.HandlePropMsg2(msg)
	END HandlePropMsg;


	(* Op *)

	PROCEDURE (op: Op) Do;
		VAR c: Control; prop: Prop;
	BEGIN
		c := op.ctrl;
		prop := Properties.CopyOf(c.prop)(Prop);
		prop.valid := op.prop.valid;	(* fields to be restored *)
		IF link IN op.prop.valid THEN c.prop.link := op.prop.link END;
		IF label IN op.prop.valid THEN c.prop.label := op.prop.label END;
		IF guard IN op.prop.valid THEN c.prop.guard := op.prop.guard END;
		IF notifier IN op.prop.valid THEN c.prop.notifier := op.prop.notifier END;
		IF level IN op.prop.valid THEN c.prop.level := op.prop.level END;
		IF opt0 IN op.prop.valid THEN c.prop.opt[0] := op.prop.opt[0] END;
		IF opt1 IN op.prop.valid THEN c.prop.opt[1] := op.prop.opt[1] END;
		IF opt2 IN op.prop.valid THEN c.prop.opt[2] := op.prop.opt[2] END;
		IF opt3 IN op.prop.valid THEN c.prop.opt[3] := op.prop.opt[3] END;
		IF opt4 IN op.prop.valid THEN c.prop.opt[4] := op.prop.opt[4] END;
		IF c.prop.guard # prop.guard THEN c.guardErr := FALSE END;
		IF c.prop.notifier # prop.notifier THEN c.notifyErr := FALSE END;
		IF c.prop.link # prop.link THEN OpenLink(c, c.prop) ELSE CallGuard(c) END;
		op.prop := prop;
		Views.Update(c, Views.rebuildFrames)
	END Do;

	PROCEDURE (op: FontOp) Do;
		VAR c: Control; custom: BOOLEAN; font: Fonts.Font;
	BEGIN
		c := op.ctrl;
		custom := c.customFont; c.customFont := op.custom; op.custom := custom;
		font := c.font; c.font := op.font; op.font := font;
		Views.Update(c, Views.rebuildFrames)
	END Do;


	(* ------------------------- standard controls ------------------------- *)

	PROCEDURE CatchCtrlMsg (c: Control; f: Views.Frame; VAR msg: Controllers.Message;
																				VAR focus: Views.View);
	BEGIN
		IF ~c.disabled THEN
			WITH f: StdCFrames.Frame DO
				WITH msg: Controllers.PollCursorMsg DO
					f.GetCursor(msg.x, msg.y, msg.modifiers, msg.cursor)
				| msg: Controllers.PollOpsMsg DO
					msg.valid := {Controllers.pasteChar}
				| msg: Controllers.TrackMsg DO
					f.MouseDown(msg.x, msg.y, msg.modifiers)
				| msg: Controllers.MarkMsg DO
					f.Mark(msg.show, msg.focus)
				|msg: Controllers.WheelMsg DO
					f.WheelMove(msg.x, msg.y, msg.op, msg.nofLines, msg.done)
				ELSE
				END
			END
		END
	END CatchCtrlMsg;
	

	(** Directory **)

	PROCEDURE (d: Directory) NewPushButton* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewCheckBox* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewRadioButton* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewField* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewUpDownField* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewDateField* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewTimeField* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewColorField* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewListBox* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewSelectionBox* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewComboBox* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewCaption* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewGroup* (p: Prop): Control, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewTreeControl* (p: Prop): Control, NEW, ABSTRACT;


	(* PushButton *)

	PROCEDURE Call (c: PushButton);
		VAR res: INTEGER; p: Control; ok: BOOLEAN; msg: Views.NotifyMsg;
	BEGIN
		IF c.item.Valid() & ((c.item.obj = Meta.procObj) OR (c.item.obj = Meta.varObj) & (c.item.typ = Meta.procTyp)) THEN
			IF cleanerInstalled = 0 THEN Kernel.PushTrapCleaner(cleaner) END;
			INC(cleanerInstalled);
			p := par; c.item.Call(ok); par := p;
			DEC(cleanerInstalled);
			IF cleanerInstalled = 0 THEN Kernel.PopTrapCleaner(cleaner) END;
			IF ~ok THEN Dialog.ShowMsg("#System:BehaviorNotAccessible") END
		ELSIF c.prop.link # "" THEN
			IF cleanerInstalled = 0 THEN Kernel.PushTrapCleaner(cleaner) END;
			INC(cleanerInstalled);
			p := par; par := c; Dialog.Call(c.prop.link, " ", res); par := p;
			DEC(cleanerInstalled);
			IF cleanerInstalled = 0 THEN Kernel.PopTrapCleaner(cleaner) END
		ELSE Dialog.ShowMsg("#System:NoBehaviorBound")
		END;
		msg.opts := {guardCheck};
		Views.Omnicast(msg)
	END Call;
	
	PROCEDURE Do (f: StdCFrames.PushButton);
	BEGIN
		Call(f.view(PushButton))
	END Do;

	PROCEDURE (c: PushButton) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, pbVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: PushButton) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(pbVersion)
	END Externalize2;

	PROCEDURE (c: PushButton) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.PushButton;
	BEGIN
		f := StdCFrames.dir.NewPushButton();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.label := c.label$;
		f.default := c.prop.opt[default];
		f.cancel := c.prop.opt[cancel];
		f.Do := Do;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: PushButton) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: PushButton) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																				VAR focus: Views.View);
	BEGIN
		IF ~c.disabled THEN
			WITH f: StdCFrames.Frame DO
				WITH msg: Controllers.EditMsg DO
					IF (msg.op = Controllers.pasteChar)
						& ((msg.char = lineChar)
							OR (msg.char = " ")
							OR (msg.char = esc) & c.prop.opt[cancel]
							OR IsShortcut(msg.char, c)) THEN f.KeyDown(msg.char) END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: PushButton) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			msg.accepts := ~c.disabled & ((msg.char = lineChar) & c.prop.opt[default]
				OR (msg.char = esc) & c.prop.opt[cancel]
				OR IsShortcut(msg.char, c))
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~ c.readOnly THEN
				msg.hotFocus := TRUE; msg.setFocus := StdCFrames.setFocus
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetPushButtonSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier, default, cancel}
		| msg: DefaultsPref DO
			IF c.prop.link # "" THEN msg.disabled := FALSE END
		ELSE
		END
	END HandlePropMsg2;
	
	PROCEDURE (c: PushButton) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.PushButton).label := c.label$;
		f(StdCFrames.Frame).Update
	END Update;
	
	PROCEDURE (c: PushButton) CheckLink (VAR ok: BOOLEAN);
	BEGIN
		ok := c.item.typ = Meta.procTyp
	END CheckLink;


	(* CheckBox *)

	PROCEDURE GetCheckBox (f: StdCFrames.CheckBox; OUT x: BOOLEAN);
		VAR c: CheckBox;
	BEGIN
		x := FALSE;
		c := f.view(CheckBox);
		IF c.item.Valid() THEN
			IF c.item.typ = Meta.boolTyp THEN x := c.item.BoolVal()
			ELSIF c.item.typ = Meta.setTyp THEN x := c.prop.level IN c.item.SetVal()
			END
		END
	END GetCheckBox;

	PROCEDURE SetCheckBox (f: StdCFrames.CheckBox; x: BOOLEAN);
		VAR c: CheckBox; s: SET;
	BEGIN
		c := f.view(CheckBox);
		IF c.item.Valid() & ~c.readOnly THEN
			IF c.item.typ = Meta.boolTyp THEN
				c.item.PutBoolVal(x); Notify(c, f, Dialog.changed, 0, 0)
			ELSIF c.item.typ = Meta.setTyp THEN
				s := c.item.SetVal();
				IF x THEN INCL(s, c.prop.level) ELSE EXCL(s, c.prop.level) END;
				c.item.PutSetVal(s);
				IF x THEN Notify(c, f, Dialog.included, c.prop.level, c.prop.level)
				ELSE Notify(c, f, Dialog.excluded, c.prop.level, c.prop.level)
				END
			END
		END
	END SetCheckBox;

	PROCEDURE (c: CheckBox) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, cbVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: CheckBox) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(cbVersion)
	END Externalize2;

	PROCEDURE (c: CheckBox) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.CheckBox;
	BEGIN
		f :=  StdCFrames.dir.NewCheckBox();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.label := c.label$;
		f.Get := GetCheckBox;
		f.Set := SetCheckBox;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: CheckBox) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: CheckBox) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
	BEGIN
		IF ~c.disabled & ~c.readOnly THEN
			WITH f: StdCFrames.Frame DO
				WITH msg: Controllers.EditMsg DO
					IF (msg.op = Controllers.pasteChar)
						& ((msg.char = " ") OR IsShortcut(msg.char, c)) THEN f.KeyDown(msg.char) END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: CheckBox) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF ~c.disabled & ~c.readOnly THEN
				IF (msg.char = tab) OR (msg.char = ltab) THEN
					(* tabs set focus to first checkbox only *)
					IF (msg.focus # NIL) & (msg.focus IS CheckBox)
							& (msg.focus(CheckBox).item.adr = c.item.adr) THEN
						msg.getFocus := FALSE
					END
				ELSIF (msg.char >= arrowLeft) & (msg.char <= arrowDown) THEN
					(* arrows set focus to next checkbox bound to same variable *)
					msg.getFocus := StdCFrames.setFocus
						& (msg.focus # NIL)
						& (msg.focus IS CheckBox)
						& (msg.focus(CheckBox).item.adr = c.item.adr);
					msg.accepts := msg.getFocus & (msg.focus # c)
				ELSIF IsShortcut(msg.char, c) THEN
					msg.accepts := TRUE; msg.getFocus := StdCFrames.setFocus
				ELSIF msg.char # " " THEN
					msg.accepts := FALSE
				END
			END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.hotFocus := TRUE; msg.setFocus := StdCFrames.setFocus
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetCheckBoxSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier, level}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: CheckBox) CheckLink (VAR ok: BOOLEAN);
	BEGIN
		ok := (c.item.typ = Meta.boolTyp) OR (c.item.typ = Meta.setTyp)
	END CheckLink;

	PROCEDURE (c: CheckBox) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		IF (op = 0) OR (c.item.typ = Meta.boolTyp) OR (c.prop.level = to) THEN
			f(StdCFrames.CheckBox).label := c.label$;
			f(StdCFrames.Frame).Update
		END
	END Update;
	

	(* RadioButton *)

	PROCEDURE GetRadioButton (f: StdCFrames.RadioButton; OUT x: BOOLEAN);
		VAR c: RadioButton;
	BEGIN
		x := FALSE;
		c := f.view(RadioButton);
		IF c.item.Valid() THEN
			IF c.item.typ = Meta.boolTyp THEN x := c.item.BoolVal() = (c.prop.level # 0)
			ELSE x := c.item.IntVal() = c.prop.level
			END
		END
	END GetRadioButton;

	PROCEDURE SetRadioButton (f: StdCFrames.RadioButton; x: BOOLEAN);
		VAR c: RadioButton; old: INTEGER;
	BEGIN
		IF x THEN
			c := f.view(RadioButton);
			IF c.item.Valid() & ~c.readOnly THEN
				IF c.item.typ = Meta.boolTyp THEN
					IF c.item.BoolVal() THEN old := 1 ELSE old := 0 END;
					IF c.prop.level # old THEN
						c.item.PutBoolVal(c.prop.level # 0);
						Notify(c, f, Dialog.changed, old, c.prop.level)
					END
				ELSE
					old := c.item.IntVal();
					IF c.prop.level # old THEN
						c.item.PutIntVal(c.prop.level);
						Notify(c, f, Dialog.changed, old, c.prop.level)
					END
				END
			END
		END
	END SetRadioButton;

	PROCEDURE (c: RadioButton) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, rbVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: RadioButton) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(rbVersion)
	END Externalize2;

	PROCEDURE (c: RadioButton) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.RadioButton;
	BEGIN
		f := StdCFrames.dir.NewRadioButton();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.label := c.label$;
		f.Get := GetRadioButton;
		f.Set := SetRadioButton;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: RadioButton) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: RadioButton) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																			VAR focus: Views.View);
	BEGIN
		IF ~c.disabled & ~c.readOnly THEN
			WITH f: StdCFrames.Frame DO
				WITH msg: Controllers.EditMsg DO
					IF (msg.op = Controllers.pasteChar)
						& ((msg.char <= " ") OR IsShortcut(msg.char, c)) THEN f.KeyDown(msg.char) END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: RadioButton) HandlePropMsg2 (VAR msg: Properties.Message);
		VAR hot: BOOLEAN;
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF ~c.disabled & ~c.readOnly THEN
				IF (msg.char = tab) OR (msg.char = ltab) THEN
					(* tabs set focus to active radio button only *)
					IF c.item.Valid() THEN
						IF c.item.typ = Meta.boolTyp THEN hot := c.item.BoolVal() = (c.prop.level # 0)
						ELSE hot := c.item.IntVal() = c.prop.level
						END
					ELSE hot := FALSE
					END;
					IF ~hot THEN msg.getFocus := FALSE END
				ELSIF (msg.char >= arrowLeft) & (msg.char <= arrowDown) THEN
					(* arrows set focus to next radio button bound to same variable *)
					msg.getFocus := StdCFrames.setFocus
						& (msg.focus # NIL) & (msg.focus IS RadioButton)
						& (msg.focus(RadioButton).item.adr = c.item.adr);
					msg.accepts := msg.getFocus & (msg.focus # c)
				ELSIF IsShortcut(msg.char, c) THEN
					msg.accepts := TRUE; msg.getFocus := StdCFrames.setFocus
				ELSIF msg.char # " " THEN
					msg.accepts := FALSE
				END
			END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.hotFocus := TRUE; msg.setFocus := StdCFrames.setFocus
 			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetRadioButtonSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier, level}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: RadioButton) CheckLink (VAR ok: BOOLEAN);
		VAR name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name);
		IF name = "List" THEN c.item.Lookup("index", c.item) END;
		ok := (c.item.typ >= Meta.byteTyp) & (c.item.typ <= Meta.intTyp) OR (c.item.typ = Meta.boolTyp)
	END CheckLink;

	PROCEDURE (c: RadioButton) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		IF (op = 0) OR (c.prop.level = to) OR (c.prop.level = from) THEN
			f(StdCFrames.RadioButton).label := c.label$;
			f(StdCFrames.Frame).Update
		END
	END Update;
	

	(* Field *)

	PROCEDURE LongToString (x: LONGINT; OUT s: ARRAY OF CHAR);
		VAR d: ARRAY 24 OF CHAR; i, j: INTEGER;
	BEGIN
		IF x = MIN(LONGINT) THEN
			s := "-9223372036854775808"
		ELSE
			i := 0; j := 0;
			IF x < 0 THEN s[0] := "-"; i := 1; x := -x END;
			REPEAT d[j] := CHR(x MOD 10 + ORD("0")); INC(j); x := x DIV 10 UNTIL x = 0;
			WHILE j > 0 DO DEC(j); s[i] := d[j]; INC(i) END;
			s[i] := 0X
		END
	END LongToString;

	PROCEDURE StringToLong (IN s: ARRAY OF CHAR; OUT x: LONGINT; OUT res: INTEGER);
		VAR i, sign, d: INTEGER;
	BEGIN
		i := 0; sign := 1; x := 0; res := 0;
		WHILE s[i] = " " DO INC(i) END;
		IF s[i] = "-" THEN sign := -1; INC(i) END;
		WHILE s[i] = " " DO INC(i) END;
		IF s[i] = 0X THEN res := 2 END;
		WHILE (s[i] >= "0") & (s[i] <= "9") DO
			d := ORD(s[i]) - ORD("0"); INC(i);
			IF x <= (MAX(LONGINT) - d) DIV 10 THEN x := 10 * x + d
			ELSE res := 1
			END
		END;
		x := x * sign;
		IF s[i] # 0X THEN res := 2 END
	END StringToLong;

	PROCEDURE FixToInt (fix: ARRAY OF CHAR; OUT int: ARRAY OF CHAR; scale: INTEGER);
		VAR i, j: INTEGER;
	BEGIN
		IF scale > 24 THEN scale := 24 ELSIF scale < 0 THEN scale := 0 END;
		i := 0; j := 0;
		WHILE (fix[i] # ".") & (fix[i] # 0X) DO int[j] := fix[i]; INC(i); INC(j) END;
		IF fix[i] = "." THEN INC(i) END;
		WHILE (scale > 0) & (fix[i] >= "0") & (fix[i] <= "9") DO int[j] := fix[i]; INC(i); INC(j); DEC(scale) END;
		WHILE scale > 0 DO int[j] := "0"; INC(j); DEC(scale) END;
		int[j] := 0X
	END FixToInt;

	PROCEDURE IntToFix (int: ARRAY OF CHAR; OUT fix: ARRAY OF CHAR; scale: INTEGER);
		VAR i, j, n: INTEGER;
	BEGIN
		IF scale > 24 THEN scale := 24 ELSIF scale < 0 THEN scale := 0 END;
		n := LEN(int$); i := 0; j := 0;
		WHILE int[i] < "0" DO fix[j] := int[i]; INC(i); INC(j); DEC(n) END;
		IF n > scale THEN
			WHILE n > scale DO fix[j] := int[i]; INC(i); INC(j); DEC(n) END
		ELSE
			fix[j] := "0"; INC(j)
		END;
		fix[j] := "."; INC(j);
		WHILE n < scale DO fix[j] := "0"; INC(j); DEC(scale) END;
		WHILE n > 0 DO  fix[j] := int[i]; INC(i); INC(j); DEC(n) END;
		fix[j] := 0X
	END IntToFix;

	PROCEDURE GetField (f: StdCFrames.Field; OUT x: ARRAY OF CHAR);
		VAR c: Field; ok: BOOLEAN; b, v: Meta.Item; mod, name: Meta.Name;
	BEGIN
		x := "";
		c := f.view(Field);
		IF c.item.Valid() THEN
			IF c.item.typ = Meta.arrTyp THEN
				c.item.GetStringVal(x, ok)
			ELSIF c.item.typ IN {Meta.byteTyp, Meta.sIntTyp, Meta.intTyp} THEN
				Strings.IntToString(c.item.IntVal(), x);
				IF c.prop.level > 0 THEN IntToFix(x, x, c.prop.level) END
			ELSIF c.item.typ = Meta.longTyp THEN
				LongToString(c.item.LongVal(), x);
				IF c.prop.level > 0 THEN IntToFix(x, x, c.prop.level) END
			ELSIF c.item.typ = Meta.sRealTyp THEN
				IF c.prop.level <= 0 THEN
					Strings.RealToStringForm(c.item.RealVal(), 7, 0, c.prop.level, " ", x)
				ELSE
					Strings.RealToStringForm(c.item.RealVal(), c.prop.level, 0, 1, " ", x)
				END
			ELSIF c.item.typ = Meta.realTyp THEN
				IF c.prop.level <= 0 THEN
					Strings.RealToStringForm(c.item.RealVal(), 16, 0, c.prop.level, " ", x)
				ELSE
					Strings.RealToStringForm(c.item.RealVal(), c.prop.level, 0, 1, " ", x)
				END
			ELSIF c.item.typ = Meta.recTyp THEN
				c.item.GetTypeName(mod, name);
				IF mod = "Dialog" THEN
					IF name = "Currency" THEN
						c.item.Lookup("val", v); c.item.Lookup("scale", b);
						LongToString(v.LongVal(), x); IntToFix(x, x, b.IntVal())
					ELSE (* Combo *)
						c.item.Lookup("item", v); (* Combo *)
						IF v.typ = Meta.arrTyp THEN v.GetStringVal(x, ok) END
					END
				END
			END
		ELSE
			x := c.label$
		END
	END GetField;

	PROCEDURE SetField (f: StdCFrames.Field; IN x: ARRAY OF CHAR);
		VAR c: Field; ok: BOOLEAN; i, res, old: INTEGER; r, or: REAL; b, v: Meta.Item;
			mod, name: Meta.Name; long, long0: LONGINT;
			s: ARRAY 1024 OF CHAR;
	BEGIN
		c := f.view(Field);
		IF c.item.Valid() & ~c.readOnly THEN
			CASE c.item.typ OF
			| Meta.arrTyp:
				c.item.GetStringVal(s, ok);
				IF ~ok OR (s$ # x$) THEN
					c.item.PutStringVal(x, ok);
					IF ok THEN Notify(c, f, Dialog.changed, 0, 0) ELSE Dialog.Beep END
				END
			| Meta.byteTyp:
				IF x = "" THEN i := 0; res := 0
				ELSIF c.prop.level > 0 THEN FixToInt(x, s, c.prop.level); Strings.StringToInt(s, i, res)
				ELSE Strings.StringToInt(x, i, res)
				END;
				IF (res = 0) & (i >= MIN(BYTE)) & (i <= MAX(BYTE)) THEN
					old := c.item.IntVal();
					IF i # old THEN c.item.PutIntVal(i); Notify(c, f, Dialog.changed, old, i) END
				ELSIF x # "-" THEN
					Dialog.Beep
				END
			| Meta.sIntTyp:
				IF x = "" THEN i := 0; res := 0
				ELSIF c.prop.level > 0 THEN FixToInt(x, s, c.prop.level); Strings.StringToInt(s, i, res)
				ELSE Strings.StringToInt(x, i, res)
				END;
				IF (res = 0) & (i >= MIN(SHORTINT)) & (i <= MAX(SHORTINT)) THEN
					old := c.item.IntVal();
					IF i # old THEN c.item.PutIntVal(i); Notify(c, f, Dialog.changed, old, i) END
				ELSIF x # "-" THEN
					Dialog.Beep
				END
			| Meta.intTyp:
				IF x = "" THEN i := 0; res := 0
				ELSIF c.prop.level > 0 THEN FixToInt(x, s, c.prop.level); Strings.StringToInt(s, i, res)
				ELSE Strings.StringToInt(x, i, res)
				END;
				IF res = 0 THEN
					old := c.item.IntVal();
					IF i # old THEN c.item.PutIntVal(i); Notify(c, f, Dialog.changed, old, i) END
				ELSIF x # "-" THEN
					Dialog.Beep
				END
			| Meta.longTyp:
				IF x = "" THEN long := 0; res := 0
				ELSE FixToInt(x, s, c.prop.level); StringToLong(s, long, res)
				END;
				IF res = 0 THEN
					long0 := c.item.LongVal();
					IF long # long0 THEN c.item.PutLongVal(long); Notify(c, f, Dialog.changed, 0, 0) END
				ELSIF x # "-" THEN
					Dialog.Beep
				END
			| Meta.sRealTyp:
				IF (x = "") OR (x = "-") THEN r := 0; res := 0 ELSE Strings.StringToReal(x, r, res) END;
				IF (res = 0) & (r >= MIN(SHORTREAL)) & (r <= MAX(SHORTREAL)) THEN
					or := c.item.RealVal();
					IF r # or THEN c.item.PutRealVal(r); Notify(c, f, Dialog.changed, 0, 0) END
				ELSIF x # "-" THEN
					Dialog.Beep
				END
			| Meta.realTyp:
				IF (x = "") OR (x = "-") THEN r := 0; res := 0 ELSE Strings.StringToReal(x, r, res) END;
				IF res = 0 THEN
					or := c.item.RealVal();
					IF r # or THEN c.item.PutRealVal(r); Notify(c, f, Dialog.changed, 0, 0) END
				ELSIF x # "-" THEN
					Dialog.Beep
				END
			| Meta.recTyp:
				c.item.GetTypeName(mod, name);
				IF mod = "Dialog" THEN
					IF name = "Currency" THEN
						c.item.Lookup("val", v); c.item.Lookup("scale", b);
						IF x = "" THEN long := 0; res := 0
						ELSE FixToInt(x, s, b.IntVal()); StringToLong(s, long, res)
						END;
						IF res = 0 THEN
							long0 := v.LongVal();
							IF long # long0 THEN v.PutLongVal(long); Notify(c, f, Dialog.changed, 0, 0) END
						ELSIF x # "-" THEN
							Dialog.Beep
						END
					ELSE	(* name = "Combo" *)
						c.item.Lookup("item", v);
						IF v.typ = Meta.arrTyp THEN
							v.GetStringVal(s, ok);
							IF ~ok OR (s$ # x$) THEN
								v.PutStringVal(x, ok);
								IF ok THEN Notify(c, f, Dialog.changed, 0, 0) ELSE Dialog.Beep END
							END
						END
					END
				END
			END
		END
	END SetField;

	PROCEDURE EqualField (f: StdCFrames.Field; IN s1, s2: ARRAY OF CHAR): BOOLEAN;
		VAR c: Field; i1, i2, res1, res2: INTEGER; r1, r2: REAL; l1, l2: LONGINT;
			mod, name: Meta.Name; t1, t2: ARRAY 64 OF CHAR; b: Meta.Item;
	BEGIN
		c := f.view(Field);
		CASE c.item.typ OF
		| Meta.arrTyp:
			RETURN s1 = s2
		| Meta.byteTyp, Meta.sIntTyp, Meta.intTyp:
			IF c.prop.level > 0 THEN
				FixToInt(s1, t1, c.prop.level); Strings.StringToInt(t1, i1, res1);
				FixToInt(s2, t2, c.prop.level); Strings.StringToInt(t2, i2, res2)
			ELSE
				Strings.StringToInt(s1, i1, res1);
				Strings.StringToInt(s2, i2, res2)
			END;
			RETURN (res1 = 0) & (res2 = 0) & (i1 = i2) 
		| Meta.longTyp:
			IF c.prop.level > 0 THEN
				FixToInt(s1, t1, c.prop.level); StringToLong(t1, l1, res1);
				FixToInt(s2, t2, c.prop.level); StringToLong(t2, l2, res2)
			ELSE
				StringToLong(s1, l1, res1);
				StringToLong(s2, l2, res2)
			END;
			RETURN (res1 = 0) & (res2 = 0) & (l1 = l2) 
		| Meta.sRealTyp, Meta.realTyp:
			Strings.StringToReal(s1, r1, res1);
			Strings.StringToReal(s2, r2, res2);
			RETURN (res1 = 0) & (res2 = 0) & (r1 = r2) 
		| Meta.recTyp:
			c.item.GetTypeName(mod, name);
			IF mod = "Dialog" THEN
				IF name = "Currency" THEN
					c.item.Lookup("scale", b); i1 := b.IntVal();
					FixToInt(s1, t1, i1); StringToLong(t1, l1, res1);
					FixToInt(s2, t2, i1); StringToLong(t2, l2, res2);
					RETURN (res1 = 0) & (res2 = 0) & (l1 =l2)
				ELSE (* name = "Combo" *)
					RETURN s1 = s2
				END
			END
		ELSE RETURN s1 = s2
		END
	END EqualField;

	PROCEDURE (c: Field) CopyFromSimpleView2 (source: Control);
	BEGIN
		WITH source: Field DO c.maxLen := source.maxLen END
	END CopyFromSimpleView2;

	PROCEDURE (c: Field) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, fldVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: Field) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(fldVersion)
	END Externalize2;

	PROCEDURE (c: Field) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.Field;
	BEGIN
		f := StdCFrames.dir.NewField();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.maxLen := c.maxLen;
		f.left := c.prop.opt[left];
		f.right := c.prop.opt[right];
		f.multiLine := c.prop.opt[multiLine];
		f.password := c.prop.opt[password];
		f.Get := GetField;
		f.Set := SetField;
		f.Equal := EqualField;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: Field) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: Field) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
		VAR ch: CHAR; mod, name: Meta.Name;
	BEGIN
		WITH f: StdCFrames.Field DO
			IF ~c.disabled & ~c.readOnly THEN
				WITH msg: Controllers.PollOpsMsg DO
					msg.selectable := TRUE;
					(* should ask Frame if there is a selection for cut or copy! *)
					msg.valid := {Controllers.pasteChar, Controllers.cut, Controllers.copy, Controllers.paste}
				| msg: Controllers.TickMsg DO
					f.Idle
				| msg: Controllers.EditMsg DO
					IF msg.op = Controllers.pasteChar THEN
						ch := msg.char;
						IF (ch = ldel) OR (ch = rdel) OR (ch >= 10X) & (ch <= 1FX)
							OR ("0" <= ch) & (ch <= "9") OR (ch = "+") OR (ch = "-")
							OR (c.item.typ = Meta.arrTyp)
							OR (c.item.typ IN {Meta.sRealTyp, Meta.realTyp}) & ((ch = ".") OR (ch = "E"))
							OR (c.prop.level > 0) & (ch = ".")
							THEN f.KeyDown(ch)
						ELSIF c.item.typ = Meta.recTyp THEN
							c.item.GetTypeName(mod, name);
							IF (mod = "Dialog") & (name = "Combo") OR (ch = ".") THEN
								f.KeyDown(ch)
							ELSE Dialog.Beep
							END
						ELSE Dialog.Beep
						END
					ELSE
						f.Edit(msg.op, msg.view, msg.w, msg.h, msg.isSingle, msg.clipboard)
					END
				| msg: Controllers.SelectMsg DO
					IF msg.set THEN f.Select(0, MAX(INTEGER))
					ELSE f.Select(-1, -1)
					END
				| msg: Controllers.MarkMsg DO
					f.Mark(msg.show, msg.focus);
					IF ~msg.show & msg.focus THEN f.Update END;
					IF msg.show & msg.focus THEN f.Select(0, MAX(INTEGER)) END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			ELSIF ~c.disabled THEN
				WITH msg: Controllers.TrackMsg DO
					f.MouseDown(msg.x, msg.y, msg.modifiers)
				ELSE
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: Field) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF msg.char = lineChar THEN msg.accepts := c.prop.opt[multiLine] & (msg.focus = c)
			ELSIF msg.char = esc THEN msg.accepts := FALSE
			END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) THEN msg.getFocus := TRUE END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.setFocus := TRUE
			ELSIF~c.disabled THEN
				msg.hotFocus := TRUE
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetFieldSize(c.maxLen, msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, level, notifier, left, right, multiLine, password}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: Field) CheckLink (VAR ok: BOOLEAN);
		VAR t: INTEGER; name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name); t := c.item.typ;
		IF (t = Meta.arrTyp) & (c.item.BaseTyp() = Meta.charTyp) THEN c.maxLen := SHORT(c.item.Len() - 1)
		ELSIF t = Meta.byteTyp THEN c.maxLen := 6
		ELSIF t = Meta.sIntTyp THEN c.maxLen := 9
		ELSIF t = Meta.intTyp THEN c.maxLen := 13
		ELSIF t = Meta.longTyp THEN c.maxLen := 24
		ELSIF t = Meta.sRealTyp THEN c.maxLen := 16
		ELSIF t = Meta.realTyp THEN c.maxLen := 24
		ELSIF name = "Combo" THEN c.maxLen := 64
		ELSIF name = "Currency" THEN c.maxLen := 16
		ELSE ok := FALSE
		END
	END CheckLink;

	PROCEDURE (c: Field) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Frame).Update
	END Update;
	

	(* UpDownField *)

	PROCEDURE GetUpDownField (f: StdCFrames.UpDownField; OUT val: INTEGER);
		VAR c: UpDownField;
	BEGIN
		val := 0;
		c := f.view(UpDownField);
		IF c.item.Valid() THEN val := c.item.IntVal() END
	END GetUpDownField;

	PROCEDURE SetUpDownField (f: StdCFrames.UpDownField; val: INTEGER);
		VAR c: UpDownField; old: INTEGER;
	BEGIN
		c := f.view(UpDownField);
		IF c.item.Valid() & ~c.readOnly THEN
			IF (val >= c.min) & (val <= c.max) THEN
				old := c.item.IntVal();
				IF old # val THEN c.item.PutIntVal(val); Notify(c, f, Dialog.changed, old, val) END
			ELSE Dialog.Beep
			END
		END
	END SetUpDownField;
	
	PROCEDURE (c: UpDownField) CopyFromSimpleView2 (source: Control);
	BEGIN
		WITH source: UpDownField DO
			c.min := source.min;
			c.max := source.max;
			c.inc := source.inc
		END
	END CopyFromSimpleView2;

	PROCEDURE (c: UpDownField) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, fldVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: UpDownField) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(fldVersion)
	END Externalize2;

	PROCEDURE (c: UpDownField) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.UpDownField;
	BEGIN
		f := StdCFrames.dir.NewUpDownField();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.min := c.min;
		f.max := c.max;
		f.inc := c.inc;
		f.Get := GetUpDownField;
		f.Set := SetUpDownField;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: UpDownField) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: UpDownField) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
		VAR ch: CHAR;
	BEGIN
		IF ~c.disabled & ~c.readOnly THEN
			WITH f: StdCFrames.UpDownField DO
				WITH msg: Controllers.PollOpsMsg DO
					msg.selectable := TRUE;
					(* should ask view if there is a selection for cut or copy! *)
					msg.valid := {Controllers.pasteChar, Controllers.cut, Controllers.copy, Controllers.paste}
				| msg: Controllers.TickMsg DO
					f.Idle
				| msg: Controllers.EditMsg DO
					IF msg.op = Controllers.pasteChar THEN
						ch := msg.char;
						IF (ch = ldel) OR (ch = rdel) OR (ch >= 10X) & (ch <= 1FX)
							OR ("0" <= ch) & (ch <= "9") OR (ch = "+") OR (ch = "-")
							OR (c.item.typ = Meta.arrTyp)
							THEN f.KeyDown(ch)
						ELSE Dialog.Beep
						END
					ELSE
						f.Edit(msg.op, msg.view, msg.w, msg.h, msg.isSingle, msg.clipboard)
					END
				| msg: Controllers.SelectMsg DO
					IF msg.set THEN f.Select(0, MAX(INTEGER))
					ELSE f.Select(-1, -1)
					END
				| msg: Controllers.MarkMsg DO
					f.Mark(msg.show, msg.focus);
					IF msg.show & msg.focus THEN f.Select(0, MAX(INTEGER)) END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: UpDownField) HandlePropMsg2 (VAR msg: Properties.Message);
		VAR m: INTEGER; n: INTEGER;
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF (msg.char = lineChar) OR (msg.char = esc) THEN msg.accepts := FALSE END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) THEN msg.getFocus := TRUE END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.setFocus := TRUE
			END
		| msg: Properties.SizePref DO
			m := -c.min;
			IF c.max > m THEN m := c.max END;
			n := 3;
			WHILE m > 99 DO INC(n); m := m DIV 10 END;
			StdCFrames.dir.GetUpDownFieldSize(n, msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: UpDownField) CheckLink (VAR ok: BOOLEAN);
	BEGIN
		IF c.item.typ = Meta.byteTyp THEN c.min := MIN(BYTE); c.max := MAX(BYTE)
		ELSIF c.item.typ = Meta.sIntTyp THEN c.min := MIN(SHORTINT); c.max := MAX(SHORTINT)
		ELSIF c.item.typ = Meta.intTyp THEN c.min := MIN(INTEGER); c.max := MAX(INTEGER)
		ELSE ok := FALSE
		END;
		c.inc := 1
	END CheckLink;

	PROCEDURE (c: UpDownField) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Frame).Update
	END Update;
	

	(* DateField *)

	PROCEDURE GetDateField (f: StdCFrames.DateField; OUT date: Dates.Date);
		VAR c: DateField; v: Meta.Item;
	BEGIN
		date.year := 1; date.month := 1; date.day := 1;
		c := f.view(DateField);
		IF c.item.Valid() THEN
			c.item.Lookup("year", v); IF v.typ = Meta.intTyp THEN date.year := SHORT(v.IntVal()) END;
			c.item.Lookup("month", v); IF v.typ = Meta.intTyp THEN date.month := SHORT(v.IntVal()) END;
			c.item.Lookup("day", v); IF v.typ = Meta.intTyp THEN date.day := SHORT(v.IntVal()) END
		END
	END GetDateField;
	
	PROCEDURE SetDateField(f:  StdCFrames.DateField; IN date: Dates.Date);
		VAR c: DateField; v: Meta.Item;
	BEGIN
		c := f.view(DateField);
		IF c.item.Valid() & ~c.readOnly THEN
			c.item.Lookup("year", v); IF v.typ = Meta.intTyp THEN v.PutIntVal(date.year) END;
			c.item.Lookup("month", v); IF v.typ = Meta.intTyp THEN v.PutIntVal(date.month) END;
			c.item.Lookup("day", v); IF v.typ = Meta.intTyp THEN v.PutIntVal(date.day) END;
			Notify(c, f, Dialog.changed, 0, 0)
		END
	END SetDateField;
	
	PROCEDURE GetDateFieldSelection (f: StdCFrames.DateField; OUT sel: INTEGER);
	BEGIN
		sel := f.view(DateField).selection
	END GetDateFieldSelection;
	
	PROCEDURE SetDateFieldSelection (f: StdCFrames.DateField; sel: INTEGER);
	BEGIN
		f.view(DateField).selection := sel
	END SetDateFieldSelection;
	
	PROCEDURE (c: DateField) CopyFromSimpleView2 (source: Control);
	BEGIN
		WITH source: DateField DO c.selection := source.selection END
	END CopyFromSimpleView2;

	PROCEDURE (c: DateField) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, dfldVersion, thisVersion);
		c.selection := 0
	END Internalize2;

	PROCEDURE (c: DateField) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(dfldVersion)
	END Externalize2;

	PROCEDURE (c: DateField) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.DateField;
	BEGIN
		f := StdCFrames.dir.NewDateField();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.Get := GetDateField;
		f.Set := SetDateField;
		f.GetSel := GetDateFieldSelection;
		f.SetSel := SetDateFieldSelection;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: DateField) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: DateField) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
	BEGIN
		IF ~c.disabled & ~c.readOnly THEN
			WITH f: StdCFrames.DateField DO
				WITH msg: Controllers.PollOpsMsg DO
					msg.valid := {Controllers.pasteChar, Controllers.copy}
				| msg: Controllers.EditMsg DO
					IF msg.op = Controllers.pasteChar THEN
						f.KeyDown(msg.char)
					ELSE
						f.Edit(msg.op, msg.view, msg.w, msg.h, msg.isSingle, msg.clipboard)
					END
				| msg: Controllers.TickMsg DO
					IF f.mark THEN
						IF c.selection = 0 THEN c.selection := 1; Views.Update(c, Views.keepFrames) END
					END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: DateField) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF (msg.char = lineChar) OR (msg.char = esc) THEN
				msg.accepts := FALSE
			ELSIF (msg.char = tab) OR (msg.char = ltab) THEN
				msg.accepts := ((msg.focus # c) & (~c.disabled & ~c.readOnly)) OR 
					(msg.focus = c) & ((msg.char = tab) & (c.selection # -1) OR (msg.char = ltab) & (c.selection # 1));
				msg.getFocus := msg.accepts
			END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) THEN msg.getFocus := TRUE END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.setFocus := TRUE
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetDateFieldSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: DateField) CheckLink (VAR ok: BOOLEAN);
		VAR name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name);
		ok := name = "Date"
	END CheckLink;

	PROCEDURE (c: DateField) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Frame).Update
	END Update;
	

	(* TimeField *)

	PROCEDURE GetTimeField (f: StdCFrames.TimeField; OUT time: Dates.Time);
		VAR c: TimeField; v: Meta.Item;
	BEGIN
		time.hour := 0; time.minute := 0; time.second := 0;
		c := f.view(TimeField);
		IF c.item.Valid() THEN
			c.item.Lookup("hour", v); IF v.typ = Meta.intTyp THEN time.hour := SHORT(v.IntVal()) END;
			c.item.Lookup("minute", v); IF v.typ = Meta.intTyp THEN time.minute := SHORT(v.IntVal()) END;
			c.item.Lookup("second", v); IF v.typ = Meta.intTyp THEN time.second := SHORT(v.IntVal()) END
		END
	END GetTimeField;
	
	PROCEDURE SetTimeField(f:  StdCFrames.TimeField; IN date: Dates.Time);
		VAR c: TimeField; v: Meta.Item;
	BEGIN
		c := f.view(TimeField);
		IF c.item.Valid() & ~c.readOnly THEN
			c.item.Lookup("hour", v); IF v.typ = Meta.intTyp THEN v.PutIntVal(date.hour) END;
			c.item.Lookup("minute", v); IF v.typ = Meta.intTyp THEN v.PutIntVal(date.minute) END;
			c.item.Lookup("second", v); IF v.typ = Meta.intTyp THEN v.PutIntVal(date.second) END;
			Notify(c, f, Dialog.changed, 0, 0)
		END
	END SetTimeField;
	
	PROCEDURE GetTimeFieldSelection (f: StdCFrames.TimeField; OUT sel: INTEGER);
	BEGIN
		sel := f.view(TimeField).selection
	END GetTimeFieldSelection;
	
	PROCEDURE SetTimeFieldSelection (f: StdCFrames.TimeField; sel: INTEGER);
	BEGIN
		f.view(TimeField).selection := sel
	END SetTimeFieldSelection;
	
	PROCEDURE (c: TimeField) CopyFromSimpleView2 (source: Control);
	BEGIN
		WITH source: TimeField DO c.selection := source.selection END
	END CopyFromSimpleView2;

	PROCEDURE (c: TimeField) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, tfldVersion, thisVersion);
		c.selection := 0
	END Internalize2;

	PROCEDURE (c: TimeField) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(tfldVersion)
	END Externalize2;

	PROCEDURE (c: TimeField) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.TimeField;
	BEGIN
		f := StdCFrames.dir.NewTimeField();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.Get := GetTimeField;
		f.Set := SetTimeField;
		f.GetSel := GetTimeFieldSelection;
		f.SetSel := SetTimeFieldSelection;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: TimeField) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: TimeField) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
	BEGIN
		IF ~c.disabled & ~c.readOnly THEN
			WITH f: StdCFrames.TimeField DO
				WITH msg: Controllers.PollOpsMsg DO
					msg.valid := {Controllers.pasteChar, Controllers.copy}
				| msg: Controllers.EditMsg DO
					IF msg.op = Controllers.pasteChar THEN
						f.KeyDown(msg.char)
					ELSE
						f.Edit(msg.op, msg.view, msg.w, msg.h, msg.isSingle, msg.clipboard)
					END
				| msg: Controllers.TickMsg DO
					IF f.mark THEN
						IF c.selection = 0 THEN c.selection := 1; Views.Update(c, Views.keepFrames) END
					END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: TimeField) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF (msg.char = lineChar) OR (msg.char = esc) THEN
				msg.accepts := FALSE
			ELSIF (msg.char = tab) OR (msg.char = ltab) THEN
				msg.accepts := (msg.focus # c) OR 
					((msg.char = tab) & (c.selection # -1)) OR ((msg.char = ltab) & (c.selection # 1))
			END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) THEN msg.getFocus := TRUE END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.setFocus := TRUE
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetTimeFieldSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: TimeField) CheckLink (VAR ok: BOOLEAN);
		VAR name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name);
		ok := name = "Time"
	END CheckLink;

	PROCEDURE (c: TimeField) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Frame).Update
	END Update;
	

	(* ColorField *)

	PROCEDURE GetColorField (f: StdCFrames.ColorField; OUT col: INTEGER);
		VAR c: ColorField; v: Meta.Item;
	BEGIN
		col := Ports.defaultColor;
		c := f.view(ColorField);
		IF c.item.Valid() THEN
			IF c.item.typ = Meta.intTyp THEN
				col := c.item.IntVal()
			ELSE
				c.item.Lookup("val", v); IF v.typ = Meta.intTyp THEN col := v.IntVal() END
			END
		END
	END GetColorField;
	
	PROCEDURE SetColorField(f:  StdCFrames.ColorField; col: INTEGER);
		VAR c: ColorField; v: Meta.Item; old: INTEGER;
	BEGIN
		c := f.view(ColorField);
		IF c.item.Valid() & ~c.readOnly THEN
			IF c.item.typ = Meta.intTyp THEN
				old := c.item.IntVal();
				IF old # col THEN c.item.PutIntVal(col); Notify(c, f, Dialog.changed, old, col) END
			ELSE
				c.item.Lookup("val", v); 
				IF v.typ = Meta.intTyp THEN 
					old := v.IntVal();
					IF old # col THEN v.PutIntVal(col); Notify(c, f, Dialog.changed, old, col) END
				END
			END
		END
	END SetColorField;
	
	PROCEDURE (c: ColorField) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, cfldVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: ColorField) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(cfldVersion)
	END Externalize2;

	PROCEDURE (c: ColorField) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.ColorField;
	BEGIN
		f := StdCFrames.dir.NewColorField();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.Get := GetColorField;
		f.Set := SetColorField;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: ColorField) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: ColorField) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
	BEGIN
		IF ~c.disabled & ~c.readOnly THEN
			WITH f: StdCFrames.ColorField DO
				WITH msg: Controllers.EditMsg DO
					IF msg.op = Controllers.pasteChar THEN
						f.KeyDown(msg.char)
					ELSE
						f.Edit(msg.op, msg.view, msg.w, msg.h, msg.isSingle, msg.clipboard)
					END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: ColorField) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			msg.accepts := ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c)
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.hotFocus := TRUE; msg.setFocus := StdCFrames.setFocus
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetColorFieldSize(msg.w, msg.h)
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: ColorField) CheckLink (VAR ok: BOOLEAN);
		VAR name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name);
		ok := (name = "Color") OR (c.item.typ = Meta.intTyp)
	END CheckLink;

	PROCEDURE (c: ColorField) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Frame).Update
	END Update;
	

	(* ListBox *)

	PROCEDURE GetListBox (f: StdCFrames.ListBox; OUT i: INTEGER);
		VAR c: ListBox; v: Meta.Item;
	BEGIN
		i := -1;
		c := f.view(ListBox);
		IF c.item.Valid() THEN
			c.item.Lookup("index", v);
			IF v.typ = Meta.intTyp THEN i := v.IntVal() END
		END
	END GetListBox;

	PROCEDURE SetListBox (f: StdCFrames.ListBox; i: INTEGER);
		VAR c: ListBox; v: Meta.Item; old: INTEGER;
	BEGIN
		c := f.view(ListBox);
		IF c.item.Valid() & ~c.readOnly THEN
			c.item.Lookup("index", v);
			IF v.typ = Meta.intTyp THEN
				old := v.IntVal();
				IF i # old THEN v.PutIntVal(i); Notify(c, f, Dialog.changed, old, i) END
			END
		END
	END SetListBox;
	
	PROCEDURE GetFName (VAR rec, par: ANYREC);
	BEGIN
		WITH par: Param DO
			WITH rec: Dialog.List DO rec.GetItem(par.i, par.n)
			| rec: Dialog.Selection DO rec.GetItem(par.i, par.n)
			| rec: Dialog.Combo DO rec.GetItem(par.i, par.n)
			ELSE par.n := ""
			END
		END
	END GetFName;
	
	PROCEDURE GetListName (f: StdCFrames.ListBox; i: INTEGER; VAR name: ARRAY OF CHAR);
		VAR c: ListBox; par: Param;
	BEGIN
		par.n := "";
		c := f.view(ListBox);
		IF c.item.Valid() THEN
			par.i := i;
			c.item.CallWith(GetFName, par)
		END;
		name := par.n$
	END GetListName;

	PROCEDURE (c: ListBox) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, lbxVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: ListBox) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(lbxVersion)
	END Externalize2;

	PROCEDURE (c: ListBox) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.ListBox;
	BEGIN
		f := StdCFrames.dir.NewListBox();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.sorted := c.prop.opt[sorted];
		f.Get := GetListBox;
		f.Set := SetListBox;
		f.GetName := GetListName;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: ListBox) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: ListBox) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
	BEGIN
		WITH f: StdCFrames.ListBox DO
			IF ~c.disabled & ~c.readOnly THEN
				WITH msg: Controllers.EditMsg DO
					IF msg.op = Controllers.pasteChar THEN f.KeyDown(msg.char) END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			ELSIF ~c.disabled THEN
				WITH msg: Controllers.TrackMsg DO
					f.MouseDown(msg.x, msg.y, msg.modifiers)
				ELSE
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: ListBox) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF (msg.char = lineChar) OR (msg.char = esc) THEN msg.accepts := FALSE END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) THEN msg.getFocus := TRUE END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.setFocus := TRUE
			ELSIF~c.disabled THEN
				msg.hotFocus := TRUE
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetListBoxSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier, sorted}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: ListBox) CheckLink (VAR ok: BOOLEAN);
		VAR name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name);
		ok := name = "List"
	END CheckLink;

	PROCEDURE (c: ListBox) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Frame).Update
	END Update;
	
	PROCEDURE (c: ListBox) UpdateList (f: Views.Frame);
	BEGIN
		f(StdCFrames.Frame).UpdateList
	END UpdateList;
	

	(* SelectionBox *)

	PROCEDURE InLargeSet (VAR rec, par: ANYREC);
	BEGIN
		WITH par: Param DO
			WITH rec: Dialog.Selection DO
				IF rec.In(par.i) THEN par.i := 1 ELSE par.i := 0 END
			ELSE par.i := 0
			END
		END
	END InLargeSet;
	
	PROCEDURE GetSelectionBox (f: StdCFrames.SelectionBox; i: INTEGER; OUT in: BOOLEAN);
		VAR c: SelectionBox; lv: SelectValue; par: Param;
	BEGIN
		in := FALSE;
		c := f.view(SelectionBox);
		IF c.item.Valid() THEN
			IF c.item.Is(lv) THEN
				par.i := i;
				c.item.CallWith(InLargeSet, par);
				in := par.i # 0
			END
		END
	END GetSelectionBox;

	PROCEDURE InclLargeSet (VAR rec, par: ANYREC);
	BEGIN
		WITH par: Param DO
			WITH rec: Dialog.Selection DO
				IF (par.from # par.to) OR ~rec.In(par.from) THEN
					rec.Incl(par.from, par.to); par.i := 1
				ELSE par.i := 0
				END
			ELSE par.i := 0
			END
		END
	END InclLargeSet;
	
	PROCEDURE InclSelectionBox (f: StdCFrames.SelectionBox; from, to: INTEGER);
		VAR c: SelectionBox; lv: SelectValue; par: Param;
	BEGIN
		c := f.view(SelectionBox);
		IF c.item.Valid() & ~c.readOnly THEN
			IF c.item.Is(lv) THEN
				par.from := from; par.to := to;
				c.item.CallWith(InclLargeSet, par);
				IF par.i # 0 THEN Notify(c, f, Dialog.included, from, to) END
			END
		END
	END InclSelectionBox;
	
	PROCEDURE ExclLargeSet (VAR rec, par: ANYREC);
	BEGIN
		WITH par: Param DO
			WITH rec: Dialog.Selection DO
				IF (par.from # par.to) OR rec.In(par.from) THEN
					rec.Excl(par.from, par.to); par.i := 1
				ELSE par.i := 0
				END
			ELSE par.i := 0
			END
		END
	END ExclLargeSet;
	
	PROCEDURE ExclSelectionBox (f: StdCFrames.SelectionBox; from, to: INTEGER);
		VAR c: SelectionBox; lv: SelectValue; par: Param;
	BEGIN
		c := f.view(SelectionBox);
		IF c.item.Valid() & ~c.readOnly THEN
			IF c.item.Is(lv) THEN
				par.from := from; par.to := to;
				c.item.CallWith(ExclLargeSet, par);
				IF par.i # 0 THEN Notify(c, f, Dialog.excluded, from, to) END
			END
		END
	END ExclSelectionBox;
	
	PROCEDURE SetSelectionBox (f: StdCFrames.SelectionBox; from, to: INTEGER);
		VAR c: SelectionBox; lv: SelectValue; par: Param;
	BEGIN
		c := f.view(SelectionBox);
		IF c.item.Valid() & ~c.readOnly THEN
			IF c.item.Is(lv) THEN
				par.from := 0; par.to := MAX(INTEGER);
				c.item.CallWith(ExclLargeSet, par);
				par.from := from; par.to := to;
				c.item.CallWith(InclLargeSet, par);
				Notify(c, f, Dialog.set, from, to)
			END
		END
	END SetSelectionBox;
	
	PROCEDURE GetSelName (f: StdCFrames.SelectionBox; i: INTEGER; VAR name: ARRAY OF CHAR);
		VAR c: SelectionBox; par: Param;
	BEGIN
		par.n := "";
		c := f.view(SelectionBox);
		IF c.item.Valid() THEN
			par.i := i;
			c.item.CallWith(GetFName, par)
		END;
		name := par.n$
	END GetSelName;

	PROCEDURE (c: SelectionBox) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, sbxVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: SelectionBox) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(sbxVersion)
	END Externalize2;

	PROCEDURE (c: SelectionBox) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.SelectionBox;
	BEGIN
		f := StdCFrames.dir.NewSelectionBox();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.sorted := c.prop.opt[sorted];
		f.Get := GetSelectionBox;
		f.Incl := InclSelectionBox;
		f.Excl := ExclSelectionBox;
		f.Set := SetSelectionBox;
		f.GetName := GetSelName;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: SelectionBox) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: SelectionBox) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																				VAR focus: Views.View);
	BEGIN
		WITH f: StdCFrames.SelectionBox DO
			IF ~c.disabled & ~c.readOnly THEN
				WITH msg: Controllers.EditMsg DO
					IF msg.op = Controllers.pasteChar THEN f.KeyDown(msg.char) END
				| msg: Controllers.SelectMsg DO
					IF msg.set THEN f.Select(0, MAX(INTEGER))
					ELSE f.Select(-1, -1)
					END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			ELSIF ~c.disabled THEN
				WITH msg: Controllers.TrackMsg DO
					f.MouseDown(msg.x, msg.y, msg.modifiers)
				ELSE
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: SelectionBox) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF (msg.char = lineChar) OR (msg.char = esc) THEN msg.accepts := FALSE END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) OR msg.getFocus THEN
				msg.getFocus := StdCFrames.setFocus
			END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.setFocus := TRUE
			ELSIF~c.disabled THEN
				msg.hotFocus := TRUE
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetSelectionBoxSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier, sorted}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: SelectionBox) CheckLink (VAR ok: BOOLEAN);
		VAR name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name);
		ok := name = "Selection"
	END CheckLink;

	PROCEDURE (c: SelectionBox) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		IF (op >= Dialog.included) & (op <= Dialog.set) THEN
			f(StdCFrames.SelectionBox).UpdateRange(op, from, to)
		ELSE
			f(StdCFrames.Frame).Update
		END
	END Update;
	
	PROCEDURE (c: SelectionBox) UpdateList (f: Views.Frame);
	BEGIN
		f(StdCFrames.Frame).UpdateList
	END UpdateList;
	

	(* ComboBox *)

	PROCEDURE GetComboBox (f: StdCFrames.ComboBox; OUT x: ARRAY OF CHAR);
		VAR c: ComboBox; ok: BOOLEAN; v: Meta.Item;
	BEGIN
		x := "";
		c := f.view(ComboBox);
		IF c.item.Valid() THEN
			c.item.Lookup("item", v);
			IF v.typ = Meta.arrTyp THEN v.GetStringVal(x, ok) END
		END
	END GetComboBox;

	PROCEDURE SetComboBox (f: StdCFrames.ComboBox; IN x: ARRAY OF CHAR);
		VAR c: ComboBox; ok: BOOLEAN; v: Meta.Item; s: ARRAY 1024 OF CHAR;
	BEGIN
		c := f.view(ComboBox);
		IF c.item.Valid() & ~c.readOnly THEN
			c.item.Lookup("item", v);
			IF v.typ = Meta.arrTyp THEN
				v.GetStringVal(s, ok);
				IF ~ok OR (s$ # x$) THEN
					v.PutStringVal(x, ok);
					IF ok THEN Notify(c, f, Dialog.changed, 0, 0) END
				END
			END
		END
	END SetComboBox;

	PROCEDURE GetComboName (f: StdCFrames.ComboBox; i: INTEGER; VAR name: ARRAY OF CHAR);
		VAR c: ComboBox; par: Param;
	BEGIN
		par.n := "";
		c := f.view(ComboBox);
		IF c.item.Valid() THEN
			par.i := i;
			c.item.CallWith(GetFName, par)
		END;
		name := par.n$
	END GetComboName;

	PROCEDURE (c: ComboBox) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, cbxVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: ComboBox) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(cbxVersion)
	END Externalize2;

	PROCEDURE (c: ComboBox) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.ComboBox;
	BEGIN
		f := StdCFrames.dir.NewComboBox();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.sorted := c.prop.opt[sorted];
		f.Get := GetComboBox;
		f.Set := SetComboBox;
		f.GetName := GetComboName;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: ComboBox) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: ComboBox) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																			VAR focus: Views.View);
	BEGIN
		WITH f: StdCFrames.ComboBox DO
			IF ~c.disabled & ~c.readOnly THEN
				WITH msg: Controllers.PollOpsMsg DO
					msg.selectable := TRUE;
					(* should ask Frame if there is a selection for cut or copy! *)
					msg.valid := {Controllers.pasteChar, Controllers.cut, Controllers.copy, Controllers.paste}
				| msg: Controllers.TickMsg DO
					f.Idle
				| msg: Controllers.EditMsg DO
					IF msg.op = Controllers.pasteChar THEN
						f.KeyDown(msg.char)
					ELSE
						f.Edit(msg.op, msg.view, msg.w, msg.h, msg.isSingle, msg.clipboard)
					END
				| msg: Controllers.SelectMsg DO
					IF msg.set THEN f.Select(0, MAX(INTEGER))
					ELSE f.Select(-1, -1)
					END
				| msg: Controllers.MarkMsg DO
					f.Mark(msg.show, msg.focus);
					IF msg.show & msg.focus THEN f.Select(0, MAX(INTEGER)) END
				| msg: Controllers.TrackMsg DO
					f.MouseDown(msg.x, msg.y, msg.modifiers)
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: ComboBox) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.ControlPref DO
			IF (msg.char = lineChar) OR (msg.char = esc) THEN msg.accepts := FALSE END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) THEN msg.getFocus := TRUE END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.setFocus := TRUE
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetComboBoxSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier, sorted}
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: ComboBox) CheckLink (VAR ok: BOOLEAN);
		VAR name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name);
		ok := name = "Combo"
	END CheckLink;

	PROCEDURE (c: ComboBox) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Frame).Update
	END Update;
	
	PROCEDURE (c: ComboBox) UpdateList (f: Views.Frame);
	BEGIN
		f(StdCFrames.Frame).UpdateList
	END UpdateList;
	

	(* Caption *)

	PROCEDURE (c: Caption) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, capVersion, thisVersion);
		IF thisVersion < 1 THEN c.prop.opt[left] := TRUE END
	END Internalize2;

	PROCEDURE (c: Caption) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		(* Save old version for captions that are compatible with the old version *)
		IF c.prop.opt[left] THEN wr.WriteVersion(0) ELSE wr.WriteVersion(capVersion) END
	END Externalize2;

	PROCEDURE (c: Caption) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.Caption;
	BEGIN
		f := StdCFrames.dir.NewCaption();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.label := c.label$;
		f.left := c.prop.opt[left];
		f.right := c.prop.opt[right];
		frame := f
	END GetNewFrame;

	PROCEDURE (c: Caption) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: Caption) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.SizePref DO
			StdCFrames.dir.GetCaptionSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, left, right}
		| msg: DefaultsPref DO
			IF c.prop.link = "" THEN msg.disabled := FALSE END
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: Caption) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Caption).label := c.label$;
		f(StdCFrames.Frame).Update
	END Update;
	

	(* Group *)

	PROCEDURE (c: Group) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, grpVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: Group) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(grpVersion)
	END Externalize2;

	PROCEDURE (c: Group) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.Group;
	BEGIN
		f := StdCFrames.dir.NewGroup();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.label := c.label$;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: Group) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;

	PROCEDURE (c: Group) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.SizePref DO
			StdCFrames.dir.GetGroupSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard}
		| msg: DefaultsPref DO
			IF c.prop.link = "" THEN msg.disabled := FALSE END
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: Group) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Group).label := c.label$;
		f(StdCFrames.Frame).Update
	END Update;
	
	
	(* TreeControl *)
	
	PROCEDURE (c: TreeControl) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, tfVersion, thisVersion)
	END Internalize2;

	PROCEDURE (c: TreeControl) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(tfVersion)
	END Externalize2;

	PROCEDURE TVNofNodesF (VAR rec, par: ANYREC);
	BEGIN
		WITH par: TVParam DO 
			WITH rec: Dialog.Tree DO par.l := rec.NofNodes()
			ELSE par.l := 0
			END
		END
	END TVNofNodesF;
	
	PROCEDURE TVNofNodes (f: StdCFrames.TreeFrame): INTEGER;
		VAR c: TreeControl; par: TVParam;
	BEGIN
		c := f.view(TreeControl); par.l := 0;
		IF c.item.Valid() THEN c.item.CallWith(TVNofNodesF, par) END;
		RETURN par.l
	END TVNofNodes;
	
	PROCEDURE TVChildF (VAR rec, par: ANYREC);
	BEGIN
		WITH par: TVParam DO 
			WITH rec: Dialog.Tree DO par.nodeOut := rec.Child(par.nodeIn, Dialog.firstPos)
			ELSE par.nodeOut := NIL
			END
		END
	END TVChildF;
	
	PROCEDURE TVChild (f: StdCFrames.TreeFrame; node: Dialog.TreeNode): Dialog.TreeNode;
		VAR c: TreeControl; par: TVParam;
	BEGIN
		c := f.view(TreeControl); par.nodeIn := node; par.nodeOut := NIL;
		IF c.item.Valid() THEN c.item.CallWith(TVChildF, par) END;
		RETURN par.nodeOut
	END TVChild;
	
	PROCEDURE TVParentF (VAR rec, par: ANYREC);
	BEGIN
		WITH par: TVParam DO 
			WITH rec: Dialog.Tree DO par.nodeOut := rec.Parent(par.nodeIn)
			ELSE par.nodeOut := NIL
			END
		END
	END TVParentF;
	
	PROCEDURE TVParent (f: StdCFrames.TreeFrame; node: Dialog.TreeNode): Dialog.TreeNode;
		VAR c: TreeControl; par: TVParam;
	BEGIN
		c := f.view(TreeControl); par.nodeIn := node; par.nodeOut := NIL;
		IF c.item.Valid() THEN c.item.CallWith(TVParentF, par) END;
		RETURN par.nodeOut
	END TVParent;
	
	PROCEDURE TVNextF (VAR rec, par: ANYREC);
	BEGIN
		WITH par: TVParam DO 
			WITH rec: Dialog.Tree DO par.nodeOut := rec.Next(par.nodeIn)
			ELSE par.nodeOut := NIL
			END
		END
	END TVNextF;
	
	PROCEDURE TVNext (f: StdCFrames.TreeFrame; node: Dialog.TreeNode): Dialog.TreeNode;
		VAR c: TreeControl; par: TVParam;
	BEGIN
		c := f.view(TreeControl); par.nodeIn := node; par.nodeOut := NIL;
		IF c.item.Valid() THEN c.item.CallWith(TVNextF, par) END;
		RETURN par.nodeOut
	END TVNext;
	
	PROCEDURE TVSelectF (VAR rec, par: ANYREC);
	BEGIN
		WITH par: TVParam DO 
			WITH rec: Dialog.Tree DO rec.Select(par.nodeIn) END
		END
	END TVSelectF;
	
	PROCEDURE TVSelect (f: StdCFrames.TreeFrame; node: Dialog.TreeNode);
		VAR c: TreeControl; par: TVParam;
	BEGIN
		c := f.view(TreeControl); par.nodeIn := node;
		IF c.item.Valid() THEN 
			c.item.CallWith(TVSelectF, par);
			Notify(c, f, Dialog.changed, 0, 0)
		END
	END TVSelect;
	
	PROCEDURE TVSelectedF (VAR rec, par: ANYREC);
	BEGIN
		WITH par: TVParam DO 
			WITH rec: Dialog.Tree DO par.nodeOut := rec.Selected()
			ELSE par.nodeOut := NIL
			END
		END
	END TVSelectedF;
	
	PROCEDURE TVSelected (f: StdCFrames.TreeFrame): Dialog.TreeNode;
		VAR c: TreeControl; par: TVParam;
	BEGIN
		c := f.view(TreeControl); par.nodeOut := NIL;
		IF c.item.Valid() THEN c.item.CallWith(TVSelectedF, par) END;
		RETURN par.nodeOut
	END TVSelected;
	
	PROCEDURE TVSetExpansionF (VAR rec, par: ANYREC);
	BEGIN
		WITH par: TVParam DO 
			par.nodeIn.SetExpansion(par.e)
		END
	END TVSetExpansionF;
	
	PROCEDURE TVSetExpansion (f: StdCFrames.TreeFrame; tn: Dialog.TreeNode; expanded: BOOLEAN);
		VAR c: TreeControl; par: TVParam;
	BEGIN
		c := f.view(TreeControl); par.e := expanded; par.nodeIn := tn;
		IF c.item.Valid() THEN c.item.CallWith(TVSetExpansionF, par) END
	END TVSetExpansion;

	PROCEDURE (c: TreeControl) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.TreeFrame;
	BEGIN
		f := StdCFrames.dir.NewTreeFrame();
		f.disabled := c.disabled;
		f.undef := c.undef;
		f.readOnly := c.readOnly;
		f.font := c.font;
		f.sorted := c.prop.opt[sorted];
		f.haslines := c.prop.opt[haslines];
		f.hasbuttons := c.prop.opt[hasbuttons];
		f.atroot := c.prop.opt[atroot];
		f.foldericons := c.prop.opt[foldericons];
		f.NofNodes := TVNofNodes;
		f.Child := TVChild;
		f.Parent := TVParent;
		f.Next := TVNext;
		f.Select := TVSelect;
		f.Selected := TVSelected;
		f.SetExpansion := TVSetExpansion;
		frame := f
	END GetNewFrame;

	PROCEDURE (c: TreeControl) UpdateList (f: Views.Frame);
	BEGIN
		f(StdCFrames.Frame).UpdateList()
	END UpdateList;
	
	PROCEDURE (c: TreeControl) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;
	
	PROCEDURE (c: TreeControl) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																			VAR focus: Views.View);
	BEGIN
		WITH f: StdCFrames.TreeFrame DO
			IF ~c.disabled & ~c.readOnly THEN
				WITH msg: Controllers.EditMsg DO
					IF (msg.op = Controllers.pasteChar) THEN 
						f.KeyDown(msg.char)
					END
				ELSE
					CatchCtrlMsg(c, f, msg, focus)
				END
			ELSIF ~c.disabled THEN
				WITH msg: Controllers.TrackMsg DO
					f.MouseDown(msg.x, msg.y, msg.modifiers)
				ELSE
				END
			END
		END
	END HandleCtrlMsg2;

	PROCEDURE (c: TreeControl) HandlePropMsg2 (VAR msg: Properties.Message);
	BEGIN			
		WITH msg: Properties.ControlPref DO
			IF (msg.char = lineChar) OR (msg.char = esc) THEN msg.accepts := FALSE END;
			IF ~c.disabled & ~c.readOnly & IsShortcut(msg.char, c) OR msg.getFocus THEN
				msg.getFocus := StdCFrames.setFocus
			END
		| msg: Properties.FocusPref DO
			IF ~c.disabled & ~c.readOnly THEN
				msg.setFocus := TRUE
			ELSIF~c.disabled THEN
				msg.hotFocus := TRUE
			END
		| msg: Properties.SizePref DO
			StdCFrames.dir.GetTreeFrameSize(msg.w, msg.h)
		| msg: PropPref DO
			msg.valid := {link, label, guard, notifier, sorted, haslines, hasbuttons, atroot, foldericons}
		| msg: Properties.ResizePref DO
			msg.horFitToWin := TRUE; msg.verFitToWin := TRUE
		ELSE
		END
	END HandlePropMsg2;

	PROCEDURE (c: TreeControl) CheckLink (VAR ok: BOOLEAN);
		VAR name: Meta.Name;
	BEGIN
		GetTypeName(c.item, name);
		ok := name = "Tree"
	END CheckLink;

	PROCEDURE (c: TreeControl) Update (f: Views.Frame; op, from, to: INTEGER);
	BEGIN
		f(StdCFrames.Frame).Update
	END Update;
	

	(* StdDirectory *)

	PROCEDURE (d: StdDirectory) NewPushButton (p: Prop): Control;
		VAR c: PushButton;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewPushButton;

	PROCEDURE (d: StdDirectory) NewCheckBox (p: Prop): Control;
		VAR c: CheckBox;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewCheckBox;

	PROCEDURE (d: StdDirectory) NewRadioButton (p: Prop): Control;
		VAR c: RadioButton;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewRadioButton;

	PROCEDURE (d: StdDirectory) NewField (p: Prop): Control;
		VAR c: Field;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewField;
	
	PROCEDURE (d: StdDirectory) NewUpDownField (p: Prop): Control;
		VAR c: UpDownField;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewUpDownField;
	
	PROCEDURE (d: StdDirectory) NewDateField (p: Prop): Control;
		VAR c: DateField;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewDateField;

	PROCEDURE (d: StdDirectory) NewTimeField (p: Prop): Control;
		VAR c: TimeField;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewTimeField;
	
	PROCEDURE (d: StdDirectory) NewColorField (p: Prop): Control;
		VAR c: ColorField;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewColorField;

	PROCEDURE (d: StdDirectory) NewListBox (p: Prop): Control;
		VAR c: ListBox;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewListBox;

	PROCEDURE (d: StdDirectory) NewSelectionBox (p: Prop): Control;
		VAR c: SelectionBox;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewSelectionBox;

	PROCEDURE (d: StdDirectory) NewComboBox (p: Prop): Control;
		VAR c: ComboBox;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewComboBox;

	PROCEDURE (d: StdDirectory) NewCaption (p: Prop): Control;
		VAR c: Caption;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewCaption;

	PROCEDURE (d: StdDirectory) NewGroup (p: Prop): Control;
		VAR c: Group;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewGroup;

	PROCEDURE (d: StdDirectory) NewTreeControl (p: Prop): Control;
		VAR c: TreeControl;
	BEGIN
		NEW(c); OpenLink(c, p); RETURN c
	END NewTreeControl;

	PROCEDURE SetDir* (d: Directory);
	BEGIN
		ASSERT(d # NIL, 20); dir := d
	END SetDir;

	PROCEDURE InitProp (VAR p: Prop);
	BEGIN
		NEW(p);
		p.link := ""; p.label := ""; p.guard := ""; p.notifier := "";
		p.level := 0;
		p.opt[0] := FALSE; p.opt[1] := FALSE;
		p.opt[2] := FALSE; p.opt[3] := FALSE;
		p.opt[4] := FALSE
	END InitProp;

	PROCEDURE DepositPushButton*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		p.label := "#System:untitled";
		Views.Deposit(dir.NewPushButton(p))
	END DepositPushButton;

	PROCEDURE DepositCheckBox*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		p.label := "#System:untitled";
		Views.Deposit(dir.NewCheckBox(p))
	END DepositCheckBox;

	PROCEDURE DepositRadioButton*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		p.label := "#System:untitled";
		Views.Deposit(dir.NewRadioButton(p))
	END DepositRadioButton;

	PROCEDURE DepositField*;
		VAR p: Prop;
	BEGIN
		InitProp(p); p.opt[left] := TRUE;
		Views.Deposit(dir.NewField(p))
	END DepositField;
	
	PROCEDURE DepositUpDownField*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		Views.Deposit(dir.NewUpDownField(p))
	END DepositUpDownField;
	
	PROCEDURE DepositDateField*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		Views.Deposit(dir.NewDateField(p))
	END DepositDateField;

	PROCEDURE DepositTimeField*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		Views.Deposit(dir.NewTimeField(p))
	END DepositTimeField;

	PROCEDURE DepositColorField*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		Views.Deposit(dir.NewColorField(p))
	END DepositColorField;

	PROCEDURE DepositListBox*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		Views.Deposit(dir.NewListBox(p))
	END DepositListBox;

	PROCEDURE DepositSelectionBox*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		Views.Deposit(dir.NewSelectionBox(p))
	END DepositSelectionBox;

	PROCEDURE DepositComboBox*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		Views.Deposit(dir.NewComboBox(p))
	END DepositComboBox;

	PROCEDURE DepositCancelButton*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		p.link := "StdCmds.CloseDialog"; p.label := "#System:Cancel"; p.opt[cancel] := TRUE;
		Views.Deposit(dir.NewPushButton(p))
	END DepositCancelButton;

	PROCEDURE DepositCaption*;
		VAR p: Prop;
	BEGIN
		InitProp(p); p.opt[left] := TRUE;
		p.label := "#System:Caption";
		Views.Deposit(dir.NewCaption(p))
	END DepositCaption;

	PROCEDURE DepositGroup*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		p.label := "#System:Caption";
		Views.Deposit(dir.NewGroup(p))
	END DepositGroup;
	
	PROCEDURE DepositTreeControl*;
		VAR p: Prop;
	BEGIN
		InitProp(p);
		p.opt[haslines] := TRUE; p.opt[hasbuttons] := TRUE; p.opt[atroot] := TRUE; p.opt[foldericons] := TRUE;
		Views.Deposit(dir.NewTreeControl(p))
	END DepositTreeControl;

	PROCEDURE Relink*;
		VAR msg: UpdateCachesMsg;
	BEGIN
		INC(stamp);
		Views.Omnicast(msg)
	END Relink;


	PROCEDURE Init;
		VAR d: StdDirectory;
	BEGIN
		par := NIL; stamp := 0;
		NEW(d); stdDir := d; dir := d;
		NEW(cleaner); cleanerInstalled := 0
	END Init;


	(* check guards action *)

	PROCEDURE (a: Action) Do;
		VAR msg: Views.NotifyMsg;
	BEGIN
		IF Windows.dir # NIL THEN
			IF a.w # NIL THEN
				INC(a.cnt);
				msg.id0 := 0; msg.id1 := 0; msg.opts := {guardCheck};
				IF a.w.seq # NIL THEN a.w.seq.Handle(msg) END;
				a.w := Windows.dir.Next(a.w);
				WHILE (a.w # NIL) & a.w.sub DO a.w := Windows.dir.Next(a.w) END
			ELSE
				IF a.cnt = 0 THEN a.resolution := Services.resolution
				ELSE a.resolution := Services.resolution DIV a.cnt DIV 2
				END;
				a.cnt := 0;
				a.w := Windows.dir.First();
				WHILE (a.w # NIL) & a.w.sub DO a.w := Windows.dir.Next(a.w) END
			END
		END;
		Services.DoLater(a, Services.Ticks() + a.resolution)
	END Do;

BEGIN
	Init;
	NEW(action); action.w := NIL; action.cnt := 0; Services.DoLater(action, Services.now)
CLOSE
	Services.RemoveAction(action)
END Controls.
