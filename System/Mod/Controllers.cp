MODULE Controllers;
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
	
	IMPORT Kernel, Services, Ports, Stores, Models, Views;

	CONST
		(** Forward target **)
		targetPath* = TRUE; frontPath* = FALSE;

		(** ScrollMsg.op **)
		decLine* = 0; incLine* = 1; decPage* = 2; incPage* = 3; gotoPos* = 4;

		(** PageMsg.op **)
		nextPageX* = 0; nextPageY* = 1; gotoPageX* = 2; gotoPageY* = 3;

		(** PollOpsMsg.valid, EditMsg.op **)
		cut* = 0; copy* = 1;
		pasteChar* = 2; (* pasteLChar* = 3; *) paste* = 4; (* pasteView* = 5; *)

		(** TrackMsg.modifiers, EditMsg.modifiers **)
		doubleClick* = 0;	(** clicking history **)
		extend* = 1; modify* = 2;	(** modifier keys **)
		(* extend = Sub.extend; modify = Sub.modify *)

		(** PollDropMsg.mark, PollDrop mark **)
		noMark* = FALSE; mark* = TRUE;
		(** PollDropMsg.show, PollDrop show **)
		hide* = FALSE; show* = TRUE;

		minVersion = 0; maxVersion = 0;


	TYPE

		(** messages **)

		Message* = Views.CtrlMessage;

		PollFocusMsg* = EXTENSIBLE RECORD (Message)
			focus*: Views.Frame	(** OUT, preset to NIL **)
		END;

		PollSectionMsg* = RECORD (Message)
			focus*, vertical*: BOOLEAN;	(** IN **)
			wholeSize*: INTEGER;	(** OUT, preset to 1 **)
			partSize*: INTEGER;	(** OUT, preset to 1 **)
			partPos*: INTEGER;	(** OUT, preset to 0 **)
			valid*, done*: BOOLEAN	(** OUT, preset to (FALSE, FALSE) **)
		END;

		PollOpsMsg* = RECORD (Message)
			type*: Stores.TypeName;	(** OUT, preset to "" **)
			pasteType*: Stores.TypeName;	(** OUT, preset to "" **)
			singleton*: Views.View;	(** OUT, preset to NIL **)
			selectable*: BOOLEAN;	(** OUT, preset to FALSE **)
			valid*: SET	(** OUT, preset to {} **)
		END;

		ScrollMsg* = RECORD (Message)
			focus*, vertical*: BOOLEAN;	(** IN **)
			op*: INTEGER;	(** IN **)
			pos*: INTEGER;	(** IN **)
			done*: BOOLEAN	(** OUT, preset to FALSE **)
		END;

		PageMsg* = RECORD (Message)
			op*: INTEGER;	(** IN **)
			pageX*, pageY*: INTEGER;	(** IN **)
			done*, eox*, eoy*: BOOLEAN	(** OUT, preset to (FALSE, FALSE, FALSE) **)
		END;

		TickMsg* = RECORD (Message)
			tick*: INTEGER	(** IN **)
		END;

		MarkMsg* = RECORD (Message)
			show*: BOOLEAN;	(** IN **)
			focus*: BOOLEAN	(** IN **)
		END;

		SelectMsg* = RECORD (Message)
			set*: BOOLEAN	(** IN **)
		END;


		RequestMessage* = ABSTRACT RECORD (Message)
			requestFocus*: BOOLEAN	(** OUT, preset (by framework) to FALSE **)
		END;

		EditMsg* = RECORD (RequestMessage)
			op*: INTEGER;	(** IN **)
			modifiers*: SET;	(** IN, valid if op IN {pasteChar, pasteLchar} **)
			char*: CHAR;	(** IN, valid if op = pasteChar **)
			view*: Views.View; w*, h*: INTEGER;	(** IN, valid if op  = paste **)
														(** OUT, valid if op IN {cut, copy} **)
			isSingle*: BOOLEAN;	(** dito **)
			clipboard*: BOOLEAN	(** IN, valid if op IN {cut, copy, paste} **)
		END;

		ReplaceViewMsg* = RECORD (RequestMessage)
			old*, new*: Views.View	(** IN **)
		END;


		CursorMessage* = ABSTRACT RECORD (RequestMessage)
			x*, y*: INTEGER	(** IN, needs translation when passed on **)
		END;

		PollCursorMsg* = RECORD (CursorMessage)
			cursor*: INTEGER;	(** OUT, preset to Ports.arrowCursor **)
			modifiers*: SET	(** IN **)
		END;

		TrackMsg* = RECORD (CursorMessage)
			modifiers*: SET	(** IN **)
		END;

		WheelMsg* = RECORD (CursorMessage)
			done*: BOOLEAN; 		(** must be set if the message is handled **)
			op*, nofLines*: INTEGER;
		END;


		TransferMessage* = ABSTRACT RECORD (CursorMessage)
			source*: Views.Frame;	(** IN, home frame of transfer originator, may be NIL if unknown **)
			sourceX*, sourceY*: INTEGER	(** IN, reference point in source frame, defined if source # NIL **)
		END;

		PollDropMsg* = RECORD (TransferMessage)
			mark*: BOOLEAN;	(** IN, request to mark drop target **)
			show*: BOOLEAN;	(** IN, if mark then show/hide target mark **)
			type*: Stores.TypeName;	(** IN, type of view to drop **)
			isSingle*: BOOLEAN;	(** IN, view to drop is singleton **)
			w*, h*: INTEGER;	(** IN, size of view to drop, may be 0, 0 **)
			rx*, ry*: INTEGER;	(** IN, reference point in view **)
			dest*: Views.Frame	(** OUT, preset to NIL, set if DropMsg is acceptable **)
		END;

		DropMsg* = RECORD (TransferMessage)
			view*: Views.View;	(** IN, drop this *)
			isSingle*: BOOLEAN;	(** IN, view to drop is singleton **)
			w*, h*: INTEGER;	(** IN, proposed size *)
			rx*, ry*: INTEGER	(** IN, reference point in view **)
		END;


		(** controllers **)

		Controller* = POINTER TO ABSTRACT RECORD (Stores.Store) END;


		(** forwarders **)

		Forwarder* = POINTER TO ABSTRACT RECORD
			next: Forwarder
		END;

		TrapCleaner = POINTER TO RECORD (Kernel.TrapCleaner) END;
		PathInfo = POINTER TO RECORD
			path: BOOLEAN; prev: PathInfo
		END;
		
		BalanceCheckAction = POINTER TO RECORD (Services.Action) 
			wait: WaitAction
		END;
		WaitAction = POINTER TO RECORD (Services.Action) 
			check: BalanceCheckAction
		END;

	VAR
		path-: BOOLEAN;

		list: Forwarder;
		
		cleaner: TrapCleaner;
		prevPath, cache: PathInfo;
		


	(** BalanceCheckAction **)
	
	PROCEDURE (a: BalanceCheckAction) Do;
	BEGIN
		Services.DoLater(a.wait, Services.resolution);
		ASSERT(prevPath = NIL, 100);
	END Do;
	
	PROCEDURE (a: WaitAction) Do;
	BEGIN
		Services.DoLater(a.check, Services.immediately)
	END Do;

	(** Cleaner **)

	PROCEDURE (c: TrapCleaner) Cleanup;
	BEGIN
		path := frontPath;
		prevPath := NIL
	END Cleanup;

	PROCEDURE NewPathInfo(): PathInfo;
		VAR c: PathInfo;
	BEGIN
		IF cache = NIL THEN NEW(c)
		ELSE c := cache; cache := cache.prev
		END;
		RETURN c
	END NewPathInfo;
	
	PROCEDURE DisposePathInfo(c: PathInfo);
	BEGIN
		c.prev := cache; cache := c
	END DisposePathInfo;


	(** Controller **)

	PROCEDURE (c: Controller) Internalize- (VAR rd: Stores.Reader), EXTENSIBLE;
	(** pre: ~c.init **)
	(** post: c.init **)
		VAR thisVersion: INTEGER;
	BEGIN
		c.Internalize^(rd);
		rd.ReadVersion(minVersion, maxVersion, thisVersion)
	END Internalize;

	PROCEDURE (c: Controller) Externalize- (VAR wr: Stores.Writer), EXTENSIBLE;
	(** pre: c.init **)
	BEGIN
		c.Externalize^(wr);
		wr.WriteVersion(maxVersion)
	END Externalize;


	(** Forwarder **)

	PROCEDURE (f: Forwarder) Forward* (target: BOOLEAN; VAR msg: Message), NEW, ABSTRACT;
	PROCEDURE (f: Forwarder) Transfer* (VAR msg: TransferMessage), NEW, ABSTRACT;

	PROCEDURE Register* (f: Forwarder);
		VAR t: Forwarder;
	BEGIN
		ASSERT(f # NIL, 20);
		t := list; WHILE (t # NIL) & (t # f) DO t := t.next END;
		IF t = NIL THEN f.next := list; list := f END
	END Register;

	PROCEDURE Delete* (f: Forwarder);
		VAR t: Forwarder;
	BEGIN
		ASSERT(f # NIL, 20);
		IF f = list THEN
			list := list.next
		ELSE
			t := list; WHILE (t # NIL) & (t.next # f) DO t := t.next END;
			IF t # NIL THEN t.next := f.next END
		END;
		f.next := NIL
	END Delete;


	PROCEDURE ForwardVia* (target: BOOLEAN; VAR msg: Message);
		VAR t: Forwarder;
	BEGIN
		t := list; WHILE t # NIL DO t.Forward(target, msg); t := t.next END
	END ForwardVia;

	PROCEDURE SetCurrentPath* (target: BOOLEAN);
		VAR p: PathInfo;
	BEGIN
		IF prevPath = NIL THEN Kernel.PushTrapCleaner(cleaner) END;
		p := NewPathInfo(); p.prev := prevPath; prevPath := p; p.path := path;
		path := target
	END SetCurrentPath;
	
	PROCEDURE ResetCurrentPath*;
		VAR p: PathInfo;
	BEGIN
		IF prevPath # NIL THEN (* otherwise trap cleaner may have already removed prefPath objects *)
			p := prevPath; prevPath := p.prev; path := p.path;
			IF prevPath = NIL THEN Kernel.PopTrapCleaner(cleaner) END;
			DisposePathInfo(p)
		END
	END ResetCurrentPath;

	PROCEDURE Forward* (VAR msg: Message);
	BEGIN
		ForwardVia(path, msg)
	END Forward;

	PROCEDURE PollOps* (VAR msg: PollOpsMsg);
	BEGIN
		msg.type := "";
		msg.pasteType := "";
		msg.singleton := NIL;
		msg.selectable := FALSE;
		msg.valid := {};
		Forward(msg)
	END PollOps;

	PROCEDURE PollCursor* (x, y: INTEGER; modifiers: SET; OUT cursor: INTEGER);
		VAR msg: PollCursorMsg;
	BEGIN
		msg.x := x; msg.y := y; msg.cursor := Ports.arrowCursor; msg.modifiers := modifiers;
		Forward(msg);
		cursor := msg.cursor
	END PollCursor;

	PROCEDURE Transfer* (x, y: INTEGER; source: Views.Frame; sourceX, sourceY: INTEGER; VAR msg: TransferMessage);
		VAR t: Forwarder;
	BEGIN
		ASSERT(source # NIL, 20);
		msg.x := x; msg.y := y;
		msg.source := source; msg.sourceX := sourceX; msg.sourceY := sourceY;
		t := list; WHILE t # NIL DO t.Transfer(msg); t := t.next END
	END Transfer;

	PROCEDURE PollDrop* (x, y: INTEGER;
							source: Views.Frame; sourceX, sourceY: INTEGER;
							mark, show: BOOLEAN;
							type: Stores.TypeName;
							isSingle: BOOLEAN;
							w, h, rx, ry: INTEGER;
							OUT dest: Views.Frame; OUT destX, destY: INTEGER);
		VAR msg: PollDropMsg;
	BEGIN
		ASSERT(source # NIL, 20);
		msg.mark := mark; msg.show := show; msg.type := type; msg.isSingle := isSingle;
		msg.w := w; msg.h := h; msg.rx := rx; msg.ry := ry; msg.dest := NIL;
		Transfer(x, y, source, sourceX, sourceY, msg);
		dest := msg.dest; destX := msg.x; destY := msg.y
	END PollDrop;

	PROCEDURE Drop* (x, y: INTEGER; source: Views.Frame; sourceX, sourceY: INTEGER;
									view: Views.View; isSingle: BOOLEAN; w, h, rx, ry: INTEGER);
		VAR msg: DropMsg;
	BEGIN
		ASSERT(source # NIL, 20); ASSERT(view # NIL, 21);
		msg.view := view; msg.isSingle := isSingle;
		msg.w := w; msg.h := h; msg.rx := rx; msg.ry := ry;
		Transfer(x, y, source, sourceX, sourceY, msg)
	END Drop;

	PROCEDURE PasteView* (view: Views.View; w, h: INTEGER; clipboard: BOOLEAN);
		VAR msg: EditMsg;
	BEGIN
		ASSERT(view # NIL, 20);
		msg.op := paste; msg.isSingle := TRUE;
		msg.clipboard := clipboard;
		msg.view := view; msg.w := w; msg.h := h;
		Forward(msg)
	END PasteView;


	PROCEDURE FocusFrame* (): Views.Frame;
		VAR msg: PollFocusMsg;
	BEGIN
		msg.focus := NIL; Forward(msg); RETURN msg.focus
	END FocusFrame;

	PROCEDURE FocusView* (): Views.View;
		VAR focus: Views.Frame;
	BEGIN
		focus := FocusFrame();
		IF focus # NIL THEN RETURN focus.view ELSE RETURN NIL END
	END FocusView;

	PROCEDURE FocusModel* (): Models.Model;
		VAR focus: Views.Frame;
	BEGIN
		focus := FocusFrame();
		IF focus # NIL THEN RETURN focus.view.ThisModel() ELSE RETURN NIL END
	END FocusModel;


	PROCEDURE HandleCtrlMsgs (op: INTEGER; f, g: Views.Frame; VAR msg: Message; VAR mark, front, req: BOOLEAN);
	(* g = f.up OR g = NIL *)
		CONST pre = 0; translate = 1; backoff = 2; final = 3;
	BEGIN
		CASE op OF
		  pre:
			WITH msg: MarkMsg DO
				IF msg.show & (g # NIL) THEN mark := TRUE; front := g.front END
			| msg: RequestMessage DO
				msg.requestFocus := FALSE
			ELSE
			END
		| translate:
			WITH msg: CursorMessage DO
				msg.x := msg.x + f.gx - g.gx;
				msg.y := msg.y + f.gy - g.gy
			ELSE
			END
		| backoff:
			WITH msg: MarkMsg DO
				IF ~msg.show THEN mark := FALSE; front := FALSE END
			| msg: RequestMessage DO
				req := msg.requestFocus
			ELSE
			END
		| final:
			WITH msg: PollFocusMsg DO
				IF msg.focus = NIL THEN msg.focus := f END
			| msg: MarkMsg DO
				IF ~msg.show THEN mark := FALSE; front := FALSE END
			| msg: RequestMessage DO
				req := msg.requestFocus
			ELSE
			END
		END
	END HandleCtrlMsgs;


	PROCEDURE Init;
		VAR action: BalanceCheckAction; w: WaitAction;
	BEGIN
		Views.InitCtrl(HandleCtrlMsgs);
		NEW(cleaner);
		NEW(action); NEW(w); action.wait := w; w.check := action; Services.DoLater(action, Services.immediately);
	END Init;

BEGIN
	Init
END Controllers.
