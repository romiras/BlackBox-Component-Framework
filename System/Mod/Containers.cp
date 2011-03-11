MODULE Containers;
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

	IMPORT Kernel, Services, Ports, Dialog, Stores, Models, Views, Controllers, Properties, Mechanisms;

	CONST
		(** Controller.opts **)
		noSelection* = 0; noFocus* = 1; noCaret* = 2;
		mask* = {noSelection, noCaret}; layout* = {noFocus};
		modeOpts = {noSelection, noFocus, noCaret};

		(** Controller.SelectAll select **)
		deselect* = FALSE; select* = TRUE;

		(** Controller.PollNativeProp/etc. selection **)
		any* = FALSE; selection* = TRUE;

		(** Mark/MarkCaret/MarkSelection/MarkSingleton show **)
		hide* = FALSE; show* = TRUE;

		indirect = FALSE; direct = TRUE;

		TAB = 9X; LTAB = 0AX; ENTER = 0DX; ESC = 01BX;
		PL = 10X; PR = 11X; PU = 12X; PD = 13X;
		DL = 14X; DR = 15; DU = 16X; DD = 17X;
		AL = 1CX; AR = 1DX; AU = 1EX; AD = 1FX;

		minVersion = 0; maxModelVersion = 0; maxViewVersion = 0; maxCtrlVersion = 0;

		(* buttons *)
		left = 16; middle = 17; right = 18; alt = 28;	(* same as in HostPorts! *)


	TYPE
		Model* = POINTER TO ABSTRACT RECORD (Models.Model) END;

		View* = POINTER TO ABSTRACT RECORD (Views.View)
			model: Model;
			controller: Controller;
			alienCtrl: Stores.Store	(* alienCtrl = NIL  OR  controller = NIL *)
		END;

		Controller* = POINTER TO ABSTRACT RECORD (Controllers.Controller)
			opts-: SET;
			model: Model;	(* connected iff model # NIL *)
			view: View;
			focus, singleton: Views.View;
			bVis: BOOLEAN	(* control visibility of focus/singleton border *)
		END;

		Directory* = POINTER TO ABSTRACT RECORD END;

		PollFocusMsg = RECORD (Controllers.PollFocusMsg)
			all: BOOLEAN;
			ctrl: Controller
		END;
		
		ViewOp = POINTER TO RECORD (Stores.Operation)
			v: View;
			controller: Controller;	(* may be NIL *)
			alienCtrl: Stores.Store
		END;

		ControllerOp = POINTER TO RECORD (Stores.Operation)
			c: Controller;
			opts: SET
		END;

		ViewMessage = ABSTRACT RECORD (Views.Message) END;

		FocusMsg = RECORD (ViewMessage)
			set: BOOLEAN
		END;

		SingletonMsg = RECORD (ViewMessage)
			set: BOOLEAN
		END;

		FadeMsg = RECORD (ViewMessage)
			show: BOOLEAN
		END;
		
		DropPref* = RECORD (Properties.Preference)
			mode-: SET;
			okToDrop*: BOOLEAN
		END;
		
		GetOpts* = RECORD (Views.PropMessage)
			valid*, opts*: SET
		END;
		
		SetOpts* = RECORD (Views.PropMessage)
			valid*, opts*: SET
		END;
	

	PROCEDURE ^ (v: View) SetController* (c: Controller), NEW;
	PROCEDURE ^ (v: View) InitModel* (m: Model), NEW;

	PROCEDURE ^ Focus* (): Controller;
	PROCEDURE ^ ClaimFocus (v: Views.View): BOOLEAN;
	PROCEDURE ^ MarkFocus (c: Controller; f: Views.Frame; show: BOOLEAN);
	PROCEDURE ^ MarkSingleton* (c: Controller; f: Views.Frame; show: BOOLEAN);
	PROCEDURE ^ FadeMarks* (c: Controller; show: BOOLEAN);
	PROCEDURE ^ CopyView (source: Controller; VAR view: Views.View; VAR w, h: INTEGER);
	PROCEDURE ^ ThisProp (c: Controller; direct: BOOLEAN): Properties.Property;
	PROCEDURE ^ SetProp (c: Controller; old, p: Properties.Property; direct: BOOLEAN);


	PROCEDURE ^ (c: Controller) InitView* (v: Views.View), NEW;
	PROCEDURE (c: Controller) InitView2* (v: Views.View), NEW, EMPTY;
	PROCEDURE ^ (c: Controller) ThisView* (): View, NEW, EXTENSIBLE;
	PROCEDURE ^ (c: Controller) ThisFocus* (): Views.View, NEW, EXTENSIBLE;
	PROCEDURE ^ (c: Controller) ConsiderFocusRequestBy* (view: Views.View), NEW;
	PROCEDURE ^ (c: Controller) RestoreMarks* (f: Views.Frame; l, t, r, b: INTEGER), NEW;
	PROCEDURE ^ (c: Controller) Neutralize*, NEW;
	(** called by view's Neutralize **)
	PROCEDURE ^ (c: Controller) HandleModelMsg* (VAR msg: Models.Message), NEW, EXTENSIBLE;
	(** called by view's HandleModelMsg after handling msg **)
	PROCEDURE ^ (c: Controller) HandleViewMsg* (f: Views.Frame; VAR msg: Views.Message), NEW, EXTENSIBLE;
	(** called by view's HandleViewMsg after handling msg **)
	PROCEDURE ^ (c: Controller) HandleCtrlMsg* (f: Views.Frame; VAR msg: Controllers.Message; VAR focus: Views.View), NEW, EXTENSIBLE;
	(** called by view's HandleCtrlMsg *before* handling msg; focus is respected/used by view **)
	PROCEDURE ^ (c: Controller) HandlePropMsg* (VAR msg: Views.PropMessage), NEW, EXTENSIBLE;
	(** called by view's HandlePropMsg after handling msg; controller can override view **)

	(** Model **)

	PROCEDURE (m: Model) Internalize- (VAR rd: Stores.Reader), EXTENSIBLE;
		VAR thisVersion: INTEGER;
	BEGIN
		m.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxModelVersion, thisVersion)
	END Internalize;

	PROCEDURE (m: Model) Externalize- (VAR wr: Stores.Writer), EXTENSIBLE;
	BEGIN
		m.Externalize^(wr);
		wr.WriteVersion(maxModelVersion)
	END Externalize;

	PROCEDURE (m: Model) GetEmbeddingLimits* (OUT minW, maxW, minH, maxH: INTEGER), NEW, ABSTRACT;
	PROCEDURE (m: Model) ReplaceView* (old, new: Views.View), NEW, ABSTRACT;
	PROCEDURE (m: Model) InitFrom- (source: Model), NEW, EMPTY;

	(** View **)

	PROCEDURE (v: View) AcceptableModel- (m: Model): BOOLEAN, NEW, ABSTRACT;
	PROCEDURE (v: View) InitModel2- (m: Model), NEW, EMPTY;
	PROCEDURE (v: View) InitModel* (m: Model), NEW;
	BEGIN
		ASSERT((v.model = NIL) OR (v.model = m), 20);
		ASSERT(m # NIL, 21);
		ASSERT(v.AcceptableModel(m), 22);
		v.model := m;
		Stores.Join(v, m);
		v.InitModel2(m)
	END InitModel;
	
	
	PROCEDURE (v: View) Externalize2- (VAR rd: Stores.Writer), NEW, EMPTY;
	PROCEDURE(v: View) Internalize2- (VAR rd: Stores.Reader), NEW, EMPTY;

	PROCEDURE (v: View) Internalize- (VAR rd: Stores.Reader);
		VAR st: Stores.Store; c: Controller; m: Model; thisVersion: INTEGER;
	BEGIN
		v.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxViewVersion, thisVersion);
		IF rd.cancelled THEN RETURN END;
		rd.ReadStore(st); ASSERT(st # NIL, 100);
		IF ~(st IS Model) THEN
			rd.TurnIntoAlien(Stores.alienComponent);
			Stores.Report("#System:AlienModel", "", "", "");
			RETURN
		END;
		m := st(Model);
		rd.ReadStore(st);
		IF st = NIL THEN c := NIL; v.alienCtrl := NIL
		ELSIF st IS Stores.Alien THEN
			c := NIL; v.alienCtrl := st; Stores.Join(v, v.alienCtrl);
			Stores.Report("#System:AlienControllerWarning", "", "", "")
		ELSE c := st(Controller); v.alienCtrl := NIL
		END;
		v.InitModel(m);
		IF c # NIL THEN v.SetController(c) ELSE v.controller := NIL END;
		v.Internalize2(rd)
	END Internalize;

	PROCEDURE (v: View) Externalize- (VAR wr: Stores.Writer);
	BEGIN
		ASSERT(v.model # NIL, 20);
		v.Externalize^(wr);
		wr.WriteVersion(maxViewVersion);
		wr.WriteStore(v.model);
		IF v.controller # NIL THEN wr.WriteStore(v.controller)
		ELSE wr.WriteStore(v.alienCtrl)
		END;
		v.Externalize2(wr)
	END Externalize;

	PROCEDURE (v: View) CopyFromModelView2- (source: Views.View; model: Models.Model), NEW, EMPTY;
	
	PROCEDURE (v: View) CopyFromModelView- (source: Views.View; model: Models.Model);
		VAR c: Controller;
	BEGIN
		WITH source: View DO
			v.InitModel(model(Model));
			IF source.controller # NIL THEN
				c := Stores.CopyOf(source.controller)(Controller)
			ELSE
				c := NIL
			END;
			IF source.alienCtrl # NIL THEN v.alienCtrl := Stores.CopyOf(source.alienCtrl)(Stores.Alien) END;
			IF c # NIL THEN v.SetController(c) ELSE v.controller := NIL END
		END;
		v.CopyFromModelView2(source, model)
	END CopyFromModelView;

	PROCEDURE (v: View) ThisModel* (): Model, EXTENSIBLE;
	BEGIN
		RETURN v.model
	END ThisModel;

	PROCEDURE (v: View) SetController* (c: Controller), NEW;
		VAR op: ViewOp;
	BEGIN
		ASSERT(v.model # NIL, 20);
		IF v.controller # c THEN
			Stores.Join(v, c);
			NEW(op); op.v := v; op.controller := c; op.alienCtrl := NIL;
			Views.Do(v, "#System:ViewSetting", op)
		END
	END SetController;

	PROCEDURE (v: View) ThisController* (): Controller, NEW, EXTENSIBLE;
	BEGIN
		RETURN v.controller
	END ThisController;
	
	PROCEDURE (v: View) GetRect* (f: Views.Frame; view: Views.View; OUT l, t, r, b: INTEGER), NEW, ABSTRACT;

	PROCEDURE (v: View) RestoreMarks* (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		IF v.controller # NIL THEN v.controller.RestoreMarks(f, l, t, r, b) END
	END RestoreMarks;

	PROCEDURE (v: View) Neutralize*;
	BEGIN
		IF v.controller # NIL THEN v.controller.Neutralize END
	END Neutralize;

	PROCEDURE (v: View) ConsiderFocusRequestBy- (view: Views.View);
	BEGIN
		IF v.controller # NIL THEN v.controller.ConsiderFocusRequestBy(view) END
	END ConsiderFocusRequestBy;


	PROCEDURE (v: View) HandleModelMsg2- (VAR msg: Models.Message), NEW, EMPTY;
	PROCEDURE (v: View) HandleViewMsg2- (f: Views.Frame; VAR msg: Views.Message), NEW, EMPTY;
	PROCEDURE (v: View) HandlePropMsg2- (VAR p: Properties.Message), NEW, EMPTY;
	PROCEDURE (v: View) HandleCtrlMsg2- (f: Views.Frame; VAR msg: Controllers.Message; 
																					VAR focus: Views.View), NEW, EMPTY;


	PROCEDURE (v: View) HandleModelMsg- (VAR msg: Models.Message);
	BEGIN
		v.HandleModelMsg2(msg);
		IF v.controller # NIL THEN v.controller.HandleModelMsg(msg) END
	END HandleModelMsg;

	PROCEDURE (v: View) HandleViewMsg- (f: Views.Frame; VAR msg: Views.Message);
	BEGIN
		v.HandleViewMsg2(f, msg);
		IF v.controller # NIL THEN v.controller.HandleViewMsg(f, msg) END
	END HandleViewMsg;

	PROCEDURE (v: View) HandleCtrlMsg* (f: Views.Frame; VAR msg: Controllers.Message; VAR focus: Views.View);
	BEGIN
		IF v.controller # NIL THEN v.controller.HandleCtrlMsg(f, msg, focus) END;
		v.HandleCtrlMsg2(f, msg, focus);
		WITH msg: Controllers.PollSectionMsg DO
			IF ~msg.focus THEN focus := NIL END
		| msg: Controllers.ScrollMsg DO
			IF ~msg.focus THEN focus := NIL END
		ELSE
		END
	END HandleCtrlMsg;

	PROCEDURE (v: View) HandlePropMsg- (VAR p: Properties.Message);
	BEGIN
		v.HandlePropMsg2(p);
		IF v.controller # NIL THEN v.controller.HandlePropMsg(p) END
	END HandlePropMsg ;


	(** Controller **)

	PROCEDURE (c: Controller) Externalize2- (VAR rd: Stores.Writer), NEW, EMPTY;
	PROCEDURE(c: Controller) Internalize2- (VAR rd: Stores.Reader), NEW, EMPTY;

	PROCEDURE (c: Controller) Internalize- (VAR rd: Stores.Reader);
		VAR v: INTEGER;
	BEGIN
		c.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxCtrlVersion, v);
		IF rd.cancelled THEN RETURN END;
		rd.ReadSet(c.opts);
		c.Internalize2(rd)
	END Internalize;

	PROCEDURE (c: Controller) Externalize- (VAR wr: Stores.Writer);
	BEGIN
		c.Externalize^(wr);
		wr.WriteVersion(maxCtrlVersion);
		wr.WriteSet(c.opts);
		c.Externalize2(wr)
	END Externalize;

	PROCEDURE (c: Controller) CopyFrom- (source: Stores.Store), EXTENSIBLE;
	BEGIN
		WITH source: Controller DO
			c.opts := source.opts;
			c.focus := NIL; c.singleton := NIL;
			c.bVis := FALSE
		END
	END CopyFrom;

	PROCEDURE (c: Controller) InitView* (v: Views.View), NEW;
		VAR view: View; model: Model;
	BEGIN
		ASSERT((v = NIL) # (c.view = NIL) OR (v = c.view), 21);
		IF c.view = NIL THEN
			ASSERT(v IS View, 22);	(* subclass may assert narrower type *)
			view := v(View);
			model := view.ThisModel(); ASSERT(model # NIL, 24);
			c.view := view; c.model := model;
			Stores.Join(c, c.view)
		ELSE
			c.view.Neutralize; c.view := NIL; c.model := NIL
		END;
		c.focus := NIL; c.singleton := NIL; c.bVis := FALSE;
		c.InitView2(v)
	END InitView;

	PROCEDURE (c: Controller) ThisView* (): View, NEW, EXTENSIBLE;
	BEGIN
		RETURN c.view
	END ThisView;


	(** options **)

	PROCEDURE (c: Controller) SetOpts* (opts: SET), NEW, EXTENSIBLE;
		VAR op: ControllerOp;
	BEGIN
		IF c.view # NIL THEN
			NEW(op); op.c := c; op.opts := opts;
			Views.Do(c.view, "#System:ChangeOptions", op)
		ELSE
			c.opts := opts
		END
	END SetOpts;


	(** subclass hooks **)

	PROCEDURE (c: Controller) GetContextType* (OUT type: Stores.TypeName), NEW, ABSTRACT;
	PROCEDURE (c: Controller) GetValidOps* (OUT valid: SET), NEW, ABSTRACT;
	PROCEDURE (c: Controller) NativeModel* (m: Models.Model): BOOLEAN, NEW, ABSTRACT;
	PROCEDURE (c: Controller) NativeView* (v: Views.View): BOOLEAN, NEW, ABSTRACT;
	PROCEDURE (c: Controller) NativeCursorAt* (f: Views.Frame; x, y: INTEGER): INTEGER, NEW, ABSTRACT;
	PROCEDURE (c: Controller) PickNativeProp* (f: Views.Frame; x, y: INTEGER; VAR p: Properties.Property), NEW, EMPTY;
	PROCEDURE (c: Controller) PollNativeProp* (selection: BOOLEAN; VAR p: Properties.Property; VAR truncated: BOOLEAN), NEW, EMPTY;
	PROCEDURE (c: Controller) SetNativeProp* (selection: BOOLEAN; old, p: Properties.Property), NEW, EMPTY;

	PROCEDURE (c: Controller) MakeViewVisible* (v: Views.View), NEW, EMPTY;
	
	PROCEDURE (c: Controller) GetFirstView* (selection: BOOLEAN; OUT v: Views.View), NEW, ABSTRACT;
	PROCEDURE (c: Controller) GetNextView* (selection: BOOLEAN; VAR v: Views.View), NEW, ABSTRACT;

	PROCEDURE (c: Controller) GetPrevView* (selection: BOOLEAN; VAR v: Views.View), NEW, EXTENSIBLE;
		VAR p, q: Views.View;
	BEGIN
		ASSERT(v # NIL, 20);
		c.GetFirstView(selection, p);
		IF p # v THEN
			WHILE (p # NIL) & (p # v) DO q := p; c.GetNextView(selection, p) END;
			ASSERT(p # NIL, 21);
			v := q
		ELSE
			v := NIL
		END
	END GetPrevView;
	
	PROCEDURE (c: Controller) CanDrop* (f: Views.Frame; x, y: INTEGER): BOOLEAN, NEW, EXTENSIBLE;
	BEGIN
		RETURN TRUE
	END CanDrop;

	PROCEDURE (c: Controller) GetSelectionBounds* (f: Views.Frame; OUT x, y, w, h: INTEGER), NEW, EXTENSIBLE;
		VAR g: Views.Frame; v: Views.View;
	BEGIN
		x := 0; y := 0; w := 0; h := 0;
		v := c.singleton;
		IF v # NIL THEN
			g := Views.ThisFrame(f, v);
			IF g # NIL THEN
				x := g.gx - f.gx; y := g.gy - f.gy;
				v.context.GetSize(w, h)
			END
		END
	END GetSelectionBounds;

	PROCEDURE (c: Controller) MarkDropTarget* (src, dst: Views.Frame;
															sx, sy, dx, dy, w, h, rx, ry: INTEGER;
															type: Stores.TypeName;
															isSingle, show: BOOLEAN), NEW, EMPTY;

	PROCEDURE (c: Controller) Drop* (src, dst: Views.Frame; sx, sy, dx, dy, w, h, rx, ry: INTEGER;
													view: Views.View; isSingle: BOOLEAN), NEW, ABSTRACT;

	PROCEDURE (c: Controller) MarkPickTarget* (src, dst: Views.Frame;
															sx, sy, dx, dy: INTEGER; show: BOOLEAN), NEW, EMPTY;

	PROCEDURE (c: Controller) TrackMarks* (f: Views.Frame; x, y: INTEGER; units, extend, add: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (c: Controller) Resize* (view: Views.View; l, t, r, b: INTEGER), NEW, ABSTRACT;
	PROCEDURE (c: Controller) DeleteSelection*, NEW, ABSTRACT;
	PROCEDURE (c: Controller) MoveLocalSelection* (src, dst: Views.Frame; sx, sy, dx, dy: INTEGER), NEW, ABSTRACT;
	PROCEDURE (c: Controller) CopyLocalSelection* (src, dst: Views.Frame; sx, sy, dx, dy: INTEGER), NEW, ABSTRACT;
	PROCEDURE (c: Controller) SelectionCopy* (): Model, NEW, ABSTRACT;
	PROCEDURE (c: Controller) NativePaste* (m: Models.Model; f: Views.Frame), NEW, ABSTRACT;
	PROCEDURE (c: Controller) ArrowChar* (f: Views.Frame; ch: CHAR; units, select: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (c: Controller) ControlChar* (f: Views.Frame; ch: CHAR), NEW, ABSTRACT;
	PROCEDURE (c: Controller) PasteChar* (ch: CHAR), NEW, ABSTRACT;
	PROCEDURE (c: Controller) PasteView* (f: Views.Frame; v: Views.View; w, h: INTEGER), NEW, ABSTRACT;


	(** selection **)

	PROCEDURE (c: Controller) HasSelection* (): BOOLEAN, NEW, EXTENSIBLE;
	(** extended by subclass to include intrinsic selections **)
	BEGIN
		ASSERT(c.model # NIL, 20);
		RETURN c.singleton # NIL
	END HasSelection;

	PROCEDURE (c: Controller) Selectable* (): BOOLEAN, NEW, ABSTRACT;

	PROCEDURE (c: Controller) Singleton* (): Views.View, NEW;	(* LEAF *)
	BEGIN
		IF c = NIL THEN RETURN NIL
		ELSE RETURN c.singleton
		END
	END Singleton;

	PROCEDURE (c: Controller) SetSingleton* (s: Views.View), NEW, EXTENSIBLE;
	(** extended by subclass to adjust intrinsic selections **)
		VAR con: Models.Context; msg: SingletonMsg;
	BEGIN
		ASSERT(c.model # NIL, 20);
		ASSERT(~(noSelection IN c.opts), 21);
		IF c.singleton # s THEN
			IF s # NIL THEN
				con := s.context;
				ASSERT(con # NIL, 22); ASSERT(con.ThisModel() = c.model, 23);
				c.view.Neutralize
			ELSIF c.singleton # NIL THEN
				c.bVis := FALSE; msg.set := FALSE; Views.Broadcast(c.view, msg)
			END;
			c.singleton := s;
			IF s # NIL THEN c.bVis := TRUE; msg.set := TRUE; Views.Broadcast(c.view, msg) END
		END
	END SetSingleton;
	
	PROCEDURE (c: Controller) SelectAll* (select: BOOLEAN), NEW, ABSTRACT;
	(** replaced by subclass to include intrinsic selections **)

	PROCEDURE (c: Controller) InSelection* (f: Views.Frame; x, y: INTEGER): BOOLEAN, NEW, ABSTRACT;
	(** replaced by subclass to include intrinsic selections **)

	PROCEDURE (c: Controller) MarkSelection* (f: Views.Frame; show: BOOLEAN), NEW, EXTENSIBLE;
	(** replaced by subclass to include intrinsic selections **)
	BEGIN
		MarkSingleton(c, f, show)
	END MarkSelection;


	(** focus **)

	PROCEDURE (c: Controller) ThisFocus* (): Views.View, NEW, EXTENSIBLE;
	BEGIN
		ASSERT(c.model # NIL, 20);
		RETURN c.focus
	END ThisFocus;

	PROCEDURE (c: Controller) SetFocus* (focus: Views.View), NEW;	(* LEAF *)
		VAR focus0: Views.View; con: Models.Context; msg: FocusMsg;
	BEGIN
		ASSERT(c.model # NIL, 20);
		focus0 := c.focus;
		IF focus # focus0 THEN
			IF focus # NIL THEN
				con := focus.context;
				ASSERT(con # NIL, 21); ASSERT(con.ThisModel() = c.model, 22);
				IF focus0 = NIL THEN c.view.Neutralize END
			END;
			IF focus0 # NIL THEN
				IF ~Views.IsInvalid(focus0) THEN focus0.Neutralize END;
				c.bVis := FALSE; msg.set := FALSE; Views.Broadcast(c.view, msg)
			END;
			c.focus := focus;
			IF focus # NIL THEN
				c.MakeViewVisible(focus);
				c.bVis := TRUE; msg.set := TRUE; Views.Broadcast(c.view, msg)
			END
		END
	END SetFocus;

	PROCEDURE (c: Controller) ConsiderFocusRequestBy* (view: Views.View), NEW;
		VAR con: Models.Context;
	BEGIN
		ASSERT(c.model # NIL, 20);
		ASSERT(view # NIL, 21); con := view.context;
		ASSERT(con # NIL, 22); ASSERT(con.ThisModel() = c.model, 23);
		IF c.focus = NIL THEN c.SetFocus(view) END
	END ConsiderFocusRequestBy;


	(** caret **)

	PROCEDURE (c: Controller) HasCaret* (): BOOLEAN, NEW, ABSTRACT;
	PROCEDURE (c: Controller) MarkCaret* (f: Views.Frame; show: BOOLEAN), NEW, ABSTRACT;


	(** general marking protocol **)

	PROCEDURE CheckMaskFocus (c: Controller; f: Views.Frame; VAR focus: Views.View);
		VAR v: Views.View;
	BEGIN
		IF f.mark & (c.opts * modeOpts = mask) & (c.model # NIL) & ((focus = NIL) OR ~ClaimFocus(focus)) THEN
			c.GetFirstView(any, v);
			WHILE (v # NIL) & ~ClaimFocus(v) DO c.GetNextView(any, v) END;
			IF v # NIL THEN
				c.SetFocus(v);
				focus := v
			ELSE c.SetFocus(NIL); focus := NIL
			END
		END
	END CheckMaskFocus;
	
	PROCEDURE (c: Controller) Mark* (f: Views.Frame; l, t, r, b: INTEGER; show: BOOLEAN), NEW, EXTENSIBLE;
	BEGIN
		MarkFocus(c, f, show); c.MarkSelection(f, show); c.MarkCaret(f, show)
	END Mark;

	PROCEDURE (c: Controller) RestoreMarks2- (f: Views.Frame; l, t, r, b: INTEGER), NEW, EMPTY;
	PROCEDURE (c: Controller) RestoreMarks* (f: Views.Frame; l, t, r, b: INTEGER), NEW;
	BEGIN
		IF f.mark THEN
			c.Mark(f, l, t, r, b, show);
			c.RestoreMarks2(f, l, t, r, b)
		END
	END RestoreMarks;

	PROCEDURE (c: Controller) Neutralize2-, NEW, EMPTY;
	(** caret needs to be removed by this method **)

	PROCEDURE (c: Controller) Neutralize*, NEW;
	BEGIN
		c.SetFocus(NIL); c.SelectAll(deselect);
		c.Neutralize2
	END Neutralize;


	(** message handlers **)

	PROCEDURE (c: Controller) HandleModelMsg* (VAR msg: Models.Message), NEW, EXTENSIBLE;
	BEGIN
		ASSERT(c.model # NIL, 20)
	END HandleModelMsg;

	PROCEDURE (c: Controller) HandleViewMsg* (f: Views.Frame; VAR msg: Views.Message), NEW, EXTENSIBLE;
		VAR g: Views.Frame; mark: Controllers.MarkMsg;
	BEGIN
		ASSERT(c.model # NIL, 20);
		IF msg.view = c.view THEN
			WITH msg: ViewMessage DO
				WITH msg: FocusMsg DO
					g := Views.ThisFrame(f, c.focus);
					IF g # NIL THEN
						IF msg.set THEN
							MarkFocus(c, f, show);
							mark.show := TRUE; mark.focus := TRUE;
							Views.ForwardCtrlMsg(g, mark)
						ELSE
							mark.show := FALSE; mark.focus := TRUE;
							Views.ForwardCtrlMsg(g, mark);
							MarkFocus(c, f, hide)
						END
					END
				| msg: SingletonMsg DO
					MarkSingleton(c, f, msg.set)
				| msg: FadeMsg DO
					MarkFocus(c, f, msg.show);
					MarkSingleton(c, f, msg.show)
				END
			ELSE
			END
		END
	END HandleViewMsg;


	PROCEDURE CollectControlPref (c: Controller; focus: Views.View; ch: CHAR; cyclic: BOOLEAN;
												VAR v: Views.View; VAR getFocus, accepts: BOOLEAN);
		VAR first, w: Views.View; p: Properties.ControlPref; back: BOOLEAN;
	BEGIN
		back := (ch = LTAB) OR (ch = AL) OR (ch = AU); first := c.focus;
		IF first = NIL THEN
			c.GetFirstView(any, first);
			IF back THEN w := first;
				WHILE w # NIL DO first := w; c.GetNextView(any, w) END
			END
		END;
		v := first;
		WHILE v # NIL DO
			p.char := ch; p.focus := focus;
			p.getFocus := (v # focus) & ((ch = TAB) OR (ch = LTAB)) & ClaimFocus(v);
			p.accepts := (v = focus) & (ch # TAB) & (ch # LTAB);
			Views.HandlePropMsg(v, p);
			IF p.accepts OR (v # focus) & p.getFocus THEN
				getFocus := p.getFocus; accepts := p.accepts;
				RETURN
			END;
			IF back THEN c.GetPrevView(any, v) ELSE c.GetNextView(any, v) END;
			IF cyclic & (v = NIL) THEN
				c.GetFirstView(any, v);
				IF back THEN w := v;
					WHILE w # NIL DO v := w; c.GetNextView(any, w) END
				END
			END;
			IF v = first THEN v := NIL END
		END;
		getFocus := FALSE; accepts := FALSE
	END CollectControlPref;
	
	PROCEDURE (c: Controller) HandlePropMsg* (VAR msg: Properties.Message), NEW, EXTENSIBLE;
		VAR v: Views.View;
	BEGIN
		ASSERT(c.model # NIL, 20);
		WITH msg: Properties.PollMsg DO
			msg.prop := ThisProp(c, indirect)
		| msg: Properties.SetMsg DO
			SetProp(c, msg.old, msg.prop, indirect)
		| msg: Properties.FocusPref DO
			IF {noSelection, noFocus, noCaret} - c.opts # {} THEN msg.setFocus := TRUE END
		| msg: GetOpts DO
			msg.valid := modeOpts; msg.opts := c.opts
		| msg: SetOpts DO
			c.SetOpts(c.opts - msg.valid + (msg.opts * msg.valid))
		| msg: Properties.ControlPref DO
			IF c.opts * modeOpts = mask THEN
				v := msg.focus;
				IF v = c.view THEN v := c.focus END;
				CollectControlPref(c, v, msg.char, FALSE, v, msg.getFocus, msg.accepts);
				IF msg.getFocus THEN msg.accepts := TRUE END
			END
		ELSE
		END
	END HandlePropMsg;


	(** Directory **)

	PROCEDURE (d: Directory) NewController* (opts: SET): Controller, NEW, ABSTRACT;

	PROCEDURE (d: Directory) New* (): Controller, NEW, EXTENSIBLE;
	BEGIN
		RETURN d.NewController({})
	END New;


	(* ViewOp *)

	PROCEDURE (op: ViewOp) Do;
		VAR v: View; c0, c1: Controller; a0, a1: Stores.Store;
	BEGIN
		v := op.v; c0 := v.controller; a0 := v.alienCtrl; c1 := op.controller; a1 := op.alienCtrl;
		IF c0 # NIL THEN c0.InitView(NIL) END;
		v.controller := c1; v.alienCtrl := a1;
		op.controller := c0; op.alienCtrl := a0;
		IF c1 # NIL THEN c1.InitView(v) END;
		Views.Update(v, Views.keepFrames)
	END Do;


	(* ControllerOp *)

	PROCEDURE (op: ControllerOp) Do;
		VAR c: Controller; opts: SET;
	BEGIN
		c := op.c;
		opts := c.opts; c.opts := op.opts; op.opts := opts;
		Views.Update(c.view, Views.keepFrames)
	END Do;


	(* Controller implementation support *)

	PROCEDURE BorderVisible (c: Controller; f: Views.Frame): BOOLEAN;
	BEGIN
		IF 31 IN c.opts THEN RETURN TRUE END;
		IF f IS Views.RootFrame THEN RETURN FALSE END;
		IF Services.Is(c.focus, "OleClient.View") THEN RETURN FALSE END;
		RETURN TRUE
	END BorderVisible;

	PROCEDURE MarkFocus (c: Controller; f: Views.Frame; show: BOOLEAN);
		VAR focus: Views.View; f1: Views.Frame; l, t, r, b: INTEGER;
	BEGIN
		focus := c.focus;
		IF f.front & (focus # NIL) & (~show OR c.bVis) & BorderVisible(c, f) & ~(noSelection IN c.opts) THEN
			f1 := Views.ThisFrame(f, focus);
			IF f1 # NIL THEN
				c.bVis := show;
				c.view.GetRect(f, focus, l, t, r, b);
				IF (l # MAX(INTEGER)) & (t # MAX(INTEGER)) THEN
					Mechanisms.MarkFocusBorder(f, focus, l, t, r, b, show)
				END
			END
		END
	END MarkFocus;

	PROCEDURE MarkSingleton* (c: Controller; f: Views.Frame; show: BOOLEAN);
		VAR l, t, r, b: INTEGER;
	BEGIN
		IF (*(f.front OR f.target) &*) (~show OR c.bVis) & (c.singleton # NIL) THEN
			c.bVis := show;
			c.view.GetRect(f, c.singleton, l, t, r, b);
			IF (l # MAX(INTEGER)) & (t # MAX(INTEGER)) THEN
				Mechanisms.MarkSingletonBorder(f, c.singleton, l, t, r, b, show)
			END
		END
	END MarkSingleton;

	PROCEDURE FadeMarks* (c: Controller; show: BOOLEAN);
		VAR msg: FadeMsg; v: Views.View; fc: Controller;
	BEGIN
		IF (c.focus # NIL) OR (c.singleton # NIL) THEN
			IF c.bVis # show THEN
				IF ~show THEN
					v := c.focus;
					WHILE (v # NIL) & (v IS View) DO
						fc := v(View).ThisController();
						fc.bVis := FALSE; v := fc.focus
					END
				END;
				c.bVis := show; msg.show := show; Views.Broadcast(c.view, msg)
			END
		END
	END FadeMarks;


	(* handle controller messages in editor mode *)

	PROCEDURE ClaimFocus (v: Views.View): BOOLEAN;
		VAR p: Properties.FocusPref;
	BEGIN
		p.atLocation := FALSE;
		p.hotFocus := FALSE; p.setFocus := FALSE;
		Views.HandlePropMsg(v, p);
		RETURN p.setFocus
	END ClaimFocus;
	
	PROCEDURE ClaimFocusAt (v: Views.View; f, g: Views.Frame; x, y: INTEGER; mask: BOOLEAN): BOOLEAN;
		VAR p: Properties.FocusPref;
	BEGIN
		p.atLocation := TRUE; p.x := x + f.gx - g.gx; p.y := y + f.gy - g.gy;
		p.hotFocus := FALSE; p.setFocus := FALSE;
		Views.HandlePropMsg(v, p);
		RETURN p.setFocus & (mask OR ~p.hotFocus)
	END ClaimFocusAt;
	
	PROCEDURE NeedFocusAt (v: Views.View; f, g: Views.Frame; x, y: INTEGER): BOOLEAN;
		VAR p: Properties.FocusPref;
	BEGIN
		p.atLocation := TRUE; p.x := x + f.gx - g.gx; p.y := y + f.gy - g.gy;
		p.hotFocus := FALSE; p.setFocus := FALSE;
		Views.HandlePropMsg(v, p);
		RETURN p.hotFocus OR p.setFocus
	END NeedFocusAt;


	PROCEDURE TrackToResize (c: Controller; f: Views.Frame; v: Views.View; x, y: INTEGER; buttons: SET);
		VAR minW, maxW, minH, maxH,  l, t, r, b,  w0, h0,  w, h: INTEGER; op: INTEGER; sg, fc: Views.View;
	BEGIN
		c.model.GetEmbeddingLimits(minW, maxW, minH, maxH);
		c.view.GetRect(f, v, l, t, r, b);
		w0 := r - l; h0 := b - t; w := w0; h := h0;
		Mechanisms.TrackToResize(f, v, minW, maxW, minH, maxH, l, t, r, b, op, x, y, buttons);
		IF op = Mechanisms.resize THEN
			sg := c.singleton; fc := c.focus;
			c.Resize(v, l, t, r, b);
			IF c.singleton # sg THEN c.SetSingleton(sg) END;
			IF c.focus # fc THEN c.focus := fc; c.bVis := FALSE END	(* delayed c.SetFocus(fc) *)
		END
	END TrackToResize;

	PROCEDURE TrackToDrop (c: Controller; f: Views.Frame; VAR x, y: INTEGER; buttons: SET;
									VAR pass: BOOLEAN);
		VAR dest: Views.Frame; m: Models.Model; v: Views.View;
			x0, y0, x1, y1, w, h, rx, ry, destX, destY: INTEGER; op: INTEGER; isDown, isSingle: BOOLEAN; mo: SET;
	BEGIN	(* drag and drop c's selection: mouse is in selection *)
		x0 := x; y0 := y;
		REPEAT
			f.Input(x1, y1, mo, isDown)
		UNTIL ~isDown OR (ABS(x1 - x) > 3 * Ports.point) OR (ABS(y1 - y) > 3 * Ports.point);
		pass := ~isDown;
		IF ~pass THEN
			v := c.Singleton();
			IF v = NIL THEN v := c.view; isSingle := FALSE
			ELSE isSingle := TRUE
			END;
			c.GetSelectionBounds(f, rx, ry, w, h);
			rx := x0 - rx; ry := y0 - ry;
			IF rx < 0 THEN rx := 0 ELSIF rx > w THEN rx := w END;
			IF ry < 0 THEN ry := 0 ELSIF ry > h THEN ry := h END;
			IF noCaret IN c.opts THEN op := Mechanisms.copy ELSE op := 0 END;
			Mechanisms.TrackToDrop(f, v, isSingle, w, h, rx, ry, dest, destX, destY, op, x, y, buttons);
			IF (op IN {Mechanisms.copy, Mechanisms.move}) THEN	(* copy or move selection *)
				IF dest # NIL THEN
					m := dest.view.ThisModel();
					IF (dest.view = c.view) OR (m # NIL) & (m = c.view.ThisModel()) THEN	(* local drop *)
						IF op = Mechanisms.copy THEN	(* local copy *)
							c.CopyLocalSelection(f, dest, x0, y0, destX, destY)
						ELSIF op = Mechanisms.move THEN	(* local move *)
							c.MoveLocalSelection(f, dest, x0, y0, destX, destY)
						END
					ELSE	(* non-local drop *)
						CopyView(c, v, w, h);	(* create copy of selection *)
						IF (op = Mechanisms.copy) OR (noCaret IN c.opts) THEN	(* drop copy *)
							Controllers.Drop(x, y, f, x0, y0, v, isSingle, w, h, rx, ry)
						ELSIF op = Mechanisms.move THEN	(* drop copy and delete original *)
							Controllers.Drop(x, y, f, x0, y0, v, isSingle, w, h, rx, ry);
							c.DeleteSelection;
						END
					END
				ELSIF (op = Mechanisms.move) & ~(noCaret IN c.opts) THEN
					c.DeleteSelection
				END
			END
		END
	END TrackToDrop;

	PROCEDURE TrackToPick (c: Controller; f: Views.Frame; x, y: INTEGER; buttons: SET;
									VAR pass: BOOLEAN);
		VAR p: Properties.Property; dest: Views.Frame; x0, y0, x1, y1, destX, destY: INTEGER;
			op: INTEGER; isDown: BOOLEAN; m: SET;
	BEGIN
		x0 := x; y0 := y;
		REPEAT
			f.Input(x1, y1, m, isDown)
		UNTIL ~isDown OR (ABS(x1 - x) > 3 * Ports.point) OR (ABS(y1 - y) > 3 * Ports.point);
		pass := ~isDown;
		IF ~pass THEN
			Mechanisms.TrackToPick(f, dest, destX, destY, op, x, y, buttons);
			IF op IN {Mechanisms.pick, Mechanisms.pickForeign} THEN
				Properties.Pick(x, y, f, x0, y0, p);
				IF p # NIL THEN SetProp(c, NIL, p, direct) END
			END
		END
	END TrackToPick;

	PROCEDURE MarkViews (f: Views.Frame);
		VAR x, y: INTEGER; isDown: BOOLEAN; root: Views.RootFrame; m: SET;
	BEGIN
		root := Views.RootOf(f);
		Views.MarkBorders(root);
		REPEAT f.Input(x, y, m, isDown) UNTIL ~isDown;
		Views.MarkBorders(root)
	END MarkViews;

	PROCEDURE Track (c: Controller; f: Views.Frame; VAR msg: Controllers.TrackMsg; VAR focus: Views.View);
		VAR res, l, t, r, b: INTEGER; cursor: INTEGER; sel: Views.View; obj: Views.Frame;
			inSel, pass, extend, add, double, popup: BOOLEAN;
	BEGIN
		cursor := Mechanisms.outside; sel := c.Singleton();
		IF focus # NIL THEN
			c.view.GetRect(f, focus, l, t, r, b);
			IF (BorderVisible(c, f) OR (f IS Views.RootFrame)) & ~(noSelection IN c.opts) THEN
				cursor := Mechanisms.FocusBorderCursor(f, focus, l, t, r, b, msg.x, msg.y)
			ELSIF (msg.x >= l) & (msg.x <= r) & (msg.y >= t) & (msg.y <= b) THEN
				cursor := Mechanisms.inside
			END
		ELSIF sel # NIL THEN
			c.view.GetRect(f, sel, l, t, r, b);
			cursor := Mechanisms.SelBorderCursor(f, sel, l, t, r, b, msg.x, msg.y)
		END;
		IF cursor >= 0 THEN
			IF focus # NIL THEN
				(* resize focus *)
				TrackToResize(c, f, focus, msg.x, msg.y, msg.modifiers);
				focus := NIL
			ELSE
				(* resize singleton *)
				TrackToResize(c, f, sel, msg.x, msg.y, msg.modifiers)
			END
		ELSIF (focus # NIL) & (cursor = Mechanisms.inside) THEN
			(* forward to focus *)
		ELSE
			IF (focus # NIL) & (c.opts * modeOpts # mask) THEN c.SetFocus(NIL) END;
			focus := NIL;
			inSel := c.InSelection(f, msg.x, msg.y);
			extend := Controllers.extend IN msg.modifiers;
			add := Controllers.modify IN msg.modifiers;
			double := Controllers.doubleClick IN msg.modifiers;
			popup := right IN msg.modifiers;
			obj := Views.FrameAt(f, msg.x, msg.y);
			IF ~inSel & (~extend OR (noSelection IN c.opts)) THEN 
				IF obj # NIL THEN
					IF ~(noFocus IN c.opts) & NeedFocusAt(obj.view, f, obj, msg.x, msg.y)
							& (~(alt IN msg.modifiers) OR (noSelection IN c.opts)) THEN
						(* set hot focus *)
						focus := obj.view;
						IF ClaimFocusAt(focus, f, obj, msg.x, msg.y, c.opts * modeOpts = mask) THEN
							(* set permanent focus *)
							c.SelectAll(deselect);
							c.SetFocus(focus)
						END
					END;
					IF (focus = NIL) & ~add & ~(noSelection IN c.opts) THEN
						(* select object *)
						c.SelectAll(deselect);
						c.SetSingleton(obj.view); inSel := TRUE
					END
				ELSIF ~add THEN c.SelectAll(deselect)
				END
			END;
			IF focus = NIL THEN
				IF inSel & double & (popup OR (alt IN msg.modifiers)) THEN (* properties *)
					Dialog.Call("StdCmds.ShowProp", "", res)
				ELSIF inSel & double & (obj # NIL) THEN (* primary verb *)
					Dialog.Call("HostMenus.PrimaryVerb", "", res)
				ELSIF ~inSel & (alt IN msg.modifiers) & extend THEN
					MarkViews(f)
				ELSE
					IF inSel & ~extend THEN (* drag *)
						IF (alt IN msg.modifiers) OR (middle IN msg.modifiers) THEN
							IF ~(noCaret IN c.opts) THEN
								TrackToPick(c, f, msg.x, msg.y, msg.modifiers, pass)
							END
						ELSE
							TrackToDrop(c, f, msg.x, msg.y, msg.modifiers, pass)
						END;
						IF ~pass THEN RETURN END
					END;
					IF ~(noSelection IN c.opts) & (~inSel OR extend OR add OR (obj = NIL) & ~popup) THEN (* select *)
						c.TrackMarks(f, msg.x, msg.y, double, extend, add)
					END;
					IF popup THEN Dialog.Call("HostMenus.PopupMenu", "", res) END
				END
			END
		END
	END Track;

	PROCEDURE CopyView (source: Controller; VAR view: Views.View; VAR w, h: INTEGER);
		VAR s: Views.View; m: Model; v: View; p: Properties.BoundsPref;
	BEGIN
		s := source.Singleton();
		IF s # NIL THEN	(* create a copy of singular selection *)
			view := Views.CopyOf(s, Views.deep); s.context.GetSize(w, h)
		ELSE	(* create a copy of view with a copy of whole selection as contents *)
			m := source.SelectionCopy();
			v := Views.CopyWithNewModel(source.view, m)(View);
			p.w := Views.undefined; p.h := Views.undefined; Views.HandlePropMsg(v, p);
			view := v; w := p.w; h := p.h
		END
	END CopyView;

	PROCEDURE Paste (c: Controller; f: Views.Frame; v: Views.View; w, h: INTEGER);
		VAR m: Models.Model;
	BEGIN
		m := v.ThisModel();
		IF (m # NIL) & c.NativeModel(m) THEN
			(* paste whole contents of source view *)
			c.NativePaste(m, f)
		ELSE
			(* paste whole view *)
			c.PasteView(f, v (* Views.CopyOf(v, Views.deep) *), w, h)
		END
	END Paste;

	PROCEDURE GetValidOps (c: Controller; VAR valid: SET);
	BEGIN
		valid := {}; c.GetValidOps(valid);
		IF noCaret IN c.opts THEN
			valid := valid
				- {Controllers.pasteChar, Controllers.pasteChar,
					Controllers.paste, Controllers.cut}
		END
	END GetValidOps;


	PROCEDURE Transfer (c: Controller; f: Views.Frame;
								VAR msg: Controllers.TransferMessage; VAR focus: Views.View);
		VAR g: Views.Frame; inSelection: BOOLEAN; dMsg: DropPref;
	BEGIN
		focus := NIL;
		g := Views.FrameAt(f, msg.x, msg.y);
		WITH msg: Controllers.PollDropMsg DO
			inSelection := c.InSelection(f, msg.x, msg.y);
			dMsg.mode := c.opts; dMsg.okToDrop := FALSE;
			IF g # NIL THEN Views.HandlePropMsg(g.view, dMsg) END;
			IF (g # NIL) & ~inSelection & (dMsg.okToDrop OR ~(noFocus IN c.opts))THEN
				focus := g.view
			ELSIF ~(noCaret IN c.opts) & c.CanDrop(f, msg.x, msg.y) THEN
				msg.dest := f;
				IF msg.mark THEN
					c.MarkDropTarget(msg.source, f, msg.sourceX, msg.sourceY, msg.x, msg.y, msg.w, msg.h, msg.rx, msg.ry,
											msg.type, msg.isSingle, msg.show)
				END
			END
		| msg: Controllers.DropMsg DO
			inSelection := c.InSelection(f, msg.x, msg.y);
			dMsg.mode := c.opts; dMsg.okToDrop := FALSE;
			IF g # NIL THEN Views.HandlePropMsg(g.view, dMsg) END;
			IF (g # NIL) & ~inSelection & (dMsg.okToDrop OR ~(noFocus IN c.opts))THEN
				focus := g.view
			ELSIF ~(noCaret IN c.opts) & c.CanDrop(f, msg.x, msg.y) THEN
				c.Drop(msg.source, f, msg.sourceX, msg.sourceY, msg.x, msg.y, msg.w, msg.h,
							msg.rx, msg.ry, msg.view, msg.isSingle)
			END
		| msg: Properties.PollPickMsg DO
			IF g # NIL THEN
				focus := g.view
			ELSE
				msg.dest := f;
				IF msg.mark THEN
					c.MarkPickTarget(msg.source, f, msg.sourceX, msg.sourceY, msg.x, msg.y, msg.show)
				END
			END
		| msg: Properties.PickMsg DO
			IF g # NIL THEN
				focus := g.view
			ELSE
				c.PickNativeProp(f, msg.x, msg.y, msg.prop)
			END
		ELSE
			IF g # NIL THEN focus := g.view END
		END
	END Transfer;

	PROCEDURE FocusHasSel (): BOOLEAN;
		VAR msg: Controllers.PollOpsMsg;
	BEGIN
		Controllers.PollOps(msg);
		RETURN msg.selectable & (Controllers.copy IN msg.valid)
	END FocusHasSel;
	
	PROCEDURE FocusEditor (): Controller;
		VAR msg: PollFocusMsg;
	BEGIN
		msg.focus := NIL; msg.ctrl := NIL; msg.all := FALSE;
		Controllers.Forward(msg);
		RETURN msg.ctrl
	END FocusEditor;

	PROCEDURE Edit (c: Controller; f: Views.Frame;
								VAR msg: Controllers.EditMsg; VAR focus: Views.View);
		VAR g: Views.Frame; v: Views.View; res: INTEGER;
			valid: SET; select, units, getFocus, accepts: BOOLEAN;
			sel: Controllers.SelectMsg;
	BEGIN
		IF (c.opts * modeOpts # mask) & (focus = NIL) THEN
			IF (msg.op = Controllers.pasteChar) & (msg.char = ESC) THEN
				c.SelectAll(FALSE)
			ELSIF (c.Singleton() # NIL) & (msg.op = Controllers.pasteChar) &
					(msg.char = ENTER) THEN
				Dialog.Call("HostMenus.PrimaryVerb", "", res)
			ELSE
				GetValidOps(c, valid);
				IF msg.op IN valid THEN
					CASE msg.op OF
					| Controllers.pasteChar:
						IF msg.char >= " " THEN
							c.PasteChar(msg.char)
						ELSIF (AL <= msg.char) & (msg.char <= AD) OR
							(PL <= msg.char) & (msg.char <= DD) THEN
							select := Controllers.extend IN msg.modifiers;
							units := Controllers.modify IN msg.modifiers;
							c.ArrowChar(f, msg.char, units, select)
						ELSE c.ControlChar(f, msg.char)
						END
					| Controllers.cut, Controllers.copy:
						CopyView(c, msg.view, msg.w, msg.h);
						msg.isSingle := c.Singleton() # NIL;
						IF msg.op = Controllers.cut THEN c.DeleteSelection END
					| Controllers.paste:
						IF msg.isSingle THEN
							c.PasteView(f, msg.view (* Views.CopyOf(msg.view, Views.deep) *), msg.w, msg.h)
						ELSE
							Paste(c, f, msg.view, msg.w, msg.h)
						END
					ELSE
					END
				END
			END
		ELSIF (c.opts * modeOpts # mask)
				& (msg.op = Controllers.pasteChar) & (msg.char = ESC)
				& (~(f IS Views.RootFrame) OR (31 IN c.opts))
				& (c = FocusEditor()) 
				& ((Controllers.extend IN msg.modifiers) OR ~FocusHasSel()) THEN
			IF 31 IN c.opts THEN INCL(msg.modifiers, 31)
			ELSE c.SetSingleton(focus)
			END;
			focus := NIL
		ELSIF (c.opts * modeOpts # mask) & (c = Focus()) THEN
			(* do some generic processing for non-container views *)
			IF (msg.op = Controllers.pasteChar) & (msg.char = ESC) THEN
				g := Views.ThisFrame(f, focus);
				IF g # NIL THEN sel.set := FALSE; Views.ForwardCtrlMsg(g, sel) END
			END
		ELSIF (c.opts * modeOpts = mask) & (msg.op = Controllers.pasteChar) THEN
			IF alt IN msg.modifiers THEN
				CollectControlPref (c, NIL, msg.char, TRUE, v, getFocus, accepts)
			ELSE
				CollectControlPref (c, focus, msg.char, TRUE, v, getFocus, accepts)
			END;
			 IF v = NIL THEN
				CheckMaskFocus(c, f, focus);
				CollectControlPref(c, focus, msg.char, TRUE, v, getFocus, accepts)
			END;
			IF v # NIL THEN
				IF getFocus & (v # focus) THEN
					c.SetFocus(v)
				END;
				IF accepts THEN
					g := Views.ThisFrame(f, v);
					IF g # NIL THEN Views.ForwardCtrlMsg(g, msg) END
				END;
				focus := NIL
			END
		END
	END Edit;

	PROCEDURE PollCursor (c: Controller; f: Views.Frame; VAR msg: Controllers.PollCursorMsg; VAR focus: Views.View);
		VAR l, t, r, b: INTEGER; cursor: INTEGER; sel: Views.View; obj: Views.Frame; inSel: BOOLEAN;
	BEGIN
		cursor := Mechanisms.outside; sel := c.Singleton();
		IF focus # NIL THEN
			c.view.GetRect(f, focus, l, t, r, b);
			IF (BorderVisible(c, f) OR (f IS Views.RootFrame)) & ~(noSelection IN c.opts) THEN
				cursor := Mechanisms.FocusBorderCursor(f, focus, l, t, r, b, msg.x, msg.y)
			ELSIF (msg.x >= l) & (msg.x <= r) & (msg.y >= t) & (msg.y <= b) THEN
				cursor := Mechanisms.inside
			END
		ELSIF sel # NIL THEN
			c.view.GetRect(f, sel, l, t, r, b);
			cursor := Mechanisms.SelBorderCursor(f, sel, l, t, r, b, msg.x, msg.y)
		END;
		IF cursor >= 0 THEN
			msg.cursor := cursor; focus := NIL
		ELSIF (focus # NIL) & (cursor = Mechanisms.inside) THEN
			msg.cursor := Ports.arrowCursor
		ELSE
			IF noCaret IN c.opts THEN msg.cursor := Ports.arrowCursor 
			ELSE msg.cursor := c.NativeCursorAt(f, msg.x, msg.y)	(* if nothing else, use native cursor *)
			END;
			focus := NIL; inSel := FALSE;
			IF ~(noSelection IN c.opts) THEN inSel := c.InSelection(f, msg.x, msg.y) END;
			IF ~inSel THEN
				obj := Views.FrameAt(f, msg.x, msg.y);
				IF obj # NIL THEN
					IF ~(noFocus IN c.opts) & NeedFocusAt(obj.view, f, obj, msg.x, msg.y) THEN
						focus := obj.view;
						msg.cursor := Ports.arrowCursor
					ELSIF ~(noSelection IN c.opts) THEN
						inSel := TRUE
					END
				END
			END;
			IF focus = NIL THEN
				IF inSel THEN
					msg.cursor := Ports.arrowCursor
				END
			END
		END
	END PollCursor;

	PROCEDURE PollOps (c: Controller; f: Views.Frame;
								VAR msg: Controllers.PollOpsMsg; VAR focus: Views.View);
	BEGIN
		IF focus = NIL THEN
			msg.type := "";
			IF ~(noSelection IN c.opts) THEN c.GetContextType(msg.type) END;
			msg.selectable := ~(noSelection IN c.opts) & c.Selectable();
			GetValidOps(c, msg.valid);
			msg.singleton := c.Singleton()
		END
	END PollOps;

	PROCEDURE ReplaceView (c: Controller; old, new: Views.View);
	BEGIN
		ASSERT(old.context # NIL, 20);
		ASSERT((new.context = NIL) OR (new.context = old.context), 22);
		IF old.context.ThisModel() = c.model THEN
			c.model.ReplaceView(old, new)
		END;
		IF c.singleton = old THEN c.singleton := new END;
		IF c.focus = old THEN c.focus := new END
	END ReplaceView;

	PROCEDURE ViewProp (v: Views.View): Properties.Property;
		VAR poll: Properties.PollMsg;
	BEGIN
		poll.prop := NIL; Views.HandlePropMsg(v, poll); RETURN poll.prop
	END ViewProp;

	PROCEDURE SetViewProp (v: Views.View; old, p: Properties.Property);
		VAR set: Properties.SetMsg;
	BEGIN
		set.old := old; set.prop := p; Views.HandlePropMsg(v, set)
	END SetViewProp;

	PROCEDURE SizeProp (v: Views.View): Properties.Property;
		VAR sp: Properties.SizeProp;
	BEGIN
		NEW(sp); sp.known := {Properties.width, Properties.height}; sp.valid := sp.known;
		v.context.GetSize(sp.width, sp.height);
		RETURN sp
	END SizeProp;

	PROCEDURE SetSizeProp (v: Views.View; p: Properties.SizeProp);
		VAR w, h: INTEGER;
	BEGIN
		IF p.valid # {Properties.width, Properties.height} THEN
			v.context.GetSize(w, h)
		END;
		IF Properties.width IN p.valid THEN w := p.width END;
		IF Properties.height IN p.valid THEN h := p.height END;
		v.context.SetSize(w, h)
	END SetSizeProp;

	PROCEDURE ThisProp (c: Controller; direct: BOOLEAN): Properties.Property;
		CONST scanCutoff = MAX(INTEGER) (* 50 *);	(* bound number of polled embedded views *)
		VAR v: Views.View; np, vp, p: Properties.Property; k: INTEGER; trunc, equal: BOOLEAN;
	BEGIN
		trunc := FALSE; k := 1;
		np := NIL; c.PollNativeProp(direct, np, trunc);
		v := NIL; c.GetFirstView(direct, v);
		IF v # NIL THEN
			Properties.Insert(np, SizeProp(v));
			vp := ViewProp(v);
			k := scanCutoff; c.GetNextView(direct, v);
			WHILE (v # NIL) & (k > 0) DO
				DEC(k);
				Properties.Insert(np, SizeProp(v));
				Properties.Intersect(vp, ViewProp(v), equal);
				c.GetNextView(direct, v)
			END;
			IF c.singleton # NIL THEN Properties.Merge(np, vp); vp := np
			ELSE Properties.Merge(vp, np)
			END
		ELSE vp := np
		END;
		IF trunc OR (k = 0) THEN
			p := vp; WHILE p # NIL DO p.valid := {}; p := p.next END
		END;
		IF noCaret IN c.opts THEN
			p := vp; WHILE p # NIL DO p.readOnly := p.valid; p := p.next END
		END;
		RETURN vp
	END ThisProp;

	PROCEDURE SetProp (c: Controller; old, p: Properties.Property; direct: BOOLEAN);
		TYPE
			ViewList = POINTER TO RECORD next: ViewList; view: Views.View END;
		VAR v: Views.View; q, sp: Properties.Property; equal: BOOLEAN; s: Stores.Operation;
			list, last: ViewList;
	BEGIN
		IF noCaret IN c.opts THEN RETURN END;
		Views.BeginScript(c.view, "#System:SetProp", s);
		q := p; WHILE (q # NIL) & ~(q IS Properties.SizeProp) DO q := q.next END;
		list := NIL; v := NIL; c.GetFirstView(direct, v);
		WHILE v # NIL DO
			IF list = NIL THEN NEW(list); last := list
			ELSE NEW(last.next); last := last.next
			END;
			last.view := v;
			c.GetNextView(direct, v)
		END;
		c.SetNativeProp(direct, old, p);
		WHILE list # NIL DO
			v := list.view; list := list.next;
			SetViewProp(v, old, p);
			IF direct & (q # NIL) THEN
				(* q IS Properties.SizeProp *)
				IF old # NIL THEN
					sp := SizeProp(v);
					Properties.Intersect(sp, old, equal);
					Properties.Intersect(sp, old, equal)
				END;
				IF (old = NIL) OR equal THEN
					SetSizeProp(v, q(Properties.SizeProp))
				END
			END
		END;
		Views.EndScript(c.view, s)
	END SetProp;

	PROCEDURE (c: Controller) HandleCtrlMsg* (f: Views.Frame;
														 VAR msg: Controllers.Message; VAR focus: Views.View), NEW, EXTENSIBLE;
	BEGIN
		focus := c.focus;
		WITH msg: Controllers.PollCursorMsg DO
			PollCursor(c, f, msg, focus)
		| msg: Controllers.PollOpsMsg DO
			PollOps(c, f, msg, focus)
		| msg: PollFocusMsg DO
			IF msg.all OR (c.opts * modeOpts # mask) & (c.focus # NIL) THEN msg.ctrl := c END
		| msg: Controllers.TrackMsg DO
			Track(c, f, msg, focus)
		| msg: Controllers.EditMsg DO
			Edit(c, f, msg, focus)
		| msg: Controllers.TransferMessage DO
			Transfer(c, f, msg, focus)
		| msg: Controllers.SelectMsg DO
			IF focus = NIL THEN c.SelectAll(msg.set) END
		| msg: Controllers.TickMsg DO
			FadeMarks(c, show);
			CheckMaskFocus(c, f, focus)
		| msg: Controllers.MarkMsg DO
			c.bVis := msg.show;
			c.Mark(f, f.l, f.t, f.r, f.b, msg.show)
		| msg: Controllers.ReplaceViewMsg DO
			ReplaceView(c, msg.old, msg.new)
		| msg: Properties.CollectMsg DO
			IF focus = NIL THEN
				msg.poll.prop := ThisProp(c, direct)
			END
		| msg: Properties.EmitMsg DO
			IF focus = NIL THEN
				SetProp(c, msg.set.old, msg.set.prop, direct)
			END
		ELSE
		END
	END HandleCtrlMsg;


	(** miscellaneous **)

	PROCEDURE Focus* (): Controller;
		VAR msg: PollFocusMsg;
	BEGIN
		msg.focus := NIL; msg.ctrl := NIL; msg.all := TRUE;
		Controllers.Forward(msg);
		RETURN msg.ctrl
	END Focus;

	PROCEDURE FocusSingleton* (): Views.View;
		VAR c: Controller; v: Views.View;
	BEGIN
		c := Focus();
		IF c # NIL THEN v := c.Singleton() ELSE v := NIL END;
		RETURN v
	END FocusSingleton;
	
	PROCEDURE CloneOf* (m: Model): Model;
		VAR h: Model;
	BEGIN
		ASSERT(m # NIL, 20);
		Kernel.NewObj(h, Kernel.TypeOf(m));
		h.InitFrom(m);
		RETURN h
	END CloneOf;

END Containers.
