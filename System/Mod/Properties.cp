MODULE Properties;
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
	
	IMPORT SYSTEM, Kernel, Math, Services, Fonts, Stores, Views, Controllers, Dialog;

	CONST
		(** StdProp.known/valid **)
		color* = 0; typeface* = 1; size* = 2; style* = 3; weight* = 4;

		(** SizeProp.known/valid **)
		width* = 0; height* = 1;

		(** PollVerbsMsg limitation **)
		maxVerbs* = 16;

		(** PollPickMsg.mark, PollPick mark **)
		noMark* = FALSE; mark* = TRUE;
		(** PollPickMsg.show, PollPick show **)
		hide* = FALSE; show* = TRUE;


	TYPE
		Property* = POINTER TO ABSTRACT RECORD
			next-: Property;	(** property lists are sorted **)	(* by TD address *)
			known*, readOnly*: SET;	(** used for polling, ignored when setting properties **)
			valid*: SET
		END;

		StdProp* = POINTER TO RECORD (Property)
			color*: Dialog.Color;
			typeface*: Fonts.Typeface;
			size*: INTEGER;
			style*: RECORD val*, mask*: SET END;
			weight*: INTEGER
		END;

		SizeProp* = POINTER TO RECORD (Property)
			width*, height*: INTEGER
		END;


		(** property messages **)

		Message* = Views.PropMessage;

		PollMsg* = RECORD (Message)
			prop*: Property	(** preset to NIL **)
		END;

		SetMsg* = RECORD (Message)
			old*, prop*: Property
		END;


		(** preferences **)

		Preference* = ABSTRACT RECORD (Message) END;

		ResizePref* = RECORD (Preference)
			fixed*: BOOLEAN;	(** OUT, preset to FALSE **)
			horFitToPage*: BOOLEAN;	(** OUT, preset to FALSE **)
			verFitToPage*: BOOLEAN;	(** OUT, preset to FALSE **)
			horFitToWin*: BOOLEAN;	(** OUT, preset to FALSE **)
			verFitToWin*: BOOLEAN;	(** OUT, preset to FALSE **)
		END;

		SizePref* = RECORD (Preference)
			w*, h*: INTEGER;	(** OUT, preset to caller's preference **)
			fixedW*, fixedH*: BOOLEAN	(** IN **)
		END;

		BoundsPref* = RECORD (Preference)
			w*, h*: INTEGER	(** OUT, preset to (Views.undefined, Views.undefined) **)
		END;

		FocusPref* = RECORD (Preference)
			atLocation*: BOOLEAN;	(** IN **)
			x*, y*: INTEGER;	(** IN, valid iff atLocation **)
			hotFocus*, setFocus*: BOOLEAN	(** OUT, preset to (FALSE, FALSE) **)
		END;

		ControlPref* = RECORD (Preference)
			char*: CHAR;	(** IN **)
			focus*: Views.View;	(** IN **)
			getFocus*: BOOLEAN;	(** OUT, valid if (v # focus), preset to ((char = [l]tab) & "FocusPref.setFocus") **)
			accepts*: BOOLEAN	(** OUT, preset to ((v = focus) & (char # [l]tab)) **)
		END;
		
		TypePref* = RECORD (Preference)
			type*: Stores.TypeName;	(** IN **)
			view*: Views.View	(** OUT, preset to NIL **)
		END;
		

		(** verbs **)

		PollVerbMsg* = RECORD (Message)
			verb*: INTEGER;	(** IN **)
			label*: ARRAY 64 OF CHAR;	(** OUT, preset to "" **)
			disabled*, checked*: BOOLEAN	(** OUT, preset to FALSE, FALSE **)
		END;
		
		DoVerbMsg* = RECORD (Message)
			verb*: INTEGER;	(** IN **)
			frame*: Views.Frame	(** IN **)
		END;
		
		
		(** controller messages **)

		CollectMsg* = RECORD (Controllers.Message)
			poll*: PollMsg	(** OUT, preset to NIL **)
		END;

		EmitMsg* = RECORD (Controllers.RequestMessage)
			set*: SetMsg	(** IN **)
		END;


		PollPickMsg* = RECORD (Controllers.TransferMessage)
			mark*: BOOLEAN;	(** IN, request to mark pick target **)
			show*: BOOLEAN;	(** IN, if mark then show/hide target mark **)
			dest*: Views.Frame	(** OUT, preset to NIL, set if PickMsg is acceptable **)
		END;

		PickMsg* = RECORD (Controllers.TransferMessage)
			prop*: Property	(** set to picked properties by destination **)
		END;


	VAR era-: INTEGER;	(* estimator to cache standard properties of focus *)


	PROCEDURE ^ IntersectSelections* (a, aMask, b, bMask: SET; OUT c, cMask: SET; OUT equal: BOOLEAN);


	(** properties **)

	PROCEDURE (p: Property) IntersectWith* (q: Property; OUT equal: BOOLEAN), NEW, ABSTRACT;

	PROCEDURE (p: StdProp) IntersectWith* (q: Property; OUT equal: BOOLEAN);
		VAR valid: SET; c, m: SET; eq: BOOLEAN;
	BEGIN
		WITH q: StdProp DO
			valid := p.valid * q.valid; equal := TRUE;
			IF p.color.val # q.color.val THEN EXCL(valid, color) END;
			IF p.typeface # q.typeface THEN EXCL(valid, typeface) END;
			IF p.size # q.size THEN EXCL(valid, size) END;
			IntersectSelections(p.style.val, p.style.mask, q.style.val, q.style.mask, c, m, eq);
			IF m = {} THEN EXCL(valid, style)
			ELSIF (style IN valid) & ~eq THEN p.style.mask := m; equal := FALSE
			END;
			IF p.weight # q.weight THEN EXCL(valid, weight) END;
			IF p.valid # valid THEN p.valid := valid; equal := FALSE END
		END
	END IntersectWith;

	PROCEDURE (p: SizeProp) IntersectWith* (q: Property; OUT equal: BOOLEAN);
		VAR valid: SET;
	BEGIN
		WITH q: SizeProp DO
			valid := p.valid * q.valid; equal := TRUE;
			IF p.width # q.width THEN EXCL(valid, width) END;
			IF p.height # q.height THEN EXCL(valid, height) END;
			IF p.valid # valid THEN p.valid := valid; equal := FALSE END
		END
	END IntersectWith;


	(** property collection and emission **)

	PROCEDURE IncEra*;
	BEGIN
		INC(era)
	END IncEra;


	PROCEDURE CollectProp* (OUT prop: Property);
		VAR msg: CollectMsg;
	BEGIN
		msg.poll.prop := NIL;
		Controllers.Forward(msg);
		prop := msg.poll.prop
	END CollectProp;

	PROCEDURE CollectStdProp* (OUT prop: StdProp);
	(** post: prop # NIL, prop.style.val = prop.style.val * prop.style.mask **)
		VAR p: Property;
	BEGIN
		CollectProp(p);
		WHILE (p # NIL) & ~(p IS StdProp) DO p := p.next END;
		IF p # NIL THEN
			prop := p(StdProp); prop.next := NIL
		ELSE
			NEW(prop); prop.known := {}
		END;
		prop.valid := prop.valid * prop.known;
		prop.style.val := prop.style.val * prop.style.mask
	END CollectStdProp;

	PROCEDURE EmitProp* (old, prop: Property);
		VAR msg: EmitMsg;
	BEGIN
		IF prop # NIL THEN
			msg.set.old := old; msg.set.prop := prop;
			Controllers.Forward(msg)
		END
	END EmitProp;


	PROCEDURE PollPick* (x, y: INTEGER;
							source: Views.Frame; sourceX, sourceY: INTEGER;
							mark, show: BOOLEAN;
							OUT dest: Views.Frame; OUT destX, destY: INTEGER);
		VAR msg: PollPickMsg;
	BEGIN
		ASSERT(source # NIL, 20);
		msg.mark := mark; msg.show := show; msg.dest := NIL;
		Controllers.Transfer(x, y, source, sourceX, sourceY, msg);
		dest := msg.dest; destX := msg.x; destY := msg.y
	END PollPick;

	PROCEDURE Pick* (x, y: INTEGER; source: Views.Frame; sourceX, sourceY: INTEGER;
							OUT prop: Property);
		VAR msg: PickMsg;
	BEGIN
		ASSERT(source # NIL, 20);
		msg.prop := NIL;
		Controllers.Transfer(x, y, source, sourceX, sourceY, msg);
		prop := msg.prop
	END Pick;


	(** property list construction **)

	PROCEDURE Insert* (VAR list: Property; x: Property);
		VAR p, q: Property; ta: INTEGER;
	BEGIN
		ASSERT(x # NIL, 20); ASSERT(x.next = NIL, 21); ASSERT(x # list, 22);
		ASSERT(x.valid - x.known = {}, 23);
		IF list # NIL THEN
			ASSERT(list.valid - list.known = {}, 24);
			ASSERT(Services.TypeLevel(list) = 1, 25)
		END;
		ta := SYSTEM.TYP(x^);
		ASSERT(Services.TypeLevel(x) = 1, 26);
		p := list; q := NIL;
		WHILE (p # NIL) & (SYSTEM.TYP(p^) < ta) DO
			q := p; p := p.next
		END;
		IF (p # NIL) & (SYSTEM.TYP(p^) = ta) THEN x.next := p.next ELSE x.next := p END;
		IF q # NIL THEN q.next := x ELSE list := x END
	END Insert;

	PROCEDURE CopyOfList* (p: Property): Property;
		VAR q, r, s: Property; t: Kernel.Type;
	BEGIN
		q := NIL; s := NIL;
		WHILE p # NIL DO
			ASSERT(Services.TypeLevel(p) = 1, 20);
			t := Kernel.TypeOf(p); Kernel.NewObj(r, t); ASSERT(r # NIL, 23);
			SYSTEM.MOVE(p, r, t.size);
			r.next := NIL;
			IF q # NIL THEN q.next := r ELSE s := r END;
			q := r; p := p.next
		END;
		RETURN s
	END CopyOfList;

	PROCEDURE CopyOf* (p: Property): Property;
		VAR r: Property; t: Kernel.Type;
	BEGIN
		IF p # NIL THEN
			ASSERT(Services.TypeLevel(p) = 1, 20);
			t := Kernel.TypeOf(p); Kernel.NewObj(r, t); ASSERT(r # NIL, 23);
			SYSTEM.MOVE(p, r, t.size);
			r.next := NIL;
		END;
		RETURN r
	END CopyOf;

	PROCEDURE Merge* (VAR base, override: Property);
		VAR p, q, r, s: Property; tp, tr: INTEGER;
	BEGIN
		ASSERT((base # override) OR (base = NIL), 20);
		p := base; q := NIL; r := override; override := NIL;
		IF p # NIL THEN
			tp := SYSTEM.TYP(p^);
			ASSERT(Services.TypeLevel(p) = 1, 21)
		END;
		IF r # NIL THEN
			tr := SYSTEM.TYP(r^);
			ASSERT(Services.TypeLevel(r) = 1, 22)
		END;
		WHILE (p # NIL) & (r # NIL) DO
			ASSERT(p # r, 23);
			WHILE (p # NIL) & (tp < tr) DO
				q := p; p := p.next;
				IF p # NIL THEN tp := SYSTEM.TYP(p^) END
			END;
			IF p # NIL THEN
				IF tp = tr THEN
					s := p.next; p.next := NIL; p := s;
					IF p # NIL THEN tp := SYSTEM.TYP(p^) END
				ELSE 
				END;
				s := r.next;
				IF q # NIL THEN q.next := r ELSE base := r END;
				q := r; r.next := p; r := s;
				IF r # NIL THEN tr := SYSTEM.TYP(r^) END
			END
		END;
		IF r # NIL THEN
			IF q # NIL THEN q.next := r ELSE base := r END
		END
	END Merge;

	PROCEDURE Intersect* (VAR list: Property; x: Property; OUT equal: BOOLEAN);
		VAR l, p, q, r, s: Property; plen, rlen, ta: INTEGER; filtered: BOOLEAN;
	BEGIN
		ASSERT((x # list) OR (list = NIL), 20);
		IF list # NIL THEN ASSERT(Services.TypeLevel(list) = 1, 21) END;
		IF x # NIL THEN ASSERT(Services.TypeLevel(x) = 1, 22) END;
		p := list; s := NIL; list := NIL; l := NIL; plen := 0;
		r := x; rlen := 0; filtered := FALSE;
		WHILE (p # NIL) & (r # NIL) DO
			q := p.next; p.next := NIL; INC(plen);
			ta := SYSTEM.TYP(p^);
			WHILE (r # NIL) & (SYSTEM.TYP(r^) < ta) DO
				r := r.next; INC(rlen)
			END;
			IF (r # NIL) & (SYSTEM.TYP(r^) = ta) THEN
				ASSERT(r # p, 23);
				IF l # NIL THEN s.next := p ELSE l := p END;
				s := p;
				p.known := p.known + r.known;
				p.IntersectWith(r, equal);
				filtered := filtered OR ~equal OR (p.valid # r.valid);
				r := r.next; INC(rlen)
			END;
			p := q
		END;
		list := l;
		equal := (p = NIL) & (r = NIL) & (plen = rlen) & ~filtered
	END Intersect;


	(** support for IntersectWith methods **)

	PROCEDURE IntersectSelections* (a, aMask, b, bMask: SET; OUT c, cMask: SET; OUT equal: BOOLEAN);
	BEGIN
		cMask := aMask * bMask - (a / b);
		c := a * cMask;
		equal := (aMask = bMask) & (bMask = cMask)
	END IntersectSelections;


	(** standard preferences protocols **)

	PROCEDURE PreferredSize* (v: Views.View; minW, maxW, minH, maxH,  defW, defH: INTEGER;
												VAR w, h: INTEGER);
		VAR p: SizePref;
	BEGIN
		ASSERT(Views.undefined < minW, 20); ASSERT(minW < maxW, 21);
		ASSERT(Views.undefined < minH, 23); ASSERT(minH < maxH, 24);
		ASSERT(Views.undefined <= defW, 26);
		ASSERT(Views.undefined <= defH, 28);
		IF (w < Views.undefined) OR (w > maxW) THEN w := defW END;
		IF (h < Views.undefined) OR (h > maxH) THEN h := defH END;
		p.w := w; p.h := h; p.fixedW := FALSE; p.fixedH := FALSE;
		Views.HandlePropMsg(v, p); w := p.w; h := p.h;
		IF w = Views.undefined THEN w := defW END;
		IF h = Views.undefined THEN h := defH END;
		IF w < minW THEN w := minW ELSIF w > maxW THEN w := maxW END;
		IF h < minH THEN h := minH ELSIF h > maxH THEN h := maxH END
	END PreferredSize;


	(** common resizing constraints **)

	PROCEDURE ProportionalConstraint* (scaleW, scaleH: INTEGER; fixedW, fixedH: BOOLEAN; VAR w, h: INTEGER);
	(** pre: w > Views.undefined, h > Views.undefined **)
	(** post: (E s: s * scaleW = w, s * scaleH = h), |w * h - w' * h'| min! **)
		VAR area: REAL;
	BEGIN
		ASSERT(scaleW > Views.undefined, 22); ASSERT(scaleH > Views.undefined, 23);
		IF fixedH THEN
			ASSERT(~fixedW, 24);
			ASSERT(h > Views.undefined, 21);
			area := h; area := area * scaleW;
			w := SHORT(ENTIER(area / scaleH))
		ELSIF fixedW THEN
			ASSERT(w > Views.undefined, 20);
			area := w; area := area * scaleH;
			h := SHORT(ENTIER(area / scaleW))
		ELSE
			ASSERT(w > Views.undefined, 20); ASSERT(h > Views.undefined, 21);
			area := w; area := area * h;
			w := SHORT(ENTIER(Math.(*L*)Sqrt(area * scaleW / scaleH)));
			h := SHORT(ENTIER(Math.(*L*)Sqrt(area * scaleH / scaleW)))
		END
	END ProportionalConstraint;

	PROCEDURE GridConstraint* (gridX, gridY: INTEGER; VAR x, y: INTEGER);
		VAR dx, dy: INTEGER;
	BEGIN
		ASSERT(gridX > Views.undefined, 20);
		ASSERT(gridY > Views.undefined, 21);
		dx := x MOD gridX;
		IF dx < gridX DIV 2 THEN DEC(x, dx) ELSE INC(x, (-x) MOD gridX) END;
		dy := y MOD gridY;
		IF dy < gridY DIV 2 THEN DEC(y, dy) ELSE INC(y, (-y) MOD gridY) END
	END GridConstraint;
	
	PROCEDURE ThisType* (view: Views.View; type: Stores.TypeName): Views.View;
		VAR msg: TypePref;
	BEGIN
		msg.type := type; msg.view := NIL;
		Views.HandlePropMsg(view, msg);
		RETURN msg.view
	END ThisType;
	
END Properties.
