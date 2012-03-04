MODULE StdHeaders;
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

(* headers / footers support the following macros:

			&p - replaced by current page number as arabic numeral
			&r - replaced by current page number as roman numeral
			&R - replaced by current page number as capital roman numeral
			&a - replaced by current page number as alphanumeric character
			&A - replaced by current page number as capital alphanumeric character
			&d - replaced by printing date 
			&t - replaced by printing time
			&&- replaced by & character
			&; - specifies split point
			&f - filename with path/title

*)

	IMPORT
		Stores, Ports, Models, Views, Properties, Printing, TextModels, Fonts, Dialog,
		TextViews, Dates, Windows, Controllers, Containers;

	CONST
		minVersion = 0; maxVersion = 2;
		mm = Ports.mm; point = Ports.point;
		maxWidth = 10000 * mm;
		alternate* = 0; number* = 1; head* = 2; foot* = 3; showFoot* = 4;

	TYPE
		Banner* = RECORD
			left*, right*: ARRAY 128 OF CHAR;
			gap*: INTEGER
		END;
		
		NumberInfo* = RECORD
			new*: BOOLEAN;
			first*: INTEGER
		END;
		
		View = POINTER TO RECORD (Views.View)
			alternate: BOOLEAN;	(* alternate left/right *)
			number: NumberInfo;	(* new page number *)
			head, foot: Banner;
			font: Fonts.Font;
			showFoot: BOOLEAN;
		END;

		Prop* = POINTER TO RECORD (Properties.Property)
			alternate*, showFoot*: BOOLEAN;
			number*: NumberInfo;
			head*, foot*: Banner
		END;

		ChangeFontOp = POINTER TO RECORD (Stores.Operation)
			header: View;
			font: Fonts.Font
		END;

		ChangeAttrOp = POINTER TO RECORD (Stores.Operation)
			header: View;
			alternate, showFoot: BOOLEAN;
			number: NumberInfo;
			head, foot: Banner
		END;

	VAR
		dialog*: RECORD
			view: View;
			alternate*, showFoot*: BOOLEAN;
			number*: NumberInfo;
			head*, foot*: Banner;
		END;
		
	PROCEDURE (p: Prop) IntersectWith* (q: Properties.Property; OUT equal: BOOLEAN);
		VAR valid: SET;
		PROCEDURE Equal(IN b1, b2: Banner): BOOLEAN;
		BEGIN
			RETURN (b1.left = b2.left) & (b1.right = b2.right) & (b1.gap = b2.gap)
		END Equal;
	BEGIN
		WITH q: Prop DO
			valid := p.valid * q.valid; equal := TRUE;
			IF p.alternate # q.alternate THEN EXCL(valid, alternate) END;
			IF p.showFoot # q.showFoot THEN EXCL(valid, showFoot) END;
			IF (p.number.new # q.number.new) OR (p.number.first # q.number.first) THEN EXCL(valid, number) END;
			IF ~Equal(p.head, q.head) THEN EXCL(valid, head) END;
			IF ~Equal(p.foot, q.foot) THEN EXCL(valid, foot) END;
			IF p.valid # valid THEN p.valid := valid; equal := FALSE END
		END
	END IntersectWith;
		
	(* SetAttrOp *)

	PROCEDURE (op: ChangeFontOp) Do;
		VAR v: View; font: Fonts.Font; asc, dsc, w: INTEGER; c: Models.Context;
	BEGIN
		v := op.header;
		font := op.font; op.font := v.font; v.font := font;
		font.GetBounds(asc, dsc, w);
		c := v.context;
		c.SetSize(maxWidth, asc + dsc + 2*point);
		Views.Update(v, Views.keepFrames)
	END Do;

	PROCEDURE DoChangeFontOp (v: View; font: Fonts.Font);
		VAR op: ChangeFontOp;
	BEGIN
		IF v.font # font THEN
			NEW(op); op.header := v; op.font := font;
			Views.Do(v,   "#System:SetProp", op)
		END
	END DoChangeFontOp;

	PROCEDURE (op: ChangeAttrOp) Do;
		VAR v: View; alternate, showFoot: BOOLEAN; number: NumberInfo; head, foot: Banner;
	BEGIN
		v := op.header;
		alternate := op.alternate; showFoot := op.showFoot; number := op.number; head := op.head; foot := op.foot;
		op.alternate := v.alternate; op.showFoot := v.showFoot; op.number := v.number; op.head := v.head;
		op.foot := v.foot;
		v.alternate := alternate; v.showFoot := showFoot; v.number := number; v.head := head; v.foot := foot;
		Views.Update(v, Views.keepFrames)
	END Do;

	PROCEDURE DoChangeAttrOp (v: View; alternate, showFoot: BOOLEAN; number: NumberInfo;
													head, foot: Banner);
		VAR op: ChangeAttrOp;
	BEGIN
		NEW(op); op.header := v; op.alternate := alternate; op.showFoot := showFoot; 
		op.number := number; op.head := head; op.foot := foot;
		Views.Do(v,   "#Std:HeaderChange", op)
	END DoChangeAttrOp;

	PROCEDURE (v: View) CopyFromSimpleView (source: Views.View);
	BEGIN
		WITH source: View DO
			v.alternate := source.alternate;
			v.number.new := source.number.new; v.number.first := source.number.first;
			v.head := source.head;
			v.foot := source.foot;
			v.font := source.font;
			v.showFoot := source.showFoot
		END
	END CopyFromSimpleView;

	PROCEDURE (v: View) Externalize (VAR wr: Stores.Writer);
	BEGIN
		v.Externalize^(wr);
		wr.WriteVersion(maxVersion);
		wr.WriteString(v.head.left);
		wr.WriteString(v.head.right);
		wr.WriteInt(v.head.gap);
		wr.WriteString(v.foot.left);
		wr.WriteString(v.foot.right);
		wr.WriteInt(v.foot.gap);
		wr.WriteString(v.font.typeface);
		wr.WriteInt(v.font.size);
		wr.WriteSet(v.font.style);
		wr.WriteInt(v.font.weight);
		wr.WriteBool(v.alternate);
		wr.WriteBool(v.number.new);
		wr.WriteInt(v.number.first);
		wr.WriteBool(v.showFoot);
	END Externalize;
	
	PROCEDURE (v: View) Internalize (VAR rd: Stores.Reader);
		VAR version: INTEGER; typeface: Fonts.Typeface; size: INTEGER; style: SET; weight: INTEGER;

	BEGIN
		v.Internalize^(rd);
		IF ~rd.cancelled THEN
			rd.ReadVersion(minVersion, maxVersion, version);
			IF ~rd.cancelled THEN
				IF version = 0 THEN
					rd.ReadXString(v.head.left);
					rd.ReadXString(v.head.right); 
					v.head.gap := 5*mm;
					rd.ReadXString(v.foot.left);
					rd.ReadXString(v.foot.right);
					v.foot.gap := 5*mm;
					rd.ReadXString(typeface);
					rd.ReadXInt(size); 
					v.font := Fonts.dir.This(typeface, size * point, {}, Fonts.normal);
					rd.ReadXInt(v.number.first);
					rd.ReadBool(v.number.new);
					rd.ReadBool(v.alternate)
				ELSE
					rd.ReadString(v.head.left);
					rd.ReadString(v.head.right);
					rd.ReadInt(v.head.gap);
					rd.ReadString(v.foot.left);
					rd.ReadString(v.foot.right);
					rd.ReadInt(v.foot.gap);
					rd.ReadString(typeface);
					rd.ReadInt(size);
					rd.ReadSet(style);
					rd.ReadInt(weight);
					v.font := Fonts.dir.This(typeface, size, style, weight);
					rd.ReadBool(v.alternate);
					rd.ReadBool(v.number.new);
					rd.ReadInt(v.number.first);
					IF version = 2 THEN rd.ReadBool(v.showFoot) ELSE v.showFoot := FALSE END
				END
			END
		END
	END Internalize;

	PROCEDURE SetProp(v: View; msg: Properties.SetMsg);
		VAR p: Properties.Property;
			typeface: Fonts.Typeface; size: INTEGER; style: SET; weight: INTEGER;
			alt, sf: BOOLEAN; num: NumberInfo; h, f: Banner;
 	BEGIN
		p := msg.prop;
		WHILE p # NIL DO
			WITH p: Properties.StdProp DO
				IF Properties.typeface IN p.valid THEN typeface := p.typeface 
				ELSE typeface := v.font.typeface
				END;
				IF Properties.size IN p.valid THEN size := p.size 
				ELSE size := v.font.size
				END;
				IF Properties.style IN p.valid THEN style := p.style.val
				ELSE style := v.font.style
				END;
				IF Properties.weight IN p.valid THEN weight := p.weight 
				ELSE weight := v.font.weight
				END;
				DoChangeFontOp (v, Fonts.dir.This(typeface, size, style, weight) );
			| p: Prop DO
				IF alternate IN p.valid THEN alt := p.alternate ELSE alt := v.alternate END;
				IF showFoot IN p.valid THEN sf := p.showFoot ELSE sf := v.showFoot END;
				IF number IN p.valid THEN num := p.number ELSE num := v.number END;
				IF head IN p.valid THEN h := p.head ELSE h := v.head END;
				IF foot IN p.valid THEN f := p.foot ELSE f := v.foot END;
				DoChangeAttrOp(v, alt, sf, num, h, f)
			ELSE
			END;
			p := p.next
		END
	END SetProp;
	
	PROCEDURE PollProp(v: View; VAR msg: Properties.PollMsg);
		VAR sp: Properties.StdProp; p: Prop;
	BEGIN
		NEW(sp);
		sp.known := {Properties.size, Properties.typeface, Properties.style, Properties.weight};
		sp.valid := sp.known;
		sp.size := v.font.size; sp.typeface := v.font.typeface; 
		sp.style.val := v.font.style; sp.style.mask := {Fonts.italic, Fonts.underline, Fonts.strikeout};
		sp.weight := v.font.weight;
		Properties.Insert(msg.prop, sp);
		NEW(p);
		p.known := {alternate, number, head, foot, showFoot}; p.valid := p.known;
		p.head := v.head; p.foot := v.foot;
		p.alternate := v.alternate;
		p.showFoot := v.showFoot;
		p.number := v.number;
		Properties.Insert(msg.prop, p)
	END PollProp;
	
	PROCEDURE PageMsg(v: View; msg: TextViews.PageMsg);
	BEGIN
		IF Printing.par # NIL THEN
			Dialog.MapString(v.head.left, Printing.par.header.left);
			Dialog.MapString(v.head.right, Printing.par.header.right);
			Dialog.MapString(v.foot.left, Printing.par.footer.left);
			Dialog.MapString(v.foot.right, Printing.par.footer.right);
			Printing.par.header.font := v.font;
			Printing.par.footer.font := v.font;
			Printing.par.page.alternate := v.alternate;
			IF v.number.new THEN
				Printing.par.page.first := v.number.first - msg.current
			END;
			Printing.par.header.gap := 5*Ports.mm;
			Printing.par.footer.gap := 5*Ports.mm
		END
	END PageMsg;

	PROCEDURE (v: View) Restore (f: Views.Frame; l, t, r, b: INTEGER);
		VAR d, w, h: INTEGER; (*line: Line; *)asc, dsc, x0, x1, y: INTEGER;
			win: Windows.Window; title: Views.Title; dec: BOOLEAN;
			pw, ph: INTEGER;
			date: Dates.Date; time: Dates.Time; pageInfo: Printing.PageInfo; banner: Printing.Banner;
	BEGIN
		IF Views.IsPrinterFrame(f) THEN (* am drucken *) END;

		v.font.GetBounds(asc, dsc, w);

		win := Windows.dir.First();
		WHILE (win # NIL) & (win.doc.Domain() # v.Domain()) DO win := Windows.dir.Next(win) END;
		IF win = NIL THEN title := "(" + Dialog.appName + ")"
		ELSE win.GetTitle(title)
		END;
		d := f.dot; 
		v.context.GetSize(w, h);
		win.doc.PollPage(pw, ph, l, t, r, b, dec);
		w := r - l;

		f.DrawRect(0, 0, w, h, Ports.fill, Ports.grey25);
		f.DrawRect(0, 0, w, h, 0, Ports.black);
		
		x0 := d; x1 := w-2*d; y := asc + d;

		Dates.GetDate(date);
		Dates.GetTime(time);
		pageInfo.alternate := FALSE;
		pageInfo.title := title;
		banner.font := v.font;		
		IF v.showFoot THEN
			banner.gap := v.foot.gap;
			Dialog.MapString(v.foot.left, banner.left); Dialog.MapString(v.foot.right, banner.right)
		ELSE
			banner.gap := v.head.gap;
			Dialog.MapString(v.head.left, banner.left); Dialog.MapString(v.head.right, banner.right)
		END;
		Printing.PrintBanner(f, pageInfo, banner, date, time, x0, x1, y)
	END Restore;

	PROCEDURE (v: View) HandlePropMsg (VAR msg: Properties.Message);
		VAR asc, dsc, w: INTEGER;
	BEGIN
		WITH msg: Properties.SizePref DO
			msg.w := maxWidth; 
			IF msg.h = Views.undefined THEN
				v.font.GetBounds(asc, dsc, w);
				msg.h := asc + dsc + 2*point
			 END
		| msg: Properties.ResizePref DO
			msg.fixed := TRUE
		| msg: TextModels.Pref DO
			msg.opts := {TextModels.hideable}
		| msg: Properties.PollMsg DO
			PollProp(v, msg)
		| msg: Properties.SetMsg DO
			SetProp(v, msg)
		| msg: TextViews.PageMsg DO
			PageMsg(v, msg)
		ELSE
		END
	END HandlePropMsg;
	
	PROCEDURE (v: View) HandleCtrlMsg (f: Views.Frame; VAR msg: Controllers.Message;
																VAR focus: Views.View);
	BEGIN
		WITH msg: Properties.EmitMsg DO Views.HandlePropMsg(v, msg.set)
		| msg: Properties.CollectMsg DO Views.HandlePropMsg(v, msg.poll)
		ELSE
		END
	END HandleCtrlMsg;
	
	PROCEDURE New*(p: Prop; f: Fonts.Font): Views.View;
		VAR v: View;
	BEGIN
		NEW(v);
		v.head := p.head;
		v.foot := p.foot;
		v.number := p.number;
		v.alternate := p.alternate;
		v.font := f; 
		v.showFoot := FALSE;
		RETURN v;	
	END New;

	PROCEDURE Deposit*;
		VAR v: View;
	BEGIN
		NEW(v);
		v.head.left := ""; v.head.right :=  "&d&;&p"; v.head.gap := 5*mm;
		v.foot.left := ""; v.foot.right := ""; v.foot.gap := 5*mm;
		v.font := Fonts.dir.Default();
		v.number.first := 1; v.number.new := FALSE; v.alternate := FALSE; v.showFoot := FALSE;
		Views.Deposit(v)
	END Deposit;
	
	(* property dialog *)
	
	PROCEDURE InitDialog*;
		VAR  p: Properties.Property;
	BEGIN
		Properties.CollectProp(p);
		WHILE p # NIL DO
			WITH p: Properties.StdProp DO
			| p: Prop DO
				dialog.alternate := p.alternate; dialog.showFoot := p.showFoot;
				dialog.number := p.number;
				dialog.head := p.head; dialog.head.gap := dialog.head.gap DIV point;
				dialog.foot := p.foot; dialog.foot.gap := dialog.foot.gap DIV point;
				Dialog.Update(dialog)
			ELSE
			END;
			p := p.next
		END
	END InitDialog;
	
	PROCEDURE Set*;
		VAR p: Prop;
	BEGIN
		NEW(p); p.valid := {alternate, number, head, foot, showFoot};
		p.alternate := dialog.alternate; p.showFoot := dialog.showFoot;
		p.number := dialog.number;
		p.head := dialog.head; p.head.gap := p.head.gap * point;
		p.foot := dialog.foot; p.foot.gap := p.foot.gap * point;
		Properties.EmitProp(NIL, p)
	END Set;
	
	PROCEDURE HeaderGuard* (VAR par: Dialog.Par);
		VAR v: Views.View;
	BEGIN
		v := Containers.FocusSingleton();
		IF (v # NIL) & (v IS View) THEN
			par.disabled := FALSE;
			IF (dialog.view = NIL) OR (dialog.view # v) THEN
				dialog.view := v(View);
				InitDialog
			END
		ELSE
			par.disabled := TRUE;
			dialog.view := NIL
		END
	END HeaderGuard;
	
	PROCEDURE AlternateGuard* (VAR par: Dialog.Par);
	BEGIN
		HeaderGuard(par);
		IF ~par.disabled THEN par.disabled := ~ dialog.alternate END
	END AlternateGuard;
	
	PROCEDURE NewNumberGuard* (VAR par: Dialog.Par);
	BEGIN
		HeaderGuard(par);
		IF ~par.disabled THEN par.disabled := ~ dialog.number.new END
	END NewNumberGuard;
	
END StdHeaders.
