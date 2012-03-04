MODULE StdFolds;
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
		Domains := Stores, Ports, Stores, Containers, Models, Views, Controllers, Fonts,
		Properties,Controls,
		TextModels, TextViews, TextControllers, TextSetters,
		Dialog, Services;

	CONST
		expanded* = FALSE; collapsed* = TRUE;
		minVersion = 0; currentVersion = 0;

		collapseFoldKey = "#Std:Collapse Fold";
		expandFoldKey = "#Std:Expand Fold";
		zoomInKey = "#Std:Zoom In";
		zoomOutKey = "#Std:Zoom Out";
		expandFoldsKey = "#Std:Expand Folds";
		collapseFoldsKey = "#Std:Collapse Folds";
		insertFoldKey = "#Std:Insert Fold";
		setLabelKey = "#Std:Set Label";


	TYPE
		Label* = ARRAY 32 OF CHAR;
		
		Fold* = POINTER TO RECORD (Views.View)
			leftSide-: BOOLEAN;
			collapsed-: BOOLEAN;
			label-: Label; (* valid iff leftSide *)
			hidden: TextModels.Model (* valid iff leftSide; NIL if no hidden text *)
		END;

		Directory* = POINTER TO ABSTRACT RECORD END;

		StdDirectory = POINTER TO RECORD (Directory) END;

		FlipOp = POINTER TO RECORD (Domains.Operation)
			text: TextModels.Model; (* containing text *)
			leftpos, rightpos: INTEGER (* position of left and right Fold *)
		END;
		
		SetLabelOp = POINTER TO RECORD (Domains.Operation)
			text: TextModels.Model; (* containing text *)
			pos: INTEGER; (* position of fold in text *)
			oldlabel: Label
		END;
		
		Action = POINTER TO RECORD (Services.Action) END;

		
	VAR
		dir-, stdDir-: Directory;

		foldData*: RECORD
			nested*: BOOLEAN;
			all*: BOOLEAN;
			findLabel*: Label;
			newLabel*: Label
		END;

		iconFont: Fonts.Typeface;
		leftExp, rightExp, leftColl, rightColl: ARRAY 8 OF SHORTCHAR;
		coloredBackg: BOOLEAN;
		action: Action;
		fingerprint: INTEGER;	(* for the property inspector *)

		PROCEDURE (d: Directory) New* (collapsed: BOOLEAN; label: Label;
																	hiddenText: TextModels.Model): Fold, NEW, ABSTRACT;


	PROCEDURE GetPair (fold: Fold; VAR l, r: Fold);
		VAR c: Models.Context; text: TextModels.Model; rd: TextModels.Reader; v: Views.View;
			nest: INTEGER;
	BEGIN
		c := fold.context; l := NIL; r := NIL;
		WITH c: TextModels.Context DO
			text := c.ThisModel(); rd := text.NewReader(NIL);
			IF fold.leftSide THEN l := fold;
				rd.SetPos(c.Pos()+1); nest := 1;
				REPEAT rd.ReadView(v);
					IF (v # NIL) & (v IS Fold) THEN
						IF v(Fold).leftSide THEN INC(nest) ELSE DEC(nest) END
					END
				UNTIL (v = NIL) OR (nest = 0);
				IF v # NIL THEN r := v(Fold) ELSE r := NIL END
			ELSE r := fold;
				rd.SetPos(c.Pos()); nest := 1;
				REPEAT rd.ReadPrevView(v);
					IF (v # NIL) & (v IS Fold) THEN
						IF ~v(Fold).leftSide THEN INC(nest) ELSE DEC(nest) END
					END
				UNTIL (v = NIL) OR (nest = 0);
				IF v # NIL THEN l := v(Fold) ELSE l := NIL END
			END
		ELSE (* fold not embedded in a text *)
		END;
		ASSERT((l = NIL) OR l.leftSide & (l.hidden # NIL), 100);
		ASSERT((r = NIL) OR ~r.leftSide & (r.hidden = NIL), 101)
	END GetPair;

	PROCEDURE (fold: Fold) HiddenText* (): TextModels.Model, NEW;
		VAR l, r: Fold;
	BEGIN
		IF fold.leftSide THEN RETURN fold.hidden
		ELSE GetPair(fold, l, r);
			IF l # NIL THEN RETURN l.hidden ELSE RETURN NIL END
		END
	END HiddenText;

	PROCEDURE (fold: Fold) MatchingFold* (): Fold, NEW;
		VAR l, r: Fold;
	BEGIN
		GetPair(fold, l, r);
		IF l # NIL THEN
			IF fold = l THEN RETURN r ELSE RETURN l END
		ELSE RETURN NIL
		END
	END MatchingFold;

	PROCEDURE GetIcon (fold: Fold; VAR icon: ARRAY OF SHORTCHAR);
	BEGIN
		IF fold.leftSide THEN
			IF fold.collapsed THEN icon := leftColl$ ELSE icon := leftExp$ END
		ELSE
			 IF fold.collapsed THEN icon := rightColl$ ELSE icon := rightExp$ END
		END
	END GetIcon;

	PROCEDURE CalcSize (f: Fold; VAR w, h: INTEGER);
		VAR icon: ARRAY 8 OF SHORTCHAR; c: Models.Context; a: TextModels.Attributes; font: Fonts.Font;
			asc, dsc, fw: INTEGER;
	BEGIN
		GetIcon(f, icon);
		c := f.context;
		IF (c # NIL) & (c IS TextModels.Context) THEN
			a := c(TextModels.Context).Attr();
			font := Fonts.dir.This(iconFont, a.font.size, {}, Fonts.normal)
		ELSE font := Fonts.dir.Default()
		END;
		w := font.SStringWidth(icon);
		font.GetBounds(asc, dsc, fw);
		h := asc + dsc
	END CalcSize;

	PROCEDURE Update (f: Fold);
		VAR w, h: INTEGER;
	BEGIN
		CalcSize(f, w, h);
		f.context.SetSize(w, h);
		Views.Update(f, Views.keepFrames)
	END Update;

	PROCEDURE FlipPair (l, r: Fold);
		VAR text, hidden: TextModels.Model; cl, cr: Models.Context;
			lpos, rpos: INTEGER;
	BEGIN
		IF (l # NIL) & (r # NIL) THEN
			ASSERT(l.leftSide, 100);
			ASSERT(~r.leftSide, 101);
			ASSERT(l.hidden # NIL, 102);
			ASSERT(r.hidden = NIL, 103);
			cl := l.context; cr := r.context;
			text := cl(TextModels.Context).ThisModel();
			lpos := cl(TextModels.Context).Pos() + 1; rpos := cr(TextModels.Context).Pos();
			ASSERT(lpos <= rpos, 104);
			hidden := TextModels.CloneOf(text); 
			hidden.Insert(0, text, lpos, rpos);
			text.Insert(lpos, l.hidden, 0, l.hidden.Length());
			l.hidden := hidden; Stores.Join(l, hidden);
			l.collapsed := ~l.collapsed;
			r.collapsed := l.collapsed;
			Update(l); Update(r);
			TextControllers.SetCaret(text, lpos)
		END
	END FlipPair;

	PROCEDURE (op: FlipOp) Do;
		VAR rd: TextModels.Reader; left, right: Views.View;
	BEGIN
		rd := op.text.NewReader(NIL);
		rd.SetPos(op.leftpos); rd.ReadView(left);
		rd.SetPos(op.rightpos); rd.ReadView(right);
		FlipPair(left(Fold), right(Fold));
		op.leftpos := left.context(TextModels.Context).Pos();
		op.rightpos := right.context(TextModels.Context).Pos()
	END Do;

	PROCEDURE (op: SetLabelOp) Do;
		VAR rd: TextModels.Reader; fold: Views.View; left, right: Fold; lab: Label;
	BEGIN
		rd := op.text.NewReader(NIL);
		rd.SetPos(op.pos); rd.ReadView(fold);
		WITH fold: Fold DO
			GetPair(fold, left, right);
			IF left # NIL THEN
				lab := fold.label; left.label := op.oldlabel; op.oldlabel := lab;
				right.label := left.label
			END
		END
	END Do;

	PROCEDURE SetProp (fold: Fold; p : Properties.Property);
		VAR op: SetLabelOp; left, right: Fold;
	BEGIN
		WHILE p # NIL DO
			WITH p: Controls.Prop DO
				IF (Controls.label IN p.valid) & (p.label # fold.label) THEN
					GetPair(fold, left, right);
					IF left # NIL THEN
						NEW(op); op.oldlabel := p.label$;
						op.text := fold.context(TextModels.Context).ThisModel();
						op.pos := fold.context(TextModels.Context).Pos();
						Views.Do(fold, setLabelKey, op)
					END
				END
			ELSE
			END;
			p := p.next
		END
	END SetProp;

	PROCEDURE (fold: Fold) Flip*, NEW;
		VAR op: FlipOp; left, right: Fold;
	BEGIN
		ASSERT(fold # NIL, 20);
		NEW(op);
		GetPair(fold, left, right);
		IF (left # NIL) & (right # NIL) THEN
			op.text := fold.context(TextModels.Context).ThisModel();
			op.leftpos := left.context(TextModels.Context).Pos();
			op.rightpos := right.context(TextModels.Context).Pos();
			Views.BeginModification(Views.clean, fold);
			IF ~left.collapsed THEN Views.Do(fold, collapseFoldKey, op)
			ELSE Views.Do(fold, expandFoldKey, op)
			END;
			Views.EndModification(Views.clean, fold)
		END
	END Flip;

	PROCEDURE ReadNext (rd: TextModels.Reader; VAR fold: Fold);
		VAR v: Views.View;
	BEGIN
		REPEAT rd.ReadView(v) UNTIL rd.eot OR (v IS Fold);
		IF ~rd.eot THEN fold := v(Fold) ELSE fold := NIL END
	END ReadNext;

	PROCEDURE (fold: Fold) FlipNested*, NEW;
		VAR text: TextModels.Model; rd: TextModels.Reader; l, r: Fold; level: INTEGER;
			op: Domains.Operation;
	BEGIN
		ASSERT(fold # NIL, 20);
		GetPair(fold, l, r);
		IF (l # NIL) & (l.context # NIL) & (l.context IS TextModels.Context) THEN
			text := l.context(TextModels.Context).ThisModel();
			Models.BeginModification(Models.clean, text);
			rd := text.NewReader(NIL);
			rd.SetPos(l.context(TextModels.Context).Pos());
			IF l.collapsed THEN
				Models.BeginScript(text, expandFoldsKey, op);
				ReadNext(rd, fold); level := 1;
				WHILE (fold # NIL) & (level > 0) DO
					IF fold.leftSide & fold.collapsed THEN fold.Flip END;
					ReadNext(rd, fold);
					IF fold.leftSide THEN INC(level) ELSE DEC(level) END
				END
			ELSE (* l.state = expanded *)
				Models.BeginScript(text, collapseFoldsKey, op);
				level := 0;
				REPEAT ReadNext(rd, fold);
					IF fold.leftSide THEN INC(level) ELSE DEC(level) END;
					IF (fold # NIL) & ~fold.leftSide & ~fold.collapsed THEN
						fold.Flip;
						rd.SetPos(fold.context(TextModels.Context).Pos()+1)
					END
				UNTIL (fold = NIL) OR (level = 0)
			END;
			Models.EndScript(text, op);
			Models.EndModification(Models.clean, text)
		END
	END FlipNested;

	PROCEDURE (fold: Fold) HandlePropMsg- (VAR msg: Properties.Message);
		VAR prop: Controls.Prop; c: Models.Context; a: TextModels.Attributes; asc, w: INTEGER;
	BEGIN
		WITH msg: Properties.SizePref DO
			CalcSize(fold, msg.w, msg.h)
		| msg: Properties.ResizePref DO
			msg.fixed := TRUE
		| msg: Properties.FocusPref DO msg.hotFocus := TRUE
		| msg: Properties.PollMsg DO NEW(prop);
			prop.known := {Controls.label}; prop.valid := {Controls.label}; prop.readOnly := {};
			prop.label := fold.label$;
			msg.prop := prop
		| msg: Properties.SetMsg DO SetProp(fold, msg.prop)
		| msg: TextSetters.Pref DO c := fold.context;
			IF (c # NIL) & (c IS TextModels.Context) THEN
				a := c(TextModels.Context).Attr();
				a.font.GetBounds(asc, msg.dsc, w)
			END
		ELSE
		END
	END HandlePropMsg;

	PROCEDURE Track (fold: Fold; f: Views.Frame; x, y: INTEGER; buttons: SET; VAR hit: BOOLEAN);
		VAR a: TextModels.Attributes; font: Fonts.Font; c: Models.Context;
			w, h, asc, dsc, fw: INTEGER; isDown, in, in0: BOOLEAN; modifiers: SET;
	BEGIN
		c := fold.context; hit := FALSE;
		WITH c: TextModels.Context DO
			a := c.Attr(); font := a.font;
			c.GetSize(w, h); in0 := FALSE;
			in := (0 <= x) & (x < w) & (0 <= y) & (y < h);
			REPEAT
				IF in # in0 THEN
					f.MarkRect(0, 0, w, h, Ports.fill, Ports.hilite, FALSE); in0 := in
				END;
				f.Input(x, y, modifiers, isDown);
				in := (0 <= x) & (x < w) & (0 <= y) & (y < h)
			UNTIL ~isDown;
			IF in0 THEN hit := TRUE;
				font.GetBounds(asc, dsc, fw);
				f.MarkRect(0, 0, w, asc + dsc, Ports.fill, Ports.hilite, FALSE)
			END
		ELSE
		END
	END Track;

		PROCEDURE (fold: Fold) HandleCtrlMsg* (f: Views.Frame; VAR msg: Views.CtrlMessage;
																							VAR focus: Views.View);
			VAR hit: BOOLEAN; pos: INTEGER; l, r: Fold;
				context: TextModels.Context; text: TextModels.Model;
		BEGIN
			WITH msg: Controllers.TrackMsg DO
				IF fold.context IS TextModels.Context THEN
					Track(fold, f, msg.x, msg.y, msg.modifiers, hit);
					IF hit THEN
						IF Controllers.modify IN msg.modifiers THEN
							fold.FlipNested
						ELSE
							fold.Flip;
							context := fold.context(TextModels.Context);
							text := context.ThisModel();
							IF TextViews.FocusText() = text THEN
								GetPair(fold, l, r);
								pos := context.Pos();
								IF fold = l THEN
									TextControllers.SetCaret(text, pos + 1)
								ELSE
									TextControllers.SetCaret(text, pos)
								END;
								TextViews.ShowRange(text, pos, pos + 1, TRUE)
							END
						END
					END
				END
			| msg: Controllers.PollCursorMsg DO
				msg.cursor := Ports.refCursor
			ELSE
			END
		END HandleCtrlMsg;

	PROCEDURE (fold: Fold) Restore* (f: Views.Frame; l, t, r, b: INTEGER);
		VAR a: TextModels.Attributes; color: Ports.Color; c: Models.Context; font: Fonts.Font;
			icon: ARRAY 8 OF SHORTCHAR; w, h: INTEGER; asc, dsc, fw: INTEGER;
	BEGIN
		GetIcon(fold, icon); c := fold.context;
		IF (c # NIL) & (c IS TextModels.Context) THEN
			a := fold.context(TextModels.Context).Attr();
			font := Fonts.dir.This(iconFont, a.font.size, {}, Fonts.normal);
			color := a.color
		ELSE font := Fonts.dir.Default(); color := Ports.black
		END;
		IF coloredBackg THEN
			fold.context.GetSize(w, h);
			f.DrawRect(f.l, f.dot, f.r, h-f.dot, Ports.fill, Ports.grey50);
			color := Ports.white
		END;
		font.GetBounds(asc, dsc, fw);
		f.DrawSString(0, asc, color, icon, font)
	END Restore;

	PROCEDURE (fold: Fold) CopyFromSimpleView- (source: Views.View);
	BEGIN
		(* fold.CopyFrom^(source); *)
		WITH source: Fold DO
			ASSERT(source.leftSide = (source.hidden # NIL), 100);
			fold.leftSide := source.leftSide;
			fold.collapsed := source.collapsed;
			fold.label := source.label;
			IF source.hidden # NIL THEN
				fold.hidden := TextModels.CloneOf(source.hidden); Stores.Join(fold.hidden, fold);
				fold.hidden.InsertCopy(0, source.hidden, 0, source.hidden.Length())
			END
		END
	END CopyFromSimpleView;

	PROCEDURE (fold: Fold) Internalize- (VAR rd: Stores.Reader);
		VAR version: INTEGER; store: Stores.Store; xint: INTEGER;
	BEGIN
		fold.Internalize^(rd);
		IF rd.cancelled THEN RETURN END;
		rd.ReadVersion(minVersion, currentVersion, version);
		IF rd.cancelled THEN RETURN END;
		rd.ReadXInt(xint);fold.leftSide := xint = 0;
		rd.ReadXInt(xint); fold.collapsed := xint = 0;
		rd.ReadXString(fold.label);
		rd.ReadStore(store);
		IF store # NIL THEN fold.hidden := store(TextModels.Model); Stores.Join(fold.hidden, fold)
		ELSE fold.hidden := NIL
		END;
		fold.leftSide := store # NIL
	END Internalize;

	PROCEDURE (fold: Fold) Externalize- (VAR wr: Stores.Writer);
		VAR xint: INTEGER;
	BEGIN
		fold.Externalize^(wr);
		wr.WriteVersion(currentVersion);
		IF fold.hidden # NIL THEN xint := 0 ELSE xint := 1 END;
		wr.WriteXInt(xint);
		IF fold.collapsed THEN xint := 0 ELSE xint := 1 END;
		wr.WriteXInt(xint);
		wr.WriteXString(fold.label);
		wr.WriteStore(fold.hidden)
	END Externalize;

	(* --------------------- expanding and collapsing in focus text ------------------------ *)

	PROCEDURE ExpandFolds* (text: TextModels.Model; nested: BOOLEAN; IN label: ARRAY OF CHAR);
		VAR op: Domains.Operation; fold, l, r: Fold; rd: TextModels.Reader;
	BEGIN
		ASSERT(text # NIL, 20);
		Models.BeginModification(Models.clean, text);
		IF nested THEN Models.BeginScript(text, expandFoldsKey, op)
		ELSE Models.BeginScript(text, zoomInKey, op)
		END;
		rd := text.NewReader(NIL); rd.SetPos(0);
		ReadNext(rd, fold);
		WHILE ~rd.eot DO
			IF fold.leftSide & fold.collapsed THEN
				IF (label = "") OR (label = fold.label) THEN
					fold.Flip;
					IF ~nested THEN 
						GetPair(fold, l, r);
						rd.SetPos(r.context(TextModels.Context).Pos())
					END
				END
			END;
			ReadNext(rd, fold)
		END;
		Models.EndScript(text, op);
		Models.EndModification(Models.clean, text)
	END ExpandFolds;

	PROCEDURE CollapseFolds* (text: TextModels.Model; nested: BOOLEAN; IN label: ARRAY OF CHAR);
		VAR op: Domains.Operation; fold, r, l: Fold; rd: TextModels.Reader;
	BEGIN
		ASSERT(text # NIL, 20);
		Models.BeginModification(Models.clean, text);
		IF nested THEN Models.BeginScript(text, collapseFoldsKey, op)
		ELSE Models.BeginScript(text, zoomOutKey, op)
		END;
		rd := text.NewReader(NIL); rd.SetPos(0);
		ReadNext(rd, fold);
		WHILE ~rd.eot DO
			IF ~fold.leftSide & ~fold.collapsed THEN
				GetPair(fold, l, r);
				IF (label = "") OR (label = l.label) THEN
					fold.Flip;
					GetPair(l, l, r);
					rd.SetPos(r.context(TextModels.Context).Pos()+1);
					IF ~nested THEN REPEAT ReadNext(rd, fold) UNTIL rd.eot OR fold.leftSide
					ELSE ReadNext(rd, fold)
					END
				ELSE ReadNext(rd, fold)
				END
			ELSE ReadNext(rd, fold)
			END
		END;
		Models.EndScript(text, op);
		Models.EndModification(Models.clean, text)
	END CollapseFolds;

	PROCEDURE ZoomIn*;
		VAR text: TextModels.Model;
	BEGIN
		text := TextViews.FocusText();
		IF text # NIL THEN ExpandFolds(text, FALSE, "") END
	END ZoomIn;

	PROCEDURE ZoomOut*;
		VAR text: TextModels.Model;
	BEGIN
		text := TextViews.FocusText();
		IF text # NIL THEN CollapseFolds(text, FALSE, "") END
	END ZoomOut;

	PROCEDURE Expand*;
		VAR text: TextModels.Model;
	BEGIN
		text := TextViews.FocusText();
		IF text # NIL THEN ExpandFolds(text, TRUE, "") END
	END Expand;

	PROCEDURE Collapse*;
		VAR text: TextModels.Model;
	BEGIN
		text := TextViews.FocusText();
		IF text # NIL THEN CollapseFolds(text, TRUE, "") END
	END Collapse;

	(* ---------------------- foldData dialogbox --------------------------- *)

	PROCEDURE FindLabelGuard* (VAR par: Dialog.Par);
	BEGIN
		par.disabled := (TextViews.Focus() = NIL) OR foldData.all
	END FindLabelGuard;
		
	PROCEDURE SetLabelGuard* ( VAR p : Dialog.Par );
		VAR v: Views.View;
	BEGIN
		Controllers.SetCurrentPath(Controllers.targetPath);
		v := Containers.FocusSingleton();
		p.disabled := (v = NIL) OR ~(v IS Fold) OR ~v(Fold).leftSide;
		Controllers.ResetCurrentPath()
	END SetLabelGuard;

	PROCEDURE ExpandLabel*;
		VAR text: TextModels.Model;
	BEGIN
		IF foldData.all & (foldData.findLabel # "") THEN
			foldData.findLabel := ""; Dialog.Update(foldData)
		END;
		text := TextViews.FocusText();
		IF text # NIL THEN
			IF ~foldData.all THEN ExpandFolds(text, foldData.nested, foldData.findLabel)
			ELSE ExpandFolds(text, foldData.nested, "")
			END
		END
	END ExpandLabel;

	PROCEDURE CollapseLabel*;
		VAR text: TextModels.Model;
	BEGIN
		IF foldData.all & (foldData.findLabel # "") THEN
			foldData.findLabel := ""; Dialog.Update(foldData)
		END;
		text := TextViews.FocusText();
		IF text # NIL THEN
			IF ~foldData.all THEN CollapseFolds(text, foldData.nested, foldData.findLabel)
			ELSE CollapseFolds(text, foldData.nested, "")
			END
		END
	END CollapseLabel;

	PROCEDURE FindFold(first: BOOLEAN);
	VAR c : TextControllers.Controller; r: TextModels.Reader; 
		v : Views.View; pos, i : INTEGER;
	BEGIN
		c := TextControllers.Focus();
		IF c # NIL THEN
			IF first THEN pos := 0 
			ELSE
				pos := c.CaretPos();
				IF pos = TextControllers.none THEN
					c.GetSelection(i, pos);
					IF pos = i THEN pos := 0 ELSE INC(pos) END;
					pos := MIN(pos, c.text.Length()-1)
				END
			END;
			r := c.text.NewReader(NIL); r.SetPos(pos);
			REPEAT r.ReadView(v)
			UNTIL r.eot OR ((v IS Fold) & v(Fold).leftSide) & (foldData.all OR (v(Fold).label$ = foldData.findLabel$));
			IF r.eot THEN
				c.SetCaret(0); Dialog.Beep
			ELSE
				pos := r.Pos();
				c.view.ShowRange(pos-1, pos, FALSE);
				c.SetSelection(pos-1, pos);
				IF LEN(v(Fold).label) > 0 THEN
					foldData.newLabel := v(Fold).label
				END;
				Dialog.Update(foldData)
			END
		ELSE
			Dialog.Beep
		END
	END FindFold;

	PROCEDURE FindNextFold*;
	BEGIN
		FindFold(FALSE)
	END FindNextFold;
	
	PROCEDURE FindFirstFold*;
	BEGIN
		FindFold(TRUE)
	END FindFirstFold;
	
	PROCEDURE SetLabel*;
		VAR v: Views.View;
	BEGIN
		Controllers.SetCurrentPath(Controllers.targetPath);
		v := Containers.FocusSingleton();
		IF (v # NIL) & (v IS Fold) & (LEN(foldData.newLabel) > 0) THEN
			v(Fold).label := foldData.newLabel
		ELSE
			Dialog.Beep
		END;
		Controllers.ResetCurrentPath()
	END SetLabel;

	PROCEDURE (a: Action) Do;
		VAR v: Views.View; fp: INTEGER;
	BEGIN
		Controllers.SetCurrentPath(Controllers.targetPath);
		v := Containers.FocusSingleton();
		IF (v = NIL) OR ~(v IS Fold) THEN 
			fingerprint := 0;
			foldData.newLabel := ""
		ELSE 
			fp := Services.AdrOf(v);
			IF fp # fingerprint THEN 
				foldData.newLabel := v(Fold).label;
				fingerprint := fp;
				Dialog.Update(foldData)
			END
		END;
		Controllers.ResetCurrentPath();
		Services.DoLater(action, Services.Ticks() + Services.resolution DIV 2)
	END Do;

	(* ------------------------ inserting folds ------------------------ *)
		
	PROCEDURE Overlaps* (text: TextModels.Model; beg, end: INTEGER): BOOLEAN;
		VAR n, level: INTEGER; rd: TextModels.Reader; v: Views.View;
	BEGIN
		ASSERT(text # NIL, 20);
		ASSERT((beg >= 0) & (end <= text.Length()) & (beg <= end), 21);
		rd := text.NewReader(NIL); rd.SetPos(beg);
		n := 0; level := 0;
		REPEAT rd.ReadView(v);
			IF ~rd.eot & (rd.Pos() <= end) THEN
				WITH v: Fold DO INC(n);
					IF v.leftSide THEN INC(level) ELSE DEC(level) END
				ELSE
				END
			END
		UNTIL rd.eot OR (level < 0) OR (rd.Pos() >= end);
		RETURN (level # 0) OR ODD(n)
	END Overlaps;

	PROCEDURE InsertionAttr (text: TextModels.Model; pos: INTEGER): TextModels.Attributes;
		VAR rd: TextModels.Reader; ch: CHAR;
	BEGIN
		rd := text.NewReader(NIL);
		rd.SetPos(pos); rd.ReadChar(ch);
		RETURN rd.attr
	END InsertionAttr;

	PROCEDURE Insert* (text: TextModels.Model; label: Label; beg, end: INTEGER; collapsed: BOOLEAN);
		VAR w: TextModels.Writer; fold: Fold; insop: Domains.Operation; a: TextModels.Attributes;
	BEGIN
		ASSERT(text # NIL, 20);
		ASSERT((beg >= 0) & (end <= text.Length()) & (beg <= end), 21);
		a := InsertionAttr(text, beg);
		w := text.NewWriter(NIL); w.SetPos(beg);
		IF a # NIL THEN w.SetAttr(a) END;
		NEW(fold);
		fold.leftSide := TRUE; fold.collapsed := collapsed;
		fold.hidden := TextModels.CloneOf(text); Stores.Join(fold, fold.hidden);
		fold.label := label$;
		Models.BeginScript(text, insertFoldKey, insop);
		w.WriteView(fold, 0, 0);
		w.SetPos(end+1);
		a := InsertionAttr(text, end+1);
		IF a # NIL THEN w.SetAttr(a) END;
		NEW(fold);
		fold.leftSide := FALSE; fold.collapsed := collapsed;
		fold.hidden := NIL; fold.label := "";
		w.WriteView(fold, 0, 0);
		Models.EndScript(text, insop)
	END Insert;

	PROCEDURE CreateGuard* (VAR par: Dialog.Par);
		VAR c: TextControllers.Controller; beg, end: INTEGER;
	BEGIN c := TextControllers.Focus();
		IF (c # NIL) &  ~(Containers.noCaret IN c.opts) THEN
			IF c.HasSelection() THEN c.GetSelection(beg, end);
				IF Overlaps(c.text, beg, end) THEN par.disabled := TRUE END
			END
		ELSE par.disabled := TRUE
		END
	END CreateGuard;

	PROCEDURE Create* (state: INTEGER);	(* menu cmd parameters don't accept Booleans *)
		VAR c: TextControllers.Controller; beg, end: INTEGER; collapsed: BOOLEAN;
	BEGIN
		collapsed := state = 0;
		c := TextControllers.Focus();
		IF (c # NIL) & ~(Containers.noCaret IN c.opts) THEN
			IF c.HasSelection() THEN c.GetSelection(beg, end);
				IF ~Overlaps(c.text, beg, end) THEN Insert(c.text, "", beg, end, collapsed) END
			ELSE beg := c.CaretPos(); Insert(c.text, "", beg, beg, collapsed)
			END
		END
	END Create;

	PROCEDURE InitIcons;
		VAR font: Fonts.Font;

		PROCEDURE DefaultAppearance;
		BEGIN
			font := Fonts.dir.Default(); iconFont := font.typeface$;
			leftExp := ">"; rightExp := "<";
			leftColl := "=>"; rightColl := "<=";
			coloredBackg := TRUE
		END DefaultAppearance;

	BEGIN
		IF Dialog.platform = Dialog.linux THEN (* Linux *)
			DefaultAppearance; 
			coloredBackg := FALSE
		ELSIF Dialog.platform DIV 10 = 1 THEN (* Windows *)
			iconFont := "Wingdings";
			font := Fonts.dir.This(iconFont, 10*Fonts.point (*arbitrary*), {}, Fonts.normal);
			IF font.IsAlien() THEN DefaultAppearance
			ELSE
				leftExp[0] := SHORT(CHR(240)); leftExp[1] := 0X;
				rightExp[0] := SHORT(CHR(239)); rightExp[1] := 0X;
				leftColl[0] := SHORT(CHR(232)); leftColl[1] := 0X;
				rightColl[0] := SHORT(CHR(231)); rightColl[1] := 0X;
				coloredBackg := FALSE
			END
		ELSIF Dialog.platform DIV 10 = 2 THEN (* Mac *)
			iconFont := "Chicago";
			font := Fonts.dir.This(iconFont, 10*Fonts.point (*arbitrary*), {}, Fonts.normal);
			IF font.IsAlien() THEN DefaultAppearance
			ELSE
				leftExp := ">"; rightExp := "<";
				leftColl := "�"; rightColl := "�";
				coloredBackg := TRUE
			END
		ELSE
			DefaultAppearance
		END
	END InitIcons;

	PROCEDURE (d: StdDirectory) New (collapsed: BOOLEAN; label: Label;
																hiddenText: TextModels.Model): Fold;
		VAR fold: Fold;
	BEGIN
		NEW(fold); fold.leftSide := hiddenText # NIL; fold.collapsed := collapsed;
		fold.label := label; fold.hidden := hiddenText; 
		IF hiddenText # NIL THEN Stores.Join(fold, fold.hidden) END;
		RETURN fold
	END  New;

	PROCEDURE SetDir* (d: Directory);
	BEGIN
		ASSERT(d # NIL, 20);
		dir := d
	END SetDir;

	PROCEDURE InitMod;
		VAR d: StdDirectory;
	BEGIN
		foldData.all := TRUE; foldData.nested := FALSE; foldData.findLabel := ""; foldData.newLabel := "";
		NEW(d); dir := d; stdDir := d;
		InitIcons;
		NEW(action); Services.DoLater(action, Services.now);
	END InitMod;

BEGIN
	InitMod
END StdFolds.
