MODULE StdCmds;
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
		Fonts, Ports, Services, Stores, Sequencers, Models, Views,
		Controllers, Containers, Properties, Dialog, Documents, Windows, Strings,
		StdDialog, StdApi;

	CONST
		illegalSizeKey = "#System:IllegalFontSize";
		defaultAllocator = "TextViews.Deposit; StdCmds.Open";

		(* wType, hType *)
		fix = 0; page = 1; window = 2;

	VAR
		size*: RECORD 
			size*: INTEGER
		END;
		layout*: RECORD
			wType*, hType*: INTEGER;
			width*, height*: REAL;
			doc: Documents.Document;
			u: INTEGER
		END;
		allocator*: Dialog.String;
		
		propEra: INTEGER;	(* (propEra, props) form cache for StdProps() *)
		props: Properties.StdProp;	(* valid iff propEra = Props.era *)
		
		prop: Properties.Property;	(* usef for copy/paste properties *)

	(* auxiliary procedures *)
	
	PROCEDURE StdProp (): Properties.StdProp;
	BEGIN
		IF propEra # Properties.era THEN
			Properties.CollectStdProp(props);
			propEra := Properties.era
		END;
		RETURN props
	END StdProp;

	PROCEDURE Append (VAR s: ARRAY OF CHAR; t: ARRAY OF CHAR);
		VAR len, i, j: INTEGER; ch: CHAR;
	BEGIN
		len := LEN(s);
		i := 0; WHILE s[i] # 0X DO INC(i) END;
		j := 0; REPEAT ch := t[j]; s[i] := ch; INC(j); INC(i) UNTIL (ch = 0X) OR (i = len);
		s[len - 1] := 0X
	END Append;	

	(* standard commands *)

	PROCEDURE OpenAuxDialog* (file, title: ARRAY OF CHAR);
		VAR v: Views.View;
	BEGIN
		StdApi.OpenAuxDialog(file, title, v)
	END OpenAuxDialog;

	PROCEDURE OpenToolDialog* (file, title: ARRAY OF CHAR);
		VAR v: Views.View;
	BEGIN
		StdApi.OpenToolDialog(file, title, v)
	END OpenToolDialog;

	PROCEDURE OpenDoc* (file: ARRAY OF CHAR);
		VAR v: Views.View;
	BEGIN
		StdApi.OpenDoc(file, v)
	END OpenDoc;
	
	PROCEDURE OpenCopyOf* (file: ARRAY OF CHAR);
		VAR v: Views.View;
	BEGIN
		StdApi.OpenCopyOf(file, v)
	END OpenCopyOf;
	
	PROCEDURE OpenAux* (file, title: ARRAY OF CHAR);
		VAR v: Views.View; 
	BEGIN
		StdApi.OpenAux(file, title, v)
	END OpenAux;

	PROCEDURE OpenBrowser* (file, title: ARRAY OF CHAR);
		VAR v: Views.View;
	BEGIN
		StdApi.OpenBrowser(file, title, v)
	END OpenBrowser;

	PROCEDURE CloseDialog*;
		VAR v: Views.View;
	BEGIN
		StdApi.CloseDialog(v)
	END CloseDialog;


	PROCEDURE Open*;
		VAR i: INTEGER; v: Views.View;
	BEGIN
		i := Views.Available();
		IF i > 0 THEN Views.Fetch(v); Views.OpenView(v)
		ELSE Dialog.ShowMsg("#System:DepositExpected")
		END
	END Open;

	PROCEDURE PasteView*;
		VAR i: INTEGER; v: Views.View;
	BEGIN
		i := Views.Available();
		IF i > 0 THEN
			Views.Fetch(v);
			Controllers.PasteView(v, Views.undefined, Views.undefined, FALSE)
		ELSE Dialog.ShowMsg("#System:DepositExpected")
		END
	END PasteView;
	
	(* file menu commands *)
	
	PROCEDURE New*;
		VAR res: INTEGER;
	BEGIN
		Dialog.Call(allocator, " ", res)
	END New;
	
	
	(* edit menu commands *)
	
	PROCEDURE Undo*;
		VAR w: Windows.Window;
	BEGIN
		w := Windows.dir.Focus(Controllers.frontPath);
		IF w # NIL THEN w.seq.Undo END
	END Undo;

	PROCEDURE Redo*;
		VAR w: Windows.Window;
	BEGIN
		w := Windows.dir.Focus(Controllers.frontPath);
		IF w # NIL THEN w.seq.Redo END
	END Redo;

	PROCEDURE CopyProp*;
	BEGIN
		Properties.CollectProp(prop)
	END CopyProp;
	
	PROCEDURE PasteProp*;
	BEGIN
		Properties.EmitProp(NIL, prop)
	END PasteProp;
	
	PROCEDURE Clear*;
	(** remove the selection of the current focus **)
		VAR msg: Controllers.EditMsg;
	BEGIN
		msg.op := Controllers.cut; msg.view := NIL;
		msg.clipboard := FALSE;
		Controllers.Forward(msg)
	END Clear;

	PROCEDURE SelectAll*;
	(** select whole content of current focus **)
		VAR msg: Controllers.SelectMsg;
	BEGIN
		msg.set := TRUE; Controllers.Forward(msg)
	END SelectAll;

	PROCEDURE DeselectAll*;
	(** select whole content of current focus **)
		VAR msg: Controllers.SelectMsg;
	BEGIN
		msg.set := FALSE; Controllers.Forward(msg)
	END DeselectAll;

	PROCEDURE SelectDocument*;
	(** select whole document **)
		VAR w: Windows.Window; c: Containers.Controller;
	BEGIN
		w := Windows.dir.Focus(Controllers.path);
		IF w # NIL THEN
			c := w.doc.ThisController();
			IF (c # NIL) & ~(Containers.noSelection IN c.opts) & (c.Singleton() = NIL) THEN
				c.SetSingleton(w.doc.ThisView())
			END
		END
	END SelectDocument;

	PROCEDURE SelectNextView*;
		VAR c: Containers.Controller; v: Views.View;
	BEGIN
		c := Containers.Focus();
		IF (c # NIL) & ~(Containers.noSelection IN c.opts) THEN
			IF c.HasSelection() THEN v := c.Singleton() ELSE v := NIL END;
			IF v = NIL THEN
				c.GetFirstView(Containers.any, v)
			ELSE
				c.GetNextView(Containers.any, v);
				IF v = NIL THEN c.GetFirstView(Containers.any, v) END
			END;
			c.SelectAll(FALSE);
			IF v # NIL THEN c.SetSingleton(v) END
		ELSE Dialog.ShowMsg("#Dev:NoTargetFocusFound")
		END
	END SelectNextView;

	
	(** font menu commands **)

	PROCEDURE Font* (typeface: Fonts.Typeface);
	(** set the selection to the given font family **)
		VAR p: Properties.StdProp;
	BEGIN
		NEW(p); p.valid := {Properties.typeface}; p.typeface := typeface;
		Properties.EmitProp(NIL, p)
	END Font;

	PROCEDURE DefaultFont*;
	(** set the selection to the default font family **)
		VAR p: Properties.StdProp;
	BEGIN
		NEW(p); p.valid := {Properties.typeface}; p.typeface := Fonts.default;
		Properties.EmitProp(NIL, p)
	END DefaultFont;


	(** attributes menu commands **)

	PROCEDURE Plain*;
	(** reset the font attribute "weight" and all font style attributes of the selection **)
		VAR p: Properties.StdProp;
	BEGIN
		NEW(p); p.valid := {Properties.style, Properties.weight};
		p.style.val := {}; p.style.mask := {Fonts.italic, Fonts.underline, Fonts.strikeout};
		p.weight := Fonts.normal;
		Properties.EmitProp(NIL, p)
	END Plain;

	PROCEDURE Bold*;
	(** change the font attribute "weight" in the selection;
	if the selection has a homogeneously bold weight: toggle to normal, else force to bold **)
		VAR p, p0: Properties.StdProp;
	BEGIN
		Properties.CollectStdProp(p0);
		NEW(p); p.valid := {Properties.weight};
		IF (Properties.weight IN p0.valid) & (p0.weight # Fonts.normal) THEN
			p.weight := Fonts.normal
		ELSE p.weight := Fonts.bold
		END;
		Properties.EmitProp(NIL, p)
	END Bold;

	PROCEDURE Italic*;
	(** change the font style attribute "italic" in the selection;
	if the selection is homogeneous wrt this attribute: toggle, else force to italic **)
		VAR p, p0: Properties.StdProp;
	BEGIN
		Properties.CollectStdProp(p0);
		NEW(p); p.valid := {Properties.style}; p.style.mask := {Fonts.italic};
		IF (Properties.style IN p0.valid) & (Fonts.italic IN p0.style.val) THEN
			p.style.val := {}
		ELSE p.style.val := {Fonts.italic}
		END;
		Properties.EmitProp(NIL, p)
	END Italic;

	PROCEDURE Underline*;
	(** change the font style attribute "underline" in the selection;
	if the selection is homogeneous wrt this attribute: toggle, else force to underline **)
		VAR p, p0: Properties.StdProp;
	BEGIN
		Properties.CollectStdProp(p0);
		NEW(p); p.valid := {Properties.style}; p.style.mask := {Fonts.underline};
		IF (Properties.style IN p0.valid) & (Fonts.underline IN p0.style.val) THEN
			p.style.val := {}
		ELSE p.style.val := {Fonts.underline}
		END;
		Properties.EmitProp(NIL, p)
	END Underline;

	PROCEDURE Strikeout*;
	(** change the font style attribute "strikeout" in the selection,
	without changing other attributes;
	if the selection is homogeneous wrt this attribute: toggle,
	else force to strikeout **)
		VAR p, p0: Properties.StdProp;
	BEGIN
		Properties.CollectStdProp(p0);
		NEW(p); p.valid := {Properties.style}; p.style.mask := {Fonts.strikeout};
		IF (Properties.style IN p0.valid) & (Fonts.strikeout IN p0.style.val) THEN
			p.style.val := {}
		ELSE p.style.val := {Fonts.strikeout}
		END;
		Properties.EmitProp(NIL, p)
	END Strikeout;

	PROCEDURE Size* (size: INTEGER);
	(** set the selection to the given font size **)
		VAR p: Properties.StdProp;
	BEGIN
		NEW(p); p.valid := {Properties.size};
		p.size := size * Ports.point;
		Properties.EmitProp(NIL, p)
	END Size;

	PROCEDURE SetSize*;
		VAR p: Properties.StdProp;
	BEGIN
		IF (0 <= size.size) & (size.size < 32768) THEN
			NEW(p); p.valid := {Properties.size};
			p.size := size.size * Fonts.point;
			Properties.EmitProp(NIL, p)
		ELSE
			Dialog.ShowMsg(illegalSizeKey)
		END
	END SetSize;

	PROCEDURE InitSizeDialog*;
		VAR p: Properties.StdProp;
	BEGIN
		Properties.CollectStdProp(p);
		IF Properties.size IN p.valid THEN size.size := p.size DIV Fonts.point END
	END InitSizeDialog;

	PROCEDURE Color* (color: Ports.Color);
	(** set the color attributes of the selection **)
		VAR p: Properties.StdProp;
	BEGIN
		NEW(p); p.valid := {Properties.color};
		p.color.val := color;
		Properties.EmitProp(NIL, p)
	END Color;

	PROCEDURE UpdateAll*;	(* for HostCmds.Toggle *)
		VAR w: Windows.Window; pw, ph: INTEGER; dirty: BOOLEAN; msg: Models.UpdateMsg;
	BEGIN
		w := Windows.dir.First();
		WHILE w # NIL DO
			IF ~w.sub THEN
				dirty := w.seq.Dirty();
				Models.Domaincast(w.doc.Domain(), msg);
				IF ~dirty THEN w.seq.SetDirty(FALSE) END	(* not perfect: "undoable dirt" ... *)
			END;
			w.port.GetSize(pw, ph);
			w.Restore(0, 0, pw, ph);
			w := Windows.dir.Next(w)
		END
	END UpdateAll;

	PROCEDURE RestoreAll*;
		VAR w: Windows.Window; pw, ph: INTEGER;
	BEGIN
		w := Windows.dir.First();
		WHILE w # NIL DO
			w.port.GetSize(pw, ph);
			w.Restore(0, 0, pw, ph);
			w := Windows.dir.Next(w)
		END
	END RestoreAll;
	
	
	(** document layout dialog **)
	
	PROCEDURE SetLayout*;
		VAR opts: SET; l, t, r, b, r0, b0: INTEGER; c: Containers.Controller; script: Stores.Operation;
	BEGIN
		c := layout.doc.ThisController();
		opts := c.opts - {Documents.pageWidth..Documents.winHeight};
		IF layout.wType = page THEN INCL(opts, Documents.pageWidth)
		ELSIF layout.wType = window THEN INCL(opts, Documents.winWidth)
		END;
		IF layout.hType = page THEN INCL(opts, Documents.pageHeight)
		ELSIF layout.hType = window THEN INCL(opts, Documents.winHeight)
		END;
		layout.doc.PollRect(l, t, r, b); r0 := r; b0 := b;
		IF layout.wType = fix THEN r := l + SHORT(ENTIER(layout.width * layout.u)) END;
		IF layout.hType = fix THEN b := t + SHORT(ENTIER(layout.height * layout.u)) END;
		IF (opts # c.opts) OR (r # r0) OR (b # b0) THEN
			Views.BeginScript(layout.doc, "#System:ChangeLayout", script);
			c.SetOpts(opts);
			layout.doc.SetRect(l, t, r, b);
			Views.EndScript(layout.doc, script)
		END
	END SetLayout;
	
	PROCEDURE InitLayoutDialog*;
	(* guard: WindowGuard *)
		VAR w: Windows.Window; c: Containers.Controller; l, t, r, b: INTEGER;
	BEGIN
		w := Windows.dir.First();
		IF w # NIL THEN
			layout.doc := w.doc;
			c := w.doc.ThisController();
			IF Documents.pageWidth IN c.opts THEN layout.wType := page
			ELSIF Documents.winWidth IN c.opts THEN layout.wType := window
			ELSE layout.wType := fix
			END;
			IF Documents.pageHeight IN c.opts THEN layout.hType := page
			ELSIF Documents.winHeight IN c.opts THEN layout.hType := window
			ELSE layout.hType := fix
			END;
			IF Dialog.metricSystem THEN layout.u := Ports.mm * 10 ELSE layout.u := Ports.inch END;
			w.doc.PollRect(l, t, r, b);
			layout.width := (r - l) DIV (layout.u DIV 100) / 100;
			layout.height := (b - t) DIV (layout.u DIV 100) / 100
		END
	END InitLayoutDialog;
	
	PROCEDURE WidthGuard* (VAR par: Dialog.Par);
	BEGIN
		IF layout.wType # fix THEN par.readOnly := TRUE END
	END WidthGuard;
	
	PROCEDURE HeightGuard* (VAR par: Dialog.Par);
	BEGIN
		IF layout.hType # fix THEN par.readOnly := TRUE END
	END HeightGuard;
	
	PROCEDURE TypeNotifier* (op, from, to: INTEGER);
		VAR w, h, l, t, r, b: INTEGER; d: BOOLEAN;
	BEGIN
		layout.doc.PollRect(l, t, r, b);
		IF layout.wType = page THEN
			layout.doc.PollPage(w, h, l, t, r, b, d)
		ELSIF layout.wType = window THEN
			layout.doc.context.GetSize(w, h); r := w - l
		END;
		layout.width := (r - l) DIV (layout.u DIV 100) / 100;
		layout.doc.PollRect(l, t, r, b);
		IF layout.hType = page THEN
			layout.doc.PollPage(w, h, l, t, r, b, d)
		ELSIF layout.hType = window THEN
			layout.doc.context.GetSize(w, h); b := h - t
		END;
		layout.height := (b - t) DIV (layout.u DIV 100) / 100;
		Dialog.Update(layout)
	END TypeNotifier;
	

	(** window menu command **)

	PROCEDURE NewWindow*;
	(** guard ModelViewGuard **)
		VAR win: Windows.Window; doc: Documents.Document; v: Views.View; title: Views.Title;
			seq: ANYPTR; clean: BOOLEAN;
	BEGIN
		win := Windows.dir.Focus(Controllers.frontPath);
		IF win # NIL THEN
			v := win.doc.ThisView();
			IF v.Domain() # NIL THEN seq := v.Domain().GetSequencer() ELSE seq := NIL END;
			clean := (seq # NIL) & ~seq(Sequencers.Sequencer).Dirty();
			doc := win.doc.DocCopyOf(v);
			(* Stores.InitDomain(doc, v.Domain()); *)
			ASSERT(doc.Domain() = v.Domain(), 100);
			win.GetTitle(title);
			Windows.dir.OpenSubWindow(Windows.dir.New(), doc, win.flags, title);
			IF clean THEN seq(Sequencers.Sequencer).SetDirty(FALSE) END
		END
	END NewWindow;
	
	(* properties *)
	
	PROCEDURE GetCmd (name: ARRAY OF CHAR; OUT cmd: ARRAY OF CHAR);
		VAR i, j: INTEGER; ch, lch: CHAR; key: ARRAY 256 OF CHAR;
	BEGIN
		i := 0; ch := name[0]; key[0] := "#"; j := 1;
		REPEAT
			key[j] := ch; INC(j); lch := ch; INC(i); ch := name[i]
		UNTIL (ch = 0X) OR (ch = ".")
			OR ((ch >= "A") & (ch <= "Z") OR (ch >= "�") & (ch # "�") & (ch <= "�"))
				& ((lch < "A") OR (lch > "Z") & (lch < "�") OR (lch = "�") OR (lch > "�"));
		IF ch = "." THEN
			key := "#System:" + name
		ELSE
			key[j] := ":"; INC(j); key[j] := 0X; j := 0;
			WHILE ch # 0X DO name[j] := ch; INC(i); INC(j); ch := name[i] END;
			name[j] := 0X; key := key + name
		END;
		Dialog.MapString(key, cmd);
		IF cmd = name THEN cmd := "" END
	END GetCmd;
	
	PROCEDURE SearchCmd (call: BOOLEAN; OUT found: BOOLEAN);
		VAR p: Properties.Property; std: BOOLEAN; v: Views.View;  cmd: ARRAY 256 OF CHAR; pos, res: INTEGER;
	BEGIN
		Controllers.SetCurrentPath(Controllers.targetPath);
		v := Containers.FocusSingleton(); found := FALSE;
		IF v # NIL THEN
			Services.GetTypeName(v, cmd);
			GetCmd(cmd, cmd);
			IF cmd # "" THEN found := TRUE;
				IF call THEN Dialog.Call(cmd, "", res) END
			END
		END;
		std := FALSE;
		Properties.CollectProp(p);
		WHILE p # NIL DO
			IF p IS Properties.StdProp THEN std := TRUE
			ELSE
				Services.GetTypeName(p, cmd);
				GetCmd(cmd, cmd);
				IF cmd # "" THEN found := TRUE;
					IF call THEN Dialog.Call(cmd, "", res) END
				ELSE
					Services.GetTypeName(p, cmd);
					Strings.Find(cmd, "Desc", LEN(cmd$)-4, pos);
					IF LEN(cmd$)-4 = pos THEN
						cmd[pos] := 0X; GetCmd(cmd, cmd);
						IF cmd # "" THEN found := TRUE;
							IF call THEN Dialog.Call(cmd, "", res) END
						END
					END
				END
			END;
			p := p.next
		END;
		IF std & ~found THEN
			Dialog.MapString("#Host:Properties.StdProp", cmd);
			IF cmd # "Properties.StdProp" THEN found := TRUE;
				IF call THEN Dialog.Call(cmd, "", res) END
			END
		END;
		IF ~found THEN
			Dialog.MapString("#System:ShowProp", cmd);
			IF cmd # "ShowProp" THEN found := TRUE;
				IF call THEN Dialog.Call(cmd, "", res) END
			END
		END;
		Controllers.ResetCurrentPath
	END SearchCmd;
	
	PROCEDURE ShowProp*;
		VAR found: BOOLEAN;
	BEGIN
		SearchCmd(TRUE, found)
	END ShowProp;
	
	PROCEDURE ShowPropGuard* (VAR par: Dialog.Par);
		VAR found: BOOLEAN;
	BEGIN
		SearchCmd(FALSE, found);
		IF ~found THEN par.disabled := TRUE END
	END ShowPropGuard;
	
	
	(* container commands *)
	
	PROCEDURE ActFocus (): Containers.Controller;
		VAR c: Containers.Controller; v: Views.View;
	BEGIN
		c := Containers.Focus();
		IF c # NIL THEN
			v := c.ThisView();
			IF v IS Documents.Document THEN
				v := v(Documents.Document).ThisView();
				IF v IS Containers.View THEN
					c := v(Containers.View).ThisController()
				ELSE c := NIL
				END
			END
		END;
		RETURN c
	END ActFocus;

	PROCEDURE ToggleNoFocus*;
		VAR c: Containers.Controller; v: Views.View;
	BEGIN
		c := ActFocus();
		IF c # NIL THEN
			v := c.ThisView();
			IF ~((v IS Documents.Document) OR (Containers.noSelection IN c.opts)) THEN
				IF Containers.noFocus IN c.opts THEN
					c.SetOpts(c.opts - {Containers.noFocus})
				ELSE
					c.SetOpts(c.opts + {Containers.noFocus})
				END
			END
		END
	END ToggleNoFocus;

	PROCEDURE OpenAsAuxDialog*;
	(** create a new sub-window onto the focus view shown in the top window, mask mode **)
		VAR win: Windows.Window; doc: Documents.Document; v, u: Views.View; title: Views.Title;
			c: Containers.Controller;
	BEGIN
		v := Controllers.FocusView();
		IF (v # NIL) & (v IS Containers.View) & ~(v IS Documents.Document) THEN
			win := Windows.dir.Focus(Controllers.frontPath); ASSERT(win # NIL, 100);
			doc := win.doc.DocCopyOf(v);
			u := doc.ThisView();
			c := u(Containers.View).ThisController();
			c.SetOpts(c.opts - {Containers.noFocus} + {Containers.noCaret, Containers.noSelection});
			IF v # win.doc.ThisView() THEN
				c := doc.ThisController();
				c.SetOpts(c.opts - {Documents.pageWidth, Documents.pageHeight}
										+ {Documents.winWidth, Documents.winHeight})
			END;
			(* Stores.InitDomain(doc, v.Domain()); already done in DocCopyOf *)
			win.GetTitle(title);
			Windows.dir.OpenSubWindow(Windows.dir.New(), doc,
	{Windows.isAux, Windows.neverDirty, Windows.noResize, Windows.noHScroll, Windows.noVScroll},
															title)
		ELSE Dialog.Beep
		END
	END OpenAsAuxDialog;

	PROCEDURE OpenAsToolDialog*;
	(** create a new sub-window onto the focus view shown in the top window, mask mode **)
		VAR win: Windows.Window; doc: Documents.Document; v, u: Views.View; title: Views.Title;
			c: Containers.Controller;
	BEGIN
		v := Controllers.FocusView();
		IF (v # NIL) & (v IS Containers.View) & ~(v IS Documents.Document) THEN
			win := Windows.dir.Focus(Controllers.frontPath); ASSERT(win # NIL, 100);
			doc := win.doc.DocCopyOf(v);
			u := doc.ThisView();
			c := u(Containers.View).ThisController();
			c.SetOpts(c.opts - {Containers.noFocus} + {Containers.noCaret, Containers.noSelection});
			IF v # win.doc.ThisView() THEN
				c := doc.ThisController();
				c.SetOpts(c.opts - {Documents.pageWidth, Documents.pageHeight}
										+ {Documents.winWidth, Documents.winHeight})
			END;
			(* Stores.InitDomain(doc, v.Domain()); already done in DocCopyOf *)
			win.GetTitle(title);
			Windows.dir.OpenSubWindow(Windows.dir.New(), doc,
	{Windows.isTool, Windows.neverDirty, Windows.noResize, Windows.noHScroll, Windows.noVScroll},
															title)
		ELSE Dialog.Beep
		END
	END OpenAsToolDialog;

	PROCEDURE RecalcFocusSize*;
		VAR c: Containers.Controller; v: Views.View; bounds: Properties.BoundsPref;
	BEGIN
		c := Containers.Focus();
		IF c # NIL THEN
			v := c.ThisView();
			bounds.w := Views.undefined; bounds.h := Views.undefined;
			Views.HandlePropMsg(v, bounds);
			v.context.SetSize(bounds.w, bounds.h)
		END
	END RecalcFocusSize;

	PROCEDURE RecalcAllSizes*;
		VAR w: Windows.Window;
	BEGIN
		w := Windows.dir.First();
		WHILE w # NIL DO
			StdDialog.RecalcView(w.doc.ThisView());
			w := Windows.dir.Next(w)
		END
	END RecalcAllSizes;
	
	PROCEDURE SetMode(opts: SET);
		VAR 
			c: Containers.Controller; v: Views.View; 
			gm: Containers.GetOpts; sm: Containers.SetOpts;
			w: Windows.Window;
	BEGIN
		c := Containers.Focus();
		gm.valid := {};
		IF (c # NIL) & (c.Singleton() # NIL) THEN
			v := c.Singleton();
			Views.HandlePropMsg(v, gm);
		END;
		IF gm.valid = {} THEN
			w := Windows.dir.Focus(Controllers.path);
			IF (w # NIL) & (w.doc.ThisView() IS Containers.View) THEN v := w.doc.ThisView() ELSE v := NIL END
		END;
		IF v # NIL THEN
			sm.valid := {Containers.noSelection, Containers.noFocus, Containers.noCaret};
			sm.opts := opts;
			Views.HandlePropMsg(v, sm);
		END;
	END SetMode;
	
	PROCEDURE GetMode(OUT found: BOOLEAN; OUT opts: SET);
		VAR c: Containers.Controller; gm: Containers.GetOpts; w: Windows.Window;
	BEGIN
		c := Containers.Focus();
		gm.valid := {};
		IF (c # NIL) & (c.Singleton() # NIL) THEN
			Views.HandlePropMsg(c.Singleton(), gm);
		END;
		IF gm.valid = {}  THEN
			w := Windows.dir.Focus(Controllers.path);
			IF (w # NIL) & (w.doc.ThisView() IS Containers.View) THEN
				Views.HandlePropMsg(w.doc.ThisView(), gm);
			END
		END;
		found := gm.valid # {};
		opts := gm.opts
	END GetMode;
	
	PROCEDURE SetMaskMode*;
	(* Guard: SetMaskGuard *)
	BEGIN
		SetMode({Containers.noSelection, Containers.noCaret})
	END SetMaskMode;

	PROCEDURE SetEditMode*;
	(* Guard: SetEditGuard *)
	BEGIN
		SetMode({})
	END SetEditMode;

	PROCEDURE SetLayoutMode*;
	(* Guard: SetLayoutGuard *)
	BEGIN
		SetMode({Containers.noFocus})
	END SetLayoutMode;

	PROCEDURE SetBrowserMode*;
	(* Guard: SetBrowserGuard *)
	BEGIN
		SetMode({Containers.noCaret})
	END SetBrowserMode;


	(* standard guards *)
	
	PROCEDURE ToggleNoFocusGuard* (VAR par: Dialog.Par);
		VAR c: Containers.Controller; v: Views.View;
	BEGIN
		c := ActFocus();
		IF c # NIL THEN
			v := c.ThisView();
			IF ~((v IS Documents.Document) OR (Containers.noSelection IN c.opts)) THEN
				IF Containers.noFocus IN c.opts THEN par.label := "#System:AllowFocus"
				ELSE par.label := "#System:PreventFocus"
				END
			ELSE par.disabled := TRUE
			END
		ELSE par.disabled := TRUE
		END
	END ToggleNoFocusGuard;

	PROCEDURE ReadOnlyGuard* (VAR par: Dialog.Par);
	BEGIN
		par.readOnly := TRUE
	END ReadOnlyGuard;
	
	PROCEDURE WindowGuard* (VAR par: Dialog.Par);
		VAR w: Windows.Window;
	BEGIN
		w := Windows.dir.First();
		IF w = NIL THEN par.disabled := TRUE END
	END WindowGuard;

	PROCEDURE ModelViewGuard* (VAR par: Dialog.Par);
		VAR w: Windows.Window;
	BEGIN
		w := Windows.dir.Focus(Controllers.frontPath);
		par.disabled := (w = NIL) OR (w.doc.ThisView().ThisModel() = NIL)
	END ModelViewGuard;

	PROCEDURE SetMaskModeGuard* (VAR par: Dialog.Par);
		CONST mode = {Containers.noSelection, Containers.noFocus, Containers.noCaret};
		VAR opts: SET; found: BOOLEAN;
	BEGIN
		GetMode(found, opts);
		IF found THEN
			par.checked := opts * mode = {Containers.noSelection, Containers.noCaret}
		ELSE
			par.disabled := TRUE
		END
	END SetMaskModeGuard;

	PROCEDURE SetEditModeGuard* (VAR par: Dialog.Par);
		CONST mode = {Containers.noSelection, Containers.noFocus, Containers.noCaret};
		VAR opts: SET; found: BOOLEAN;
	BEGIN
		GetMode(found, opts);
		IF found THEN
			par.checked := opts * mode = {}
		ELSE
			par.disabled := TRUE
		END
	END SetEditModeGuard;

	PROCEDURE SetLayoutModeGuard* (VAR par: Dialog.Par);
		CONST mode = {Containers.noSelection, Containers.noFocus, Containers.noCaret};
		VAR opts: SET; found: BOOLEAN;
	BEGIN
		GetMode(found, opts);
		IF found THEN
			par.checked := opts * mode = {Containers.noFocus}
		ELSE
			par.disabled := TRUE
		END
	END SetLayoutModeGuard;

	PROCEDURE SetBrowserModeGuard* (VAR par: Dialog.Par);
		CONST mode = {Containers.noSelection, Containers.noFocus, Containers.noCaret};
		VAR opts: SET; found: BOOLEAN;
	BEGIN
		GetMode(found, opts);
		IF found THEN
			par.checked := opts * mode = {Containers.noCaret}
		ELSE
			par.disabled := TRUE
		END
	END SetBrowserModeGuard;

	PROCEDURE SelectionGuard* (VAR par: Dialog.Par);
		VAR ops: Controllers.PollOpsMsg;
	BEGIN
		Controllers.PollOps(ops);
		IF ops.valid * {Controllers.cut, Controllers.copy} = {} THEN par.disabled := TRUE END
	END SelectionGuard;

	PROCEDURE SingletonGuard* (VAR par: Dialog.Par);
		VAR ops: Controllers.PollOpsMsg;
	BEGIN
		Controllers.PollOps(ops);
		IF ops.singleton = NIL THEN par.disabled := TRUE END
	END SingletonGuard;
	
	PROCEDURE SelectAllGuard* (VAR par: Dialog.Par);
		VAR ops: Controllers.PollOpsMsg;
	BEGIN
		Controllers.PollOps(ops);
		IF ~ops.selectable THEN par.disabled := TRUE END
	END SelectAllGuard;

	PROCEDURE CaretGuard* (VAR par: Dialog.Par);
		VAR ops: Controllers.PollOpsMsg;
	BEGIN
		Controllers.PollOps(ops);
		IF ops.valid * {Controllers.pasteChar .. Controllers.paste} = {} THEN par.disabled := TRUE END
	END CaretGuard;

	PROCEDURE PasteCharGuard* (VAR par: Dialog.Par);
		VAR ops: Controllers.PollOpsMsg;
	BEGIN
		Controllers.PollOps(ops);
		IF ~(Controllers.pasteChar IN ops.valid) THEN par.disabled := TRUE END
	END PasteCharGuard;

	PROCEDURE PasteLCharGuard* (VAR par: Dialog.Par);
		VAR ops: Controllers.PollOpsMsg;
	BEGIN
		Controllers.PollOps(ops);
		IF ~(Controllers.pasteChar IN ops.valid) THEN par.disabled := TRUE END
	END PasteLCharGuard;

	PROCEDURE PasteViewGuard* (VAR par: Dialog.Par);
		VAR ops: Controllers.PollOpsMsg;
	BEGIN
		Controllers.PollOps(ops);
		IF ~(Controllers.paste IN ops.valid) THEN par.disabled := TRUE END
	END PasteViewGuard;
	
	PROCEDURE ContainerGuard* (VAR par: Dialog.Par);
	BEGIN
		IF Containers.Focus() = NIL THEN par.disabled := TRUE END
	END ContainerGuard;
	
	PROCEDURE UndoGuard* (VAR par: Dialog.Par);
		VAR f: Windows.Window; opName: Stores.OpName;
	BEGIN
		Dialog.MapString("#System:Undo", par.label);
		f := Windows.dir.Focus(Controllers.frontPath);
		IF (f # NIL) & f.seq.CanUndo() THEN
			f.seq.GetUndoName(opName);
			Dialog.MapString(opName, opName);
			Append(par.label, " ");
			Append(par.label, opName)
		ELSE
			par.disabled := TRUE
		END
	END UndoGuard;

	PROCEDURE RedoGuard* (VAR par: Dialog.Par);
		VAR f: Windows.Window; opName: Stores.OpName;
	BEGIN
		Dialog.MapString("#System:Redo", par.label);
		f := Windows.dir.Focus(Controllers.frontPath);
		IF (f # NIL) & f.seq.CanRedo() THEN
			f.seq.GetRedoName(opName);
			Dialog.MapString(opName, opName);
			Append(par.label, " ");
			Append(par.label, opName)
		ELSE
			par.disabled := TRUE
		END
	END RedoGuard;
	
	PROCEDURE PlainGuard* (VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF props.known * {Properties.style, Properties.weight} # {} THEN
			par.checked := (Properties.style IN props.valid)
				& (props.style.val = {}) & ({Fonts.italic, Fonts.underline, Fonts.strikeout} - props.style.mask = {})
				& (Properties.weight IN props.valid) & (props.weight = Fonts.normal)
		ELSE
			par.disabled := TRUE
		END
	END PlainGuard;

	PROCEDURE BoldGuard* (VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF Properties.weight IN props.known THEN
			par.checked := (Properties.weight IN props.valid) & (props.weight = Fonts.bold)
		ELSE
			par.disabled := TRUE
		END
	END BoldGuard;

	PROCEDURE ItalicGuard* (VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF Properties.style IN props.known THEN
			par.checked := (Properties.style IN props.valid) & (Fonts.italic IN props.style.val)
		ELSE
			par.disabled := TRUE
		END
	END ItalicGuard;

	PROCEDURE UnderlineGuard* (VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF Properties.style IN props.known THEN
			par.checked := (Properties.style IN props.valid) & (Fonts.underline IN props.style.val)
		ELSE
			par.disabled := TRUE
		END
	END UnderlineGuard;

	PROCEDURE StrikeoutGuard* (VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF Properties.style IN props.known THEN
			par.checked := (Properties.style IN props.valid) & (Fonts.strikeout IN props.style.val)
		ELSE
			par.disabled := TRUE
		END
	END StrikeoutGuard;

	PROCEDURE SizeGuard* (size: INTEGER; VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF Properties.size IN props.known THEN
			par.checked := (Properties.size IN props.valid) & (size = props.size DIV Ports.point)
		ELSE
			par.disabled := TRUE
		END
	END SizeGuard;

	PROCEDURE ColorGuard* (color: INTEGER; VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF Properties.color IN props.known THEN
			par.checked := (Properties.color IN props.valid) & (color = props.color.val)
		ELSE
			par.disabled := TRUE
		END
	END ColorGuard;
	
	PROCEDURE DefaultFontGuard* (VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF Properties.typeface IN props.known THEN
			par.checked := (Properties.typeface IN props.valid) & (props.typeface = Fonts.default)
		ELSE
			par.disabled := TRUE
		END
	END DefaultFontGuard;

	PROCEDURE TypefaceGuard* (VAR par: Dialog.Par);
		VAR props: Properties.StdProp;
	BEGIN
		props := StdProp();
		IF ~(Properties.typeface IN props.known) THEN par.disabled := TRUE END
	END TypefaceGuard;

	
	(* standard notifiers *)
	
	PROCEDURE DefaultOnDoubleClick* (op, from, to: INTEGER);
		VAR  msg: Controllers.EditMsg; c: Containers.Controller;
	BEGIN
		IF (op = Dialog.pressed) & (from = 1) THEN
			Controllers.SetCurrentPath(Controllers.frontPath);
			c := Containers.Focus();
			Controllers.ResetCurrentPath;
			IF {Containers.noSelection, Containers.noCaret} - c.opts = {} THEN
				msg.op := Controllers.pasteChar;
				msg.char := 0DX; msg.modifiers := {};
				Controllers.ForwardVia(Controllers.frontPath, msg)
			END
		END
	END DefaultOnDoubleClick;


	PROCEDURE Init;
	BEGIN
		allocator := defaultAllocator;
		propEra := -1
	END Init;

BEGIN
	Init
END StdCmds.
