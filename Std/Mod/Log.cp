MODULE StdLog;
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
		Log, Fonts, Ports, Stores, Models, Views, Dialog, HostDialog, StdDialog,
		TextModels, TextMappers, TextRulers, TextViews, TextControllers;

	CONST
		(** IntForm base **)
		charCode* = TextMappers.charCode; decimal* = TextMappers.decimal; hexadecimal* = TextMappers.hexadecimal;

		(** IntForm showBase **)
		hideBase* = TextMappers.hideBase; showBase* = TextMappers.showBase;

		mm = Ports.mm;

	TYPE
		ShowHook = POINTER TO RECORD (Dialog.ShowHook) END;
		LogHook = POINTER TO RECORD (Log.Hook) END;

	VAR
		logAlerts: BOOLEAN;

		text-, buf-: TextModels.Model;
		defruler-: TextRulers.Ruler;
		dir-: TextViews.Directory;

		out, subOut: TextMappers.Formatter;
		
		showHook: ShowHook;


	PROCEDURE Flush;
	BEGIN
		text.Append(buf); Views.RestoreDomain(text.Domain())
	END Flush;

	PROCEDURE Char* (ch: CHAR);
	BEGIN
		out.WriteChar(ch); Flush
	END Char;

	PROCEDURE Int* (i: LONGINT);
	BEGIN
		out.WriteChar(" "); out.WriteInt(i); Flush
	END Int;

	PROCEDURE Real* (x: REAL);
	BEGIN
		out.WriteChar(" "); out.WriteReal(x); Flush
	END Real;

	PROCEDURE String* (IN str: ARRAY OF CHAR);
	BEGIN
		out.WriteString(str); Flush
	END String;

	PROCEDURE Bool* (x: BOOLEAN);
	BEGIN
		out.WriteChar(" "); out.WriteBool(x); Flush
	END Bool;

	PROCEDURE Set* (x: SET);
	BEGIN
		out.WriteChar(" "); out.WriteSet(x); Flush
	END Set;

	PROCEDURE IntForm* (x: LONGINT; base, minWidth: INTEGER; fillCh: CHAR; showBase: BOOLEAN);
	BEGIN
		out.WriteIntForm(x, base, minWidth, fillCh, showBase); Flush
	END IntForm;

	PROCEDURE RealForm* (x: REAL; precision, minW, expW: INTEGER; fillCh: CHAR);
	BEGIN
		out.WriteRealForm(x, precision, minW, expW, fillCh); Flush
	END RealForm;

	PROCEDURE Tab*;
	BEGIN
		out.WriteTab; Flush
	END Tab;

	PROCEDURE Ln*;
	BEGIN
		out.WriteLn; Flush;
		TextViews.ShowRange(text, text.Length(), text.Length(), TextViews.any)
	END Ln;

	PROCEDURE Para*;
	BEGIN
		out.WritePara; Flush;
		TextViews.ShowRange(text, text.Length(), text.Length(), TextViews.any)
	END Para;

	PROCEDURE View* (v: Views.View);
	BEGIN
		out.WriteView(v); Flush
	END View;

	PROCEDURE ViewForm* (v: Views.View; w, h: INTEGER);
	BEGIN
		out.WriteViewForm(v, w, h); Flush
	END ViewForm;

	PROCEDURE ParamMsg* (IN msg, p0, p1, p2: ARRAY OF CHAR);
	BEGIN
		out.WriteParamMsg(msg, p0, p1, p2); Flush
	END ParamMsg;

	PROCEDURE Msg* (IN msg: ARRAY OF CHAR);
	BEGIN
		out.WriteMsg(msg); Flush
	END Msg;
	

	PROCEDURE^ Open*;

	PROCEDURE (hook: ShowHook) ShowParamMsg (IN s, p0, p1, p2: ARRAY OF CHAR);
	BEGIN
		IF Dialog.showsStatus THEN
			Dialog.ShowParamStatus(s, p0, p1, p2);
			IF logAlerts THEN
				ParamMsg(s, p0, p1, p2); Ln
			END
		ELSE
			IF logAlerts THEN
				Open;
				ParamMsg(s, p0, p1, p2); Ln
			ELSE
				HostDialog.ShowParamMsg(s, p0, p1, p2)
			END
		END
	END ShowParamMsg;
	
	PROCEDURE (hook: ShowHook) ShowParamStatus (IN s, p0, p1, p2: ARRAY OF CHAR);
	BEGIN
		HostDialog.ShowParamStatus(s, p0, p1, p2)
	END ShowParamStatus;


	PROCEDURE NewView* (): TextViews.View;
		VAR v: TextViews.View;
	BEGIN
		Flush;
		Dialog.SetShowHook(showHook); 	(* attach alert dialogs *)
		v := dir.New(text);
		v.SetDefaults(TextRulers.CopyOf(defruler, Views.deep), dir.defAttr);
		RETURN v
	END NewView;

	PROCEDURE New*;
	BEGIN
		Views.Deposit(NewView())
	END New;


	PROCEDURE SetDefaultRuler* (ruler: TextRulers.Ruler);
	BEGIN
		defruler := ruler
	END SetDefaultRuler;

	PROCEDURE SetDir* (d: TextViews.Directory);
	BEGIN
		ASSERT(d # NIL, 20); dir := d
	END SetDir;


	PROCEDURE Open*;
		VAR v: Views.View; pos: INTEGER;
	BEGIN
		v := NewView();
		StdDialog.Open(v, "#Dev:Log", NIL, "", NIL, FALSE, TRUE, FALSE, FALSE, TRUE);
		Views.RestoreDomain(text.Domain());
		pos := text.Length();
		TextViews.ShowRange(text, pos, pos, TextViews.any);
		TextControllers.SetCaret(text, pos)
	END Open;

	PROCEDURE Clear*;
	BEGIN
		Models.BeginModification(Models.notUndoable, text);
		text.Delete(0, text.Length());
		buf.Delete(0, buf.Length());
		Models.EndModification(Models.notUndoable, text)
	END Clear;
	

	(* Sub support *)

	PROCEDURE* Guard (o: ANYPTR): BOOLEAN;
	BEGIN
		RETURN
			(o # NIL) &
			~(		(o IS TextModels.Model) & (o = text)
				OR   (o IS Stores.Domain) & (o = text.Domain())
				OR   (o IS TextViews.View) & (o(TextViews.View).ThisModel() = text)
			)
	END Guard;

	PROCEDURE* ClearBuf;
		VAR subBuf: TextModels.Model;
	BEGIN
		subBuf := subOut.rider.Base(); subBuf.Delete(0, subBuf.Length())
	END ClearBuf;

	PROCEDURE* FlushBuf;
		VAR buf: TextModels.Model;
	BEGIN
		buf := subOut.rider.Base();
		IF buf.Length() > 0 THEN
			IF ~Log.synch THEN Open() END;
			text.Append(buf)
		END
	END FlushBuf;

	PROCEDURE* SubFlush;
	BEGIN
		IF Log.synch THEN
			FlushBuf;
			IF Log.force THEN Views.RestoreDomain(text.Domain()) END
		END;
	END SubFlush;




	PROCEDURE (log: LogHook) Guard* (o: ANYPTR): BOOLEAN;
	BEGIN RETURN Guard(o)
	END Guard;
	
	PROCEDURE (log: LogHook) ClearBuf*;
	BEGIN ClearBuf
	END ClearBuf;
	
	PROCEDURE (log: LogHook) FlushBuf*;
	BEGIN FlushBuf
	END FlushBuf;
	
	PROCEDURE (log: LogHook) Beep*;
	BEGIN Dialog.Beep
	END Beep;
	
	PROCEDURE (log: LogHook) Char* (ch: CHAR);
	BEGIN
		subOut.WriteChar(ch); SubFlush
	END Char;
	
	PROCEDURE (log: LogHook) Int* (n: INTEGER);
	BEGIN
		subOut.WriteChar(" "); subOut.WriteInt(n); SubFlush
	END Int;
	
	PROCEDURE (log: LogHook) Real* (x: REAL);
	BEGIN
		subOut.WriteChar(" "); subOut.WriteReal(x); SubFlush
	END Real;
	
	PROCEDURE (log: LogHook) String* (IN str: ARRAY OF CHAR);
	BEGIN
		subOut.WriteString(str); SubFlush
	END String;
	
	PROCEDURE (log: LogHook) Bool* (x: BOOLEAN);
	BEGIN
		subOut.WriteChar(" "); subOut.WriteBool(x); SubFlush
	END Bool;
	
	PROCEDURE (log: LogHook) Set* (x: SET);
	BEGIN
		subOut.WriteChar(" "); subOut.WriteSet(x); SubFlush
	END Set;
	
	PROCEDURE (log: LogHook) IntForm* (x: INTEGER; base, minWidth: INTEGER; fillCh: CHAR; showBase: BOOLEAN);
	BEGIN
		subOut.WriteIntForm(x, base, minWidth, fillCh, showBase); SubFlush
	END IntForm;
	
	PROCEDURE (log: LogHook) RealForm* (x: REAL; precision, minW, expW: INTEGER; fillCh: CHAR);
	BEGIN
		subOut.WriteRealForm(x, precision, minW, expW, fillCh); SubFlush
	END RealForm;
	
	PROCEDURE (log: LogHook) Tab*;
	BEGIN
		subOut.WriteTab; SubFlush
	END Tab;
	
	PROCEDURE (log: LogHook) Ln*;
	BEGIN
		subOut.WriteLn; SubFlush;
		IF Log.synch THEN Views.RestoreDomain(text.Domain()) END	
	END Ln;
	
	PROCEDURE (log: LogHook) Para*;
	BEGIN
		subOut.WritePara; SubFlush;
		IF Log.synch THEN Views.RestoreDomain(text.Domain()) END
	END Para;
	
	PROCEDURE (log: LogHook) View* (v: ANYPTR);
	BEGIN
		IF (v # NIL) & (v IS Views.View) THEN
			subOut.WriteView(v(Views.View)); SubFlush
		END
	END View;
	
	PROCEDURE (log: LogHook) ViewForm* (v: ANYPTR; w, h: INTEGER);
	BEGIN
		ASSERT(v # NIL, 20);
		IF (v # NIL) & (v IS Views.View) THEN
			subOut.WriteViewForm(v(Views.View), w, h); SubFlush
		END
	END ViewForm;

	PROCEDURE (log: LogHook) ParamMsg* (IN s, p0, p1, p2: ARRAY OF CHAR);
		VAR msg: ARRAY 256 OF CHAR; i: INTEGER; ch: CHAR;
	BEGIN
		IF logAlerts THEN
			IF Log.synch THEN Open END;
			Dialog.MapParamString(s, p0, p1, p2, msg);
			i := 0; ch := msg[0];
			WHILE ch # 0X DO
				IF ch = TextModels.line THEN subOut.WriteLn
				ELSIF ch = TextModels.para THEN subOut.WritePara
				ELSIF ch = TextModels.tab THEN subOut.WriteTab
				ELSIF ch >= " " THEN subOut.WriteChar(ch)
				END;
				INC(i); ch := msg[i];
			END;
			subOut.WriteLn; SubFlush
		ELSE
			HostDialog.ShowParamMsg(s, p0, p1, p2)
		END
	END ParamMsg;


	PROCEDURE AttachSubLog;
		VAR h: LogHook;
	BEGIN
		subOut.ConnectTo(TextModels.dir.New());
		NEW(h);
		Log.SetHook(h);
	END AttachSubLog;

	PROCEDURE DetachSubLog;
	BEGIN
		Log.SetHook(NIL);
	END DetachSubLog;


	PROCEDURE Init;
		VAR font: Fonts.Font; p: TextRulers.Prop; x: INTEGER; i: INTEGER;
	BEGIN
		logAlerts := TRUE; (* logReports := FALSE; *)

		text := TextModels.dir.New();
		buf := TextModels.CloneOf(text);
		out.ConnectTo(buf);

		font := TextModels.dir.attr.font;
		defruler := TextRulers.dir.New(NIL);
		TextRulers.SetRight(defruler, 80*mm);
		dir := TextViews.dir;
		NEW(showHook)
	END Init;

BEGIN
	Init; AttachSubLog
CLOSE
	DetachSubLog; 
END StdLog.
