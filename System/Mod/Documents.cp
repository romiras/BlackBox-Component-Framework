MODULE Documents;
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
		Kernel, Files, Ports, Dates, Printers,
		Stores, Sequencers, Models, Views, Controllers, Properties,
		Dialog, Printing, Containers;

	CONST
		(** Document.SetPage/PollPage decorate **)
		plain* = FALSE; decorate* = TRUE;
		
		(** Controller.opts **)
		pageWidth* = 16; pageHeight* = 17; winWidth* = 18; winHeight* = 19;

		point = Ports.point;
		mm = Ports.mm;

		defB = 8 * point;	(* defB also used by HostWindows in DefBorders *)

		scrollUnit = 16 * point;
		abort = 1;

		resizingKey = "#System:Resizing";
		pageSetupKey = "#System:PageSetup";
		
		docTag = 6F4F4443H; docVersion = 0;

		minVersion = 0; maxModelVersion = 0; maxCtrlVersion = 0;
		maxDocVersion = 0; maxStdDocVersion = 0;


	TYPE
		Document* = POINTER TO ABSTRACT RECORD (Containers.View) END;

		Context* = POINTER TO ABSTRACT RECORD (Models.Context) END;

		Directory* = POINTER TO ABSTRACT RECORD END;


		Model = POINTER TO RECORD (Containers.Model)
			doc: StdDocument;
			view: Views.View;
			l, t, r, b: INTEGER	(* possibly  r, b >= Views.infinite *)
			(* l, t: constant (= defB) *)
			(* r-l, b-t: invalid in some cases, use PollRect *)
		END;

		Controller = POINTER TO RECORD (Containers.Controller)
			doc: StdDocument
		END;

		StdDocument = POINTER TO RECORD (Document)
			model: Model;
			original: StdDocument;	(* original # NIL => d IS copy of original *)
			pw, ph, pl, pt, pr, pb: INTEGER;	(* invalid if original # NIL, use PollPage *)
			decorate: BOOLEAN;
			x, y: INTEGER	(* scroll state *)
		END;

		StdContext = POINTER TO RECORD (Context)
			model: Model
		END;

		StdDirectory = POINTER TO RECORD (Directory) END;
		
		SetRectOp = POINTER TO RECORD (Stores.Operation)
			model: Model;
			w, h: INTEGER
		END;
		SetPageOp = POINTER TO RECORD (Stores.Operation)
			d: StdDocument;
			pw, ph, pl, pt, pr, pb: INTEGER;
			decorate: BOOLEAN
		END;
		ReplaceViewOp = POINTER TO RECORD (Stores.Operation)
			model: Model;
			new: Views.View
		END;

		PrinterContext = POINTER TO RECORD (Models.Context)
			param: Printing.Par;
			date: Dates.Date;
			time: Dates.Time;
			pr: Printers.Printer;
			l, t, r, b: INTEGER;	(* frame *)
			pw, ph: INTEGER	(* paper *)
		END;
		
		UpdateMsg = RECORD (Views.Message)
			doc: StdDocument
		END;
		
		
		PContext = POINTER TO RECORD (Models.Context)
			view: Views.View;
			w, h: INTEGER	(* content size *)
		END;
		Pager = POINTER TO RECORD (Views.View)
			con: PContext;
			w, h: INTEGER;	(* page size *)
			x, y: INTEGER	(* origin *)
		END;
		
		PrintingHook = POINTER TO RECORD (Printing.Hook) END;

		TrapCleaner = POINTER TO RECORD (Kernel.TrapCleaner) END;

	VAR
		dir-, stdDir-: Directory;
		cleaner: TrapCleaner;
		current: INTEGER;


	(** Cleaner **)

	PROCEDURE (c: TrapCleaner) Cleanup;
	BEGIN
		Printing.par := NIL; current := -1
	END Cleanup;


	(** Document **)

	PROCEDURE (d: Document) Internalize2- (VAR rd: Stores.Reader), EXTENSIBLE;
		VAR thisVersion: INTEGER;
	BEGIN
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxDocVersion, thisVersion)
	END Internalize2;

	PROCEDURE (d: Document) Externalize2- (VAR wr: Stores.Writer), EXTENSIBLE;
	BEGIN
		wr.WriteVersion(maxDocVersion)
	END Externalize2;

	PROCEDURE (d: Document) GetNewFrame* (VAR frame: Views.Frame);
		VAR f: Views.RootFrame;
	BEGIN
		NEW(f); frame := f
	END GetNewFrame;

	PROCEDURE (d: Document) GetBackground* (VAR color: Ports.Color);
	BEGIN
		color := Ports.background
	END GetBackground;
	
	PROCEDURE (d: Document) DocCopyOf* (v: Views.View): Document, NEW, ABSTRACT;
	PROCEDURE (d: Document) SetView* (view: Views.View; w, h: INTEGER), NEW, ABSTRACT;
	PROCEDURE (d: Document) ThisView* (): Views.View, NEW, ABSTRACT;
	PROCEDURE (d: Document) OriginalView* (): Views.View, NEW, ABSTRACT;

	PROCEDURE (d: Document) SetRect* (l, t, r, b: INTEGER), NEW, ABSTRACT;
	PROCEDURE (d: Document) PollRect* (VAR l, t, r, b: INTEGER), NEW, ABSTRACT;
	PROCEDURE (d: Document) SetPage* (w, h, l, t, r, b: INTEGER; decorate: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (d: Document) PollPage* (VAR w, h, l, t, r, b: INTEGER;
																VAR decorate: BOOLEAN), NEW, ABSTRACT;


	(** Context **)

	PROCEDURE (c: Context) ThisDoc* (): Document, NEW, ABSTRACT;


	(** Directory **)

	PROCEDURE (d: Directory) New* (view: Views.View; w, h: INTEGER): Document, NEW, ABSTRACT;


	(* operations *)

	PROCEDURE (op: SetRectOp) Do;
		VAR m: Model; w, h: INTEGER; upd: UpdateMsg;
	BEGIN
		m := op.model;
		w := m.r - m.l; h := m.b - m.t;
		m.r := m.l + op.w; m.b := m.t + op.h;
		op.w := w; op.h := h;
		IF m.doc.context # NIL THEN
			upd.doc := m.doc;
			Views.Domaincast(m.doc.Domain(), upd)
		END
	END Do;

	PROCEDURE (op: SetPageOp) Do;
		VAR d: StdDocument; pw, ph, pl, pt, pr, pb: INTEGER; decorate: BOOLEAN; upd: UpdateMsg;
	BEGIN
		d := op.d;
		pw := d.pw; ph := d.ph; pl := d.pl; pt := d.pt; pr := d.pr; pb := d.pb;
		decorate := d.decorate;
		d.pw := op.pw; d.ph := op.ph; d.pl := op.pl; d.pt := op.pt; d.pr := op.pr; d.pb := op.pb;
		d.decorate := op.decorate;
		op.pw := pw; op.ph := d.ph; op.pl := pl; op.pt := pt; op.pr := pr; op.pb := pb;
		op.decorate := decorate;
		IF d.context # NIL THEN
			upd.doc := d;
			Views.Domaincast(d.Domain(), upd)
		END
	END Do;

	PROCEDURE (op: ReplaceViewOp) Do;
		VAR new: Views.View; upd: UpdateMsg;
	BEGIN
		new := op.new; op.new := op.model.view; op.model.view := new;
		upd.doc := op.model.doc;
		IF upd.doc.context # NIL THEN
			Views.Domaincast(upd.doc.Domain(), upd)
		END
	END Do;


	(* printing support for StdDocument *)

	PROCEDURE CheckOrientation (d: Document; prt: Printers.Printer);
		VAR w, h, l, t, r, b: INTEGER; decorate: BOOLEAN;
	BEGIN
		d.PollPage(w, h, l, t, r, b, decorate);
		prt.SetOrientation(w > h)
	END CheckOrientation;

	PROCEDURE NewPrinterContext (d: Document; prt: Printers.Printer; p: Printing.Par): PrinterContext;
		VAR c: PrinterContext;
			pw, ph,  x0, y0, x1, y1, l, t, r, b: INTEGER; decorate: BOOLEAN;
	BEGIN
		prt.GetRect(x0, y0, x1, y1);
		d.PollPage(pw, ph, l, t, r, b, decorate);
		INC(l, x0); INC(t, y0); INC(r, x0); INC(b, y0);
		NEW(c); (* c.Domain() := d.Domain(); (* dom *)*) c.param := p; Dates.GetDate(c.date); Dates.GetTime(c.time);
		c.pr := prt;
		c.l := l; c.t := t; c.r := r; c.b := b;
		c.pw := pw + 2 * x0; c.ph := ph + 2 * y0;	(* paper reduced to printer range *)
		RETURN c
	END NewPrinterContext;

	PROCEDURE Decorate (c: PrinterContext; f: Views.Frame);
		VAR p: Printing.Par; x0, x1, y, asc, dsc, w: INTEGER; alt: BOOLEAN;
	BEGIN
		p := c.param;
		alt := p.page.alternate & ~ODD(p.page.first + Printing.Current() (* p.page.current *));
		IF alt THEN x0 := c.pw - c.r; x1 := c.pw - c.l
		ELSE x0 := c.l; x1 := c.r
		END;
		IF (alt & (p.header.left # "")) OR (~alt & (p.header.right # "")) THEN
			p.header.font.GetBounds(asc, dsc, w);
			y := c.t - p.header.gap - dsc;
			Printing.PrintBanner(f, p.page, p.header, c.date, c.time, x0, x1, y)
		END;
		IF (alt & (p.footer.left # "")) OR (~alt & (p.footer.right # "")) THEN
			p.footer.font.GetBounds(asc, dsc, w);
			y := c.b + p.footer.gap + asc;
			Printing.PrintBanner(f, p.page, p.footer, c.date, c.time, x0, x1, y)
		END
	END Decorate;


	(* support for StdDocument paging *)

	PROCEDURE HasFocus (v: Views.View; f: Views.Frame): BOOLEAN;
		VAR focus: Views.View; dummy: Controllers.PollFocusMsg;
	BEGIN
		focus := NIL; dummy.focus := NIL;
		v.HandleCtrlMsg(f, dummy, focus);
		RETURN focus # NIL
	END HasFocus;
	
	PROCEDURE ScrollDoc(v: StdDocument; x, y: INTEGER);
	BEGIN
		IF (x # v.x) OR (y # v.y) THEN
			Views.Scroll(v, x - v.x, y - v.y);
			v.x := x; v.y := y
		END
	END ScrollDoc;

	PROCEDURE PollSection (v: StdDocument; f: Views.Frame; VAR msg: Controllers.PollSectionMsg);
		VAR mv: Views.View; g: Views.Frame; vs, ps, ws, p, l, t, r, b: INTEGER; c: Containers.Controller;
	BEGIN
		mv := v.model.view;
		g := Views.ThisFrame(f, mv);
		c := v.ThisController();
		IF c.Singleton() # NIL THEN g := NIL END;
		IF g # NIL THEN Views.ForwardCtrlMsg(g, msg) END;
		IF (g = NIL) OR ~msg.done & (~msg.focus OR ~HasFocus(mv, g)) THEN
			v.PollRect(l, t, r, b);
			IF msg.vertical THEN
				ps := f.b - f.t; vs := b + t; p := -v.y
			ELSE
				ps := f.r - f.l; vs := r + l;  p := -v.x
			END;
			IF ps > vs THEN ps := vs END;
			ws := vs - ps;
			IF p > ws THEN
				p := ws;
				IF msg.vertical THEN ScrollDoc(v, v.x, -p)
				ELSE ScrollDoc(v, -p, v.y)
				END
			END;
			msg.wholeSize := vs;
			msg.partSize := ps;
			msg.partPos := p;
			msg.valid := ws > Ports.point
		END;
		msg.done := TRUE
	END PollSection;

	PROCEDURE Scroll (v: StdDocument; f: Views.Frame; VAR msg: Controllers.ScrollMsg);
		VAR mv: Views.View; g: Views.Frame; vs, ps, ws, p, l, t, r, b: INTEGER; c: Containers.Controller;
	BEGIN
		mv := v.model.view;
		g := Views.ThisFrame(f, mv);
		c := v.ThisController();
		IF c.Singleton() # NIL THEN g := NIL END;
		IF g # NIL THEN Views.ForwardCtrlMsg(g, msg) END;
		IF (g = NIL) OR ~msg.done & (~msg.focus OR ~HasFocus(mv, g)) THEN
			v.PollRect(l, t, r, b);
			IF msg.vertical THEN
				ps := f.b - f.t; vs := b + t; p := -v.y
			ELSE
				ps := f.r - f.l; vs := r + l; p := -v.x
			END;
			ws := vs - ps;
			CASE msg.op OF
			  Controllers.decLine: p := MAX(0, p - scrollUnit)
			| Controllers.incLine: p := MIN(ws, p + scrollUnit)
			| Controllers.decPage: p := MAX(0, p - ps + scrollUnit)
			| Controllers.incPage: p := MIN(ws, p + ps - scrollUnit)
			| Controllers.gotoPos: p := MAX(0, MIN(ws, msg.pos))
			ELSE
			END;
			IF msg.vertical THEN ScrollDoc(v, v.x, -p)
			ELSE ScrollDoc(v, -p, v.y)
			END
		END;
		msg.done := TRUE
	END Scroll;
	
	PROCEDURE MakeVisible* (d: Document; f: Views.Frame; l, t, r, b: INTEGER);
		VAR x, y, w, h, dw, dh, ml, mt, mr, mb: INTEGER;
	BEGIN
		WITH d: StdDocument DO
			d.context.GetSize(w, h);
			x := -d.x; y := -d.y;
			d.PollRect(ml, mt, mr, mb);
			dw := mr + ml - w; dh := mb + mt - h;
			IF dw > 0 THEN
				IF r > x + w - 2 * ml THEN x := r - w + 2 * ml END;
				IF l < x THEN x := l END;
				IF x < 0 THEN x := 0 ELSIF x > dw THEN x := dw END
			END;
			IF dh > 0 THEN
				IF b > y + h - 2 * mt THEN y := b - h + 2 * mt END;
				IF t < y THEN y := t END;
				IF y < 0 THEN y := 0 ELSIF y > dh THEN y := dh END
			END;
			ScrollDoc(d, -x, -y)
		END
	END MakeVisible;

	PROCEDURE Page (d: StdDocument; f: Views.Frame;
								VAR msg: Controllers.PageMsg);
		VAR g: Views.Frame;
	BEGIN
		g := Views.ThisFrame(f, d.model.view);
		IF g # NIL THEN Views.ForwardCtrlMsg(g, msg) END
	END Page;
	

	(* Model *)

	PROCEDURE (m: Model) Internalize (VAR rd: Stores.Reader);
		VAR c: StdContext; thisVersion: INTEGER; l, t, r, b: INTEGER;
	BEGIN
		m.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxModelVersion, thisVersion);
		IF rd.cancelled THEN RETURN END;
		Views.ReadView(rd, m.view);
		rd.ReadInt(l); rd.ReadInt(t); rd.ReadInt(r); rd.ReadInt(b);
		m.l := defB; m.t := defB; m.r := defB + r - l; m.b := defB + b - t;
		NEW(c); c.model := m; m.view.InitContext(c)
	END Internalize;

	PROCEDURE (m: Model) Externalize (VAR wr: Stores.Writer);
	BEGIN
		ASSERT(m.doc.original = NIL, 100);
		m.Externalize^(wr);
		wr.WriteVersion(maxModelVersion);
		Views.WriteView(wr, m.view);
		wr.WriteInt(m.l); wr.WriteInt(m.t); wr.WriteInt(m.r); wr.WriteInt(m.b)
	END Externalize;

	PROCEDURE (m: Model) CopyFrom (source: Stores.Store);
		VAR c: StdContext;
	BEGIN
		WITH source: Model DO
			m.view := Stores.CopyOf(source.view)(Views.View);
			m.l := source.l; m.t := source.t; m.r := source.r; m.b := source.b;
			NEW(c); c.model := m; m.view.InitContext(c)
		END
	END CopyFrom;
	
	PROCEDURE (m: Model) InitFrom (source: Containers.Model);
		VAR c: StdContext;
	BEGIN
		WITH source: Model DO
			m.view := Stores.CopyOf(source.view)(Views.View);
			m.l := source.l; m.t := source.t; m.r := source.r; m.b := source.b;
			NEW(c); c.model := m; m.view.InitContext(c)
		END
	END InitFrom;

	PROCEDURE (m: Model) GetEmbeddingLimits (OUT minW, maxW, minH, maxH: INTEGER);
	BEGIN
		minW := 5 * mm; minH := 5 * mm;
		maxW := MAX(INTEGER) DIV 2; maxH := MAX(INTEGER) DIV 2
	END GetEmbeddingLimits;

	PROCEDURE (m: Model) ReplaceView (old, new: Views.View);
		VAR con: Models.Context; op: ReplaceViewOp;
	BEGIN
		ASSERT(old # NIL, 20); con := old.context;
		ASSERT(con # NIL, 21); ASSERT(con.ThisModel() = m, 22);
		ASSERT(new # NIL, 23);
		ASSERT((new.context = NIL) OR (new.context = con), 24);
		IF new # old THEN
			IF new.context = NIL THEN new.InitContext(con) END;
			Stores.Join(m, new);
			NEW(op); op.model := m; op.new := new;
			Models.Do(m, "#System:ReplaceView", op)
		END
	END ReplaceView;


	(* StdDocument *)

	PROCEDURE (d: StdDocument) Internalize2 (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER; c: Containers.Controller;
	BEGIN
		d.Internalize2^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, maxStdDocVersion, thisVersion);
		IF rd.cancelled THEN RETURN END;
		rd.ReadInt(d.pw); rd.ReadInt(d.ph);
		rd.ReadInt(d.pl); rd.ReadInt(d.pt); rd.ReadInt(d.pr); rd.ReadInt(d.pb);
		rd.ReadBool(d.decorate);
		(* change infinite height to "fit to window" *)
		c := d.ThisController();
		IF (c # NIL) & (d.model.b >= 29000 * Ports.mm) & (c.opts * {winHeight, pageHeight} = {}) THEN
			c.SetOpts(c.opts + {winHeight})
		END;
		c.SetOpts(c.opts - {Containers.noSelection});
		d.x := 0; d.y := 0;
		Stores.InitDomain(d)
	END Internalize2;

	PROCEDURE (d: StdDocument) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		ASSERT(d.original = NIL, 100);
		d.Externalize2^(wr);
		wr.WriteVersion(maxStdDocVersion);
		wr.WriteInt(d.pw); wr.WriteInt(d.ph);
		wr.WriteInt(d.pl); wr.WriteInt(d.pt); wr.WriteInt(d.pr); wr.WriteInt(d.pb);
		wr.WriteBool(d.decorate)
	END Externalize2;

	PROCEDURE (d: StdDocument) CopyFromModelView2 (source: Views.View; model: Models.Model);
	BEGIN
		WITH source: StdDocument DO
			d.pw := source.pw; d.ph := source.ph;
			d.pl := source.pl; d.pt := source.pt; d.pr := source.pr; d.pb := source.pb;
			d.decorate := source.decorate
		END
	END CopyFromModelView2;
	
	PROCEDURE (d: StdDocument) AcceptableModel (m: Containers.Model): BOOLEAN;
	BEGIN
		RETURN m IS Model
	END AcceptableModel;
	
	PROCEDURE (d: StdDocument) InitModel2 (m: Containers.Model);
	BEGIN
		ASSERT((d.model = NIL) OR (d.model = m), 20);
		ASSERT(m IS Model, 23);
		WITH m: Model DO d.model := m; m.doc := d END
	END InitModel2;
	
	PROCEDURE (d: StdDocument) PollRect (VAR l, t, r, b: INTEGER);
		VAR c: Containers.Controller; doc: StdDocument; ww, wh, pw, ph: INTEGER;
	BEGIN
		IF d.original = NIL THEN doc := d ELSE doc := d.original END;
		l := d.model.l; t := d.model.t;
		pw := doc.pr - doc.pl; ph := doc.pb - doc.pt;
		IF d.context = NIL THEN ww := 0; wh := 0
		ELSIF d.context IS PrinterContext THEN ww := pw; wh := ph
		ELSE d.context.GetSize(ww, wh); DEC(ww, 2 * l); DEC(wh, 2 * t)
		END;
		c := d.ThisController();
		IF pageWidth IN c.opts THEN r := l + pw
		ELSIF winWidth IN c.opts THEN
			IF ww > 0 THEN r := l + ww ELSE r := d.model.r END
		ELSE r := l + doc.model.r - doc.model.l
		END;
		IF pageHeight IN c.opts THEN b := t + ph
		ELSIF winHeight IN c.opts THEN 
			IF wh > 0 THEN b := t + wh ELSE b := d.model.b END
		ELSE b := t + doc.model.b - doc.model.t
		END;
		ASSERT(r > l, 60); ASSERT(b > t, 61)
	END PollRect;

	PROCEDURE (d: StdDocument) PollPage (VAR w, h, l, t, r, b: INTEGER; VAR decorate: BOOLEAN);
		VAR doc: StdDocument;
	BEGIN
		IF d.original = NIL THEN doc := d ELSE doc := d.original END;
		w := doc.pw; h := doc.ph;
		l := doc.pl; t := doc.pt; r := doc.pr; b := doc.pb;
		decorate := doc.decorate
	END PollPage;

	PROCEDURE (d: StdDocument) DocCopyOf (v: Views.View): Document;
		VAR c0, c1: Containers.Controller; u: Views.View; new: Document; w, h: INTEGER;
	BEGIN
		ASSERT(v # NIL, 20);
		ASSERT(~(v IS Document), 21);
		ASSERT(d.Domain() = v.Domain(), 22);
		ASSERT(d.Domain() # NIL, 23);
		Views.BeginModification(3, v);  
		u := Views.CopyOf(v, Views.shallow);
		v.context.GetSize(w, h);
		new := dir.New(u, w, h);
		WITH new: StdDocument DO
			IF d.original # NIL THEN new.original := d.original ELSE new.original := d END
		END;
		c0 := d.ThisController();
		c1 := new.ThisController();
		c1.SetOpts(c0.opts);
		Views.EndModification(3, v);
		RETURN new
	END DocCopyOf;

	PROCEDURE (d: StdDocument) Restore (f: Views.Frame; l, t, r, b: INTEGER);
		VAR c: Containers.Controller; m: Model; con: Models.Context; s: Views.View;
	BEGIN
		m := d.model; con := d.context;
		WITH con: PrinterContext DO
			IF con.param.page.alternate & ~ODD(con.param.page.first + Printing.Current()) THEN
				Views.InstallFrame(f, m.view, con.pw - con.r, con.t, 0, FALSE)
			ELSE
				Views.InstallFrame(f, m.view, con.l, con.t, 0, FALSE)
			END
		ELSE
			c := d.ThisController(); s := c.Singleton();
			Views.InstallFrame(f, m.view, m.l + d.x, m.t + d.y, 0, s = NIL)
		END
	END Restore;

	PROCEDURE (d: StdDocument) GetRect (f: Views.Frame; view: Views.View; OUT l, t, r, b: INTEGER);
		VAR l0, t0, r0, b0: INTEGER;
	BEGIN
		d.PollRect(l0, t0, r0, b0);
		l := l0 + d.x; t := t0 + d.y; r := r0 + d.x; b := b0 + d.y
	END GetRect;

	PROCEDURE (d: StdDocument) SetView (view: Views.View; w, h: INTEGER);
		CONST
			wA4 = 210 * mm; hA4 = 296 * mm;	(* A4 default paper size *)
			lm = 20 * mm; tm = 20 * mm; rm = 20 * mm; bm = 20 * mm;
		VAR m: Model; c: StdContext; prt: Printers.Printer;
			ctrl: Containers.Controller; opts: SET; rp: Properties.ResizePref;
			u, minW, maxW, minH, maxH,  defW, defH,  dw, dh,  pw, ph,
			pageW, pageH,  paperW, paperH,  leftM, topM, rightM, botM: INTEGER;
			l, t, r, b: INTEGER; port: Ports.Port;
	BEGIN
		ASSERT(view # NIL, 20); ASSERT(~(view IS Document), 21);
		ASSERT(d.original = NIL, 100);
		m := d.model;
		NEW(c); c.model := m; view.InitContext(c);
		IF d.context # NIL THEN Stores.Join(d, view) END;
		IF Printers.dir # NIL THEN prt := Printers.dir.Current() ELSE prt := NIL END;
		IF prt # NIL THEN
			prt.SetOrientation(FALSE);
			port := prt.ThisPort(); prt.GetRect(l, t, r, b);
			port.GetSize(pw, ph); u := port.unit;
			paperW := r - l; paperH := b - t;
			pageW := paperW - lm - rm; pageH := paperH - tm - bm;
			leftM := lm; topM := tm; rightM := rm; botM := bm;
			IF pageW > pw * u THEN pageW := pw * u END;
			IF pageH > ph * u THEN pageH := ph * u END;
			IF leftM + l < 0 THEN dw := -(leftM + l)
			ELSIF paperW - rightM + l > pw * u THEN dw := pw * u - (paperW - rightM + l)
			ELSE dw := 0
			END;
			IF topM + t < 0 THEN dh := -(topM + t)
			ELSIF paperH - botM + t > ph * u THEN dh := ph * u - (paperH - botM + t)
			ELSE dh := 0
			END;
			INC(leftM, dw); INC(topM, dh); INC(rightM, dw); INC(botM, dh)
		ELSE
			paperW := wA4; paperH := hA4;
			pageW := paperW - lm - rm; pageH := paperH - tm - bm;
			leftM := lm; topM := tm; rightM := rm; botM := bm
		END;
		m.GetEmbeddingLimits(minW, maxW, minH, maxH);
		defW := MAX(minW, pageW - m.l - defB);
		defH := MAX(minH, pageH - m.t - defB);
		Properties.PreferredSize(view, minW, maxW, minH, maxH, defW, defH, w, h);
		opts := {}; rp.fixed := FALSE;
		rp.horFitToPage := FALSE;
		rp.verFitToPage := FALSE;
		rp.horFitToWin := FALSE;
		rp.verFitToWin := FALSE;
		Views.HandlePropMsg(view, rp);
		IF rp.horFitToPage THEN INCL(opts, pageWidth)
		ELSIF rp.horFitToWin THEN INCL(opts, winWidth)
		END;
		IF rp.verFitToPage THEN INCL(opts, pageHeight)
		ELSIF rp.verFitToWin THEN INCL(opts, winHeight)
		END;
		Views.BeginModification(Views.notUndoable, d);
		m.view := view; d.x := 0; d.y := 0;
		ctrl := d.ThisController();
		ctrl.SetOpts(ctrl.opts - {pageWidth..winHeight});
		d.SetPage(paperW, paperH, leftM, topM, paperW - rightM, paperH - botM, plain);
		ASSERT(w > 0, 100); ASSERT(h > 0, 101);
		d.SetRect(m.l, m.t, m.l + w, m.t + h);
		ctrl.SetOpts(ctrl.opts + opts);
		Views.EndModification(Views.notUndoable, d);
		Stores.Join(d, view);
		Views.Update(d, Views.rebuildFrames)
	END SetView;

	PROCEDURE (d: StdDocument) ThisView (): Views.View;
	BEGIN
		RETURN d.model.view
	END ThisView;
	
	PROCEDURE (d: StdDocument) OriginalView (): Views.View;
	BEGIN
		IF d.original = NIL THEN RETURN d.model.view
		ELSE RETURN d.original.model.view
		END
	END OriginalView;

	PROCEDURE (d: StdDocument) SetRect (l, t, r, b: INTEGER);
		VAR m: Model; op: SetRectOp; c: Containers.Controller; w, h: INTEGER;
	BEGIN
		ASSERT(l < r, 22); ASSERT(t < b, 25);
		m := d.model;
		IF (m.l # l) OR (m.t # t) THEN
			m.r := l + m.r - m.l; m.l := l;
			m.b := t + m.b - m.t; m.t := t;
			Views.Update(d, Views.rebuildFrames)
		END;
		IF d.original # NIL THEN m := d.original.model END;
		c := d.ThisController(); w := r - l; h := b - t;
		IF (pageWidth IN c.opts) OR (winWidth IN c.opts) THEN w := m.r - m.l END;
		IF (pageHeight IN c.opts) OR (winHeight IN c.opts) THEN h := m.b - m.t END;
		IF (w # m.r - m.l) OR (h # m.b - m.t) THEN
			NEW(op); op.model := m; op.w:= w; op.h := h;
			Views.Do(d, resizingKey, op)
		END
	END SetRect;

	PROCEDURE (d: StdDocument) SetPage (pw, ph, pl, pt, pr, pb: INTEGER; decorate: BOOLEAN);
		VAR op: SetPageOp; doc: StdDocument;
	BEGIN
		IF d.original = NIL THEN doc := d ELSE doc := d.original END;
		IF (doc.pw # pw) OR (doc.ph # ph) OR (doc.decorate # decorate)
		OR (doc.pl # pl) OR (doc.pt # pt) OR (doc.pr # pr) OR (doc.pb # pb) THEN
			ASSERT(0 <= pw, 20);
			ASSERT(0 <= ph, 22);
			ASSERT(0 <= pl, 24); ASSERT(pl < pr, 25); ASSERT(pr <= pw, 26);
			ASSERT(0 <= pt, 27); ASSERT(pt < pb, 28); ASSERT(pb <= ph, 29);
			NEW(op);
			op.d := doc;
			op.pw := pw; op.ph := ph; op.pl := pl; op.pt := pt; op.pr := pr; op.pb := pb;
			op.decorate := decorate;
			Views.Do(doc, pageSetupKey, op)
		END
	END SetPage;

	PROCEDURE (v: StdDocument) HandleViewMsg2 (f: Views.Frame; VAR msg: Views.Message);
	BEGIN
		WITH msg: UpdateMsg DO
			IF (msg.doc = v) OR (msg.doc = v.original) THEN
				Views.Update(v, Views.rebuildFrames)
			END
		ELSE
		END
	END HandleViewMsg2;

	PROCEDURE (d: StdDocument) HandleCtrlMsg2 (f: Views.Frame; VAR msg: Controllers.Message;
																				VAR focus: Views.View);
	BEGIN
		WITH f: Views.RootFrame DO
			WITH msg: Controllers.PollSectionMsg DO
				PollSection(d, f, msg); focus := NIL
			| msg: Controllers.ScrollMsg DO
				Scroll(d, f, msg); focus := NIL
			| msg: Controllers.PageMsg DO
				Page(d, f, msg); focus := NIL
			ELSE
			END
		END
	END HandleCtrlMsg2;


	(* Controller *)

	PROCEDURE (c: Controller) Internalize2 (VAR rd: Stores.Reader);
		VAR v: INTEGER;
	BEGIN
		rd.ReadVersion(minVersion, maxCtrlVersion, v)
	END Internalize2;

	PROCEDURE (c: Controller) Externalize2 (VAR wr: Stores.Writer);
	BEGIN
		wr.WriteVersion(maxCtrlVersion)
	END Externalize2;

	PROCEDURE (c: Controller) InitView2 (v: Views.View);
	BEGIN
		IF v # NIL THEN c.doc := v(StdDocument) ELSE c.doc := NIL END
	END InitView2;

	PROCEDURE (c: Controller) GetContextType (OUT type: Stores.TypeName);
	END GetContextType;

	PROCEDURE (c: Controller) GetValidOps (OUT valid: SET);
	BEGIN
		IF c.Singleton() # NIL THEN
			valid := {Controllers.copy}
		END
	END GetValidOps;

	PROCEDURE (c: Controller) NativeModel (m: Models.Model): BOOLEAN;
	BEGIN
		RETURN m IS Model
	END NativeModel;

	PROCEDURE (c: Controller) NativeView (v: Views.View): BOOLEAN;
	BEGIN
		RETURN v IS StdDocument
	END NativeView;

	PROCEDURE (c: Controller) NativeCursorAt (f: Views.Frame; x, y: INTEGER): INTEGER;
	BEGIN
		RETURN Ports.arrowCursor
	END NativeCursorAt;

	PROCEDURE (c: Controller) PollNativeProp (selection: BOOLEAN; VAR p: Properties.Property;
																		VAR truncated: BOOLEAN);
	END PollNativeProp;

	PROCEDURE (c: Controller) SetNativeProp (selection: BOOLEAN; p, old: Properties.Property);
	END SetNativeProp;

	PROCEDURE (c: Controller) GetFirstView (selection: BOOLEAN; OUT v: Views.View);
	BEGIN
		IF selection THEN v := c.Singleton() ELSE v := c.doc.model.view END
	END GetFirstView;

	PROCEDURE (c: Controller) GetNextView (selection: BOOLEAN; VAR v: Views.View);
	BEGIN
		v := NIL
	END GetNextView;

	PROCEDURE (c: Controller) GetPrevView (selection: BOOLEAN; VAR v: Views.View);
	BEGIN
		v := NIL
	END GetPrevView;

	PROCEDURE (c: Controller) TrackMarks (f: Views.Frame; x, y: INTEGER;
															units, extend, add: BOOLEAN);
	BEGIN
		c.Neutralize
	END TrackMarks;
	
	PROCEDURE (c: Controller) RestoreMarks2 (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		IF c.doc.context IS PrinterContext THEN Decorate(c.doc.context(PrinterContext), f) END
	END RestoreMarks2;

	PROCEDURE (c: Controller) Resize (view: Views.View; l, t, r, b: INTEGER);
		VAR d: StdDocument; l0, t0: INTEGER;
	BEGIN
		d := c.doc;
		ASSERT(view = d.model.view, 20);
		l0 := d.model.l; t0 := d.model.t;
		d.SetRect(l0, t0, l0 + r - l, t0 + b - t)
	END Resize;

	PROCEDURE (c: Controller) DeleteSelection;
	END DeleteSelection;

	PROCEDURE (c: Controller) MoveLocalSelection (f, dest: Views.Frame; x, y: INTEGER;
														dx, dy: INTEGER);
		VAR m: Model; l, t, r, b: INTEGER;
	BEGIN
		IF f = dest THEN
			m := c.doc.model; DEC(dx, x); DEC(dy, y);
			l := m.l + dx; t := m.t + dy;
			r := m.r + dx; b := m.b + dy;
			c.Resize(m.view, l, t, r, b);
			IF c.Singleton() = NIL THEN c.SetSingleton(m.view) END
		END
	END MoveLocalSelection;

	PROCEDURE (c: Controller) SelectionCopy (): Model;
	BEGIN
		RETURN NIL
	END SelectionCopy;

	PROCEDURE (c: Controller) NativePaste (m: Models.Model; f: Views.Frame);
		VAR m0: Model;
	BEGIN
		WITH m: Model DO
			m0 := c.doc.model;
			m0.ReplaceView(m0.view, m.view);
			c.doc.SetRect(m.l, m.t, m.r, m.b)
		END
	END NativePaste;

	PROCEDURE (c: Controller) PasteView (f: Views.Frame; v: Views.View; w, h: INTEGER);
		VAR m: Model; minW, maxW, minH, maxH, defW, defH: INTEGER;
	BEGIN
		m := c.doc.model;
		m.GetEmbeddingLimits(minW, maxW, minH, maxH);
		defW := m.r - m.l; defH := m.b - m.t;
		Properties.PreferredSize(v, minW, maxW, minH, maxH, defW, defH, w, h);
		m.ReplaceView(m.view, v);
		c.doc.SetRect(m.l, m.t, m.l + w, m.t + h)
	END PasteView;

	PROCEDURE (c: Controller) Drop (src, dst: Views.Frame; sx, sy, x, y, w, h, rx, ry: INTEGER;
												v: Views.View; isSingle: BOOLEAN);
		VAR m: Model; minW, maxW, minH, maxH, defW, defH: INTEGER;
	BEGIN
		m := c.doc.model;
		m.GetEmbeddingLimits(minW, maxW, minH, maxH);
		defW := m.r - m.l; defH := m.b - m.t;
		Properties.PreferredSize(v, minW, maxW, minH, maxH, defW, defH, w, h);
		m.ReplaceView(m.view, v);
		c.doc.SetRect(m.l, m.t, m.l + w, m.t + h)
	END Drop;

	(* selection *)

	PROCEDURE (c: Controller) Selectable (): BOOLEAN;
	BEGIN
		RETURN TRUE
	END Selectable;

	PROCEDURE (c: Controller) SelectAll (select: BOOLEAN);
	BEGIN
		IF ~select & (c.Singleton() # NIL) THEN
			c.SetSingleton(NIL)
		ELSIF select & (c.Singleton() = NIL) THEN
			c.SetSingleton(c.doc.model.view)
		END
	END SelectAll;

	PROCEDURE (c: Controller) InSelection (f: Views.Frame; x, y: INTEGER): BOOLEAN;
	BEGIN
		RETURN c.Singleton() # NIL
	END InSelection;

	(* caret *)

	PROCEDURE (c: Controller) HasCaret (): BOOLEAN;
	BEGIN
		RETURN FALSE
	END HasCaret;

	PROCEDURE (c: Controller) MarkCaret (f: Views.Frame; show: BOOLEAN);
	END MarkCaret;

	PROCEDURE (c: Controller) CanDrop (f: Views.Frame; x, y: INTEGER): BOOLEAN;
	BEGIN
		RETURN FALSE
	END CanDrop;

	(* handlers *)

	PROCEDURE (c: Controller) HandleCtrlMsg (f: Views.Frame;
								 VAR msg: Controllers.Message; VAR focus: Views.View);
		VAR l, t, r, b: INTEGER;
	BEGIN
		IF ~(Containers.noFocus IN c.opts) THEN
			WITH msg: Controllers.TickMsg DO
				IF c.Singleton() = NIL THEN c.SetFocus(c.doc.model.view) END
			| msg: Controllers.CursorMessage DO
				IF c.Singleton() = NIL THEN	(* delegate to focus, even if not directly hit *)
					focus := c.ThisFocus();
					c.doc.GetRect(f, focus, l, t, r, b);	(* except for resize in lower right corner *)
					IF (c.opts * {pageWidth..winHeight} # {})
						OR (msg.x < r) OR (msg.y < b) THEN RETURN END
				END
			ELSE
			END
		END;
		c.HandleCtrlMsg^(f, msg, focus)
	END HandleCtrlMsg;
	
	
	PROCEDURE (c: Controller) PasteChar (ch: CHAR);
	END PasteChar;
	
	PROCEDURE (c: Controller) ControlChar (f: Views.Frame; ch: CHAR);
	END ControlChar;
	
	PROCEDURE (c: Controller) ArrowChar (f: Views.Frame; ch: CHAR; units, select: BOOLEAN);
	END ArrowChar;
	
	PROCEDURE (c: Controller) CopyLocalSelection (src, dst: Views.Frame; sx, sy, dx, dy: INTEGER);
	END CopyLocalSelection;


	(* StdContext *)

	PROCEDURE (c: StdContext) ThisModel (): Models.Model;
	BEGIN
		RETURN c.model
	END ThisModel;

	PROCEDURE (c: StdContext) GetSize (OUT w, h: INTEGER);
		VAR m: Model; dc: Models.Context; l, t, r, b: INTEGER;
	BEGIN
		m := c.model;
		m.doc.PollRect(l, t, r, b); w := r - l; h := b - t;
		dc := m.doc.context;
		IF dc # NIL THEN
			WITH dc: PrinterContext DO
				w := MIN(w, dc.r - dc.l); h := MIN(h, dc.b - dc.t)
			ELSE
			END
		END;
		ASSERT(w > 0, 60); ASSERT(h > 0, 61)
	END GetSize;

	PROCEDURE (c: StdContext) SetSize (w, h: INTEGER);
		VAR m: Model; d: StdDocument; minW, maxW, minH, maxH,  defW, defH: INTEGER;
	BEGIN
		m := c.model; d := m.doc; ASSERT(d # NIL, 20);
		m.GetEmbeddingLimits(minW, maxW, minH, maxH);
		defW := m.r - m.l; defH := m.b - m.t;
		Properties.PreferredSize(m.view, minW, maxW, minH, maxH, defW, defH, w, h);
		d.SetRect(m.l, m.t, m.l + w, m.t + h)
	END SetSize;

	PROCEDURE (c: StdContext) Normalize (): BOOLEAN;
	BEGIN
		RETURN TRUE
	END Normalize;

	PROCEDURE (c: StdContext) ThisDoc (): Document;
	BEGIN
		RETURN c.model.doc
	END ThisDoc;

	PROCEDURE (c: StdContext) MakeVisible (l, t, r, b: INTEGER);
	BEGIN
		MakeVisible(c.model.doc, NIL, l, t, r, b)
	END MakeVisible;


	(* PrinterContext *)

	PROCEDURE (c: PrinterContext) GetSize (OUT w, h: INTEGER);
		VAR p: Ports.Port;
	BEGIN
		p := c.pr.ThisPort();
		p.GetSize(w, h);
		w := w * p.unit;
		h := h * p.unit
	END GetSize;

	PROCEDURE (c: PrinterContext) Normalize (): BOOLEAN;
	BEGIN
		RETURN TRUE
	END Normalize;
	
	PROCEDURE (c: PrinterContext) SetSize (w, h: INTEGER);
	END SetSize;
	
	PROCEDURE (c: PrinterContext) ThisModel (): Models.Model;
	BEGIN
		RETURN NIL
	END ThisModel;


	(* StdDirectory *)

	PROCEDURE (d: StdDirectory) New (view: Views.View; w, h: INTEGER): Document;
		VAR doc: StdDocument; m: Model; c: Controller;
	BEGIN
		ASSERT(view # NIL, 20); ASSERT(~(view IS Document), 21);
		NEW(m);
		NEW(doc); doc.InitModel(m);
		NEW(c); doc.SetController(c);
		doc.SetRect(defB, defB, defB + 1, defB + 1);	(* set top-left point *)
		doc.SetView(view, w, h);	(* joins store graphs of doc and view *)
		Stores.InitDomain(doc);	(* domains of new documents are bound *)
		RETURN doc
	END New;


	(** PContext **)

	PROCEDURE (c: PContext) GetSize (OUT w, h: INTEGER);
	BEGIN
		w := c.w; h := c.h
	END GetSize;
	
	PROCEDURE (c: PContext) Normalize (): BOOLEAN;
	BEGIN
		RETURN TRUE
	END Normalize;
	
	PROCEDURE (c: PContext) SetSize (w, h: INTEGER);
	END SetSize;
	
	PROCEDURE (c: PContext) ThisModel (): Models.Model;
	BEGIN
		RETURN NIL
	END ThisModel;
	

	(** Pager **)
	

	PROCEDURE (p: Pager) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		Views.InstallFrame(f, p.con.view, -p.x, -p.y, 0, FALSE)
	END Restore;
	
	PROCEDURE (p: Pager) HandleCtrlMsg (f: Views.Frame; VAR msg: Views.CtrlMessage; VAR focus: Views.View);
		VAR v: Views.View; g: Views.Frame;
	BEGIN
		WITH msg: Controllers.PageMsg DO
			v := p.con.view; g := Views.ThisFrame(f, v);
			IF g = NIL THEN
				Views.InstallFrame(f, v, 0, 0, 0, FALSE);
				g := Views.ThisFrame(f, v)
			END;
			IF g # NIL THEN
				Views.ForwardCtrlMsg(g, msg);
				IF ~msg.done THEN
					IF p.con.w > p.w THEN 	(* needs horizontal paging *)
						IF msg.op = Controllers.gotoPageX THEN p.x := msg.pageX * p.w; msg.done := TRUE
						ELSIF msg.op = Controllers.nextPageX THEN p.x := p.x + p.w; msg.done := TRUE
						END;
						IF p.x >= p.con.w THEN msg.eox := TRUE; p.x := 0 END
					END;
					IF p.con.h > p.h THEN	(* needs vertical paging *)
						IF msg.op = Controllers.gotoPageY THEN p.y := msg.pageY * p.h; msg.done := TRUE
						ELSIF msg.op = Controllers.nextPageY THEN p.y := p.y + p.h; msg.done := TRUE
						END;
						IF p.y >= p.con.h THEN msg.eoy := TRUE; p.y := 0 END
					END
				END
			END
		ELSE focus := p.con.view
		END
	END HandleCtrlMsg;
	
	PROCEDURE NewPager (v: Views.View; w, h, pw, ph: INTEGER): Pager;
		VAR p: Pager; c: PContext;
	BEGIN
		NEW(c); c.view := v; c.w := w; c.h := h; v.InitContext(c);
		NEW(p); p.con := c; p.w := pw; p.h := ph; p.x := 0; p.y := 0;
		Stores.Join(v, p);
		RETURN p
	END NewPager;
	
	PROCEDURE PrinterDoc (d: Document; c: PrinterContext): Document;
		VAR v, u, p: Views.View; w, h, l, t, r, b, pw, ph: INTEGER; pd: Document;
			ct: Containers.Controller; dec: BOOLEAN; seq: ANYPTR;
	BEGIN
		v := d.ThisView();

		IF d.Domain() # NIL THEN seq:=d.Domain().GetSequencer();
			IF seq#NIL THEN seq(Sequencers.Sequencer).BeginModification(Sequencers.invisible, d) END
		END;
		u := Views.CopyOf(v, Views.shallow);
		IF d.Domain() # NIL THEN seq:=d.Domain().GetSequencer();
			IF seq#NIL THEN seq(Sequencers.Sequencer).EndModification(Sequencers.invisible, d) END
		END;

		d.PollPage(w, h, l, t, r, b, dec); pw := r - l; ph := b - t;	(* page size *)
		v.context.GetSize(w, h);
		ct := d.ThisController();
		IF winWidth IN ct.opts THEN w := pw END;	(* fit to win -> fit to page *)
		IF winHeight IN ct.opts THEN h := ph END;
		p := NewPager(u, w, h, pw, ph);
		ASSERT(Stores.Joined(p, d), 100);
		pd := dir.New(p, pw, ph);
		pd.InitContext(c);
		RETURN pd
	END PrinterDoc;
	

	(** miscellaneous **)

	PROCEDURE Print* (d: Document; p: Printers.Printer; par: Printing.Par);
		VAR dom: Stores.Domain; d1: Document; f: Views.RootFrame; g: Views.Frame;
			c: PrinterContext; from, to, this, copies, w, h, u, k: INTEGER; page: Controllers.PageMsg;
			title: Views.Title; port: Ports.Port;
	BEGIN
		ASSERT(d # NIL, 20); ASSERT(p # NIL, 21);
		ASSERT(par # NIL, 22);
		ASSERT(par.page.from >= 0, 23); ASSERT(par.page.from <= par.page.to, 24);
		ASSERT(par.copies > 0, 25);
		IF (par.header.right # "") OR (par.page.alternate & (par.header.left # "")) THEN
			ASSERT(par.header.font # NIL, 26)
		END;
		IF (par.footer.right # "") OR (par.page.alternate & (par.footer.left # "")) THEN
			ASSERT(par.footer.font # NIL, 27)
		END;
		IF par.page.title = "" THEN title := "(" + Dialog.appName + ")" ELSE title := par.page.title END;
		from := par.page.from; to := par.page.to;
		copies := par.copies;
		CheckOrientation(d, p);
		p.OpenJob(copies, title);
		IF p.res = 0 THEN
			dom := d.Domain();
			ASSERT(dom # NIL, 100);
			c := NewPrinterContext(d, p, par);
			d1 := PrinterDoc(d, c);
			CheckOrientation(d, p);	(* New in PrinterDoc resets printer orientation *)
			d1.GetNewFrame(g); f := g(Views.RootFrame); f.ConnectTo(p.ThisPort());
			Views.SetRoot(f, d1, FALSE, {}); Views.AdaptRoot(f);
			current := 0; (*par.page.current := 0; *)
			d1.Restore(f, 0, 0, 0, 0);	(* install frame for doc's view *)
			Kernel.PushTrapCleaner(cleaner);
			port := p.ThisPort();
			Printing.par := par;
			page.op := Controllers.gotoPageX; page.pageX := 0;
			page.done := FALSE; page.eox := FALSE;
			Views.ForwardCtrlMsg(f, page);
			IF page.done THEN this := 0 ELSE this := from END;
			page.op := Controllers.gotoPageY; page.pageY := this;
			page.done := FALSE; page.eoy := FALSE;
			Views.ForwardCtrlMsg(f, page);
			IF ~page.done & (from > 0) OR page.eox OR page.eoy THEN to := -1 END;
			WHILE this <= to DO
				IF this >= from THEN
					current := this; (*par.page.current := this;*)
					port.GetSize(w, h); u := port.unit;
					FOR k := copies TO par.copies DO
						p.OpenPage;
						IF p.res = 0 THEN
							Views.RemoveFrames(f, 0, 0, w * u, h * u);
							Views.RestoreRoot(f, 0, 0, w * u, h * u)
						END;
						p.ClosePage
					END
				END;
				IF p.res # abort THEN INC(this) ELSE to := -1 END;
				IF this <= to THEN
					page.op := Controllers.nextPageX;
					page.done := FALSE; page.eox := FALSE;
					Views.ForwardCtrlMsg(f, page);
					IF ~page.done OR page.eox THEN
						IF page.done THEN
							page.op := Controllers.gotoPageX; page.pageX := 0;
							page.done := FALSE; page.eox := FALSE;
							Views.ForwardCtrlMsg(f, page)
						END;
						page.op := Controllers.nextPageY;
						page.done := FALSE; page.eoy := FALSE;
						Views.ForwardCtrlMsg(f, page);
						IF ~page.done OR page.eoy THEN to := -1 END
					END
				END
			END;
			Printing.par := NIL;
			Kernel.PopTrapCleaner(cleaner)
		ELSE Dialog.ShowMsg("#System:FailedToOpenPrintJob")
		END;
		p.CloseJob
	END Print;

	PROCEDURE (hook: PrintingHook) Current(): INTEGER;
	BEGIN
		RETURN current
	END Current;
	
	PROCEDURE (hook: PrintingHook) Print (v: Views.View; par: Printing.Par);
		VAR dom: Stores.Domain;  d: Document; f: Views.RootFrame; c: PrinterContext;
			w, h, u: INTEGER; p: Printers.Printer; g: Views.Frame; title: Views.Title;
			k, copies: INTEGER; port: Ports.Port;
	BEGIN
		ASSERT(v # NIL, 20);
		p := Printers.dir.Current();
		ASSERT(p # NIL, 21);
		IF v IS Document THEN Print(v(Document), p, par); RETURN END;
		IF (v.context # NIL) & (v.context IS Context) THEN
			Print(v.context(Context).ThisDoc(), p, par); RETURN
		END;
		p.SetOrientation(FALSE);
		IF par.page.title = "" THEN title := "(" + Dialog.appName + ")" ELSE title := par.page.title END;
		copies := par.copies;
		p.OpenJob(copies, title);
		IF p.res = 0 THEN
			Printing.par := par;
			Stores.InitDomain(v);
			dom := v.Domain();
			v := Views.CopyOf(v, Views.shallow) ;
			d := dir.New(v, Views.undefined, Views.undefined);
			c := NewPrinterContext(d, (* dom, *) p, par);
			d.InitContext(c); (* Stores.InitDomain(d, c.Domain()); (* nicht mehr noetig *) *)
			d.GetNewFrame(g); f := g(Views.RootFrame); 
			port := p.ThisPort(); f.ConnectTo(port);
			Views.SetRoot(f, d, FALSE, {}); Views.AdaptRoot(f);
			port.GetSize(w, h); u := port.unit;
			FOR k := copies TO par.copies DO
				p.OpenPage;
				IF p.res = 0 THEN
					Views.RemoveFrames(f, 0, 0, w * u, h * u); Views.RestoreRoot(f, 0, 0, w * u, h * u)
				END;
				p.ClosePage
			END
		END;
		Printing.par := NIL;
		p.CloseJob
	END Print;


	PROCEDURE ImportDocument* (f: Files.File; OUT s: Stores.Store);
		VAR r: Stores.Reader; tag, version: INTEGER;
	BEGIN
		ASSERT(f # NIL, 20);
		r.ConnectTo(f);
		r.ReadInt(tag);
		IF tag = docTag THEN
			r.ReadInt(version);
			ASSERT(version = docVersion, 100);
			r.ReadStore(s);
			IF s IS Document THEN s := s(Document).ThisView()
			ELSE s := NIL
			END
		END
	END ImportDocument;

	PROCEDURE ExportDocument* (s: Stores.Store; f: Files.File);
		VAR w: Stores.Writer; v: Views.View;
	BEGIN
		ASSERT(s # NIL, 20);
		ASSERT(s IS Views.View, 21);
		ASSERT(f # NIL, 22);
		v := s(Views.View);
		IF (v.context # NIL) & (v.context IS Context) THEN
			v := v.context(Context).ThisDoc()
		END;
		IF ~(v IS Document) THEN
			IF v.context # NIL THEN
				v := Views.CopyOf(v, Views.shallow)
			END;
			v := dir.New(v, Views.undefined, Views.undefined)
		END;
		w.ConnectTo(f);
		w.WriteInt(docTag); w.WriteInt(docVersion);
		w.WriteStore(v)
	END ExportDocument;


	PROCEDURE SetDir* (d: Directory);
	BEGIN
		ASSERT(d # NIL, 20);
		dir := d;
		IF stdDir = NIL THEN stdDir := d END
	END SetDir;
	
	PROCEDURE Init;
		VAR d: StdDirectory; h: PrintingHook;
	BEGIN
		NEW(d); SetDir(d);
		NEW(h); Printing.SetHook(h);
		NEW(cleaner)
	END Init;

BEGIN
	Init
END Documents.
