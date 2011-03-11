MODULE Ports;
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
	
	IMPORT Fonts;

	CONST
		(** colors **)
		black* = 00000000H; white* = 00FFFFFFH;
		grey6* = 00F0F0F0H; grey12* = 00E0E0E0H; grey25* = 00C0C0C0H;
		grey50* = 00808080H; grey75* = 00404040H;
		red* = 000000FFH; green* = 0000FF00H; blue* = 00FF0000H;
		defaultColor* = 01000000H;

		(** measures **)
		mm* = 36000;
		point* = 12700;
		inch* = 914400;

		(** size parameter for the DrawRect, DrawOval, DrawLine, DrawPath, and MarkRect procedures **)
		fill* = -1;

		(** path parameter for DrawPath **)
		openPoly* = 0; closedPoly* = 1; openBezier* = 2; closedBezier* = 3;

		(** modes for MarkRect **)
		invert* = 0; hilite* = 1; dim25* = 2; dim50* = 3; dim75* = 4;

		hide* = FALSE; show* = TRUE;

		(** cursors **)
		arrowCursor* = 0;
		textCursor* = 1; graphicsCursor* = 2; tableCursor* = 3; bitmapCursor* = 4; refCursor* = 5;
		
		(** RestoreRect **)
		keepBuffer* = FALSE; disposeBuffer* = TRUE;


		(** PageMode **)
		printer* = TRUE; screen* = FALSE;

		
	TYPE
		Color* = INTEGER;

		Point* = RECORD
			x*, y*: INTEGER
		END;

		Port* = POINTER TO ABSTRACT RECORD
			unit-: INTEGER;
			printerMode: BOOLEAN;
		END;

		Rider* = POINTER TO ABSTRACT RECORD END;

		Frame* = POINTER TO ABSTRACT RECORD
			unit-, dot-: INTEGER;	(** inv: dot = point - point MOD unit **)
			rider-: Rider;
			gx-, gy-: INTEGER
		END;


	VAR
		background*: Color;
		dialogBackground*: Color;


	(** Port **)

	PROCEDURE (p: Port) Init* (unit: INTEGER; printerMode: BOOLEAN), NEW;
	BEGIN
		ASSERT((p.unit = 0) OR (p.unit = unit), 20); ASSERT(unit > 0, 21);
		ASSERT((p.unit = 0) OR (p.printerMode = printerMode), 22);
		p.unit := unit;
		p.printerMode := printerMode;
	END Init;

	PROCEDURE (p: Port) GetSize* (OUT w, h: INTEGER), NEW, ABSTRACT;
	PROCEDURE (p: Port) SetSize* (w, h: INTEGER), NEW, ABSTRACT;
	PROCEDURE (p: Port) NewRider* (): Rider, NEW, ABSTRACT;
	PROCEDURE (p: Port) OpenBuffer* (l, t, r, b: INTEGER), NEW, ABSTRACT;
	PROCEDURE (p: Port) CloseBuffer* (), NEW, ABSTRACT;


	(** Rider **)

	PROCEDURE (rd: Rider) SetRect* (l, t, r, b: INTEGER), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) GetRect* (OUT l, t, r, b: INTEGER), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) Base* (): Port, NEW, ABSTRACT;
	PROCEDURE (rd: Rider) Move* (dx, dy: INTEGER), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) SaveRect* (l, t, r, b: INTEGER; VAR res: INTEGER), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) RestoreRect* (l, t, r, b: INTEGER; dispose: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) DrawRect* (l, t, r, b, s: INTEGER; col: Color), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) DrawOval* (l, t, r, b, s: INTEGER; col: Color), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) DrawLine* (x0, y0, x1, y1, s: INTEGER; col: Color), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) DrawPath* (IN p: ARRAY OF Point; n, s: INTEGER; col: Color;
															path: INTEGER), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) MarkRect* (l, t, r, b, s, mode: INTEGER; show: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) Scroll* (dx, dy: INTEGER), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) SetCursor* (cursor: INTEGER), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) Input* (OUT x, y: INTEGER; OUT modifiers: SET;
														OUT isDown: BOOLEAN), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) DrawString* (x, y: INTEGER; col: Color; IN s: ARRAY OF CHAR;
																font: Fonts.Font), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) CharIndex* (x, pos: INTEGER; IN s: ARRAY OF CHAR;
																font: Fonts.Font): INTEGER, NEW, ABSTRACT;
	PROCEDURE (rd: Rider) CharPos* (x, index: INTEGER; IN s: ARRAY OF CHAR;
																font: Fonts.Font): INTEGER, NEW, ABSTRACT;
	PROCEDURE (rd: Rider) DrawSString* (x, y: INTEGER; col: Color; IN s: ARRAY OF SHORTCHAR;
																font: Fonts.Font), NEW, ABSTRACT;
	PROCEDURE (rd: Rider) SCharIndex* (x, pos: INTEGER; IN s: ARRAY OF SHORTCHAR;
																font: Fonts.Font): INTEGER, NEW, ABSTRACT;
	PROCEDURE (rd: Rider) SCharPos* (x, index: INTEGER; IN s: ARRAY OF SHORTCHAR;
																font: Fonts.Font): INTEGER, NEW, ABSTRACT;
	
	
	(** Frame **)

	PROCEDURE (f: Frame) ConnectTo* (p: Port), NEW, EXTENSIBLE;
		VAR w, h: INTEGER;
	BEGIN
		IF p # NIL THEN
			f.rider := p.NewRider(); f.unit := p.unit;
			p.GetSize(w, h);
			f.dot := point - point MOD f.unit;
		ELSE
			f.rider := NIL; f.unit := 0
		END
	END ConnectTo;

	PROCEDURE (f: Frame) SetOffset* (gx, gy: INTEGER), NEW, EXTENSIBLE;
		VAR u: INTEGER;
	BEGIN
		u := f.unit;
		IF ((gx - f.gx) MOD u = 0) & ((gy - f.gy) MOD u = 0) THEN
			f.rider.Move((gx - f.gx) DIV u, (gy - f.gy) DIV u)
		END;
		f.gx := gx; f.gy := gy
	END SetOffset;
	
	PROCEDURE (f: Frame) SaveRect* (l, t, r, b: INTEGER; VAR res: INTEGER), NEW;
		VAR u: INTEGER;
	BEGIN
		ASSERT((l <= r) & (t <= b), 20);
		u := f.unit;
		l := (f.gx + l) DIV u; t := (f.gy + t) DIV u;
		r := (f.gx + r) DIV u; b := (f.gy + b) DIV u;
		f.rider.SaveRect(l, t, r, b, res);
	END SaveRect;
	
	PROCEDURE (f: Frame) RestoreRect* (l, t, r, b: INTEGER; dispose: BOOLEAN), NEW;
		VAR u: INTEGER;
	BEGIN
		ASSERT((l <= r) & (t <= b), 20);
		u := f.unit;
		l := (f.gx + l) DIV u; t := (f.gy + t) DIV u;
		r := (f.gx + r) DIV u; b := (f.gy + b) DIV u;
		f.rider.RestoreRect(l, t, r, b, dispose);
	END RestoreRect;
	
	PROCEDURE (f: Frame) DrawRect* (l, t, r, b, s: INTEGER; col: Color), NEW;
		VAR u: INTEGER;
	BEGIN
		ASSERT((l <= r) & (t <= b), 20); ASSERT(s >= fill, 21);
		u := f.unit;
		l := (f.gx + l) DIV u; t := (f.gy + t) DIV u;
		r := (f.gx + r) DIV u; b := (f.gy + b) DIV u;
		s := s DIV u;
		f.rider.DrawRect(l, t, r, b, s, col)
	END DrawRect;

	PROCEDURE (f: Frame) DrawOval* (l, t, r, b, s: INTEGER; col: Color), NEW;
		VAR u: INTEGER;
	BEGIN
		ASSERT((l <= r) & (t <= b), 20); ASSERT(s >= fill, 21);
		u := f.unit;
		l := (f.gx + l) DIV u; t := (f.gy + t) DIV u;
		r := (f.gx + r) DIV u; b := (f.gy + b) DIV u;
		s := s DIV u;
		f.rider.DrawOval(l, t, r, b, s, col)
	END DrawOval;

	PROCEDURE (f: Frame) DrawLine* (x0, y0, x1, y1, s: INTEGER; col: Color), NEW;
		VAR u: INTEGER;
	BEGIN
		ASSERT(s >= fill, 20);
		u := f.unit;
		x0 := (f.gx + x0) DIV u; y0 := (f.gy + y0) DIV u;
		x1 := (f.gx + x1) DIV u; y1 := (f.gy + y1) DIV u;
		s := s DIV u;
		f.rider.DrawLine(x0, y0, x1, y1, s, col)
	END DrawLine;

	PROCEDURE (f: Frame) DrawPath* (IN p: ARRAY OF Point; n, s: INTEGER; col: Color; path: INTEGER), NEW;

		PROCEDURE Draw(p: ARRAY OF Point);
			VAR i, u: INTEGER;
		BEGIN
			u := f.unit; s := s DIV u;
			i := 0;
			WHILE i # n DO
				p[i].x := (f.gx + p[i].x) DIV u; p[i].y := (f.gy + p[i].y) DIV u;
				INC(i)
			END;
			f.rider.DrawPath(p, n, s, col, path)
		END Draw;

	BEGIN
		ASSERT(n >= 0, 20); ASSERT(n <= LEN(p), 21);
		ASSERT((s # fill) OR (path = closedPoly) OR (path = closedBezier), 22);
		ASSERT(s >= fill, 23);
		Draw(p)
	END DrawPath;

	PROCEDURE (f: Frame) MarkRect* (l, t, r, b, s: INTEGER; mode: INTEGER; show: BOOLEAN), NEW;
		VAR u: INTEGER;
	BEGIN
		(* ASSERT((l <= r) & (t <= b), 20); *) ASSERT(s >= fill, 21);
		u := f.unit;
		l := (f.gx + l) DIV u; t := (f.gy + t) DIV u;
		r := (f.gx + r) DIV u; b := (f.gy + b) DIV u;
		s := s DIV u;
		f.rider.MarkRect(l, t, r, b, s, mode, show)
	END MarkRect;

	PROCEDURE (f: Frame) Scroll* (dx, dy: INTEGER), NEW;
		VAR u: INTEGER;
	BEGIN
		u := f.unit;
		ASSERT(dx MOD u = 0, 20); ASSERT(dy MOD u = 0, 20);
		f.rider.Scroll(dx DIV u, dy DIV u)
	END Scroll;

	PROCEDURE (f: Frame) SetCursor* (cursor: INTEGER), NEW;
	BEGIN
		f.rider.SetCursor(cursor)
	END SetCursor;

	PROCEDURE (f: Frame) Input* (OUT x, y: INTEGER; OUT modifiers: SET; OUT isDown: BOOLEAN), NEW;
		VAR u: INTEGER;
	BEGIN
		f.rider.Input(x, y, modifiers, isDown);
		u := f.unit;
		x := x * u - f.gx; y := y * u - f.gy
	END Input;

	PROCEDURE (f: Frame) DrawString* (x, y: INTEGER; col: Color; IN s: ARRAY OF CHAR;
															font: Fonts.Font), NEW;
		VAR u: INTEGER;
	BEGIN
		u := f.unit;
		x := (f.gx + x) DIV u; y := (f.gy + y) DIV u;
		f.rider.DrawString(x, y, col, s, font)
	END DrawString;

	PROCEDURE (f: Frame) CharIndex* (x, pos: INTEGER; IN s: ARRAY OF CHAR;
															font: Fonts.Font): INTEGER, NEW;
		VAR u: INTEGER;
	BEGIN
		u := f.unit;
		x := (f.gx + x) DIV u; pos := (f.gx + pos) DIV u;
		RETURN f.rider.CharIndex(x, pos, s, font)
	END CharIndex;

	PROCEDURE (f: Frame) CharPos* (x, index: INTEGER; IN s: ARRAY OF CHAR;
															font: Fonts.Font): INTEGER, NEW;
		VAR u: INTEGER;
	BEGIN
		u := f.unit;
		x := (f.gx + x) DIV u;
		RETURN f.rider.CharPos(x, index, s, font) * u - f.gx
	END CharPos;

	PROCEDURE (f: Frame) DrawSString* (x, y: INTEGER; col: Color; IN s: ARRAY OF SHORTCHAR;
																font: Fonts.Font), NEW;
		VAR u: INTEGER;
	BEGIN
		u := f.unit;
		x := (f.gx + x) DIV u; y := (f.gy + y) DIV u;
		f.rider.DrawSString(x, y, col, s, font)
	END DrawSString;

	PROCEDURE (f: Frame) SCharIndex* (x, pos: INTEGER; IN s: ARRAY OF SHORTCHAR;
																font: Fonts.Font): INTEGER, NEW;
		VAR u: INTEGER;
	BEGIN
		u := f.unit;
		x := (f.gx + x) DIV u; pos := (f.gx + pos) DIV u;
		RETURN f.rider.SCharIndex(x, pos, s, font)
	END SCharIndex;

	PROCEDURE (f: Frame) SCharPos* (x, index: INTEGER; IN s: ARRAY OF SHORTCHAR;
															font: Fonts.Font): INTEGER, NEW;
		VAR u: INTEGER;
	BEGIN
		u := f.unit;
		x := (f.gx + x) DIV u;
		RETURN f.rider.SCharPos(x, index, s, font) * u - f.gx
	END SCharPos;

	PROCEDURE RGBColor* (red, green, blue: INTEGER): Color;
	BEGIN
		ASSERT((red >= 0) & (red < 256), 20);
		ASSERT((green >= 0) & (green < 256), 21);
		ASSERT((blue >= 0) & (blue < 256), 22);
		RETURN (blue * 65536) + (green * 256) + red
	END RGBColor;

	PROCEDURE IsPrinterPort*(p: Port): BOOLEAN;
	BEGIN
		RETURN p.printerMode
	END IsPrinterPort;

BEGIN
	background := white; dialogBackground := white
END Ports.
