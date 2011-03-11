MODULE Windows;
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
		Kernel, Ports, Files, Services,
		Stores, Sequencers, Models, Views, Controllers, Properties,
		Dialog, Converters, Containers, Documents;

	CONST
		(** Window.flags **)
		isTool* = 0; isAux* = 1;
		noHScroll* = 2; noVScroll* = 3; noResize* = 4;
		allowDuplicates* = 5; neverDirty* = 6;

		(** Directory.Select lazy **)
		eager* = FALSE; lazy* = TRUE;

		notRecorded = 3;

	TYPE
		Window* = POINTER TO ABSTRACT RECORD
			port-: Ports.Port;
			frame-: Views.RootFrame;
			doc-: Documents.Document;
			seq-: Sequencers.Sequencer;
			link-: Window;	(* ring of windows with same sequencer *)
			sub-: BOOLEAN;
			flags-: SET;
			loc-: Files.Locator;
			name-: Files.Name;
			conv-: Converters.Converter
		END;

		Directory* = POINTER TO ABSTRACT RECORD
			l*, t*, r*, b*: INTEGER;
			minimized*, maximized*: BOOLEAN
		END;


		OpElem = POINTER TO RECORD
			next: OpElem;
			st: Stores.Store;
			op: Stores.Operation;
			name: Stores.OpName;
			invisible, transparent: BOOLEAN
		END;

		Script = POINTER TO RECORD (Stores.Operation)
			up: Script;
			list: OpElem;
			level: INTEGER;	(* nestLevel at creation time *)
			name: Stores.OpName
		END;

		StdSequencer = POINTER TO RECORD (Sequencers.Sequencer)
			home: Window;
			trapEra: INTEGER;	(* last observed TrapCount value *)
			modLevel: INTEGER;	(* dirty if modLevel > 0 *)
			entryLevel: INTEGER;	(* active = (entryLevel > 0) *)
			nestLevel: INTEGER;	(* nesting level of BeginScript/Modification *)
			modStack: ARRAY 64 OF RECORD store: Stores.Store; type: INTEGER END;
			lastSt: Stores.Store;
			lastOp: Stores.Operation;
			script: Script;
			undo, redo: OpElem;	(* undo/redo stacks *)
			noUndo: BOOLEAN;	(* script # NIL and BeginModification called *)
			invisibleLevel, transparentLevel, notRecordedLevel: INTEGER
		END;
		
		SequencerDirectory = POINTER TO RECORD (Sequencers.Directory) END;

		Forwarder = POINTER TO RECORD (Controllers.Forwarder) END;
		
		RootContext = POINTER TO RECORD (Models.Context)
			win: Window
		END;
		
		Reducer = POINTER TO RECORD (Kernel.Reducer) END;

		Hook = POINTER TO RECORD (Views.MsgHook) END;
		
		CheckAction = POINTER TO RECORD (Services.Action) 
			wait: WaitAction
		END;

		WaitAction = POINTER TO RECORD (Services.Action) 
			check: CheckAction
		END;
		
		LangNotifier = POINTER TO RECORD (Dialog.LangNotifier) END;

	VAR dir-, stdDir-: Directory;

	PROCEDURE ^ Reset (s: StdSequencer);


	PROCEDURE CharError;
	BEGIN
		Dialog.Beep
	END CharError;

	
	
	(** Window **)

	PROCEDURE (w: Window) Init* (port: Ports.Port), NEW;
	BEGIN
		ASSERT(w.port = NIL, 20); ASSERT(port # NIL, 21);
		w.port := port
	END Init;

	PROCEDURE (w: Window) SetTitle* (title: Views.Title), NEW, ABSTRACT;
	PROCEDURE (w: Window) GetTitle* (OUT title: Views.Title), NEW, ABSTRACT;
	PROCEDURE (w: Window) RefreshTitle* (), NEW, ABSTRACT;

	PROCEDURE (w: Window) SetSpec* (loc: Files.Locator; name: Files.Name; conv: Converters.Converter), NEW, EXTENSIBLE;
		VAR u: Window;
	BEGIN
		u := w;
		REPEAT
			u := u.link;
			u.loc := loc; u.name := name$; u.conv := conv
		UNTIL u = w
	END SetSpec;

	PROCEDURE (w: Window) Restore* (l, t, r, b: INTEGER), NEW;
		VAR f: Views.Frame; u, pw, ph: INTEGER;
	BEGIN
		f := w.frame;
		IF f # NIL THEN
			w.port.GetSize(pw, ph); u := w.port.unit;
			IF r > pw THEN r := pw END;
			IF b > ph THEN b := ph END;
			l := l * u - f.gx; t := t * u - f.gy; r := r * u - f.gx; b := b * u - f.gy;
			(* only adds to the BlackBox region, but doesn't draw: *)
			Views.UpdateRoot(w.frame, l, t, r, b, Views.keepFrames)	
		END
	END Restore;

	PROCEDURE (w: Window) Update*, NEW;
	BEGIN
		ASSERT(w.frame # NIL, 20);
		(* redraws the whole accumulated BlackBox region: *)
		Views.ValidateRoot(w.frame)
	END Update;

	PROCEDURE (w: Window) GetSize*(OUT width, height: INTEGER), NEW, EXTENSIBLE;
	BEGIN
		w.port.GetSize(width, height)
	END GetSize;
	
	PROCEDURE (w: Window) SetSize* (width, height: INTEGER), NEW, EXTENSIBLE;
		VAR c: Containers.Controller; w0, h0: INTEGER;
	BEGIN
		w.port.GetSize(w0, h0);
		w.port.SetSize(width, height);
		IF w.frame # NIL THEN Views.AdaptRoot(w.frame) END;
		c := w.doc.ThisController();
		IF c.opts * {Documents.winWidth, Documents.winHeight} # {} THEN
			w.Restore(0, 0, width, height)
		END
	END SetSize;

	PROCEDURE (w: Window) BroadcastModelMsg* (VAR msg: Models.Message), NEW, EXTENSIBLE;
	BEGIN
		IF w.frame # NIL THEN
			Views.BroadcastModelMsg(w.frame, msg)
		END
	END BroadcastModelMsg;

	PROCEDURE (w: Window) BroadcastViewMsg* (VAR msg: Views.Message), NEW, EXTENSIBLE;
	BEGIN
		IF w.frame # NIL THEN
			Views.BroadcastViewMsg(w.frame, msg)
		END
	END BroadcastViewMsg;

	PROCEDURE (w: Window) ForwardCtrlMsg* (VAR msg: Controllers.Message), NEW, EXTENSIBLE;
	BEGIN
		IF w.frame # NIL THEN
			WITH msg: Controllers.CursorMessage DO
				DEC(msg.x, w.frame.gx); DEC(msg.y, w.frame.gy)
			ELSE
			END;
			Views.ForwardCtrlMsg(w.frame, msg)
		END
	END ForwardCtrlMsg;

	PROCEDURE (w: Window) MouseDown* (x, y, time: INTEGER; modifiers: SET), NEW, ABSTRACT;

	PROCEDURE (w: Window) KeyDown* (ch: CHAR; modifiers: SET), NEW, EXTENSIBLE;
		VAR key: Controllers.EditMsg;
	BEGIN
		IF ch = 0X THEN
			CharError
		ELSE
			key.op := Controllers.pasteChar; key.char := ch;
			key.modifiers:= modifiers;
			w.ForwardCtrlMsg(key)
		END
	END KeyDown;

	PROCEDURE (w: Window) Close*, NEW, EXTENSIBLE;
		VAR u: Window; f: Views.Frame; s: Sequencers.Sequencer; msg: Sequencers.RemoveMsg;
	BEGIN
		u := w.link; WHILE u.link # w DO u := u.link END;
		u.link := w.link;
		f := w.frame; s := w.seq;
		IF ~w.sub THEN s.Notify(msg) END;
		WITH s: StdSequencer DO
			IF s.home = w THEN s.home := NIL END
		ELSE
		END;
		w.port.SetSize(0, 0); Views.AdaptRoot(w.frame);
		w.port := NIL; w.frame := NIL; w.doc := NIL; w.seq := NIL; w.link := NIL; w.loc := NIL;
		f.Close
	END Close;


	(** Directory **)

	PROCEDURE (d: Directory) NewSequencer* (): Sequencers.Sequencer, NEW;
		VAR s: StdSequencer;
	BEGIN
		NEW(s); Reset(s); RETURN s
	END NewSequencer;


	PROCEDURE (d: Directory) First* (): Window, NEW, ABSTRACT;
	PROCEDURE (d: Directory) Next* (w: Window): Window, NEW, ABSTRACT;

	PROCEDURE (d: Directory) New* (): Window, NEW, ABSTRACT;
	
	PROCEDURE (d: Directory) Open* (w: Window; doc: Documents.Document;
																		flags: SET; name: Views.Title;
																		loc: Files.Locator; fname: Files.Name;
																		conv: Converters.Converter),
																		NEW, EXTENSIBLE;
		VAR v: Views.View; c: RootContext; s: Sequencers.Sequencer; f: Views.Frame; any: ANYPTR;
	BEGIN
		ASSERT(w # NIL, 20); ASSERT(doc # NIL, 21); ASSERT(doc.context = NIL, 22);
		v := doc.ThisView(); ASSERT(v # NIL, 23);
		ASSERT(w.doc = NIL, 24); ASSERT(w.port # NIL, 25);
		IF w.link = NIL THEN w.link := w END;	(* create new window ring *)
		w.doc := doc; w.flags := flags;
		IF w.seq = NIL THEN
			ASSERT(doc.Domain() # NIL, 27);
			any := doc.Domain().GetSequencer();
			IF any # NIL THEN
				ASSERT(any IS Sequencers.Sequencer, 26);
				w.seq := any(Sequencers.Sequencer)
			ELSE
				w.seq := d.NewSequencer();
				doc.Domain().SetSequencer(w.seq)
			END			
		END;
		s := w.seq;
		WITH s: StdSequencer DO
			IF s.home = NIL THEN s.home := w END
		ELSE
		END;
		NEW(c); c.win := w; doc.InitContext(c);
		doc.GetNewFrame(f); w.frame := f(Views.RootFrame);
		w.frame.ConnectTo(w.port);
		Views.SetRoot(w.frame, w.doc, FALSE, w.flags);
		w.SetSpec(loc, fname, conv)
	END Open;

	PROCEDURE (d: Directory) OpenSubWindow* (w: Window; doc: Documents.Document; flags: SET; name: Views.Title), NEW, EXTENSIBLE;
		VAR u: Window; title: Views.Title;
	BEGIN
		ASSERT(w # NIL, 20); ASSERT(doc # NIL, 21);
		u := d.First(); WHILE (u # NIL) & (u.seq # doc.Domain().GetSequencer()) DO u := d.Next(u) END;
		IF u # NIL THEN
			w.sub := TRUE;
			w.link := u.link; u.link := w;
			w.seq := u.seq; w.loc := u.loc; w.name := u.name; w.conv := u.conv;
			u.GetTitle(title);
			d.Open(w, doc, flags, title, u.loc, u.name, u.conv)
		ELSE
			d.Open(w, doc, flags, name, NIL, "", NIL)
		END
	END OpenSubWindow;

	PROCEDURE ^ RestoreSequencer(seq: Sequencers.Sequencer);

	PROCEDURE (d: Directory) Focus* (target: BOOLEAN): Window, NEW, ABSTRACT;
	PROCEDURE (d: Directory) GetThisWindow* (p: Ports.Port; px, py: INTEGER; OUT x, y: INTEGER; OUT w: Window), NEW, ABSTRACT;
	PROCEDURE (d: Directory) Select* (w: Window; lazy: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (d: Directory) Close* (w: Window), NEW, ABSTRACT;

	PROCEDURE (d: Directory) Update* (w: Window), NEW;
		VAR u: Window;
	BEGIN
		(* redraws the BlackBox region of a given window, or of all windows *)
		u := d.First();
		WHILE u # NIL DO
			ASSERT(u.frame # NIL, 101);
			IF (u = w) OR (w = NIL) THEN RestoreSequencer(u.seq) END;
			u := d.Next(u)
		END
	END Update;
	
	PROCEDURE (d: Directory) GetBounds* (OUT w, h: INTEGER), NEW, ABSTRACT;


	(* RootContext *)

	PROCEDURE (c: RootContext) GetSize (OUT w, h: INTEGER);
	BEGIN
		c.win.port.GetSize(w, h);
		w := w * c.win.port.unit; h := h * c.win.port.unit
	END GetSize;

	PROCEDURE (c: RootContext) SetSize (w, h: INTEGER);
	END SetSize;
	
	PROCEDURE (c: RootContext) Normalize (): BOOLEAN;
	BEGIN
		RETURN TRUE
	END Normalize;
	
	PROCEDURE (c: RootContext) ThisModel (): Models.Model;
	BEGIN
		RETURN NIL
	END ThisModel;


	(* sequencing utilities *)

	PROCEDURE Prepend (s: Script; st: Stores.Store; IN name: Stores.OpName; op: Stores.Operation);
		VAR e: OpElem;
	BEGIN
		ASSERT(op # NIL, 20);
		NEW(e); e.st := st; e.op := op; e.name := name;
		e.next := s.list; s.list := e
	END Prepend;

	PROCEDURE Push (VAR list, e: OpElem);
	BEGIN
		e.next := list; list := e
	END Push;

	PROCEDURE Pop (VAR list, e: OpElem);
	BEGIN
		e := list; list := list.next
	END Pop;

	PROCEDURE Reduce (VAR list: OpElem; max: INTEGER);
		VAR e: OpElem;
	BEGIN
		e := list; WHILE (max > 1) & (e # NIL) DO DEC(max); e := e.next END;
		IF e # NIL THEN e.next := NIL END
	END Reduce;
	
	PROCEDURE (r: Reducer) Reduce (full: BOOLEAN);
		VAR e: OpElem; n: INTEGER; w: Window;
	BEGIN
		IF dir # NIL THEN
			w := dir.First();
			WHILE w # NIL DO
				IF w.seq IS StdSequencer THEN
					IF full THEN
						n := 1
					ELSE
						n := 0; e := w.seq(StdSequencer).undo;
						WHILE e # NIL DO INC(n); e := e.next END;
						IF n > 20 THEN n := n DIV 2 ELSE n := 10 END
					END;
					Reduce(w.seq(StdSequencer).undo, n)
				END;
				w := dir.Next(w)
			END
		END;
		Kernel.InstallReducer(r)
	END Reduce;

	PROCEDURE Reset (s: StdSequencer);
	BEGIN
		s.trapEra := Kernel.trapCount;
		IF (s.entryLevel # 0) OR (s.nestLevel # 0) THEN
			s.modLevel := 0;
			s.entryLevel := 0;
			s.nestLevel := 0;
			s.lastSt := NIL;
			s.lastOp := NIL;
			s.script := NIL;
			s.noUndo := FALSE;
			s.undo := NIL; s.redo := NIL;
			s.invisibleLevel := 0;
			s.transparentLevel := 0;
			s.notRecordedLevel := 0
		END
	END Reset;

	PROCEDURE Neutralize (st: Stores.Store);
		VAR neutralize: Models.NeutralizeMsg;
	BEGIN
		IF st # NIL THEN	(* st = NIL for scripts *)
			WITH st: Models.Model DO
				Models.Broadcast(st, neutralize)
			| st: Views.View DO
				st.Neutralize
			ELSE
			END
		END
	END Neutralize;

	PROCEDURE Do (s: StdSequencer; st: Stores.Store; op: Stores.Operation);
	BEGIN
		INC(s.entryLevel); s.lastSt := NIL; s.lastOp := NIL;
		Neutralize(st); op.Do;
		DEC(s.entryLevel)
	END Do;

	PROCEDURE AffectsDoc (s: StdSequencer; st: Stores.Store): BOOLEAN;
		VAR v, w: Window;
	BEGIN
		w := s.home;
		IF (w = NIL) OR (st = w.doc) OR (st = w.doc.ThisView()) THEN
			RETURN TRUE
		ELSE
			v := w.link;
			WHILE (v # w) & (st # v.doc) & (st # v.doc.ThisView()) DO v := v.link END;
			RETURN v = w
		END
	END AffectsDoc;


	(* Script *)

	PROCEDURE (s: Script) Do;
		VAR e, f, g: OpElem;
	BEGIN
		e := s.list; f := NIL;
		REPEAT
			Neutralize(e.st); e.op.Do;
			g := e.next; e.next := f; f := e; e := g
		UNTIL e = NIL;
		s.list := f
	END Do;


	(* StdSequencer *)

	PROCEDURE (s: StdSequencer) Handle (VAR msg: ANYREC);
	(* send message to all windows attached to s *)
		VAR w: Window;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		WITH msg: Models.Message DO
			IF msg IS Models.UpdateMsg THEN
				Properties.IncEra;
				IF s.entryLevel = 0 THEN
					(* updates in dominated model bypassed the sequencer *)
					Reset(s);	(* panic reset: clear sequencer *)
					INC(s.modLevel)	(* but leave dirty *)
				END
			END;
			w := dir.First();
			WHILE w # NIL DO
				IF w.seq = s THEN w.BroadcastModelMsg(msg) END;
				w := dir.Next(w)
			END
		| msg: Views.Message DO
			w := dir.First();
			WHILE w # NIL DO
				IF w.seq = s THEN w.BroadcastViewMsg(msg) END;
				w := dir.Next(w)
			END
		ELSE
		END
	END Handle;


	PROCEDURE (s: StdSequencer) Dirty (): BOOLEAN;
	BEGIN
		RETURN s.modLevel > 0
	END Dirty;

	PROCEDURE (s: StdSequencer) SetDirty (dirty: BOOLEAN);
	BEGIN
		IF dirty THEN INC(s.modLevel) ELSE s.modLevel := 0 END
	END SetDirty;

	PROCEDURE (s: StdSequencer) LastOp (st: Stores.Store): Stores.Operation;
	BEGIN
		ASSERT(st # NIL, 20);
		IF s.lastSt = st THEN RETURN s.lastOp ELSE RETURN NIL END
	END LastOp;


	PROCEDURE (s: StdSequencer) BeginScript (IN name: Stores.OpName; VAR script: Stores.Operation);
		VAR sop: Script;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		INC(s.nestLevel);
		IF (s.nestLevel = 1) & (s.invisibleLevel = 0) & (s.transparentLevel = 0) & (s.notRecordedLevel = 0) THEN
			INC(s.modLevel)
		END;
		s.lastSt := NIL; s.lastOp := NIL;
		NEW(sop); sop.up := s.script; sop.list := NIL; sop.level := s.nestLevel; sop.name := name;
		s.script := sop;
		script := sop
	END BeginScript;

	PROCEDURE (s: StdSequencer) Do (st: Stores.Store; IN name: Stores.OpName; op: Stores.Operation);
		VAR e: OpElem;
	BEGIN
		ASSERT(st # NIL, 20); ASSERT(op # NIL, 21);
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		Do(s, st, op);
		IF s.noUndo THEN	(* cannot undo: unbalanced BeginModification pending *)
			s.lastSt := NIL; s.lastOp := NIL
		ELSIF (s.entryLevel = 0)	(* don't record when called from within op.Do *)
		& AffectsDoc(s, st) THEN	(* don't record when Do affected child window only *)
			s.lastSt := st; s.lastOp := op;
			s.redo := NIL;	(* clear redo stack *)
			IF s.script # NIL THEN
				Prepend(s.script, st, name, op)
			ELSE
				IF (s.invisibleLevel = 0) & (s.transparentLevel = 0) & (s.notRecordedLevel = 0) THEN INC(s.modLevel) END;
				NEW(e); e.st := st; e.op := op; e.name := name;
				e.invisible := s.invisibleLevel > 0; e.transparent := s.transparentLevel > 0;
				IF (s.notRecordedLevel=0) THEN Push(s.undo, e) END
			END
		END
	END Do;

	PROCEDURE (s: StdSequencer) Bunch (st: Stores.Store);
		VAR lastOp: Stores.Operation;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		ASSERT(st # NIL, 20); ASSERT(st = s.lastSt, 21);
		lastOp := s.lastOp;
		Do(s, st, lastOp);
		IF s.noUndo THEN
			s.lastSt := NIL; s.lastOp := NIL
		ELSIF (s.entryLevel = 0)	(* don't record when called from within op.Do *)
		& AffectsDoc(s, st) THEN	(* don't record when Do affected child window only *)
			s.lastSt := st; s.lastOp := lastOp
		END
	END Bunch;

	PROCEDURE (s: StdSequencer) EndScript (script: Stores.Operation);
		VAR e: OpElem;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		ASSERT(script # NIL, 20); ASSERT(s.script = script, 21);
		WITH script: Script DO
			ASSERT(s.nestLevel = script.level, 22);
			s.script := script.up;
			IF s.entryLevel = 0 THEN	(* don't record when called from within op.Do *)
				IF script.list # NIL THEN
					IF s.script # NIL THEN
						Prepend(s.script, NIL, script.name, script)
					ELSE	(* outermost scripting level *)
						s.redo := NIL;	(* clear redo stack *)
						IF ~s.noUndo THEN
							NEW(e); e.st := NIL; e.op := script; e.name := script.name;
							e.invisible := s.invisibleLevel > 0; e.transparent := s.transparentLevel > 0;
							IF s.notRecordedLevel=0 THEN Push(s.undo, e) END
						END;
						s.lastSt := NIL; s.lastOp := NIL
					END
				ELSE
					IF (s.script = NIL) & (s.modLevel > 0) & (s.invisibleLevel = 0) & (s.transparentLevel = 0) THEN 
						DEC(s.modLevel)
					END
				END
			END
		END;
		DEC(s.nestLevel);
		IF s.nestLevel = 0 THEN ASSERT(s.script = NIL, 22); s.noUndo := FALSE END
	END EndScript;

	PROCEDURE (s: StdSequencer) StopBunching;
	BEGIN
		s.lastSt := NIL; s.lastOp := NIL
	END StopBunching;

	PROCEDURE (s: StdSequencer) BeginModification (type: INTEGER; st: Stores.Store);
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		IF s.nestLevel < LEN(s.modStack) THEN s.modStack[s.nestLevel].store := st; s.modStack[s.nestLevel].type := type END;
		INC(s.nestLevel);
		IF type = Sequencers.notUndoable THEN
			INC(s.modLevel);	(* unbalanced! *)
			s.noUndo := TRUE; s.undo := NIL; s.redo := NIL;
			s.lastSt := NIL; s.lastOp := NIL;
			INC(s.entryLevel)	(* virtual entry of modification "operation" *)
		ELSIF type = Sequencers.invisible THEN
			INC(s.invisibleLevel)
		ELSIF type = Sequencers.clean THEN
			INC(s.transparentLevel)
		ELSIF type = notRecorded THEN
			INC(s.notRecordedLevel)
		END
	END BeginModification;

	PROCEDURE (s: StdSequencer) EndModification (type: INTEGER; st: Stores.Store);
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		ASSERT(s.nestLevel > 0, 20);
		IF s.nestLevel <= LEN(s.modStack) THEN
			ASSERT((s.modStack[s.nestLevel - 1].store = st) & (s.modStack[s.nestLevel - 1].type = type), 21)
		END;
		DEC(s.nestLevel);
		IF type = Sequencers.notUndoable THEN
			DEC(s.entryLevel)
		ELSIF type = Sequencers.invisible THEN
			DEC(s.invisibleLevel)
		ELSIF type = Sequencers.clean THEN
			DEC(s.transparentLevel)
		ELSIF type = notRecorded THEN
			DEC(s.notRecordedLevel)
		END;
		IF s.nestLevel = 0 THEN ASSERT(s.script = NIL, 22); s.noUndo := FALSE END
	END EndModification;

	PROCEDURE (s: StdSequencer) CanUndo (): BOOLEAN;
		VAR op: OpElem;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		op := s.undo;
		WHILE (op # NIL) & op.invisible DO op := op.next END;
		RETURN op # NIL
	END CanUndo;

	PROCEDURE (s: StdSequencer) CanRedo (): BOOLEAN;
		VAR op: OpElem;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		op := s.redo;
		WHILE (op # NIL) & op.invisible DO op := op.next END;
		RETURN op # NIL
	END CanRedo;

	PROCEDURE (s: StdSequencer) GetUndoName (VAR name: Stores.OpName);
		VAR op: OpElem;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		op := s.undo;
		WHILE (op # NIL) & op.invisible DO op := op.next END;
		IF op # NIL THEN name := op.name$ ELSE name[0] := 0X END
	END GetUndoName;

	PROCEDURE (s: StdSequencer) GetRedoName (VAR name: Stores.OpName);
		VAR op: OpElem;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		op := s.redo;
		WHILE (op # NIL) & op.invisible DO op := op.next END;
		IF op # NIL THEN name := op.name$ ELSE name[0] := 0X END
	END GetRedoName;

	PROCEDURE (s: StdSequencer) Undo;
		VAR e: OpElem;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		IF s.undo # NIL THEN
			REPEAT
				Pop(s.undo, e); Do(s, e.st, e.op); Push(s.redo, e)
			UNTIL ~e.invisible OR (s.undo = NIL);
			IF ~e.transparent THEN
				IF s.modLevel > 0 THEN DEC(s.modLevel) END
			END
		END
	END Undo;

	PROCEDURE (s: StdSequencer) Redo;
		VAR e: OpElem;
	BEGIN
		IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
		IF s.redo # NIL THEN
			Pop(s.redo, e); Do(s, e.st, e.op); Push(s.undo, e);
			WHILE (s.redo # NIL) & s.redo.invisible DO
				Pop(s.redo, e); Do(s, e.st, e.op); Push(s.undo, e)
			END;
			IF ~e.transparent THEN
				INC(s.modLevel)
			END
		END
	END Redo;


	(* Forwarder *)

	PROCEDURE (f: Forwarder) Forward (target: BOOLEAN; VAR msg: Controllers.Message);
		VAR w: Window;
	BEGIN
		w := dir.Focus(target);
		IF w # NIL THEN w.ForwardCtrlMsg(msg) END
	END Forward;

	PROCEDURE (f: Forwarder) Transfer (VAR msg: Controllers.TransferMessage);
		VAR w: Window; h: Views.Frame; p: Ports.Port; sx, sy, tx, ty, pw, ph: INTEGER;
	BEGIN
		h := msg.source; p := h.rider.Base();
		(* (msg.x, msg.y) is point in local coordinates of source frame *)
		sx := (msg.x + h.gx) DIV h.unit;
		sy := (msg.y + h.gy) DIV h.unit;
		(* (sx, sy) is point in global coordinates of source port *)
		dir.GetThisWindow(p, sx, sy, tx, ty, w);
		IF w # NIL THEN
			(* (tx, ty) is point in global coordinates of target port *)
			w.port.GetSize(pw, ph);
			msg.x := tx * w.port.unit;
			msg.y := ty * w.port.unit;
			(* (msg.x, msg.y) is point in coordinates of target window *)
			w.ForwardCtrlMsg(msg)
		END
	END Transfer;


	(** miscellaneous **)

	PROCEDURE SetDir* (d: Directory);
	BEGIN
		ASSERT(d # NIL, 20);
		IF stdDir = NIL THEN stdDir := d END;
		dir := d
	END SetDir;

	PROCEDURE SelectBySpec* (loc: Files.Locator; name: Files.Name; conv: Converters.Converter; VAR done: BOOLEAN);
		VAR w: Window;
	BEGIN
		Kernel.MakeFileName(name, "");
		w := dir.First();
		WHILE (w # NIL) & ((loc = NIL) OR (w.loc = NIL) OR (loc.res = 77) OR  (w.loc.res = 77) OR
										 (name = "") OR (w.name = "") OR
										~Files.dir.SameFile(loc, name, w.loc, w.name) OR (w.conv # conv)) DO
			w := dir.Next(w)
		END;
		IF w # NIL THEN dir.Select(w, lazy); done := TRUE ELSE done := FALSE END
	END SelectBySpec;

	PROCEDURE SelectByTitle* (v: Views.View; flags: SET; title: Views.Title; VAR done: BOOLEAN);
		VAR w: Window; t: Views.Title; n1, n2: ARRAY 64 OF CHAR;
	BEGIN
		done := FALSE;
		IF v # NIL THEN
			IF v IS Documents.Document THEN v := v(Documents.Document).ThisView() END;
			Services.GetTypeName(v, n1)
		ELSE n1 := ""
		END;
		w := dir.First();
		WHILE w # NIL DO
			IF ((w.flags / flags) * {isAux, isTool} = {}) & ~(allowDuplicates IN w.flags) THEN
				w.GetTitle(t);
				IF t = title THEN
					Services.GetTypeName(w.doc.ThisView(), n2);
					IF (n1 = "") OR (n1 = n2) THEN dir.Select(w, lazy); done := TRUE; RETURN END
				END
			END;
			w := dir.Next(w)
		END
	END SelectByTitle;


	PROCEDURE (h: Hook) Omnicast (VAR msg: ANYREC);
		VAR w: Window;
	BEGIN
		w := dir.First();
		WHILE w # NIL DO
			IF ~w.sub THEN w.seq.Handle(msg) END;
			w := dir.Next(w)
		END
	END Omnicast;

	PROCEDURE RestoreSequencer (seq: Sequencers.Sequencer);
		VAR w: Window;
	BEGIN
		w := dir.First();
		WHILE w # NIL DO
			ASSERT(w.frame # NIL, 100);
			IF (seq = NIL) OR (w.seq = seq) THEN
				w.Update	(* causes redrawing of BlackBox region *)
			END;
			w := dir.Next(w)
		END
	END RestoreSequencer;

	PROCEDURE (h: Hook) RestoreDomain (d: Stores.Domain);
		VAR seq: ANYPTR;
	BEGIN
		IF d = NIL THEN
			RestoreSequencer(NIL)
		ELSE
			seq := d.GetSequencer();
			IF seq # NIL THEN
				RestoreSequencer (seq(Sequencers.Sequencer))
			END
		END
	END RestoreDomain;


	(* SequencerDirectory *)
	
	PROCEDURE (d: SequencerDirectory) New (): Sequencers.Sequencer;
	BEGIN
		RETURN dir.NewSequencer()
	END New;

	(** CheckAction **)
	
	PROCEDURE (a: CheckAction) Do;
		VAR w: Window; s: StdSequencer;
	BEGIN
		Services.DoLater(a.wait, Services.resolution);
		w := dir.First();
		WHILE w # NIL DO
			s := w.seq(StdSequencer);
			IF s.trapEra # Kernel.trapCount THEN Reset(s) END;
			ASSERT(s.nestLevel = 0, 100);
			(* unbalanced calls of Views.BeginModification/EndModification or Views.BeginScript/EndScript *)
			w := dir.Next(w)
		END
	END Do;
	
	PROCEDURE (a: WaitAction) Do;
	BEGIN
		Services.DoLater(a.check, Services.immediately)
	END Do;


	PROCEDURE (n: LangNotifier) Notify;
		VAR w: Window; pw, ph: INTEGER;
	BEGIN
		w := dir.First();
		WHILE w # NIL DO
			w.port.GetSize(pw, ph);
			w.Restore(0, 0, pw, ph);
			w.RefreshTitle;
			w := dir.Next(w)
		END
	END Notify;
	
	PROCEDURE Init;
		VAR f: Forwarder; r: Reducer; sdir: SequencerDirectory;
			a: CheckAction; w: WaitAction; h: Hook;  ln: LangNotifier;
	BEGIN
		NEW(sdir); Sequencers.SetDir(sdir);
		NEW(h); Views.SetMsgHook(h);
		NEW(f); Controllers.Register(f);
		NEW(r); Kernel.InstallReducer(r);
		NEW(a); NEW(w); a.wait := w; w.check := a; Services.DoLater(a, Services.immediately);
		NEW(ln); Dialog.RegisterLangNotifier(ln)
	END Init;

BEGIN
	Init
END Windows.
