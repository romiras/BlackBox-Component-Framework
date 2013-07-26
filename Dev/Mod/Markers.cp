﻿MODULE DevMarkers;
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
	
	(* cas 28.03.01 *)
	(* ww	28.03.01	error file renamed from "error" to "Error" to avoid problems on Unix *)
	(* dg	06.05.99	replaced SetDirty-hack by transparent operations *)
	(* dg	02.12.98	changes due to renaming of Views.CopyFrom => Views.CopyFromSimpleView *)
	(* dg	24.11.98	Domains 2000 *)
	(* dg 01.10.98 Kernel.RemoveCleaner call added *)
	(* bh 23.3.96 Unmark without era *)
	(* bh 18.3.96 dir.NewMsg *)

	IMPORT
		Kernel, Files, Stores, Fonts, Ports, Models, Views, Controllers, Properties, Dialog,
		TextModels, TextSetters, TextViews, TextControllers, TextMappers;

	CONST
		(** View.mode **)
		undefined* = 0; mark* = 1; message* = 2; 
		firstMode = 1; lastMode = 2;

		(** View.err **)
		noCode* = 9999;

		errFile = "Errors"; point = Ports.point;

	TYPE
		View* = POINTER TO ABSTRACT RECORD (Views.View)
			mode-: INTEGER;
			err-: INTEGER;
			msg-: POINTER TO ARRAY OF CHAR;
			era: INTEGER
		END;

		Directory* = POINTER TO ABSTRACT RECORD END;


		StdView = POINTER TO RECORD (View) END;

		StdDirectory = POINTER TO RECORD (Directory) END;

		SetModeOp = POINTER TO RECORD (Stores.Operation)
			view: View;
			mode: INTEGER
		END;
		

	VAR
		dir-, stdDir-: Directory;

		globR: TextModels.Reader; globW: TextModels.Writer;	(* recycling done in Load, Insert *)
		
		thisEra: INTEGER;


	(** View **)

	PROCEDURE (v: View) CopyFromSimpleView- (source: Views.View), EXTENSIBLE;
	BEGIN
		(* v.CopyFrom^(source); *)
		WITH source: View DO
			v.err := source.err; v.mode := source.mode;
			IF source.msg # NIL THEN
				NEW(v.msg, LEN(source.msg^)); v.msg^ := source.msg^$
			END
		END
	END CopyFromSimpleView;

(*
	PROCEDURE (v: View) InitContext* (context: Models.Context), EXTENSIBLE;
	BEGIN
		ASSERT(v.mode # undefined, 20);
		v.InitContext^(context)
	END InitContext;
*)

	PROCEDURE (v: View) InitErr* (err: INTEGER), NEW, EXTENSIBLE;
	BEGIN
		ASSERT(v.msg = NIL, 20);
		IF v.err # err THEN v.err := err; v.mode := mark END;
		IF v.mode = undefined THEN v.mode := mark END
	END InitErr;

	PROCEDURE (v: View) InitMsg* (msg: ARRAY OF CHAR), NEW, EXTENSIBLE;
		VAR i: INTEGER; str: ARRAY 1024 OF CHAR;
	BEGIN
		ASSERT(v.msg = NIL, 20);
		Dialog.MapString(msg, str);
		i := 0; WHILE str[i] # 0X DO INC(i) END;
		NEW(v.msg, i + 1); v.msg^ := str$;
		v.mode := mark
	END InitMsg;

	PROCEDURE (v: View) SetMode* (mode: INTEGER), NEW, EXTENSIBLE;
		VAR op: SetModeOp;
	BEGIN
		ASSERT((firstMode <= mode) & (mode <= lastMode), 20);
		IF v.mode # mode THEN
			NEW(op); op.view := v; op.mode := mode;
			Views.Do(v, "#System:ViewSetting", op)
		END
	END SetMode;


	(** Directory **)

	PROCEDURE (d: Directory) New* (type: INTEGER): View, NEW, ABSTRACT;
	PROCEDURE (d: Directory) NewMsg* (msg: ARRAY OF CHAR): View, NEW, ABSTRACT;


	(* SetModeOp *)

	PROCEDURE (op: SetModeOp) Do;
		VAR v: View; mode: INTEGER;
	BEGIN
		v := op.view;
		mode := v.mode; v.mode := op.mode; op.mode := mode;
		Views.Update(v, Views.keepFrames);
		IF v.context # NIL THEN v.context.SetSize(Views.undefined, Views.undefined) END
	END Do;

	PROCEDURE ToggleMode (v: View);
		VAR mode: INTEGER;
	BEGIN
		IF ABS(v.err) # noCode THEN
			IF v.mode < lastMode THEN mode := v.mode + 1 ELSE mode := firstMode END
		ELSE
			IF v.mode < message THEN mode := v.mode + 1 ELSE mode := firstMode END
		END;
		v.SetMode(mode)
	END ToggleMode;


	(* primitives for StdView *)

	PROCEDURE NumToStr (x: INTEGER; VAR s: ARRAY OF CHAR; VAR i: INTEGER);
		VAR j: INTEGER; m: ARRAY 32 OF CHAR;
	BEGIN
		ASSERT(x >= 0, 20);
		j := 0; REPEAT m[j] := CHR(x MOD 10 + ORD("0")); x := x DIV 10; INC(j) UNTIL x = 0;
		i := 0; REPEAT DEC(j); s[i] := m[j]; INC(i) UNTIL j = 0;
		s[i] := 0X
	END NumToStr;

	PROCEDURE Load (v: StdView);
		VAR view: Views.View; t: TextModels.Model; s: TextMappers.Scanner;
			err: INTEGER; i: INTEGER; ch: CHAR; loc: Files.Locator;
			msg: ARRAY 1024 OF CHAR;
	BEGIN
		err := ABS(v.err); NumToStr(err, msg, i);
		loc := Files.dir.This("Dev"); IF loc = NIL THEN RETURN END;
		loc := loc.This("Rsrc"); IF loc = NIL THEN RETURN END;
		view := Views.OldView(loc, errFile);
		IF (view # NIL) & (view IS TextViews.View) THEN
			t := view(TextViews.View).ThisModel();
			IF t # NIL THEN
				s.ConnectTo(t);
				REPEAT
					s.Scan
				UNTIL ((s.type = TextMappers.int) & (s.int = err)) OR (s.type = TextMappers.eot);
				IF s.type = TextMappers.int THEN
					s.Skip(ch); i := 0;
					WHILE (ch >= " ") & (i < LEN(msg) - 1) DO
						msg[i] := ch; INC(i); s.rider.ReadChar(ch)
					END;
					msg[i] := 0X
				END
			END
		END;
		NEW(v.msg, i + 1); v.msg^ := msg$
	END Load;

	PROCEDURE DrawMsg (v: StdView; f: Views.Frame; font: Fonts.Font; color: Ports.Color);
		VAR w, h, asc, dsc: INTEGER;
	BEGIN
		CASE v.mode OF
		  mark:
			v.context.GetSize(w, h);
			f.DrawLine(point, 0, w - 2 * point, h, 0, color);
			f.DrawLine(w - 2 * point, 0, point, h, 0, color)
		| message:
			font.GetBounds(asc, dsc, w);
			f.DrawString(2 * point, asc, color, v.msg^, font)
		END
	END DrawMsg;
	
	PROCEDURE ShowMsg (v: StdView);
	BEGIN
		IF v.msg = NIL THEN Load(v) END;
		Dialog.ShowStatus(v.msg^)
	END ShowMsg;

	PROCEDURE Track (v: StdView; f: Views.Frame; x, y: INTEGER; buttons: SET);
		VAR c: Models.Context; t: TextModels.Model; u, w, h: INTEGER; isDown, in, in0: BOOLEAN; m: SET;
	BEGIN
		v.context.GetSize(w, h); u := f.dot; in0 := FALSE;
		in := (0 <= x) & (x < w) & (0 <= y) & (y < h);
		REPEAT
			IF in # in0 THEN
				f.MarkRect(u, 0, w - u, h, Ports.fill, Ports.invert, Ports.show); in0 := in
			END;
			f.Input(x, y, m, isDown);
			in := (0 <= x) & (x < w) & (0 <= y) & (y < h)
		UNTIL ~isDown;
		IF in0 THEN
			f.MarkRect(u, 0, w - u, h, Ports.fill, Ports.invert, Ports.hide);
			IF Dialog.showsStatus & ~(Controllers.modify IN buttons) & ~(Controllers.doubleClick IN buttons) THEN
				ShowMsg(v)
			ELSE
				ToggleMode(v)
			END;
			c := v.context;
			WITH c: TextModels.Context DO
				t := c.ThisModel();
				TextControllers.SetCaret(t, c.Pos() + 1)
			ELSE
			END
		END
	END Track;

	PROCEDURE SizePref (v: StdView; VAR p: Properties.SizePref);
		VAR c: Models.Context; a: TextModels.Attributes; font: Fonts.Font; asc, dsc, w: INTEGER;
	BEGIN
		c := v.context;
		IF (c # NIL) & (c IS TextModels.Context) THEN a := c(TextModels.Context).Attr(); font := a.font
		ELSE font := Fonts.dir.Default()
		END;
		font.GetBounds(asc, dsc, w);
		p.h := asc + dsc;
		CASE v.mode OF
		  mark:
			p.w := p.h + 2 * point
		| message:
			IF v.msg = NIL THEN Load(v) END;
			p.w := font.StringWidth(v.msg^) + 4 * point
		END
	END SizePref;


	(* StdView *)

	PROCEDURE (v: StdView) ExternalizeAs (VAR s1: Stores.Store);
	BEGIN
		s1 := NIL
	END ExternalizeAs;

	PROCEDURE (v: StdView) SetMode(mode: INTEGER);
	BEGIN v.SetMode^(mode); ShowMsg(v)
	END SetMode;

	PROCEDURE (v: StdView) Restore (f: Views.Frame; l, t, r, b: INTEGER);
		VAR c: Models.Context; a: TextModels.Attributes; font: Fonts.Font; color: Ports.Color;
			w, h: INTEGER;
	BEGIN
		c := v.context; c.GetSize(w, h);
		WITH c: TextModels.Context DO a := c.Attr(); font := a.font ELSE font := Fonts.dir.Default() END;
		IF TRUE (*f.colors >= 4*) THEN color := Ports.grey50 ELSE color := Ports.defaultColor END;
		IF v.err >= 0 THEN
			f.DrawRect(point, 0, w - point, h, Ports.fill, color);
			DrawMsg(v, f, font, Ports.background)
		ELSE
			f.DrawRect(point, 0, w - point, h, 0, color);
			DrawMsg(v, f, font, Ports.defaultColor)
		END
	END Restore;

	PROCEDURE (v: StdView) GetBackground (VAR color: Ports.Color);
	BEGIN
		color := Ports.background
	END GetBackground;

	PROCEDURE (v: StdView) HandleCtrlMsg (f: Views.Frame; VAR msg: Controllers.Message;
																		VAR focus: Views.View);
	BEGIN
		WITH msg: Controllers.TrackMsg DO
			Track(v, f, msg.x, msg.y, msg.modifiers)
		ELSE
		END
	END HandleCtrlMsg;

	PROCEDURE (v: StdView) HandlePropMsg (VAR msg: Properties.Message);
		VAR c: Models.Context; a: TextModels.Attributes; font: Fonts.Font; asc, w: INTEGER;
	BEGIN
		WITH msg: Properties.Preference DO
			WITH msg: Properties.SizePref DO
				SizePref(v, msg)
			| msg: Properties.ResizePref DO
				msg.fixed := TRUE
			| msg: Properties.FocusPref DO
				msg.hotFocus := TRUE
(*
			| msg: Properties.StorePref DO
				msg.view := NIL
*)
			| msg: TextSetters.Pref DO
				c := v.context;
				IF (c # NIL) & (c IS TextModels.Context) THEN
					a := c(TextModels.Context).Attr(); font := a.font
				ELSE
					font := Fonts.dir.Default()
				END;
				font.GetBounds(asc, msg.dsc, w)
			ELSE
			END
		ELSE
		END
	END HandlePropMsg;


	(* StdDirectory *)

	PROCEDURE (d: StdDirectory) New (err: INTEGER): View;
		VAR v: StdView;
	BEGIN
		NEW(v); v.InitErr(err); RETURN v
	END New;

	PROCEDURE (d: StdDirectory) NewMsg (msg: ARRAY OF CHAR): View;
		VAR v: StdView;
	BEGIN
		NEW(v); v.InitErr(noCode); v.InitMsg(msg); RETURN v
	END NewMsg;


	(** Cleaner **)

	PROCEDURE Cleanup;
	BEGIN
		globR := NIL; globW := NIL
	END Cleanup;
	

	(** miscellaneous **)

	PROCEDURE Insert* (text: TextModels.Model; pos: INTEGER; v: View);
		VAR w: TextModels.Writer; r: TextModels.Reader;
	BEGIN
		ASSERT(v.era = 0, 20);
		Models.BeginModification(Models.clean, text);
		v.era := thisEra;
		IF pos > text.Length() THEN pos := text.Length() END;
		globW := text.NewWriter(globW); w := globW; w.SetPos(pos);
		IF pos > 0 THEN DEC(pos) END;
		globR := text.NewReader(globR); r := globR; r.SetPos(pos); r.Read;
		IF r.attr # NIL THEN w.SetAttr(r.attr) END;
		w.WriteView(v, Views.undefined, Views.undefined);
		Models.EndModification(Models.clean, text);
	END Insert;

	PROCEDURE Unmark* (text: TextModels.Model);
		VAR r: TextModels.Reader; v: Views.View; pos: INTEGER;
			script: Stores.Operation;
	BEGIN
		Models.BeginModification(Models.clean, text);
		Models.BeginScript(text, "#Dev:DeleteMarkers", script);
		r := text.NewReader(NIL); r.ReadView(v);
		WHILE ~r.eot DO
			IF r.view IS View THEN
				pos := r.Pos() - 1; text.Delete(pos, pos + 1); r.SetPos(pos)
			END;
			r.ReadView(v)
		END;
		INC(thisEra);
		Models.EndScript(text, script);
		Models.EndModification(Models.clean, text);
	END Unmark;

	PROCEDURE ShowFirstError* (text: TextModels.Model; focusOnly: BOOLEAN);
		VAR v1: Views.View; pos: INTEGER;
	BEGIN
		globR := text.NewReader(globR); globR.SetPos(0);
		REPEAT globR.ReadView(v1) UNTIL globR.eot OR (v1 IS View);
		IF ~globR.eot THEN
			pos := globR.Pos();
			TextViews.ShowRange(text, pos, pos, focusOnly);
			TextControllers.SetCaret(text, pos);
			v1(View).SetMode(v1(View).mode)
		END
	END ShowFirstError;


	(** commands **)

	PROCEDURE UnmarkErrors*;
		VAR t: TextModels.Model;
	BEGIN
		t := TextViews.FocusText();
		IF t # NIL THEN Unmark(t) END
	END UnmarkErrors;

	PROCEDURE NextError*;
		VAR c: TextControllers.Controller; t: TextModels.Model; v1: Views.View;
			beg, pos: INTEGER;
	BEGIN
		c := TextControllers.Focus();
		IF c # NIL THEN
			t := c.text;
			IF c.HasCaret() THEN pos := c.CaretPos()
			ELSIF c.HasSelection() THEN c.GetSelection(beg, pos)
			ELSE pos := 0
			END;
			TextControllers.SetSelection(t, TextControllers.none, TextControllers.none);
			globR := t.NewReader(globR); globR.SetPos(pos);
			REPEAT globR.ReadView(v1) UNTIL globR.eot OR (v1 IS View);
			IF ~globR.eot THEN
				pos := globR.Pos(); v1(View).SetMode(v1(View).mode);
				TextViews.ShowRange(t, pos, pos, TextViews.focusOnly)
			ELSE
				pos := 0; Dialog.Beep
			END;
			TextControllers.SetCaret(t, pos);
			globR := NIL
		END
	END NextError;

	PROCEDURE ToggleCurrent*;
		VAR c: TextControllers.Controller; t: TextModels.Model; v: Views.View; pos: INTEGER;
	BEGIN
		c := TextControllers.Focus();
		IF (c # NIL) & c.HasCaret() THEN
			t := c.text; pos := c.CaretPos();
			globR := t.NewReader(globR); globR.SetPos(pos); globR.ReadPrev;
			v := globR.view;
			IF (v # NIL) & (v IS View) THEN ToggleMode(v(View)) END;
			TextViews.ShowRange(t, pos, pos, TextViews.focusOnly);
			TextControllers.SetCaret(t, pos);
			globR := NIL
		END
	END ToggleCurrent;


	PROCEDURE SetDir* (d: Directory);
	BEGIN
		dir := d
	END SetDir;


	PROCEDURE Init;
		VAR d: StdDirectory;
	BEGIN
		thisEra := 1;
		NEW(d); dir := d; stdDir := d
	END Init;

BEGIN
	Init; Kernel.InstallCleaner(Cleanup)
CLOSE
	Kernel.RemoveCleaner(Cleanup)
END DevMarkers.
