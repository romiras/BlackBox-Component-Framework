MODULE StdClocks;
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
		Dates, Math, Domains := Stores, Ports, Stores, Models, Views, Services, Properties,
		TextModels;

	CONST
		minSize = 25 * Ports.point; niceSize = 42 * Ports.point;
		minVersion = 0; maxVersion = 0;

	TYPE
		StdView = POINTER TO RECORD (Views.View)
			time: Dates.Time
		END;

		TickAction = POINTER TO RECORD (Services.Action) END;

		Msg = RECORD (Models.Message)
			consumed: BOOLEAN;
			time: Dates.Time
		END;

	VAR
		clockTime: Dates.Time;
		action: TickAction;
		actionIsAlive: BOOLEAN;


	PROCEDURE Cos (r, g: INTEGER): INTEGER;
	BEGIN
		RETURN SHORT(ENTIER(r * Math.Cos(2 * Math.Pi() * g / 60) + 0.5))
	END Cos;

	PROCEDURE Sin (r, g: INTEGER): INTEGER;
	BEGIN
		RETURN SHORT(ENTIER(r * Math.Sin(2 * Math.Pi() * g / 60) + 0.5))
	END Sin;

	PROCEDURE (a: TickAction) Do;
		VAR msg: Msg; time: Dates.Time;
	BEGIN
		Dates.GetTime(time);
		IF clockTime.second = time.second THEN
			Services.DoLater(action, Services.Ticks() + Services.resolution DIV 2)
		ELSE
			clockTime := time;
			msg.consumed := FALSE;
			msg.time := time;
			Views.Omnicast(msg);
			IF msg.consumed THEN
				Services.DoLater(action, Services.Ticks() + Services.resolution DIV 2)
			ELSE
				actionIsAlive := FALSE
			END
		END
	END Do;


	(* View *)

	PROCEDURE DrawTick (f: Views.Frame; m, d0, d1, s, g: INTEGER; c: Ports.Color);
	BEGIN
		f.DrawLine(m + Sin(d0, g), m - Cos(d0, g), m + Sin(d1, g), m - Cos(d1, g), s, c)
	END DrawTick;


	PROCEDURE (v: StdView) Externalize (VAR wr: Stores.Writer);
	BEGIN
		v.Externalize^(wr);
		wr.WriteVersion(maxVersion);
		wr.WriteByte(9)
	END Externalize;

	PROCEDURE (v: StdView) Internalize (VAR rd: Stores.Reader);
		VAR thisVersion: INTEGER; format: BYTE;
	BEGIN
		v.Internalize^(rd);
		IF ~rd.cancelled THEN
			rd.ReadVersion(minVersion, maxVersion, thisVersion);
			IF ~rd.cancelled THEN
				rd.ReadByte(format);
				v.time.second := -1
			END
		END
	END Internalize;

	PROCEDURE (v: StdView) CopyFromSimpleView (source: Views.View);
	BEGIN
		WITH source: StdView DO
			v.time.second := -1
		END
	END CopyFromSimpleView;

	PROCEDURE (v: StdView) Restore (f: Views.Frame; l, t, r, b: INTEGER);
		VAR c: Models.Context; a: TextModels.Attributes; color: Ports.Color;
			time: Dates.Time;
			i, m, d, u, hs, hd1, ms, md1, ss, sd0, sd1,  w, h: INTEGER;
	BEGIN
		IF ~actionIsAlive THEN
			 actionIsAlive := TRUE; Services.DoLater(action, Services.now)
		END;
		IF v.time.second = -1 THEN Dates.GetTime(v.time) END;
		c := v.context; c.GetSize(w, h);
		WITH c: TextModels.Context DO a := c.Attr(); color := a.color
		ELSE color := Ports.defaultColor
		END;
		u := f.unit;
		d := h DIV u * u;
		IF ~ODD(d DIV u) THEN DEC(d, u) END;
		m := (h - u) DIV 2;
		IF d >= niceSize - 2 * Ports.point THEN
			hs := 3 * u; ms := 3 * u; ss := u;
			hd1 := m * 4 DIV 6; md1 := m * 5 DIV 6; sd0 := -(m DIV 6); sd1 := m - 4 * u;
			i := 0; WHILE i < 12 DO DrawTick(f, m, m * 11 DIV 12, m, u, i  * 5, color); INC(i) END
		ELSE
			hd1 := m * 2 DIV 4; hs := u; ms := u; ss := u;
			md1 := m * 3 DIV 4; sd0 := 0; sd1 := 3 * u
		END;
		time := v.time;
		f.DrawOval(0, 0, d, d, u, color);
		DrawTick(f, m, 0, m * 4 DIV 6, hs, time.hour MOD 12 * 5 + time.minute DIV 12, color);
		DrawTick(f, m, 0, md1, ms, time.minute, color);
		DrawTick(f, m, sd0, sd1, ss, time.second, color)
	END Restore;

	PROCEDURE (v: StdView) HandleModelMsg (VAR msg: Models.Message);
		VAR w, h: INTEGER;
	BEGIN
		WITH msg: Msg DO
			msg.consumed := TRUE;
			IF v.time.second # msg.time.second THEN	(* execute only once per view *)
				Views.Update(v, Views.keepFrames);
				v.time := msg.time
			END
		ELSE
		END
	END HandleModelMsg;

	PROCEDURE SizePref (v: StdView; VAR p: Properties.SizePref);
	BEGIN
		IF (p.w > Views.undefined) & (p.h > Views.undefined) THEN
			Properties.ProportionalConstraint(1, 1, p.fixedW, p.fixedH, p.w, p.h);
			IF p.w < minSize THEN p.w := minSize; p.h := minSize END
		ELSE
			p.w := niceSize; p.h := niceSize
		END
	END SizePref;

	PROCEDURE (v: StdView) HandlePropMsg (VAR msg: Properties.Message);
	BEGIN
		WITH msg: Properties.Preference DO
			WITH msg: Properties.SizePref DO
				SizePref(v, msg)
			ELSE
			END
		ELSE
		END
	END HandlePropMsg;


	(** allocation **)

	PROCEDURE New* (): Views.View;
		VAR v: StdView;
	BEGIN
		NEW(v); v.time.second := -1; RETURN v
	END New;

	PROCEDURE Deposit*;
	BEGIN
		Views.Deposit(New())
	END Deposit;


BEGIN
	clockTime.second := -1;
	NEW(action); actionIsAlive := FALSE
CLOSE
	IF actionIsAlive THEN Services.RemoveAction(action) END
END StdClocks.
