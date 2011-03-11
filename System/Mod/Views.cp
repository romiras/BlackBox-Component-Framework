MODULE Views;
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

	IMPORT SYSTEM,
		Kernel, Log, Dialog, Files, Services, Fonts, Stores, Converters, Ports, Sequencers, Models;

	CONST
		(** View.Background color **)
		transparent* = 0FF000000H;

		(** Views.CopyModel / Views.CopyOf shallow **)
		deep* = FALSE; shallow* = TRUE;

		(** Update, UpdateIn rebuild **)
		keepFrames* = FALSE; rebuildFrames* = TRUE;

		(** Deposit, QualifiedDeposit, Fetch w, h **)
		undefined* = 0;
		
		(** OldView, RegisterView ask **)
		dontAsk* = FALSE; ask* = TRUE;

		(* method numbers (UNSAFE!) *)
		(* copyFrom = 1; *)
		copyFromModelView = 7; copyFromSimpleView = 8;

		(* Frame.state *)
		new = 0; open = 1; closed = 2;
		
		maxN = 30;	(* max number of rects used to approximate a region *)

		minVersion = 0; maxVersion = 0;
		
		(* actOp *)
		handler = 1; restore = 2; externalize = 3;

		markBorderSize = 2;

		clean* = Sequencers.clean; 
		notUndoable* = Sequencers.notUndoable;
		invisible* = Sequencers.invisible; 


	TYPE

		(** views **)

		View* = POINTER TO ABSTRACT RECORD (Stores.Store)
			context-: Models.Context;	(** stable context # NIL **)
			era: INTEGER;
			guard: INTEGER;	(* = TrapCount()+1 if view is addressee of ongoing broadcast *)
			bad: SET
		END;

		Alien* = POINTER TO LIMITED RECORD (View)
			store-: Stores.Alien
		END;

		Title* = ARRAY 64 OF CHAR;

		TrapAlien = POINTER TO RECORD (Stores.Store) END;


		(** frames **)

		Frame* = POINTER TO ABSTRACT RECORD (Ports.Frame)
			l-, t-, r-, b-: INTEGER;	(** l < r, t < b **)
			view-: View;	(** opened => view # NIL, view.context # NIL, view.seq # NIL **)
			front-, mark-: BOOLEAN;
			state: BYTE;
			x, y: INTEGER;	(* origin in coordinates of environment *)
			gx0, gy0: INTEGER;	(* global origin w/o local scrolling compensation *)
			sx, sy: INTEGER;	(* cumulated local sub-pixel scrolling compensation *)
			next, down, up, focus: Frame;
			level: INTEGER	(* used for partial z-ordering *)
		END;
		

		Message* = ABSTRACT RECORD
			view-: View	(** view # NIL **)
		END;

		NotifyMsg* = EXTENSIBLE RECORD (Message)
			id0*, id1*: INTEGER;
			opts*: SET
		END;
		
		NotifyHook = POINTER TO RECORD (Dialog.NotifyHook) END;
		
		UpdateCachesMsg* = EXTENSIBLE RECORD (Message) END;
		
		ScrollClassMsg* = RECORD (Message)
			allowBitmapScrolling*: BOOLEAN (** OUT, preset to FALSE **)
		END;


		(** property messages **)

		PropMessage* = ABSTRACT RECORD END;


		(** controller messages **)

		CtrlMessage* = ABSTRACT RECORD END;

		CtrlMsgHandler* = PROCEDURE (op: INTEGER; f, g: Frame; VAR msg: CtrlMessage; VAR mark, front, req: BOOLEAN);

		UpdateMsg = RECORD (Message)
			scroll, rebuild, all: BOOLEAN;
			l, t, r, b, dx, dy: INTEGER
		END;


		Rect = RECORD
			v: View;
			rebuild: BOOLEAN;
			l, t, r, b: INTEGER
		END;

		Region = POINTER TO RECORD
			n: INTEGER;
			r: ARRAY maxN OF Rect
		END;

		RootFrame* = POINTER TO RECORD (Frame)
			flags-: SET;
			update: Region	(* allocated lazily by SetRoot *)
		END;
		
		StdFrame = POINTER TO RECORD (Frame) END;


		(* view producer/consumer decoupling *)

		QueueElem = POINTER TO RECORD
			next: QueueElem;
			view: View
		END;
		
		GetSpecHook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;
		ViewHook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;
		MsgHook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;
		


	VAR
		HandleCtrlMsg-: CtrlMsgHandler;

		domainGuard: INTEGER;	(* = TrapCount()+1 if domain is addressee of ongoing domaincast *)
		
		actView: View;
		actFrame: RootFrame;
		actOp: INTEGER;

		copyModel: Models.Model;	(* context for (View)CopyFrom; reset by TrapCleanup *)

		queue: RECORD
			len: INTEGER;
			head, tail: QueueElem
		END;
		
		getSpecHook: GetSpecHook;
		viewHook: ViewHook;
		msgHook: MsgHook;
	
	
	PROCEDURE Overwritten (v: View; mno: INTEGER): BOOLEAN;
		VAR base, actual: PROCEDURE;
	BEGIN
		SYSTEM.GET(SYSTEM.TYP(View) - 4 * (mno + 1), base);
		SYSTEM.GET(SYSTEM.TYP(v) - 4 * (mno + 1), actual);
		RETURN actual # base
	END Overwritten;

	(** Hooks **)
	
	PROCEDURE (h: GetSpecHook) GetExtSpec* (s: Stores.Store; VAR loc: Files.Locator; 
											VAR name: Files.Name; VAR conv: Converters.Converter), NEW, ABSTRACT;
	PROCEDURE (h: GetSpecHook) GetIntSpec* (VAR loc: Files.Locator; VAR name: Files.Name; 
											VAR conv: Converters.Converter), NEW, ABSTRACT;

	PROCEDURE SetGetSpecHook*(h: GetSpecHook);
	BEGIN
		getSpecHook := h
	END SetGetSpecHook;
	
	PROCEDURE (h: ViewHook) OldView* (loc: Files.Locator; name: Files.Name; 
				VAR conv: Converters.Converter): View, NEW, ABSTRACT;
	PROCEDURE (h: ViewHook) Open* (s: View; title: ARRAY OF CHAR;
				loc: Files.Locator; name: Files.Name; conv: Converters.Converter; 
				asTool, asAux, noResize, allowDuplicates, neverDirty: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (h: ViewHook) RegisterView* (s: View; loc: Files.Locator; 
				name: Files.Name; conv: Converters.Converter), NEW, ABSTRACT;

	PROCEDURE SetViewHook*(h: ViewHook);
	BEGIN
		viewHook := h
	END SetViewHook;
	
	PROCEDURE (h: MsgHook) Omnicast* (VAR msg: ANYREC), NEW, ABSTRACT;
	PROCEDURE (h: MsgHook) RestoreDomain* (domain: Stores.Domain), NEW, ABSTRACT;

	PROCEDURE SetMsgHook*(h: MsgHook);
	BEGIN
		msgHook := h
	END SetMsgHook;

	
	(** Model protocol **)

	PROCEDURE (v: View) CopyFromSimpleView- (source: View), NEW, EMPTY;
	PROCEDURE (v: View) CopyFromModelView- (source: View; model: Models.Model), NEW, EMPTY;

	PROCEDURE (v: View) ThisModel* (): Models.Model, NEW, EXTENSIBLE;
	BEGIN
		RETURN NIL
	END ThisModel;


	(** Store protocol **)

	PROCEDURE (v: View) CopyFrom- (source: Stores.Store);
		VAR tm, fm: Models.Model; c: Models.Context;
	BEGIN
		tm := copyModel; copyModel := NIL;
		WITH source: View DO
			v.era := source.era;
			actView := NIL;
			IF tm = NIL THEN	(* if copyModel wasn't preset then use deep copy as default *)
				fm := source.ThisModel();
				IF fm # NIL THEN tm := Stores.CopyOf(fm)(Models.Model) END
			END;
			actView := v;
			IF Overwritten(v, copyFromModelView) THEN	(* new View *)
				ASSERT(~Overwritten(v, copyFromSimpleView), 20);
				c := v.context;
				v.CopyFromModelView(source, tm);
				ASSERT(v.context = c, 60)
			ELSE	(* old or simple View *)
				(* IF tm # NIL THEN v.InitModel(tm) END *)
				c := v.context;
				v.CopyFromSimpleView(source);
				ASSERT(v.context = c, 60)
			END
		END
	END CopyFrom;

	PROCEDURE (v: View) Internalize- (VAR rd: Stores.Reader), EXTENSIBLE;
		VAR thisVersion: INTEGER;
	BEGIN
		v.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxVersion, thisVersion)
	END Internalize;

	PROCEDURE (v: View) Externalize- (VAR wr: Stores.Writer), EXTENSIBLE;
	BEGIN
		v.Externalize^(wr);
		wr.WriteVersion(maxVersion)
	END Externalize;


	(** embedding protocol **)

	PROCEDURE (v: View) InitContext* (context: Models.Context), NEW, EXTENSIBLE;
	BEGIN
		ASSERT(context # NIL, 21);
		ASSERT((v.context = NIL) OR (v.context = context), 22);
		v.context := context
	END InitContext;

	PROCEDURE (v: View) GetBackground* (VAR color: Ports.Color), NEW, EMPTY;
	PROCEDURE (v: View) ConsiderFocusRequestBy- (view: View), NEW, EMPTY;
	PROCEDURE (v: View) Neutralize*, NEW, EMPTY;


	(** Frame protocol **)

	PROCEDURE (v: View) GetNewFrame* (VAR frame: Frame), NEW, EMPTY;
	PROCEDURE (v: View) Restore* (f: Frame; l, t, r, b: INTEGER), NEW, ABSTRACT;
	PROCEDURE (v: View) RestoreMarks* (f: Frame; l, t, r, b: INTEGER), NEW, EMPTY;


	(** handlers **)

	PROCEDURE (v: View) HandleModelMsg- (VAR msg: Models.Message), NEW, EMPTY;
	PROCEDURE (v: View) HandleViewMsg- (f: Frame; VAR msg: Message), NEW, EMPTY;
	PROCEDURE (v: View) HandleCtrlMsg* (f: Frame; VAR msg: CtrlMessage; VAR focus: View), NEW, EMPTY;
	PROCEDURE (v: View) HandlePropMsg- (VAR msg: PropMessage), NEW, EMPTY;


	(** Alien **)

	PROCEDURE (a: Alien) Externalize- (VAR wr: Stores.Writer);
	BEGIN
		HALT(100)
	END Externalize;

	PROCEDURE (a: Alien) Internalize- (VAR rd: Stores.Reader);
	BEGIN
		HALT(100)
	END Internalize;

	PROCEDURE (a: Alien) CopyFromSimpleView- (source: View);
	BEGIN
		a.store := Stores.CopyOf(source(Alien).store)(Stores.Alien); Stores.Join(a, a.store)
	END CopyFromSimpleView;

	PROCEDURE (a: Alien) Restore* (f: Frame; l, t, r, b: INTEGER);
		VAR u, w, h: INTEGER;
	BEGIN
		u := f.dot; a.context.GetSize(w, h);
		f.DrawRect(0, 0, w, h, Ports.fill, Ports.grey25);
		f.DrawRect(0, 0, w, h, 2 * u, Ports.grey75);
		f.DrawLine(0, 0, w - u, h - u, u, Ports.grey75);
		f.DrawLine(w - u, 0, 0, h - u, u, Ports.grey75)
	END Restore;
	

	(** TrapAlien **)

	PROCEDURE (v: TrapAlien) Internalize (VAR rd: Stores.Reader);
	BEGIN
		v.Internalize^(rd);
		rd.TurnIntoAlien(3)
	END Internalize;
	
	PROCEDURE (v: TrapAlien) Externalize (VAR rd: Stores.Writer);
	END Externalize;
	
	PROCEDURE (v: TrapAlien) CopyFrom (source: Stores.Store), EMPTY;


	(** Frame **)

	PROCEDURE (f: Frame) Close* (), NEW, EMPTY;


	(* Rect, Region *)

	PROCEDURE Union (VAR u: Rect; r: Rect);
	BEGIN
		IF r.v # u.v THEN u.v := NIL END;
		IF r.rebuild THEN u.rebuild := TRUE END;
		IF r.l < u.l THEN u.l := r.l END;
		IF r.t < u.t THEN u.t := r.t END;
		IF r.r > u.r THEN u.r := r.r END;
		IF r.b > u.b THEN u.b := r.b END
	END Union;

	PROCEDURE Add (rgn: Region; v: View; rebuild: BOOLEAN; gl, gt, gr, gb: INTEGER);
		(* does not perfectly maintain invariant of non-overlapping approx rects ... *)
		VAR q: Rect; i, j, n: INTEGER; x: ARRAY maxN OF BOOLEAN;
	BEGIN
		q.v := v; q.rebuild := rebuild; q.l := gl; q.t := gt; q.r := gr; q.b := gb;
		n := rgn.n + 1;
		i := 0;
		WHILE i < rgn.n DO
			x[i] := (gl < rgn.r[i].r) & (rgn.r[i].l < gr) & (gt < rgn.r[i].b) & (rgn.r[i].t < gb);
			IF x[i] THEN Union(q, rgn.r[i]); DEC(n) END;
			INC(i)
		END;
		IF n > maxN THEN
			(* n = maxN + 1 -> merge q with arbitrarily picked rect and Add *)
			Union(q, rgn.r[maxN - 1]); Add(rgn, v, q.rebuild, q.l, q.t, q.r, q.b)
		ELSE
			i := 0; WHILE (i < rgn.n) & ~x[i] DO INC(i) END;
			rgn.r[i] := q; INC(i); WHILE (i < rgn.n) & ~x[i] DO INC(i) END;
			j := i; WHILE (i < rgn.n) & x[i] DO INC(i) END;
			WHILE i < rgn.n DO	(* ~x[i] *)
				rgn.r[j] := rgn.r[i]; INC(j); INC(i);
				WHILE (i < rgn.n) & x[i] DO INC(i) END
			END;
			rgn.n := n
		END
	END Add;

	PROCEDURE AddRect (root: RootFrame; f: Frame; l, t, r, b: INTEGER; rebuild: BOOLEAN);
		VAR rl, rt, rr, rb: INTEGER; i: INTEGER;
	BEGIN
		INC(l, f.gx); INC(t, f.gy); INC(r, f.gx); INC(b, f.gy);
		rl := root.l + root.gx; rt := root.t + root.gy; rr := root.r + root.gx; rb := root.b + root.gy;
		IF l < rl THEN l := rl END;
		IF t < rt THEN t := rt END;
		IF r > rr THEN r := rr END;
		IF b > rb THEN b := rb END;
		IF (l < r) & (t < b) THEN
			Add(root.update, f.view, rebuild, l, t, r, b);
			i := 0;
			WHILE (i < root.update.n)
				& (~root.update.r[i].rebuild OR (root.update.r[i].v # NIL)) DO INC(i) END;
			IF i < root.update.n THEN Add(root.update, root.view, TRUE, rl, rt, rr, rb) END
		END
	END AddRect;


	(** miscellaneous **)

	PROCEDURE RestoreDomain* (domain: Stores.Domain);
	BEGIN
		ASSERT(msgHook # NIL, 100);
		msgHook.RestoreDomain(domain)
	END RestoreDomain;

	PROCEDURE MarkBorder* (host: Ports.Frame; view: View; l, t, r, b: INTEGER);
		VAR s: INTEGER;
	BEGIN
		IF view # NIL THEN 
			s := markBorderSize * host.dot;
			host.MarkRect(l - s, t - s, r + s, b + s, s, Ports.dim50, Ports.show)
		END
	END MarkBorder;
	


	(** views **)

	PROCEDURE SeqOf (v: View): Sequencers.Sequencer;
		VAR (*c: Models.Context;*) d: Stores.Domain; seq: Sequencers.Sequencer; any: ANYPTR;
	BEGIN
		d := v.Domain(); seq := NIL;
		IF d # NIL THEN
			any := d.GetSequencer();
			IF (any # NIL) & (any IS Sequencers.Sequencer) THEN
				seq := any(Sequencers.Sequencer)
			END
		END;
		RETURN seq
	END SeqOf;


	PROCEDURE Era* (v: View): INTEGER;
	(** pre: v # NIL *)
	(** post:
		v.ThisModel() # NIL
			in-synch(v) iff Era(v) = Models.Era(v.ThisModel())
	**)
	BEGIN
		ASSERT(v # NIL, 20);
		RETURN v.era
	END Era;

	PROCEDURE BeginScript* (v: View; name: Stores.OpName; OUT script: Stores.Operation);
	(** pre: v # NIL *)
	(** post: (script # NIL) iff (v.seq # NIL) **)
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20);
		seq := SeqOf(v);
		IF seq # NIL THEN seq.BeginScript(name, script)
		ELSE script := NIL
		END
	END BeginScript;

	PROCEDURE Do* (v: View; name: Stores.OpName; op: Stores.Operation);
	(** pre: v # NIL, op # NIL, ~op.inUse **)
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20); ASSERT(op # NIL, 21); (* ASSERT(~op.inUse, 22); *)
		seq := SeqOf(v);
		IF seq # NIL THEN seq.Do(v, name, op) ELSE op.Do END
	END Do;

	PROCEDURE LastOp* (v: View): Stores.Operation;
	(** pre: v # NIL **)
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20);
		seq := SeqOf(v);
		IF seq # NIL THEN RETURN seq.LastOp(v) ELSE RETURN NIL END
	END LastOp;

	PROCEDURE Bunch* (v: View);
	(** pre: v # NIL **)
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20);
		seq := SeqOf(v); ASSERT(seq # NIL, 21);
		seq.Bunch(v)
	END Bunch;

	PROCEDURE StopBunching* (v: View);
	(** pre: v # NIL **)
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20);
		seq := SeqOf(v);
		IF seq # NIL THEN seq.StopBunching END
	END StopBunching;

	PROCEDURE EndScript* (v: View; script: Stores.Operation);
	(** pre: (script # NIL) iff (v.seq # NIL) **)
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20);
		seq := SeqOf(v);
		IF seq # NIL THEN ASSERT(script # NIL, 21); seq.EndScript(script)
		ELSE ASSERT(script = NIL, 22)
		END
	END EndScript;


	PROCEDURE BeginModification* (type: INTEGER; v: View);
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20);
		seq := SeqOf(v);
		IF seq # NIL THEN seq.BeginModification(type, v) END
	END BeginModification;

	PROCEDURE EndModification* (type: INTEGER; v: View);
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20);
		seq := SeqOf(v);
		IF seq # NIL THEN seq.EndModification(type, v) END
	END EndModification;

	PROCEDURE SetDirty* (v: View);
		VAR seq: Sequencers.Sequencer;
	BEGIN
		ASSERT(v # NIL, 20);
		seq := SeqOf(v);
		IF seq # NIL THEN seq.SetDirty(TRUE) END
	END SetDirty;


	PROCEDURE Domaincast* (domain: Stores.Domain; VAR msg: Message);
		VAR g: INTEGER; seq: ANYPTR;
	BEGIN
		IF domain # NIL THEN
			seq := domain.GetSequencer();
			IF seq # NIL THEN
				msg.view := NIL;
				g := Kernel.trapCount + 1;
				IF domainGuard > 0 THEN ASSERT(domainGuard # g, 20) END;
				domainGuard := g;
				seq(Sequencers.Sequencer).Handle(msg);
				domainGuard := 0
			END
		END
	END Domaincast;

	PROCEDURE Broadcast* (v: View; VAR msg: Message);
		VAR seq: Sequencers.Sequencer; g: INTEGER;
	BEGIN
		ASSERT(v # NIL, 20);
		msg.view := v;
		seq := SeqOf(v);
		IF seq # NIL THEN
			g := Kernel.trapCount + 1;
			IF v.guard > 0 THEN ASSERT(v.guard # g, 21) END;
			v.guard := g;
			seq.Handle(msg);
			v.guard := 0
		END
	END Broadcast;


	PROCEDURE Update* (v: View; rebuild: BOOLEAN);
		VAR upd: UpdateMsg;
	BEGIN
		ASSERT(v # NIL, 20);
		upd.scroll := FALSE; upd.rebuild := rebuild; upd.all := TRUE;
		Broadcast(v, upd)
	END Update;

	PROCEDURE UpdateIn* (v: View; l, t, r, b: INTEGER; rebuild: BOOLEAN);
		VAR upd: UpdateMsg;
	BEGIN
		ASSERT(v # NIL, 20);
		upd.scroll := FALSE; upd.rebuild := rebuild; upd.all := FALSE;
		upd.l := l; upd.t := t; upd.r := r; upd.b := b;
		Broadcast(v, upd)
	END UpdateIn;

	PROCEDURE Scroll* (v: View; dx, dy: INTEGER);
		VAR scroll: UpdateMsg;
	BEGIN
		ASSERT(v # NIL, 20); ASSERT(v.Domain() # NIL, 21);
		RestoreDomain(v.Domain());
		scroll.scroll := TRUE; scroll.dx := dx; scroll.dy := dy;
		Broadcast(v, scroll)
	END Scroll;

	PROCEDURE CopyOf* (v: View; shallow: BOOLEAN): View;
		VAR w, a: View; op: INTEGER; b: Alien;
	BEGIN
		ASSERT(v # NIL, 20);
		IF ~(handler IN v.bad) THEN
			a := actView; op := actOp; actView := NIL; actOp := handler;
			IF shallow THEN copyModel := v.ThisModel() END;
			actView := v;
			w := Stores.CopyOf(v)(View);
			actView := a; actOp := op
		ELSE
			NEW(b); w := b; w.bad := {handler..externalize}
		END;
		IF shallow THEN Stores.Join(w, v) END;
		RETURN w
	END CopyOf;

	PROCEDURE CopyWithNewModel* (v: View; m: Models.Model): View;
		VAR w, a: View; op: INTEGER; b: Alien; fm: Models.Model;
	BEGIN
		ASSERT(v # NIL, 20);
		fm := v.ThisModel(); ASSERT(fm # NIL, 21);
		ASSERT(m # NIL, 22);
		ASSERT(Services.SameType(m, fm), 23);
		IF ~(handler IN v.bad) THEN
			a := actView; op := actOp; actView := v; actOp := handler;
			copyModel := m;
			w := Stores.CopyOf(v)(View);
			actView := a; actOp := op
		ELSE
			NEW(b); w := b; w.bad := {handler..externalize}
		END;
		RETURN w
	END CopyWithNewModel;

	PROCEDURE ReadView* (VAR rd: Stores.Reader; OUT v: View);
		VAR st: Stores.Store; a: Alien;
	BEGIN
		rd.ReadStore(st);
		IF st = NIL THEN
			v := NIL
		ELSIF st IS Stores.Alien THEN
			NEW(a);
			a.store := st(Stores.Alien); Stores.Join(a, a.store);
			v := a
		ELSE
			v := st(View)
		END
	END ReadView;

	PROCEDURE WriteView* (VAR wr: Stores.Writer; v: View);
		VAR a: TrapAlien; av: View; op: INTEGER;
	BEGIN
		IF v = NIL THEN wr.WriteStore(v)
		ELSIF externalize IN v.bad THEN NEW(a); wr.WriteStore(a)
		ELSIF v IS Alien THEN wr.WriteStore(v(Alien).store)
		ELSE
			av := actView; op := actOp; actView := v; actOp := externalize;
			wr.WriteStore(v);
			actView := av; actOp := op
		END
	END WriteView;


	(* frames *)

	PROCEDURE SetClip (f: Frame; l, t, r, b: INTEGER);
		VAR u: INTEGER;
	BEGIN
		ASSERT(f.rider # NIL, 20); ASSERT(l <= r, 21); ASSERT(t <= b, 22);
		u := f.unit;
		f.rider.SetRect((l + f.gx) DIV u, (t + f.gy) DIV u, (r + f.gx) DIV u, (b + f.gy) DIV u);
		f.l := l; f.t := t; f.r := r; f.b := b
	END SetClip;

	PROCEDURE Close (f: Frame);
	BEGIN
		f.Close;
		f.state := closed;
		f.up := NIL; f.down := NIL; f.next := NIL; f.view := NIL;
		f.ConnectTo(NIL)
	END Close;

	PROCEDURE AdaptFrameTo (f: Frame; orgX, orgY: INTEGER);
		VAR g, p, q: Frame; port: Ports.Port;
			w, h,  pl, pt, pr, pb,  gl, gt, gr, gb,  gx, gy: INTEGER;
	BEGIN
		(* pre: environment (i.e. parent frame / port) has already been set up *)
		ASSERT(f.view # NIL, 20); ASSERT(f.view.context # NIL, 21);
		f.x := orgX; f.y := orgY;	(* set new origin *)
		g := f.up;
		IF g # NIL THEN	(* parent frame is environment *)
			f.gx0 := g.gx + orgX; f.gy0 := g.gy + orgY;
			f.SetOffset(f.gx0 - f.sx, f.gy0 - f.sy);
			pl := g.gx + g.l; pt := g.gy + g.t; pr := g.gx + g.r; pb := g.gy + g.b
		ELSE	(* port is environment *)
			f.gx0 := orgX; f.gy0 := orgY;
			f.SetOffset(f.gx0 - f.sx, f.gy0 - f.sy);
			port := f.rider.Base();
			port.GetSize(w, h);
			pl := 0; pt := 0; pr := w * f.unit; pb := h * f.unit
		END;
		(* (pl, pt, pr, pb) is parent clipping rectangle, in global coordinates, and in units *)
		gx := f.gx; gy := f.gy; f.view.context.GetSize(w, h);
		gl := gx; gt := gy; gr := gx + w; gb := gy + h;
		(* (gl, gt, gr, gb) is desired clipping rectangle, in global coordinates, and in units *)
		IF gl < pl THEN gl := pl END;
		IF gt < pt THEN gt := pt END;
		IF gr > pr THEN gr := pr END;
		IF gb > pb THEN gb := pb END;
		IF (gl >= gr) OR (gt >= gb) THEN gr := gl; gb := gt END;
		SetClip(f, gl - gx + f.sx, gt - gy + f.sy, gr - gx + f.sx, gb - gy + f.sy);
		(* (f.l, f.t, f.r, f.b) is final clipping rectangle, in local coordinates, and in units *)
		g := f.down; f.down := NIL; p := NIL;
		WHILE g # NIL DO	(* adapt child frames *)
			q := g.next; g.next := NIL;
			AdaptFrameTo(g, g.x, g.y);
			IF g.l = g.r THEN	(* empty child frame: remove *)
				Close(g)
			ELSE	(* insert in new frame list *)
				IF p = NIL THEN f.down := g ELSE p.next := g END;
				p := g
			END;
			g := q
		END
		(* post: frame is set; child frames are set, nonempty, and clipped to frame *)
	END AdaptFrameTo;

	PROCEDURE SetRoot* (root: RootFrame; view: View; front: BOOLEAN; flags: SET);
	BEGIN
		ASSERT(root # NIL, 20); ASSERT(root.rider # NIL, 21);
		ASSERT(view # NIL, 22); ASSERT(view.context # NIL, 23);
		ASSERT(view.Domain() # NIL, 24);
		ASSERT(root.state IN {new, open}, 25);
		root.view := view;
		root.front := front; root.mark := TRUE; root.flags := flags;
		root.state := open;
		IF root.update = NIL THEN NEW(root.update); root.update.n := 0 END
	END SetRoot;

	PROCEDURE AdaptRoot* (root: RootFrame);
	BEGIN
		ASSERT(root # NIL, 20); ASSERT(root.state = open, 21);
		AdaptFrameTo(root, root.x, root.y)
	END AdaptRoot;

	PROCEDURE UpdateRoot* (root: RootFrame; l, t, r, b: INTEGER; rebuild: BOOLEAN);
	BEGIN
		ASSERT(root # NIL, 20); ASSERT(root.state = open, 21);
		AddRect(root, root, l, t, r, b, rebuild)
	END UpdateRoot;

	PROCEDURE RootOf* (f: Frame): RootFrame;
	BEGIN
		ASSERT(f # NIL, 20); ASSERT(f.state = open, 21);
		WHILE f.up # NIL DO f := f.up END;
		RETURN f(RootFrame)
	END RootOf;
	
	PROCEDURE HostOf* (f: Frame): Frame;
	BEGIN
		ASSERT(f # NIL, 20);
		RETURN f.up
	END HostOf;

	PROCEDURE IsPrinterFrame* (f: Frame): BOOLEAN;
		VAR p: Ports.Port;
	BEGIN
		ASSERT(f # NIL, 20); ASSERT(f.state = open, 21);
		p := f.rider.Base();
		RETURN Ports.IsPrinterPort(p)
	END IsPrinterFrame;

	PROCEDURE InstallFrame* (host: Frame; view: View; x, y, level: INTEGER; focus: BOOLEAN);
		VAR e, f, g: Frame; w, h,  l, t, r, b: INTEGER; m: Models.Model; std: StdFrame;
			msg: UpdateCachesMsg; a: View; op: INTEGER;
	BEGIN
		ASSERT(host # NIL, 20); ASSERT(host.state = open, 21);
		ASSERT(view # NIL, 22); ASSERT(view.context # NIL, 23);
		ASSERT(view.Domain() # NIL, 24);
		e := NIL; g := host.down; WHILE (g # NIL) & (g.view # view) DO e := g; g := g.next END;
		IF g = NIL THEN	(* frame for view not yet in child frame list *)
			view.context.GetSize(w, h);
			IF w > MAX(INTEGER) DIV 2 THEN w := MAX(INTEGER) DIV 2 END;
			IF h > MAX(INTEGER) DIV 2 THEN h := MAX(INTEGER) DIV 2 END;
			l := x; t := y; r := x + w; b := y + h;
			(* (l, t, r, b) is child frame rectangle, in local coordinates, and in units *)
			IF (l < host.r) & (t < host.b) & (r > host.l) & (b > host.t) THEN	(* visible *)
				g := NIL; view.GetNewFrame(g);
				IF g = NIL THEN NEW(std); g := std END;
				ASSERT(~(g IS RootFrame), 100);
				e := NIL; f := host.down; WHILE (f # NIL) & (f.level <= level) DO e := f; f := f.next END;
				IF e = NIL THEN g.next := host.down; host.down := g ELSE g.next := e.next; e.next := g END;
				g.down := NIL; g.up := host; g.level := level;
				g.view := view;
				g.ConnectTo(host.rider.Base());
				g.state := open;
				AdaptFrameTo(g, x, y);
				IF ~(handler IN view.bad) THEN
					a := actView; op := actOp; actView := view; actOp := handler;
					view.HandleViewMsg(g, msg);
					actView := a; actOp := op
				END;
				m := view.ThisModel();
				IF m # NIL THEN view.era := Models.Era(m) END;
			END
		ELSE
			IF g.level # level THEN	(* adapt to modified z-order *)
				IF e = NIL THEN host.down := g.next ELSE e.next := g.next END;
				e := NIL; f := host.down; WHILE (f # NIL) & (f.level <= level) DO e := f; f := f.next END;
				IF e = NIL THEN g.next := host.down; host.down := g ELSE g.next := e.next; e.next := g END;
				g.level := level
			END;
			AdaptFrameTo(g, x, y)	(* may close g, leaving g.state = closed *)
			(* possibly optimize: don't call Adapt if x=g.x, y=g.y, "host.era=g.era" *)
		END;
		IF (g # NIL) & (g.state = open) THEN
			IF focus THEN
				g.front := host.front; g.mark := host.mark
			ELSE
				g.front := FALSE; g.mark := FALSE
			END
		END
	END InstallFrame;

	PROCEDURE RemoveAll (f: Frame);
		VAR g, p: Frame;
	BEGIN
		g := f.down; WHILE g # NIL DO p := g.next; RemoveAll(g); Close(g); g := p END;
		f.down := NIL
	END RemoveAll;

	PROCEDURE RemoveFrame* (host, f: Frame);
		VAR g, h: Frame;
	BEGIN
		ASSERT(host # NIL, 20); ASSERT(host.state = open, 21);
		ASSERT(f # NIL, 22); ASSERT(f.up = host, 23);
		g := host.down; h := NIL;
		WHILE (g # NIL) & (g # f) DO h := g; g := g.next END;
		ASSERT(g = f, 24);
		IF h = NIL THEN host.down := f.next ELSE h.next := f.next END;
		RemoveAll(f); Close(f)
	END RemoveFrame;

	PROCEDURE RemoveFrames* (host: Frame; l, t, r, b: INTEGER);
		VAR f, g: Frame; gl, gt, gr, gb: INTEGER;
	BEGIN
		ASSERT(host # NIL, 20); ASSERT(host.state = open, 21);
		IF l < host.l THEN l := host.l END;
		IF t < host.t THEN t := host.t END;
		IF r > host.r THEN r := host.r END;
		IF b > host.b THEN b := host.b END;
		IF (l < r) & (t < b) THEN
			gl := l + host.gx; gt := t + host.gy; gr := r + host.gx; gb := b + host.gy;
			f := host.down;
			WHILE f # NIL DO
				g := f; f := f.next;
				IF (gl < g.r + g.gx) & (g.l + g.gx < gr) & (gt < g.b + g.gy) & (g.t + g.gy < gb) THEN
					RemoveFrame(host, g)
				END
			END
		END
	END RemoveFrames;

	PROCEDURE ThisFrame* (host: Frame; view: View): Frame;
		VAR g: Frame;
	BEGIN
		ASSERT(host # NIL, 20); ASSERT(host.state = open, 21);
		g := host.down; WHILE (g # NIL) & (g.view # view) DO g := g.next END;
		RETURN g
	END ThisFrame;

	PROCEDURE FrameAt* (host: Frame; x, y: INTEGER): Frame;
	(** return frontmost sub-frame of host that contains (x, y) **)
		VAR g, h: Frame;
	BEGIN
		ASSERT(host # NIL, 20); ASSERT(host.state = open, 21);
		g := host.down; h := NIL; INC(x, host.gx); INC(y, host.gy);
		WHILE g # NIL DO
			IF (g.gx + g.l <= x) & (x < g.gx + g.r) & (g.gy + g.t <= y) & (y < g.gy + g.b) THEN
				h := g
			END;
			g := g.next
		END;
		RETURN h
	END FrameAt;

	PROCEDURE ShiftFrames (f: Frame; dx, dy: INTEGER);
		VAR g, h: Frame;
	BEGIN
		g := f.down;
		WHILE g # NIL DO
			h := g; g := g.next;
			AdaptFrameTo(h, h.x + dx, h.y + dy);
			IF h.l = h.r THEN RemoveFrame(f, h) END
		END
	END ShiftFrames;

	PROCEDURE UpdateExposedArea (f: Frame; dx, dy: INTEGER);
		VAR root: RootFrame;
	BEGIN
		root := RootOf(f);
		IF dy > 0 THEN
			AddRect(root, f, f.l, f.t, f.r, f.t + dy, keepFrames);
			IF dx > 0 THEN
				AddRect(root, f, f.l, f.t + dy, f.l + dx, f.b, keepFrames)
			ELSE
				AddRect(root, f, f.r + dx, f.t + dy, f.r, f.b, keepFrames)
			END
		ELSE
			AddRect(root, f, f.l, f.b + dy, f.r, f.b, keepFrames);
			IF dx > 0 THEN
				AddRect(root, f, f.l, f.t, f.l + dx, f.b + dy, keepFrames)
			ELSE
				AddRect(root, f, f.r + dx, f.t, f.r, f.b + dy, keepFrames)
			END
		END
	END UpdateExposedArea;

	PROCEDURE ScrollFrame (f: Frame; dx, dy: INTEGER);
		VAR g: Frame; u, dx0, dy0: INTEGER; bitmapScrolling: BOOLEAN; msg: ScrollClassMsg;
	BEGIN
		g := f.up;
		bitmapScrolling := TRUE;
		IF (g # NIL) THEN
			WHILE bitmapScrolling & (g.up # NIL) DO
				msg.allowBitmapScrolling := FALSE; g.view.HandleViewMsg(g, msg);
				bitmapScrolling := bitmapScrolling & msg.allowBitmapScrolling;
				g  := g.up
			END
		END;
		IF bitmapScrolling THEN
			u := f.unit; dx0 := dx; dy0 := dy;
			INC(dx, f.sx); INC(dy, f.sy); DEC(f.l, f.sx); DEC(f.t, f.sy); DEC(f.r, f.sx); DEC(f.b, f.sy);
			f.sx := dx MOD u; f.sy := dy MOD u;
			DEC(dx, f.sx); DEC(dy, f.sy); INC(f.l, f.sx); INC(f.t, f.sy); INC(f.r, f.sx); INC(f.b, f.sy);
			f.SetOffset(f.gx0 - f.sx, f.gy0 - f.sy);
			ShiftFrames(f, dx0, dy0);
			f.Scroll(dx, dy);
			UpdateExposedArea(f, dx, dy)
		ELSE AddRect(RootOf(f), f, f.l, f.t, f.r, f.b, rebuildFrames)
		END
	END ScrollFrame;

	PROCEDURE BroadcastModelMsg* (f: Frame; VAR msg: Models.Message);
		VAR v, a: View; send: BOOLEAN; op: INTEGER;
	BEGIN
		ASSERT(f # NIL, 20); ASSERT(f.state = open, 21);
		v := f.view;
		IF ~(handler IN v.bad) THEN
			a := actView; op := actOp; actView := v; actOp := handler;
			IF msg.model # NIL THEN
				IF (msg.model = v.ThisModel()) & (msg.era > v.era) THEN
					send := (msg.era - v.era = 1);
					v.era := msg.era;
					IF ~send THEN
						Log.synch := FALSE;
						HALT(100)
					END
				ELSE send := FALSE
				END
			ELSE send := TRUE
			END;
			IF send THEN
				WITH msg: Models.NeutralizeMsg DO
					v.Neutralize
				ELSE
					 v.HandleModelMsg(msg)
				END
			END;
			actView := a; actOp := op
		END;
		f := f.down; WHILE f # NIL DO BroadcastModelMsg(f, msg); f := f.next END
	END BroadcastModelMsg;

	PROCEDURE HandleUpdateMsg (f: Frame; VAR msg: UpdateMsg);
		VAR root: RootFrame; g: Frame; l, t, r, b,  dx, dy: INTEGER;
	BEGIN
		root := RootOf(f);
		IF msg.scroll THEN
			IF root.update.n = 0 THEN
				ScrollFrame(f, msg.dx, msg.dy)
			ELSE
				AddRect(root, f, f.l, f.t, f.r, f.b, msg.rebuild)
			END
		ELSE
			IF msg.all THEN
				IF f # root THEN g := f.up ELSE g := root END;
				dx := f.gx - g.gx; dy := f.gy - g.gy;
				AddRect(root, g, f.l + dx, f.t + dy, f.r + dx, f.b + dy, msg.rebuild)
			ELSE
				l := msg.l; t := msg.t; r := msg.r; b := msg.b;
				IF l < f.l THEN l := f.l END;
				IF t < f.t THEN t := f.t END;
				IF r > f.r THEN r := f.r END;
				IF b > f.b THEN b := f.b END;
				AddRect(root, f, l, t, r, b, msg.rebuild)
			END
		END
	END HandleUpdateMsg;

	PROCEDURE BroadcastViewMsg* (f: Frame; VAR msg: Message);
		VAR v, a: View; op: INTEGER;
	BEGIN
		ASSERT(f # NIL, 20); ASSERT(f.state = open, 21);
		v := f.view;
		IF (msg.view = v) OR (msg.view = NIL) THEN
			WITH msg: UpdateMsg DO
				HandleUpdateMsg(f, msg)
			ELSE
				IF ~(handler IN v.bad) THEN
					a := actView; op := actOp; actView := v; actOp := handler;
					v.HandleViewMsg(f, msg);
					actView := a; actOp := op
				END
			END
		END;
		IF msg.view # v THEN
			f := f.down; WHILE f # NIL DO BroadcastViewMsg(f, msg); f := f.next END
		END
	END BroadcastViewMsg;

	PROCEDURE ForwardCtrlMsg* (f: Frame; VAR msg: CtrlMessage);
		CONST pre = 0; translate = 1; backoff = 2; final = 3;
		VAR v, focus, a: View; g, h: Frame; op: INTEGER; req: BOOLEAN;
	BEGIN
		ASSERT(f # NIL, 20); ASSERT(f.state = open, 21);
		v := f.view;
		focus := NIL; g := f.up; req := FALSE;
		HandleCtrlMsg(pre, f, g, msg, f.mark, f.front, req);
		IF ~(handler IN v.bad) THEN
			a := actView; op := actOp; actView := v; actOp := handler;
			v.HandleCtrlMsg(f, msg, focus);
			actView := a; actOp := op
		END;
		IF focus # NIL THEN	(* propagate msg to another view *)
			IF (f.focus # NIL) & (f.focus.view = focus) THEN	(* cache hit *)
				h := f.focus
			ELSE	(* cache miss *)
				h := f.down; WHILE (h # NIL) & (h.view # focus) DO h := h.next END
			END;
			IF h # NIL THEN
				HandleCtrlMsg(translate, f, h, msg, f.mark, f.front, req);
				f.focus := h; ForwardCtrlMsg(h, msg);
				HandleCtrlMsg(backoff, f, g, msg, f.mark, f.front, req)
			END
		ELSE
			HandleCtrlMsg(final, f, g, msg, f.mark, f.front, req)
		END;
		IF req & (g # NIL) THEN g.view.ConsiderFocusRequestBy(f.view) END
	END ForwardCtrlMsg;


	PROCEDURE RestoreFrame (f: Frame; l, t, r, b: INTEGER);
		VAR rd: Ports.Rider; g: Frame; v, a: View; op: INTEGER;
			u, w, h,  cl, ct, cr, cb,  dx, dy: INTEGER; col: Ports.Color;
	BEGIN
		IF l < f.l THEN l := f.l END;
		IF t < f.t THEN t := f.t END;
		IF r > f.r THEN r := f.r END;
		IF b > f.b THEN b := f.b END;
		IF (l < r) & (t < b) THEN	(* non-empty rectangle to be restored *)
			v := f.view; rd := f.rider; u := f.unit;
			rd.GetRect(cl, ct, cr, cb);	(* save clip rectangle *)
			rd.SetRect((f.gx + l) DIV u, (f.gy + t) DIV u, (f.gx + r) DIV u, (f.gy + b) DIV u);
			IF ~(restore IN v.bad) THEN
				a := actView; op := actOp; actView := v; actOp := restore;
				col := transparent; v.GetBackground(col);
				IF col # transparent THEN f.DrawRect(l, t, r, b, Ports.fill, col) END;
				v.Restore(f, l, t, r, b);
				g := f.down;
				WHILE g # NIL DO	(* loop over all subframes to handle overlaps *)
					dx := f.gx - g.gx; dy := f.gy - g.gy;
					RestoreFrame(g, l + dx, t + dy, r + dx, b + dy);
					g := g.next
				END;
				v.RestoreMarks(f, l, t, r, b);
				actView := a; actOp := op
			END;
			IF v.bad # {} THEN
				IF externalize IN v.bad THEN
					u := f.dot; v.context.GetSize(w, h);
					f.DrawLine(0, 0, w - u, h - u, u, Ports.grey75);
					f.DrawLine(w - u, 0, 0, h - u, u, Ports.grey75)
				END;
				f.MarkRect(l, t, r, b, Ports.fill, Ports.dim25, Ports.show)
			END;
			rd.SetRect(cl, ct, cr, cb)	(* restore current clip rectangle *)
		END
	END RestoreFrame;

	PROCEDURE RestoreRoot* (root: RootFrame; l, t, r, b: INTEGER);
		VAR port: Ports.Port; rd: Ports.Rider;
			u,  gl, gt, gr, gb: INTEGER; col: Ports.Color;
	BEGIN
		ASSERT(root # NIL, 20); ASSERT(root.state = open, 21);
		ASSERT(root.update.n = 0, 22);
		IF l < root.l THEN l := root.l END;
		IF t < root.t THEN t := root.t END;
		IF r > root.r THEN r := root.r END;
		IF b > root.b THEN b := root.b END;
		IF (l < r) & (t < b) THEN
			u := root.unit;
			gl := l + root.gx; gt := t + root.gy; gr := r + root.gx; gb := b + root.gy;
			rd := root.rider; port := rd.Base();
			actFrame := root;
			IF ~IsPrinterFrame(root) THEN port.OpenBuffer(gl DIV u, gt DIV u, gr DIV u, gb DIV u) END;
			col := transparent; root.view.GetBackground(col);
			ASSERT(col # transparent, 100);
			RestoreFrame(root, l, t, r, b);
			IF ~IsPrinterFrame(root) THEN port.CloseBuffer END;
			actFrame := NIL
		END
	END RestoreRoot;

	PROCEDURE ThisCand (f: Frame; v: View): Frame;
	(* find frame g with g.view = v *)
		VAR g: Frame;
	BEGIN
		WHILE (f # NIL) & (f.view # v) DO
			g := ThisCand(f.down, v);
			IF g # NIL THEN f := g ELSE f := f.next END
		END;
		RETURN f
	END ThisCand;

	PROCEDURE ValidateRoot* (root: RootFrame);
		VAR rgn: Region; f: Frame; v: View; i, n: INTEGER;
	BEGIN
		ASSERT(root # NIL, 20); ASSERT(root.state = open, 21);
		rgn := root.update; n := rgn.n; rgn.n := 0; i := 0;
		WHILE i < n DO
			IF rgn.r[i].rebuild THEN
				v := rgn.r[i].v;
				IF v # NIL THEN f := ThisCand(root, v) ELSE f := NIL END;
				IF f = NIL THEN f := root END;
				RemoveFrames(f, rgn.r[i].l - f.gx, rgn.r[i].t - f.gy, rgn.r[i].r - f.gx, rgn.r[i].b - f.gy)
			END;
			INC(i)
		END;
		i := 0;
		WHILE i < n DO
			RestoreRoot(root, rgn.r[i].l - root.gx, rgn.r[i].t - root.gy, rgn.r[i].r - root.gx, rgn.r[i].b - root.gy);
			INC(i)
		END
	END ValidateRoot;

	PROCEDURE MarkBordersIn (f: Frame);
		VAR g: Frame; w, h: INTEGER;
	BEGIN
		g := f.down;
		WHILE g # NIL DO
			g.view.context.GetSize(w, h);
			MarkBorder(f, g.view, g.x, g.y, g.x + w, g.y + h);
			MarkBordersIn(g);
			g := g.next
		END
	END MarkBordersIn;

	PROCEDURE MarkBorders* (root: RootFrame);
	BEGIN
		MarkBordersIn(root)
	END MarkBorders;

	PROCEDURE ReadFont* (VAR rd: Stores.Reader; OUT f: Fonts.Font);
		VAR version: INTEGER;
			fingerprint, size: INTEGER; typeface: Fonts.Typeface; style: SET; weight: INTEGER;
	BEGIN
		rd.ReadVersion(0, 0, version);
		rd.ReadInt(fingerprint);
		rd.ReadXString(typeface); rd.ReadInt(size); rd.ReadSet(style); rd.ReadXInt(weight);
		f := Fonts.dir.This(typeface, size, style, weight); ASSERT(f # NIL, 60);
		IF f.IsAlien() THEN
			Stores.Report("#System:AlienFont", typeface, "", "")
		END
	END ReadFont;

	PROCEDURE WriteFont* (VAR wr: Stores.Writer; f: Fonts.Font);
	BEGIN
		ASSERT(f # NIL, 20);
		wr.WriteVersion(0);
		wr.WriteInt(0);
		wr.WriteXString(f.typeface); wr.WriteInt(f.size); wr.WriteSet(f.style); wr.WriteXInt(f.weight)
	END WriteFont;


	(** view/file interaction **)

	PROCEDURE Old* (ask: BOOLEAN;
						VAR loc: Files.Locator; VAR name: Files.Name; VAR conv: Converters.Converter): View;
		VAR v: View;
	BEGIN
		ASSERT(ask OR (loc # NIL), 20);
		ASSERT(ask OR (name # ""), 21);
		IF ask THEN
			ASSERT(getSpecHook # NIL, 101);
			getSpecHook.GetIntSpec(loc, name, conv)
		END;
		IF (loc # NIL) & (name # "") THEN
			ASSERT(viewHook # NIL, 100);
			v := viewHook.OldView(loc, name, conv)
		ELSE v := NIL
		END;
		RETURN v
	END Old;
	
	PROCEDURE OldView* (loc: Files.Locator; name: Files.Name): View;
		VAR conv: Converters.Converter;
	BEGIN
		conv := NIL;
		RETURN Old(dontAsk, loc, name, conv)
	END OldView;

	PROCEDURE Register* (view: View; ask: BOOLEAN;
				VAR loc: Files.Locator; VAR name: Files.Name; VAR conv: Converters.Converter; OUT res: INTEGER);
	BEGIN
		ASSERT(viewHook # NIL, 100);
		ASSERT(getSpecHook # NIL, 101);
		ASSERT(view # NIL, 20);
		ASSERT(ask OR (loc # NIL), 22); ASSERT(ask OR (name # ""), 23);
		IF ask OR (loc = NIL) OR (name = "") OR (loc.res = 77) THEN
			getSpecHook.GetExtSpec(view, loc, name, conv)
		END;
		IF (loc # NIL) & (name # "") THEN
			viewHook.RegisterView(view, loc, name, conv); res := loc.res
		ELSE res := 7
		END
	END Register;

	PROCEDURE RegisterView* (view: View; loc: Files.Locator; name: Files.Name);
		VAR res: INTEGER; conv: Converters.Converter;
	BEGIN
		conv := NIL;
		Register(view, dontAsk, loc, name, conv, res)
	END RegisterView;

	(** direct view opening **)

	PROCEDURE Open* (view: View; loc: Files.Locator; name: Files.Name; conv: Converters.Converter);
	BEGIN
		ASSERT(view # NIL, 20); ASSERT((loc = NIL) = (name = ""), 21);
		ASSERT(viewHook # NIL, 100);
		viewHook.Open(view, name, loc, name, conv, FALSE, FALSE, FALSE, FALSE, FALSE)
	END Open;

	PROCEDURE OpenView* (view: View);
	BEGIN
		ASSERT(view # NIL, 20);
		Open(view, NIL, "", NIL)
	END OpenView;

	PROCEDURE OpenAux* (view: View; title: Title);
	BEGIN
		ASSERT(view # NIL, 20); ASSERT(viewHook # NIL, 100);
		IF title = "" THEN title := "#System:untitled" END;
		viewHook.Open(view, title, NIL, "", NIL, FALSE, TRUE, FALSE, TRUE, TRUE)
	END OpenAux;


	(** view producer/consumer decoupling **)

	PROCEDURE Deposit* (view: View);
		VAR q: QueueElem;
	BEGIN
		ASSERT(view # NIL, 20);
		NEW(q); q.view := view;
		IF queue.head = NIL THEN queue.head := q ELSE queue.tail.next := q END;
		queue.tail := q; INC(queue.len)
	END Deposit;

	PROCEDURE Fetch* (OUT view: View);
		VAR q: QueueElem;
	BEGIN
		q := queue.head; ASSERT(q # NIL, 20);
		DEC(queue.len); queue.head := q.next;
		IF queue.head = NIL THEN queue.tail := NIL END;
		view := q.view
	END Fetch;

	PROCEDURE Available* (): INTEGER;
	BEGIN
		RETURN queue.len
	END Available;

	PROCEDURE ClearQueue*;
	BEGIN
		queue.len := 0; queue.head := NIL; queue.tail := NIL;
		actView := NIL	(* HACK! prevents invalidation of view due to trap in Dialog.Call *)
	END ClearQueue;


	(** attach controller framework **)

	PROCEDURE InitCtrl* (p: CtrlMsgHandler);
	BEGIN
		ASSERT(HandleCtrlMsg = NIL, 20); HandleCtrlMsg := p
	END InitCtrl;

	PROCEDURE (h: NotifyHook) Notify (id0, id1: INTEGER; opts: SET);
		VAR msg: NotifyMsg;
	BEGIN
		ASSERT(msgHook # NIL, 100);
		msg.id0 := id0; msg.id1 := id1; msg.opts := opts;
		msgHook.Omnicast(msg)
	END Notify;
	
	PROCEDURE Omnicast* (VAR msg: ANYREC);
	BEGIN
		msgHook.Omnicast(msg)
	END Omnicast;

	PROCEDURE HandlePropMsg* (v: View; VAR msg: PropMessage);
		VAR a: View; op: INTEGER;
	BEGIN
		IF ~(handler IN v.bad) THEN
			a := actView; op := actOp; actView := v; actOp := handler;
			v.HandlePropMsg(msg);
			actView := a; actOp := op
		END
	END HandlePropMsg;
	
	
	(* view invalidation *)
	
	PROCEDURE IsInvalid* (v: View): BOOLEAN;
	BEGIN
		RETURN v.bad # {}
	END IsInvalid;
	
	PROCEDURE RevalidateView* (v: View);
	BEGIN
		v.bad := {};
		Update(v, keepFrames)
	END RevalidateView;
	
	PROCEDURE TrapCleanup;
	BEGIN
		copyModel := NIL;
		IF actView # NIL THEN
			INCL(actView.bad, actOp);
			IF actFrame # NIL THEN
				UpdateRoot(actFrame, actFrame.l, actFrame.t, actFrame.r, actFrame.b, keepFrames);
				actFrame := NIL
			END;
			Update(actView, keepFrames);
			actView := NIL
		END
	END TrapCleanup;
	
	PROCEDURE Init;
		VAR h: NotifyHook;
	BEGIN
		NEW(h); Dialog.SetNotifyHook(h);
		domainGuard := 0; ClearQueue;
		Kernel.InstallTrapChecker(TrapCleanup)
	END Init;

BEGIN
	Init
END Views.
