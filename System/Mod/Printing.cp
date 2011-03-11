MODULE Printing;
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

	IMPORT Kernel, Fonts, Ports, Dates, Printers, Views, Dialog, Strings;
	
	CONST maxNrOfSegments = 16;

	TYPE
		PageInfo* = RECORD
			first*, from*, to*: INTEGER; (** current IN **)
				(** first, from, to: OUT, preset to (0, 0, 9999) **)
			alternate*: BOOLEAN;
			title*: Views.Title
		END;

		Banner* = RECORD
			font*: Fonts.Font;
			gap*: INTEGER;	(** OUT, prest to (0,0) **)
			left*, right*: ARRAY 128 OF CHAR	(** OUT, preset to "", "" **)
				(** anywhere in header or footer:
					&p - replaced by current page number as arabic numeral
					&r - replaced by current page number as roman numeral
					&R - replaced by current page number as capital roman numeral
					&a - replaced by current page number as alphanumeric character
					&A - replaced by current page number as capital alphanumeric character
					&d - replaced by printing date 
					&t - replaced by printing time
					&&- replaced by & character
					&; - specifies split point
					&f - filename without path/title
				**)
		END;

		Par* = POINTER TO LIMITED RECORD
			page*: PageInfo;
			header*, footer*: Banner;
			copies-: INTEGER
		END;

		Line = RECORD
			buf: ARRAY 256 OF CHAR;
			beg: ARRAY maxNrOfSegments OF BYTE;
			len: INTEGER
		END;

		Hook* = POINTER TO ABSTRACT RECORD (Kernel.Hook) END;

	VAR
		par*: Par;

		month: ARRAY 12 * 3 + 1 OF CHAR;
		printingHook: Hook;

	PROCEDURE (h: Hook) Print* (v: Views.View; par: Par), NEW, ABSTRACT;
	PROCEDURE (h: Hook) Current* (): INTEGER, NEW, ABSTRACT;

	PROCEDURE SetHook* (p: Hook);
	BEGIN
		printingHook := p
	END SetHook;

	PROCEDURE NewPar* (IN page: PageInfo; IN header, footer: Banner; copies: INTEGER): Par;
		VAR par: Par;
	BEGIN
		NEW(par);
		par.page := page;
		par.header := header;
		par.footer := footer;
		par.copies := copies;
		IF par.header.font = NIL THEN par.header.font := Fonts.dir.Default() END;
		IF par.footer.font = NIL THEN par.footer.font := Fonts.dir.Default() END;
		RETURN par
	END NewPar;

	PROCEDURE NewDefaultPar* (title: Views.Title): Par;
		VAR par: Par;
	BEGIN
		NEW(par);
		par.page.first := 1;
		par.page.from := 0;
		par.page.to := 9999;
		par.page.alternate := FALSE;
		par.copies := 1;
		par.header.gap := 0; par.header.left := ""; par.header.right := ""; par.header.font := Fonts.dir.Default();
		par.footer.gap := 0; par.footer.left := ""; par.footer.right := ""; par.header.font := Fonts.dir.Default();
		par.page.title := title;
		RETURN par
	END NewDefaultPar;

	PROCEDURE PrintView* (view: Views.View; p: Par);
	BEGIN
		ASSERT(view # NIL, 20); ASSERT(p # NIL, 21);
		ASSERT(par = NIL, 22); (* no recursive printing *)
		IF Printers.dir.Available() THEN
			ASSERT(p.page.first >= 0, 23);
			ASSERT(p.page.from >= 0, 24);
			ASSERT(p.page.to >= p.page.from, 25);
			ASSERT(printingHook # NIL, 100);
			printingHook.Print(view, p)
		ELSE Dialog.ShowMsg("#System:NoPrinterFound")
		END
	END PrintView;

	PROCEDURE GetDateAndTime (IN date: Dates.Date; IN time: Dates.Time; 
										VAR d, t: ARRAY OF CHAR);
		VAR i, j, k: INTEGER; s: ARRAY 8 OF CHAR;
	BEGIN
		Strings.IntToStringForm (date.day, Strings.decimal, 0, "0", FALSE, d);
		
		j := date.month * 3; i := j - 3; k := 0;
		WHILE i < j DO s[k] := month[i]; INC(k); INC(i) END; s[k] := 0X;
		d := d + "-" + s;
		
		Strings.IntToStringForm (date.year, Strings.decimal, 0, "0", FALSE, s);
		d := d + "-" + s;
		
		Strings.IntToStringForm (time.hour, Strings.decimal, 0, "0", FALSE, t);
		Strings.IntToStringForm (time.minute, Strings.decimal, 2, "0", FALSE, s);
		t := t + ":" + s;
	END GetDateAndTime;

	PROCEDURE Expand (s: ARRAY OF CHAR; IN date: Dates.Date; IN time: Dates.Time;
										IN title: Views.Title; pno: INTEGER; printing: BOOLEAN; VAR line: Line);
		VAR i, l: INTEGER; ch: CHAR; j: BYTE;
			p, d, t, r, rl: ARRAY 32 OF CHAR;
	BEGIN
		IF printing THEN 
			Strings.IntToStringForm (pno, Strings.decimal, 0, "0", FALSE, p);
			IF (0 < pno) & (pno < 4000) THEN 
				Strings.IntToStringForm(pno, Strings.roman, 0, " ", FALSE, r)
			ELSE
				r := p
			END;
		ELSE p := "#"; r := "#"
		END;
		
		GetDateAndTime(date, time, d, t);
		
		i := 0; ch := s[i]; line.len := 0; j := 0;
		WHILE ch # 0X DO
			IF ch = "&" THEN
				INC(i); ch := s[i];
				IF ch = "p" THEN
					l := 0; WHILE p[l] # 0X DO line.buf[j] := p[l]; INC(j); INC(l) END
				ELSIF ch = "r" THEN
					Strings.ToLower(r, rl);
					l := 0; WHILE rl[l] # 0X DO line.buf[j] := rl[l]; INC(j); INC(l) END
				ELSIF ch = "R" THEN
					l := 0; WHILE r[l] # 0X DO line.buf[j] := r[l]; INC(j); INC(l) END
				ELSIF (ch = "a") OR (ch = "A") THEN
					IF printing & (0 < pno) & (pno <= 26) THEN line.buf[j] := CHR(pno + ORD(ch) - 1); INC(j)
					ELSE l := 0; WHILE p[l] # 0X DO line.buf[j] := p[l]; INC(j); INC(l) END
					END
				ELSIF ch = "d" THEN
					l := 0; WHILE d[l] # 0X DO line.buf[j] := d[l]; INC(j); INC(l) END
				ELSIF ch = "t" THEN
					l := 0; WHILE t[l] # 0X DO line.buf[j] := t[l]; INC(j); INC(l) END
				ELSIF ch = "f" THEN
					l := 0; WHILE title[l] # 0X DO line.buf[j] := title[l]; INC(j); INC(l) END
				ELSIF ch = ";" THEN
					IF (line.len < maxNrOfSegments-1) THEN line.beg[line.len] := j; INC(line.len) 
					ELSE line.buf[j] := " "; INC(j)
					END
				ELSIF ch = "&" THEN
					line.buf[j] := "&"; INC(j)
				END;
				IF ch # 0X THEN INC(i); ch := s[i] END
			ELSE line.buf[j] := ch; INC(j); INC(i); ch := s[i]
			END
		END;
		line.buf[j] := 0X; line.beg[line.len] := j; INC(line.len)
	END Expand;

	PROCEDURE PrintLine (f: Views.Frame; font: Fonts.Font;
												x0, x1, y: INTEGER; VAR line: Line);
		VAR sp, dx, x: INTEGER; i, j, k: INTEGER; buf: ARRAY 128 OF CHAR;
	BEGIN
		sp := (x1 - x0 - font.StringWidth(line.buf));
		IF line.len = 1 THEN (* center *)
			f.DrawString(x0 + sp DIV 2, y, Ports.defaultColor, line.buf, font)
		ELSE
			IF sp > 0 THEN dx := sp DIV (line.len - 1) ELSE dx := 0 END;
			k := 0; j := 0; x := x0;
			WHILE k < line.len DO
				i := 0;
				WHILE j < line.beg[k] DO
					buf[i] := line.buf[j]; INC(i); INC(j)
				END;
				buf[i] := 0X;
				f.DrawString(x, y, Ports.defaultColor, buf, font);
				x := x + font.StringWidth(buf) + dx;
				INC(k)
			END
		END
	END PrintLine;

	PROCEDURE PrintBanner* (f: Views.Frame; IN p: PageInfo; IN b: Banner; 
			IN date: Dates.Date; IN time: Dates.Time; x0, x1, y: INTEGER);
		VAR line: Line; printing: BOOLEAN;
	BEGIN
		printing := par # NIL;
		IF printing THEN
			ASSERT(printingHook # NIL, 100);
			IF p.alternate & ~ODD(p.first + printingHook.Current()) THEN
				Expand(b.left, date, time, p.title, p.first + printingHook.Current(), printing, line)
			ELSE
				Expand(b.right, date, time, p.title, p.first + printingHook.Current(), printing, line)
			END
		ELSE
			Expand(b.right, date, time, p.title, 0, printing, line)
		END;
		PrintLine(f, b.font, x0, x1, y, line)
	END PrintBanner;

	PROCEDURE Current*(): INTEGER;
	BEGIN
		ASSERT(par # NIL, 21);
		ASSERT(printingHook # NIL, 100);
		RETURN printingHook.Current()
	END Current;

BEGIN
	month := "JanFebMarAprMayJunJulAugSepOctNovDec"
END Printing.
