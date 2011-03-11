MODULE StdScrollers;
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

	IMPORT Dialog, Ports, Services, Stores, Models, Views, Properties, Controllers, StdCFrames;
	
	
	CONST
		(* properties & options *)
		horBar* = 0; verBar* = 1; horHide* = 2; verHide* = 3; width* = 4; height* = 5; showBorder* = 6; savePos* = 7;
		

	TYPE
		Prop* = POINTER TO RECORD (Properties.Property)
			horBar*, verBar*: BOOLEAN;
			horHide*, verHide*: BOOLEAN;
			width*, height*: INTEGER;
			showBorder*: BOOLEAN;
			savePos*: BOOLEAN
		END;
		
		ScrollBar = POINTER TO RECORD (Views.View)
			v: View;
			ver: BOOLEAN
		END;
		
		InnerView = POINTER TO RECORD (Views.View)
			v: View
		END;
		
		View = POINTER TO RECORD (Views.View);
			view: Views.View;
			sbW: INTEGER;
			orgX, orgY: INTEGER;
			w, h: INTEGER;						(* = 0: adapt to container *)
			opts: SET;
			(* not persistent *)
			hor, ver: ScrollBar;
			inner: InnerView;
			rgap, bgap: INTEGER;				(* = 0: no scrollbar *)
			border: INTEGER;
			update: Action
		END;
		
		Context = POINTER TO RECORD (Models.Context)
			v: View;
			type: INTEGER
		END;
		
		Action = POINTER TO RECORD (Services.Action)
			v: View
		END;
	
		Op = POINTER TO RECORD (Stores.Operation)
			v: View;
			p: Prop
		END;
		
		SOp = POINTER TO RECORD (Stores.Operation)
			v: View;
			x, y: INTEGER
		END;
		
		UpdateMsg = RECORD (Views.Message)
			changed: BOOLEAN
		END;

	
	VAR
		dialog*: RECORD
			horizontal*, vertical*: RECORD
				mode*: INTEGER;
				adapt*: BOOLEAN;
				size*: REAL
			END;
			showBorder*: BOOLEAN;
			savePos*: BOOLEAN;
			valid, readOnly: SET
		END;
		

	(* tools *)
	
	PROCEDURE CheckPos (v: View; VAR x, y: INTEGER);
		VAR w, h: INTEGER;
	BEGIN
		v.context.GetSize(w, h);
		DEC(w, v.rgap + 2 * v.border);
		DEC(h, v.bgap + 2 * v.border);
		IF x > v.w - w THEN x := v.w - w END;
		IF x < 0 THEN x := 0 END;
		IF y > v.h - h THEN y := v.h - h END;
		IF y < 0 THEN y := 0 END
	END CheckPos;
	
	PROCEDURE InnerFrame (v: View; f: Views.Frame): Views.Frame;
		VAR g, h: Views.Frame;
	BEGIN
		g := Views.ThisFrame(f, v.inner);
		IF g = NIL THEN
			Views.InstallFrame(f, v.inner, v.border, v.border, 0, TRUE);
			g := Views.ThisFrame(f, v.inner)
		END;
		IF g # NIL THEN
			h := Views.ThisFrame(g, v.view);
			IF h = NIL THEN
				Views.InstallFrame(g, v.view, -v.orgX, -v.orgY, 0, TRUE);
				h := Views.ThisFrame(g, v.view)
			END
		END;
		RETURN h
	END InnerFrame;

	PROCEDURE Scroll (v: View; dir: INTEGER; ver: BOOLEAN; p: INTEGER; OUT pos: INTEGER);
		VAR x, y: INTEGER; last: Stores.Operation; op: SOp;
	BEGIN
		x := v.orgX; y := v.orgY;
		IF ver THEN pos := y ELSE pos := x END;
		IF dir = StdCFrames.lineUp THEN
			DEC(pos, 10 * Ports.mm)
		ELSIF dir = StdCFrames.lineDown THEN
			INC(pos, 10 * Ports.mm)
		ELSIF dir = StdCFrames.pageUp THEN
			DEC(pos, 40 * Ports.mm)
		ELSIF dir = StdCFrames.pageDown THEN
			INC(pos, 40 * Ports.mm)
		ELSIF dir = Controllers.gotoPos THEN
			pos := p
		END;
		IF ver THEN CheckPos(v, x, pos); y := pos
		ELSE CheckPos(v, pos, y); x := pos
		END;
		IF (x # v.orgX) OR (y # v.orgY) THEN
			last := Views.LastOp(v);
			IF ~(savePos IN v.opts) OR (last # NIL) & (last IS SOp) THEN
				v.orgX := x; v.orgY := y;
				Views.Update(v.view, Views.keepFrames)
			ELSE
				NEW(op); op.v := v; op.x := x; op.y := y;
				Views.Do(v, "#System:Scrolling", op)
			END
		END
	END Scroll;
	
	PROCEDURE PollSection (v: View; ver: BOOLEAN; OUT size, sect, pos: INTEGER);
		VAR w, h: INTEGER;
	BEGIN
		v.context.GetSize(w, h);
		IF ver THEN size := v.h; sect := h - v.bgap - 2 * v.border; pos := v.orgY
		ELSE size := v.w; sect := w - v.rgap - 2 * v.border; pos := v.orgX
		END
	END PollSection;
	
	
	(* SOp *)
	
	PROCEDURE (op: SOp) Do;
		VAR x, y: INTEGER;
	BEGIN
		x := op.x; op.x := op.v.orgX; op.v.orgX := x;
		y := op.y; op.y := op.v.orgY; op.v.orgY := y;
		Views.Update(op.v.view, Views.keepFrames)
	END Do;
	

	(* properties *)
	
	PROCEDURE (p: Prop) IntersectWith* (q: Properties.Property; OUT equal: BOOLEAN);
		VAR valid: SET;
	BEGIN
		WITH q: Prop DO
			valid := p.valid * q.valid; equal := TRUE;
			IF p.horBar # q.horBar THEN EXCL(valid, horBar) END;
			IF p.verBar # q.verBar THEN EXCL(valid, verBar) END;
			IF p.horHide # q.horHide THEN EXCL(valid, horHide) END;
			IF p.verHide # q.verHide THEN EXCL(valid, verHide) END;
			IF p.width # q.width THEN EXCL(valid, width) END;
			IF p.height # q.height THEN EXCL(valid, height) END;
			IF p.showBorder # q.showBorder THEN EXCL(valid, showBorder) END;
			IF p.savePos # q.savePos THEN EXCL(valid, savePos) END;
			IF p.valid # valid THEN p.valid := valid; equal := FALSE END
		END
	END IntersectWith;
	
	PROCEDURE SetProp (v: View; p: Properties.Property);
		VAR op: Op;
	BEGIN
		WITH p: Prop DO
			NEW(op); op.v := v; op.p := p;
			Views.Do(v, "#System:SetProp", op)
		END
	END SetProp;
	
	PROCEDURE PollProp (v: View; OUT prop: Prop);
		VAR p: Prop;
	BEGIN
		NEW(p);
		p.valid := {horBar, verBar, horHide, verHide, width, height, showBorder, savePos};
		p.readOnly := {width, height} - v.opts;
		p.horBar := horBar IN v.opts;
		p.verBar := verBar IN v.opts;
		p.horHide := horHide IN v.opts;
		p.verHide := verHide IN v.opts;
		p.width := v.w;
		p.height := v.h;
		p.showBorder := showBorder IN v.opts;
		p.savePos := savePos IN v.opts;
		p.known := p.valid; prop := p
	END PollProp;
	
	
	(* Op *)
	
	PROCEDURE (op: Op) Do;
		VAR p: Prop; v: View; valid: SET;
	BEGIN
		v := op.v; p := op.p; PollProp(v, op.p); op.p.valid := p.valid;
		valid := p.valid * ({horBar, verBar, horHide, verHide, showBorder, savePos} + v.opts * {width, height});
		IF horBar IN valid THEN
			IF p.horBar THEN INCL(v.opts, horBar) ELSE EXCL(v.opts, horBar) END
		END;
		IF verBar IN valid THEN
			IF p.verBar THEN INCL(v.opts, verBar) ELSE EXCL(v.opts, verBar) END
		END;
		IF horHide IN valid THEN
			IF p.horHide THEN INCL(v.opts, horHide) ELSE EXCL(v.opts, horHide) END
		END;
		IF verHide IN valid THEN
			IF p.verHide THEN INCL(v.opts, verHide) ELSE EXCL(v.opts, verHide) END
		END;
		IF width IN valid THEN v.w := p.width END;
		IF height IN valid THEN v.h := p.height END;
		IF showBorder IN valid THEN
			IF p.showBorder THEN INCL(v.opts, showBorder); v.border := 2 * Ports.point
			ELSE EXCL(v.opts, showBorder); v.border := 0
			END
		END;
		IF savePos IN valid THEN
			IF p.savePos THEN INCL(v.opts, savePos) ELSE EXCL(v.opts, savePos) END
		END;
		Views.Update(v, Views.rebuildFrames)
	END Do;
	
	
	(* Action *)
	
	PROCEDURE (a: Action) Do;
		VAR msg: UpdateMsg;
	BEGIN
		msg.changed := FALSE;
		Views.Broadcast(a.v, msg);
		IF msg.changed THEN Views.Update(a.v, Views.keepFrames)
		ELSE
			Views.Broadcast(a.v.hor, msg);
			Views.Broadcast(a.v.ver, msg)
		END
	END Do;
	
	
	(* ScrollBars *)
	
	PROCEDURE TrackSB (f: StdCFrames.ScrollBar; dir: INTEGER; VAR pos: INTEGER);
		VAR s: ScrollBar; msg: Controllers.ScrollMsg; pmsg: Controllers.PollSectionMsg; host, inner: Views.Frame;
	BEGIN
		s := f.view(ScrollBar); host := Views.HostOf(f);
		msg.focus := FALSE; msg.vertical := s.ver;
		msg.op := dir; msg.done := FALSE;
		inner := InnerFrame(s.v, host);
		IF inner # NIL THEN Views.ForwardCtrlMsg(inner, msg) END;
		IF msg.done THEN
			pmsg.focus := FALSE; pmsg.vertical := s.ver;
			pmsg.valid := FALSE; pmsg.done := FALSE;
			inner := InnerFrame(s.v, host);
			IF inner # NIL THEN Views.ForwardCtrlMsg(inner, pmsg) END;
			IF pmsg.done THEN
				pos := pmsg.partPos
			END
		ELSE
			Scroll(s.v, dir, s.ver, 0, pos);
			Views.ValidateRoot(Views.RootOf(host))
		END
	END TrackSB;

	PROCEDURE SetSB (f: StdCFrames.ScrollBar; pos: INTEGER);
		VAR s: ScrollBar; msg: Controllers.ScrollMsg; p: INTEGER; host, inner: Views.Frame;
	BEGIN
		s := f.view(ScrollBar); host := Views.HostOf(f);
		msg.focus := FALSE; msg.vertical := s.ver;
		msg.op := Controllers.gotoPos; msg.pos := pos;
		msg.done := FALSE;
		inner := InnerFrame(s.v, host);
		IF inner # NIL THEN Views.ForwardCtrlMsg(inner, msg) END;
		IF ~msg.done THEN
			Scroll(s.v, Controllers.gotoPos, s.ver, pos, p);
			Views.ValidateRoot(Views.RootOf(host))
		END
	END SetSB;

	PROCEDURE GetSB (f: StdCFrames.ScrollBar; OUT size, sect, pos: INTEGER);
		VAR s: ScrollBar; msg: Controllers.PollSectionMsg; host, inner: Views.Frame;
	BEGIN
		s := f.view(ScrollBar); host := Views.HostOf(f);
		msg.focus := FALSE; msg.vertical := s.ver;
		msg.wholeSize := 1; msg.partSize := 0; msg.partPos := 0;
		msg.valid := FALSE; msg.done := FALSE;
		inner := InnerFrame(s.v, host);
		IF inner # NIL THEN Views.ForwardCtrlMsg(inner, msg) END;
		IF msg.done THEN
			IF msg.valid THEN
				size := msg.wholeSize; sect := msg.partSize; pos := msg.partPos
			ELSE
				size := 1; sect := 1; pos := 0
			END
		ELSE
			PollSection(s.v, s.ver, size, sect, pos)
		END
	END GetSB;

	PROCEDURE (s: ScrollBar) GetNewFrame (VAR frame: Views.Frame);
		VAR f: StdCFrames.ScrollBar;
	BEGIN
		f := StdCFrames.dir.NewScrollBar();
		f.disabled := FALSE; f.undef := FALSE; f.readOnly := FALSE;
		f.Track := TrackSB; f.Get := GetSB; f.Set := SetSB;
		frame := f
	END GetNewFrame;

	PROCEDURE (s: ScrollBar) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		WITH f: StdCFrames.Frame DO f.Restore(l, t, r, b) END
	END Restore;
	
	PROCEDURE (s: ScrollBar) HandleCtrlMsg (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
	BEGIN
		WITH f: StdCFrames.Frame DO
			WITH msg: Controllers.PollCursorMsg DO
				f.GetCursor(msg.x, msg.y, msg.modifiers, msg.cursor)
			| msg: Controllers.TrackMsg DO
				f.MouseDown(msg.x, msg.y, msg.modifiers)
			ELSE
			END
		END
	END HandleCtrlMsg;
	
	PROCEDURE (s: ScrollBar) HandleViewMsg (f: Views.Frame; VAR msg: Views.Message);
	BEGIN
		WITH msg: UpdateMsg DO
			WITH f: StdCFrames.Frame DO f.Update() END
		ELSE
		END
	END HandleViewMsg;

	
	(* View *)
	
	PROCEDURE Update (v: View; f: Views.Frame);
		VAR msg: Controllers.PollSectionMsg; w, h: INTEGER; depends: BOOLEAN; inner: Views.Frame;
	BEGIN
		v.bgap := 0; v.rgap := 0; depends := FALSE;
		v.context.GetSize(w, h);
		DEC(w, 2 * v.border); DEC(h, 2 * v.border);
		IF horBar IN v.opts THEN
			IF horHide IN v.opts THEN
				msg.focus := FALSE; msg.vertical := FALSE;
				msg.wholeSize := 1; msg.partSize := 0; msg.partPos := 0;
				msg.valid := FALSE; msg.done := FALSE;
				inner := InnerFrame(v, f);
				IF inner # NIL THEN Views.ForwardCtrlMsg(inner, msg) END;
				IF msg.done THEN
					IF msg.valid THEN v.bgap := v.sbW END
				ELSIF v.w > 0 THEN
					IF w < v.w THEN v.bgap := v.sbW
					ELSIF w - v.sbW < v.w THEN depends := TRUE
					END
				END
			ELSE v.bgap := v.sbW
			END
		END;
		IF verBar IN v.opts THEN
			IF verHide IN v.opts THEN
				msg.focus := FALSE; msg.vertical := TRUE;
				msg.wholeSize := 1; msg.partSize := 0; msg.partPos := 0;
				msg.valid := FALSE; msg.done := FALSE;
				inner := InnerFrame(v, f);
				IF inner # NIL THEN Views.ForwardCtrlMsg(inner, msg) END;
				IF msg.done THEN
					IF msg.valid THEN v.rgap := v.sbW END
				ELSIF v.h > 0 THEN
					IF h - v.bgap < v.h THEN v.rgap := v.sbW END
				END
			ELSE v.rgap := v.sbW
			END
		END;
		IF depends & (v.rgap > 0) THEN v.bgap := v.sbW END;
		CheckPos(v, v.orgX, v.orgY)
	END Update;
	
	PROCEDURE Init (v: View; newView: BOOLEAN);
		CONST min = 2 * Ports.mm; max = MAX(INTEGER); default = 50 * Ports.mm;
		VAR c: Context; x: INTEGER; msg: Properties.ResizePref;
	BEGIN
		IF newView THEN
			v.opts := v.opts + {horBar, verBar, horHide, verHide};
			StdCFrames.dir.GetScrollBarSize(x, v.sbW);
			IF v.view.context # NIL THEN
				v.view.context.GetSize(v.w, v.h);
				v.view := Views.CopyOf(v.view, Views.shallow)
			ELSE
				v.w := Views.undefined; v.h := Views.undefined;
				Properties.PreferredSize(v.view, min, max, min, max, default, default, v.w, v.h)
			END;
			msg.fixed := FALSE;
			msg.horFitToWin := FALSE; msg.verFitToWin := FALSE;
			msg.horFitToPage := FALSE; msg.verFitToPage := FALSE;
			Views.HandlePropMsg(v.view, msg);
			IF ~msg.fixed THEN
				INCL(v.opts, width); INCL(v.opts, height);
				IF msg.horFitToWin OR msg.horFitToPage THEN v.w := 0 END;
				IF msg.verFitToWin OR msg.verFitToPage THEN v.h := 0 END
			END
		END;
		v.rgap := 0; v.bgap := 0;
		IF showBorder IN v.opts THEN v.border := 2 * Ports.point ELSE v.border := 0 END;
		NEW(v.inner); v.inner.v := v;
		NEW(c); c.v := v; c.type := 3; v.inner.InitContext(c);
		NEW(v.hor); v.hor.ver := FALSE; v.hor.v := v;
		NEW(c); c.v := v; c.type := 2; v.hor.InitContext(c);
		NEW(v.ver); v.ver.ver := TRUE; v.ver.v := v;
		NEW(c); c.v := v; c.type := 1; v.ver.InitContext(c);
		NEW(v.update); v.update.v := v;
		Stores.Join(v, v.view);
		Stores.Join(v, v.inner);
		Stores.Join(v, v.hor);
		Stores.Join(v, v.ver);
		Services.DoLater(v.update, Services.now)
	END Init;
	
	PROCEDURE (v: View) Internalize (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER;
	BEGIN
		v.Internalize^(rd);
		IF ~rd.cancelled THEN
			rd.ReadVersion(0, 0, thisVersion);
			IF ~rd.cancelled THEN
				Views.ReadView(rd, v.view);
				rd.ReadInt(v.sbW);
				rd.ReadInt(v.orgX);
				rd.ReadInt(v.orgY);
				rd.ReadInt(v.w);
				rd.ReadInt(v.h);
				rd.ReadSet(v.opts);
				Init(v, FALSE)
			END
		END
	END Internalize;

	PROCEDURE (v: View) Externalize (VAR wr: Stores.Writer);
	BEGIN
		v.Externalize^(wr);
		wr.WriteVersion(0);
		Views.WriteView(wr, v.view);
		wr.WriteInt(v.sbW);
		IF savePos IN v.opts THEN
			wr.WriteInt(v.orgX);
			wr.WriteInt(v.orgY)
		ELSE
			wr.WriteInt(0);
			wr.WriteInt(0)
		END;
		wr.WriteInt(v.w);
		wr.WriteInt(v.h);
		wr.WriteSet(v.opts);
	END Externalize;

	PROCEDURE (v: View) ThisModel(): Models.Model;
	BEGIN
		RETURN v.view.ThisModel()
	END ThisModel;
	
	PROCEDURE (v: View) CopyFromModelView (source: Views.View; model: Models.Model);
	BEGIN
		WITH source: View DO
			IF model = NIL THEN v.view := Views.CopyOf(source.view, Views.deep)
			ELSE v.view := Views.CopyWithNewModel(source.view, model)
			END;
			v.sbW := source.sbW;
			v.orgX := source.orgX;
			v.orgY := source.orgY;
			v.w := source.w;
			v.h := source.h;
			v.opts := source.opts;
		END;
		Init(v, FALSE)
	END CopyFromModelView;
	
	PROCEDURE (v: View) InitContext (context: Models.Context);
		VAR c: Context;
	BEGIN
		v.InitContext^(context);
		IF v.view.context = NIL THEN
			NEW(c); c.v := v; c.type := 0; v.view.InitContext(c)
		END
	END InitContext;

	PROCEDURE (v: View) Neutralize;
	BEGIN
		v.view.Neutralize
	END Neutralize;

	PROCEDURE (v: View) Restore (f: Views.Frame; l, t, r, b: INTEGER);
		VAR w, h: INTEGER;
	BEGIN
		v.context.GetSize(w, h);
		IF showBorder IN v.opts THEN
			v.border := 2 * f.dot;
			f.DrawRect(0, f.dot, w, v.border, Ports.fill, Ports.black);
			f.DrawRect(f.dot, 0, v.border, h, Ports.fill, Ports.black);
			f.DrawRect(0, h - v.border, w, h - f.dot, Ports.fill, Ports.grey25);
			f.DrawRect(w - v.border, 0, w - f.dot, h, Ports.fill, Ports.grey25);
			f.DrawRect(0, 0, w, f.dot, Ports.fill, Ports.grey50);
			f.DrawRect(0, 0, f.dot, h, Ports.fill, Ports.grey50);
			f.DrawRect(0, h - f.dot, w, h, Ports.fill, Ports.white);
			f.DrawRect(w - f.dot, 0, w, h, Ports.fill, Ports.white)
		END;
		Views.InstallFrame(f, v.inner, v.border, v.border, 0, TRUE);
		IF v.bgap > 0 THEN Views.InstallFrame(f, v.hor, v.border, h - v.border - v.bgap, 0, FALSE) END;
		IF v.rgap > 0 THEN Views.InstallFrame(f, v.ver, w - v.border - v.rgap, v.border, 0, FALSE) END
	END Restore;
	
	PROCEDURE (v: View) HandleCtrlMsg (f: Views.Frame; VAR msg: Controllers.Message; VAR focus: Views.View);
		VAR w, h, p, n: INTEGER;smsg: Controllers.ScrollMsg; inner: Views.Frame;
	BEGIN
		WITH msg: Controllers.WheelMsg DO
			smsg.focus := FALSE; smsg.op := msg.op; smsg.pos := 0; smsg.done := FALSE; n := msg.nofLines;
			IF (v.rgap > 0) OR (v.bgap > 0) THEN
				smsg.vertical := v.rgap > 0;
				REPEAT
					smsg.done := FALSE;
					inner := InnerFrame(v, f);
					IF inner # NIL THEN Views.ForwardCtrlMsg(inner, smsg) END;
					IF ~smsg.done THEN
						Scroll(v, smsg.op, smsg.vertical, 0, p);
						Views.ValidateRoot(Views.RootOf(f))
					END;
					DEC(n)
				UNTIL n <= 0;
				msg.done := TRUE
			ELSE
				focus := v.inner
			END
		| msg: Controllers.CursorMessage DO
			v.context.GetSize(w, h);
			IF msg.x > w - v.border - v.rgap THEN
				IF msg.y <= h - v.border - v.bgap THEN focus := v.ver END
			ELSIF msg.y > h - v.border - v.bgap THEN focus := v.hor
			ELSE focus := v.inner
			END
		| msg: Controllers.PollSectionMsg DO
			inner := InnerFrame(v, f);
			IF inner # NIL THEN Views.ForwardCtrlMsg(inner, msg) END;
			IF ~msg.done THEN
				PollSection(v, msg.vertical, msg.wholeSize, msg.partSize, msg.partPos);
				msg.valid := msg.partSize < msg.wholeSize;
				msg.done := TRUE
			END
		| msg: Controllers.ScrollMsg DO
			inner := InnerFrame(v, f);
			IF inner # NIL THEN Views.ForwardCtrlMsg(inner, msg) END;
			IF ~msg.done THEN
				Scroll(v, msg.op, msg.vertical, msg.pos, p);
				Views.ValidateRoot(Views.RootOf(f));
				msg.done := TRUE
			END
		ELSE focus := v.inner
		END;
		IF ~(msg IS Controllers.TickMsg) THEN
			Services.DoLater(v.update, Services.now)
		END
	END HandleCtrlMsg;

	PROCEDURE (v: View) HandleViewMsg (f: Views.Frame; VAR msg: Views.Message);
		VAR b, r: INTEGER;
	BEGIN
		WITH msg: UpdateMsg DO
			b := v.bgap; r := v.rgap;
			Update(v, f);
			IF (v.bgap # b) OR (v.rgap # r) THEN msg.changed := TRUE END
		ELSE
		END
	END HandleViewMsg;

	PROCEDURE (v: View) HandlePropMsg (VAR msg: Properties.Message);
		VAR w, h: INTEGER; p: Properties.Property; prop: Prop; fv: Views.View;
	BEGIN
		WITH msg: Properties.FocusPref DO
			v.context.GetSize(w, h);
			Views.HandlePropMsg(v.view, msg);
			IF msg.atLocation THEN
				IF (msg.x > w - v.border - v.rgap) & (msg.y > h - v.border - v.bgap) THEN
					msg.hotFocus := FALSE; msg.setFocus := FALSE
				ELSIF ((msg.x > w - v.border - v.rgap) OR (msg.y > h - v.border - v.bgap)) & ~msg.setFocus THEN
					msg.hotFocus := TRUE
				END
			END
		| msg: Properties.SizePref DO
			IF (v.w > 0) & (v.h > 0) THEN
				IF msg.w = Views.undefined THEN msg.w := 50 * Ports.mm END;
				IF msg.h = Views.undefined THEN msg.h := 50 * Ports.mm END
			ELSE
				IF msg.w > v.rgap THEN DEC(msg.w, v.rgap + 2 * v.border) END;
				IF msg.h > v.bgap THEN DEC(msg.h, v.bgap + 2 * v.border) END;
				Views.HandlePropMsg(v.view, msg);
				IF msg.w > 0 THEN INC(msg.w, v.rgap + 2 * v.border) END;
				IF msg.h > 0 THEN INC(msg.h, v.bgap + 2 * v.border) END
			END;
			IF msg.w < 3 * v.sbW THEN msg.w := 3 * v.sbW END;
			IF msg.h < 3 * v.sbW THEN msg.h := 3 * v.sbW END
		| msg: Properties.ResizePref DO
			Views.HandlePropMsg(v.view, msg);
			IF v.w > 0 THEN
				msg.fixed := FALSE;
				msg.horFitToWin := TRUE;
				msg.horFitToPage := FALSE
			END;
			IF v.h > 0 THEN
				msg.fixed := FALSE;
				msg.verFitToWin := TRUE;
				msg.verFitToPage := FALSE
			END
		| msg: Properties.BoundsPref DO
			Views.HandlePropMsg(v.view, msg);
			INC(msg.w, 2 * v.border);
			INC(msg.h, 2 * v.border);
			IF (horBar IN v.opts) & ~(horHide IN v.opts) THEN INC(msg.w, v.sbW) END;
			IF (verBar IN v.opts) & ~(verHide IN v.opts) THEN INC(msg.h, v.sbW) END
		| msg: Properties.PollMsg DO
			Views.HandlePropMsg(v.view, msg);
			PollProp(v, prop); Properties.Insert(msg.prop, prop)
		| msg: Properties.SetMsg DO
			p := msg.prop; WHILE (p # NIL) & ~(p IS Prop) DO p := p.next END;
			IF p # NIL THEN SetProp(v, p) END;
			Views.HandlePropMsg(v.view, msg);
		| msg: Properties.ControlPref DO
			fv := msg.focus;
			IF fv = v THEN msg.focus := v.view END;
			Views.HandlePropMsg(v.view, msg);
			msg.focus := fv
		ELSE
			Views.HandlePropMsg(v.view, msg);
		END;
	END HandlePropMsg;
	
	
	(* InnerView *)
	
	PROCEDURE (v: InnerView) GetBackground (VAR color: Ports.Color);
	BEGIN
		color := Ports.background
	END GetBackground;

	PROCEDURE (v: InnerView) Restore (f: Views.Frame; l, t, r, b: INTEGER);
	BEGIN
		Views.InstallFrame(f, v.v.view, -v.v.orgX, -v.v.orgY, 0, TRUE)
	END Restore;
	
	PROCEDURE (v: InnerView) HandleCtrlMsg (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
	BEGIN
		focus := v.v.view
	END HandleCtrlMsg;


	(* Context *)
	
	PROCEDURE (c: Context) MakeVisible (l, t, r, b: INTEGER);
		VAR w, h, x, y: INTEGER;
	BEGIN
		IF ~(savePos IN c.v.opts) THEN
			c.v.context.GetSize(w, h);
			x := c.v.orgX; y := c.v.orgY;
			IF c.v.w > 0 THEN
				DEC(w, c.v.rgap + 2 * c.v.border);
				IF r > x + w - Ports.point THEN x := r - w + Ports.point END;
				IF l < x + Ports.point THEN x := l - Ports.point END;
			END;
			IF c.v.h > 0 THEN
				DEC(h, c.v.bgap + 2 * c.v.border);
				IF b > y + h - Ports.point THEN y := b - h + Ports.point END;
				IF t < y + Ports.point THEN y := t - Ports.point END;
			END;
			IF (x # c.v.orgX) OR (y # c.v.orgY) THEN
				CheckPos(c.v, x, y); c.v.orgX := x; c.v.orgY := y; 
				Views.Update(c.v.view, Views.keepFrames)
			END;
			Services.DoLater(c.v.update, Services.now)
		END
	END MakeVisible;
	
	PROCEDURE (c: Context) Consider (VAR p: Models.Proposal);
	BEGIN
		c.v.context.Consider(p)
	END Consider;
	
	PROCEDURE (c: Context) Normalize (): BOOLEAN;
	BEGIN
		RETURN ~(savePos IN c.v.opts)
	END Normalize;
	
	PROCEDURE (c: Context) GetSize (OUT w, h: INTEGER);
	BEGIN
		c.v.context.GetSize(w, h);
		DEC(w, c.v.rgap + 2 * c.v.border);
		DEC(h, c.v.bgap + 2 * c.v.border);
		IF c.type = 0 THEN
			IF c.v.w > 0 THEN w := c.v.w END;
			IF c.v.h > 0 THEN h := c.v.h END
		ELSIF c.type = 1 THEN
			w := c.v.rgap
		ELSIF c.type = 2 THEN
			h := c.v.bgap
		END
	END GetSize;
	
	PROCEDURE (c: Context) SetSize (w, h: INTEGER);
		VAR w0, h0, w1, h1: INTEGER;
	BEGIN
		ASSERT(c.type = 0, 100);
		c.v.context.GetSize(w0, h0); w1 := w0; h1 := h0;
		IF c.v.w > 0 THEN c.v.w := w
		ELSE w1 := w + c.v.rgap + 2 * c.v.border
		END;
		IF c.v.h > 0 THEN c.v.h := h
		ELSE h1 := h + c.v.bgap + 2 * c.v.border
		END;
		IF (w1 # w0) OR (h1 # h0) THEN
			c.v.context.SetSize(w1, h1)
		END
	END SetSize;
	
	PROCEDURE (c: Context) ThisModel (): Models.Model;
	BEGIN
		RETURN NIL
	END ThisModel;
	
	
	(* dialog *)
	
	PROCEDURE InitDialog*;
		VAR  p: Properties.Property; u: INTEGER;
	BEGIN
		Properties.CollectProp(p);
		WHILE (p # NIL) & ~(p IS Prop) DO p := p.next END;
		IF p # NIL THEN
			WITH p: Prop DO
				IF Dialog.metricSystem THEN u := Ports.mm DIV 10 ELSE u := Ports.inch DIV 100 END;
				dialog.valid := p.valid;
				dialog.readOnly := p.readOnly;
				IF ~p.horBar THEN dialog.horizontal.mode := 0
				ELSIF p.horHide THEN dialog.horizontal.mode := 1
				ELSE dialog.horizontal.mode := 2
				END;
				IF ~p.verBar THEN dialog.vertical.mode := 0
				ELSIF p.verHide THEN dialog.vertical.mode := 1
				ELSE dialog.vertical.mode := 2
				END;
				dialog.horizontal.size := p.width DIV u / 100;
				dialog.vertical.size := p.height DIV u / 100;
				dialog.horizontal.adapt := p.width = 0;
				dialog.vertical.adapt := p.height = 0;
				dialog.showBorder := p.showBorder;
				dialog.savePos := p.savePos
			END
		END
	END InitDialog;
	
	PROCEDURE Set*;
		VAR p: Prop; u: INTEGER;
	BEGIN
		IF Dialog.metricSystem THEN u := 10 * Ports.mm ELSE u := Ports.inch END;
		NEW(p); p.valid := dialog.valid;
		p.horBar := dialog.horizontal.mode # 0;
		p.verBar := dialog.vertical.mode # 0;
		p.horHide := dialog.horizontal.mode = 1;
		p.verHide := dialog.vertical.mode = 1;
		IF ~dialog.horizontal.adapt THEN p.width := SHORT(ENTIER(dialog.horizontal.size * u)) END;
		IF ~dialog.vertical.adapt THEN p.height := SHORT(ENTIER(dialog.vertical.size * u)) END;
		p.showBorder := dialog.showBorder;
		p.savePos := dialog.savePos;
		Properties.EmitProp(NIL, p)
	END Set;
	
	PROCEDURE DialogGuard* (VAR par: Dialog.Par);
		VAR  p: Properties.Property;
	BEGIN
		Properties.CollectProp(p);
		WHILE (p # NIL) & ~(p IS Prop) DO p := p.next END;
		IF p = NIL THEN par.disabled := TRUE END
	END DialogGuard;
	
	PROCEDURE HorAdaptGuard* (VAR par: Dialog.Par);
	BEGIN
		IF width IN dialog.readOnly THEN par.readOnly := TRUE END
	END HorAdaptGuard;
	
	PROCEDURE VerAdaptGuard* (VAR par: Dialog.Par);
	BEGIN
		IF height IN dialog.readOnly THEN par.readOnly := TRUE END
	END VerAdaptGuard;
	
	PROCEDURE WidthGuard* (VAR par: Dialog.Par);
	BEGIN
		IF dialog.horizontal.adapt THEN par.disabled := TRUE
		ELSIF width IN dialog.readOnly THEN par.readOnly := TRUE
		END
	END WidthGuard;
	
	PROCEDURE HeightGuard* (VAR par: Dialog.Par);
	BEGIN
		IF dialog.vertical.adapt THEN par.disabled := TRUE
		ELSIF height IN dialog.readOnly THEN par.readOnly := TRUE
		END
	END HeightGuard;
	
	
	(* commands *)	
	
	PROCEDURE AddScroller*;
		VAR poll: Controllers.PollOpsMsg; v: View; replace: Controllers.ReplaceViewMsg;
	BEGIN
		Controllers.PollOps(poll);
		IF (poll.singleton # NIL) & ~(poll.singleton IS View) THEN
			NEW(v); v.view := poll.singleton; Init(v, TRUE);
			replace.old := poll.singleton; replace.new := v;
			Controllers.Forward(replace)
		ELSE Dialog.Beep
		END
	END AddScroller;

	PROCEDURE RemoveScroller*;
		VAR poll: Controllers.PollOpsMsg; replace: Controllers.ReplaceViewMsg;
	BEGIN
		Controllers.PollOps(poll);
		IF (poll.singleton # NIL) & (poll.singleton IS View) THEN
			replace.old := poll.singleton;
			replace.new := Views.CopyOf(poll.singleton(View).view, Views.shallow);
			Controllers.Forward(replace)
		ELSE Dialog.Beep
		END
	END RemoveScroller;

END StdScrollers.
