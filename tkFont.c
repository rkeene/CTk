/* 
 * tkFont.c (CTk) --
 *
 *	CTk does not have fonts, but Tk's utility procedures
 *	for measuring and displaying text are provided.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 * Copyright (c) 1994-1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
 */

#include "tkPort.h"
#include "tkInt.h"

/*
 * Characters used when displaying control sequences.
 */

static char hexChars[] = "0123456789abcdefxtnvr\\";

/*
 * The following table maps some control characters to sequences
 * like '\n' rather than '\x10'.  A zero entry in the table means
 * no such mapping exists, and the table only maps characters
 * less than 0x10.
 */

static char mapChars[] = {
    0, 0, 0, 0, 0, 0, 0,
    'a', 'b', 't', 'n', 'v', 'f', 'r',
    0
};

/*
 * Width of tabs, in characters.
 */

#define TAB_WIDTH	8


/*
 *--------------------------------------------------------------
 *
 * TkMeasureChars --
 *
 *	Measure the number of characters from a string that
 *	will fit in a given horizontal span.  The measurement
 *	is done under the assumption that TkDisplayChars will
 *	be used to actually display the characters.
 *
 * Results:
 *	The return value is the number of characters from source
 *	that fit in the span given by startX and maxX.  *nextXPtr
 *	is filled in with the x-coordinate at which the first
 *	character that didn't fit would be drawn, if it were to
 *	be drawn.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
TkMeasureChars(source, maxChars, startX, maxX, tabOrigin, flags, nextXPtr)
    char *source;		/* Characters to be displayed.  Need not
				 * be NULL-terminated. */
    int maxChars;		/* Maximum # of characters to consider from
				 * source. */
    int startX;			/* X-postion at which first character will
				 * be drawn. */
    int maxX;			/* Don't consider any character that would
				 * cross this x-position. */
    int tabOrigin;		/* X-location that serves as "origin" for
				 * tab stops. */
    int flags;			/* Various flag bits OR-ed together.
				 * TK_WHOLE_WORDS means stop on a word boundary
				 * (just before a space character) if
				 * possible.  TK_AT_LEAST_ONE means always
				 * return a value of at least one, even
				 * if the character doesn't fit. 
				 * TK_PARTIAL_OK means it's OK to display only
				 * a part of the last character in the line.
				 * TK_NEWLINES_NOT_SPECIAL means that newlines
				 * are treated just like other control chars:
				 * they don't terminate the line.
				 * TK_IGNORE_TABS means give all tabs zero
				 * width. */
    int *nextXPtr;		/* Return x-position of terminating
				 * character here. */
{
    register char *p;		/* Current character. */
    register int c;
    char *term;			/* Pointer to most recent character that
				 * may legally be a terminating character. */
    int termX;			/* X-position just after term. */
    int curX;			/* X-position corresponding to p. */
    int newX;			/* X-position corresponding to p+1. */
    int rem;

    /*
     * Scan the input string one character at a time, until a character
     * is found that crosses maxX.
     */

    newX = curX = startX;
    termX = 0;		/* Not needed, but eliminates compiler warning. */
    term = source;
    for (p = source, c = *p & 0xff; maxChars > 0; p++, maxChars--) {
	if (isprint(UCHAR(c))) {
	    newX++;
	} else if (c == '\t') {
	    if (!(flags & TK_IGNORE_TABS)) {
		newX += TAB_WIDTH;
		rem = (newX - tabOrigin) % TAB_WIDTH;
		if (rem < 0) {
		    rem += TAB_WIDTH;
		}
		newX -= rem;
	    }
	} else {
	    if (c == '\n' && !(flags & TK_NEWLINES_NOT_SPECIAL)) {
		break;
	    }
	    if (c >= 0 && c < sizeof(mapChars) && mapChars[c]) {
		newX += 2;
	    } else {
		newX += 4;
	    }
	}

	if (newX > maxX) {
	    break;
	}
	if (maxChars > 1) {
	    c = p[1] & 0xff;
	} else {
	    c = 0;
	}
	if (isspace(UCHAR(c)) || (c == 0)) {
	    term = p+1;
	    termX = newX;
	}
	curX = newX;
    }

    /*
     * P points to the first character that doesn't fit in the desired
     * span.  Use the flags to figure out what to return.
     */

    if ((flags & TK_PARTIAL_OK) && (curX < maxX)) {
	curX = newX;
	p++;
    }
    if ((flags & TK_AT_LEAST_ONE) && (term == source) && (maxChars > 0)
	     && !isspace(UCHAR(*term))) {
	term = p;
	termX = curX;
	if (term == source) {
	    term++;
	    termX = newX;
	}
    } else if ((maxChars == 0) || !(flags & TK_WHOLE_WORDS)) {
	term = p;
	termX = curX;
    }
    *nextXPtr = termX;
    return term-source;
}

/*
 *--------------------------------------------------------------
 *
 * CtkDisplayChars --
 *
 *	Draw a string of characters on the screen, converting
 *	tabs to the right number of spaces and control characters
 *	to sequences of the form "\xhh" where hh are two hex
 *	digits.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets drawn on the screen.
 *
 *--------------------------------------------------------------
 */

void
TkDisplayChars(win, style, string, numChars, x, y, tabOrigin, flags)
    Tk_Window win;		/* Window in which to draw. */
    Ctk_Style style;		/* Display characters using this style. */
    char *string;		/* Characters to be displayed. */
    int numChars;		/* Number of characters to display from
				 * string. */
    int x, y;			/* Coordinates at which to draw string. */
    int tabOrigin;		/* X-location that serves as "origin" for
				 * tab stops. */
    int flags;			/* Flags to control display.  Only
				 * TK_NEWLINES_NOT_SPECIAL and TK_IGNORE_TABS
				 * are supported right now.  See
				 * TkMeasureChars for information about it. */
{
    register char *p;		/* Current character being scanned. */
    register int c;
    char *start;		/* First character waiting to be displayed. */
    int startX;			/* X-coordinate corresponding to start. */
    int curX;			/* X-coordinate corresponding to p. */
    char replace[10];
    int rem;

    /*
     * Scan the string one character at a time.  Display control
     * characters immediately, but delay displaying normal characters
     * in order to pass many characters to the server all together.
     */

    startX = curX = x;
    start = string;
    for (p = string; numChars > 0; numChars--, p++) {
	c = *p & 0xff;
	if (isprint(UCHAR(c))) {
	    curX++;
	    continue;
	}
	if (p != start) {
	    Ctk_DrawString(win, startX, y, style, start, p - start);
	    startX = curX;
	}
	if (c == '\t') {
	    if (!(flags & TK_IGNORE_TABS)) {
		curX += TAB_WIDTH;
		rem = (curX - tabOrigin) % TAB_WIDTH;
		if (rem < 0) {
		    rem += TAB_WIDTH;
		}
		curX -= rem;
		Ctk_FillRect(win, startX, y, startX+1, y+1, style, ' ');
	    }
	} else {
	    if (c == '\n' && !(flags & TK_NEWLINES_NOT_SPECIAL)) {
		y++;
		curX = x;
	    } else {
	    	if (c >= 0 && c < sizeof(mapChars) && mapChars[c]) {
		    replace[0] = '\\';
		    replace[1] = mapChars[c];
		    Ctk_DrawString(win, startX, y, style, replace, 2);
		    curX += 2;
	    	} else {
		    replace[0] = '\\';
		    replace[1] = 'x';
		    replace[2] = hexChars[(c >> 4) & 0xf];
		    replace[3] = hexChars[c & 0xf];
		    Ctk_DrawString(win, startX, y, style, replace, 4);
		    curX += 4;
		}
	    }
	}
	startX = curX;
	start = p+1;
    }

    /*
     * At the very end, there may be one last batch of normal characters
     * to display.
     */

    if (p != start) {
	Ctk_DrawString(win, startX, y, style, start, p - start);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkComputeTextGeometry --
 *
 *	This procedure computes the amount of screen space needed to
 *	display a multi-line string of text.
 *
 * Results:
 *	There is no return value.  The dimensions of the screen area
 *	needed to display the text are returned in *widthPtr, and *heightPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkComputeTextGeometry(string, numChars, wrapLength, widthPtr, heightPtr)
    char *string;		/* String whose dimensions are to be
				 * computed. */
    int numChars;		/* Number of characters to consider from
				 * string. */
    int wrapLength;		/* Longest permissible line length, in
				 * pixels.  <= 0 means no automatic wrapping:
				 * just let lines get as long as needed. */
    int *widthPtr;		/* Store width of string here. */
    int *heightPtr;		/* Store height of string here. */
{
    int thisWidth, maxWidth, numLines;
    char *p;

    if (wrapLength <= 0) {
	wrapLength = INT_MAX;
    }
    maxWidth = 0;
    for (numLines = 1, p = string; (p - string) < numChars; numLines++) {
	p += TkMeasureChars(p, numChars - (p - string), 0,
		wrapLength, 0, TK_WHOLE_WORDS|TK_AT_LEAST_ONE, &thisWidth);
	if (thisWidth > maxWidth) {
	    maxWidth = thisWidth;
	}
	if (*p == 0) {
	    break;
	}

	/*
	 * If the character that didn't fit in this line was a white
	 * space character then skip it.
	 */

	if (isspace(UCHAR(*p))) {
	    p++;
	}
    }
    *widthPtr = maxWidth;
    *heightPtr = numLines;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDisplayText --
 *
 *	Display a text string on one or more lines.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The text given by "string" gets displayed at the given location
 *	in the given window with the given style etc.
 *
 *----------------------------------------------------------------------
 */

void
TkDisplayText(win, style, string, numChars, x, y,
	length, justify, underline)
    Tk_Window win;		/* Window in which to draw the text. */
    Ctk_Style style;		/* Style in which to draw characters
    				 * (except for underlined char). */
    char *string;		/* String to display;  may contain embedded
				 * newlines. */
    int numChars;		/* Number of characters to use from string. */
    int x, y;			/* Pixel coordinates within drawable of
				 * upper left corner of display area. */
    int length;			/* Line length in pixels;  used to compute
				 * word wrap points and also for
				 * justification.   Must be > 0. */
    Tk_Justify justify;		/* How to justify lines. */
    int underline;		/* Index of character to underline, or < 0
				 * for no underlining. */
{
    char *p;
    int charsThisLine, lengthThisLine, xThisLine;
    int underlineOffset;

    /*
     * Work through the string one line at a time.  Display each line
     * in four steps:
     *     1. Compute the line's length.
     *     2. Figure out where to display the line for justification.
     *     3. Display the line.
     *     4. Underline one character if needed.
     */

    for (p = string; numChars > 0; ) {
	charsThisLine = TkMeasureChars(p, numChars, 0, length, 0,
		TK_WHOLE_WORDS|TK_AT_LEAST_ONE, &lengthThisLine);
	if (justify == TK_JUSTIFY_LEFT) {
	    xThisLine = x;
	} else if (justify == TK_JUSTIFY_CENTER) {
	    xThisLine = x + (length - lengthThisLine)/2;
	} else {
	    xThisLine = x + (length - lengthThisLine);
	}
	TkDisplayChars(win, style, p, charsThisLine,
		xThisLine, y, xThisLine, 0);
	if ((underline >= 0) && (underline < charsThisLine)) {
	    (void) TkMeasureChars(p, underline, 0, length, 0,
		    TK_WHOLE_WORDS|TK_AT_LEAST_ONE, &underlineOffset);
	    TkDisplayChars(win, CTK_UNDERLINE_STYLE, p+underline, 1,
		    xThisLine+underlineOffset, y, xThisLine, 0);
	}
	p += charsThisLine;
	numChars -= charsThisLine;
	underline -= charsThisLine;
	y++;

	/*
	 * If the character that didn't fit was a space character, skip it.
	 */

	if (isspace(UCHAR(*p))) {
	    p++;
	    numChars--;
	    underline--;
	}
    }
}
