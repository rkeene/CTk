/* 
 * ctkDisplay.c (CTk) --
 *
 *	CTK display functions (hides all curses functions).
 *
 * Copyright (c) 1994-1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
 */

#include "tkPort.h"
#include "tkInt.h"
#include <sys/times.h>
#ifdef HAVE_CURSES_H
#  include <curses.h>
#elif defined(HAVE_CURSES_CURSES_H)
#  include <curses/curses.h>
#elif defined(HAVE_CURSES_NCURSES_H)
#  include <curses/ncurses.h>
#elif defined(HAVE_NCURSES_NCURSES_H)
#  include <ncurses/ncurses.h>
#endif
#ifdef CLK_TCK
#   define MS_PER_CLOCK	(1000.0/CLK_TCK)
#elif defined HZ
#   define MS_PER_CLOCK	(1000.0/HZ)
#else
    /*
     * If all else fails, assume 60 clock ticks per second -
     * hope that is okay!
     */
#   define MS_PER_CLOCK	(1000.0/60)
#endif

/*
 * Definitions for weak curses implementations.
 */

#ifndef ACS_ULCORNER
/*
 * This curses does not define the alternate character set constants.
 * Define them locally.
 */
#   define ACS_ULCORNER    '+'
#   define ACS_LLCORNER    '+'
#   define ACS_URCORNER    '+'
#   define ACS_LRCORNER    '+'
#   define ACS_HLINE       '-'
#   define ACS_VLINE       '|'
#   define ACS_PLUS        '+'
#endif /* ACS_ULCORNER */

#ifndef HAVE_CURS_SET
/*
 * Don't have curs_set() function - ignore it.
 *
 * The cursor gets pretty annoying, but haven't found any other
 * way to turn it off.
 */
#   define curs_set(mode)	((void) 0)
#endif

#ifndef A_STANDOUT
    typedef int chtype;
#   define attrset(attr)	((attr) ? standout() : standend())
#   define A_STANDOUT	1
#   define A_INVIS	0
#   define A_NORMAL	0
#   define A_UNDERLINE	0
#   define A_REVERSE	0
#   define A_DIM	0
#   define A_BOLD	0
#   define A_DIM	0
#   define A_BOLD	0
#   define A_REVERSE	0
#endif

#ifdef HAVE_SET_TERM
#   define SetDisplay(dispPtr) \
	if (curDispPtr != (dispPtr)) \
	set_term((SCREEN *) (curDispPtr = (dispPtr))->display)
#else
#   define SetDisplay(dispPtr)		((void) 0)
#   define newterm(type, outPtr, inPtr)	initscr()
#endif

#ifndef HAVE_KEYPAD
#   define keypad(win, flag)		((void) 0)
#endif

#ifndef HAVE_BEEP
#   define beep()			((void) 0)
#endif

/*
 * Macros for the most often used curses operations.  This
 * will hopefully help if someone wants to convert to a different
 * terminal I/O library (like DOS BIOS?).
 */
#define Move(x,y)		move(y,x)
#define PutChar(ch)		addch(ch)
#define SetStyle(style)		attrset(styleAttributes[style])


/*
 * TextInfo - client data passed to DrawTextSpan() when drawing text.
 */
typedef struct {
    char *str;		/* String being drawn. */
    int left;		/* Absolute X coordinate to draw first character
    			 * of string at. */
} TextInfo;

/*
 * Curses attributes that correspond to CTk styles.
 * This definition must be modified in concert with
 * the Ctk_Style definition in tk.h
 */
chtype styleAttributes[] = {
    A_NORMAL, A_NORMAL, A_UNDERLINE, A_REVERSE, A_DIM, A_BOLD,
    A_DIM, A_BOLD, A_STANDOUT, A_REVERSE
};

/*
 * Current display for input/output.  Changed by SetDisplay().
 */

TkDisplay *curDispPtr = NULL;

/*
 * The data structure and hash table below are used to map from
 * raw keycodes (curses) to keysyms and modifier masks.
 */

typedef struct {
    int code;			/* Curses key code. */
    KeySym sym;			/* Key sym. */
    int modMask;		/* Modifiers. */
} KeyCodeInfo;

static KeyCodeInfo keyCodeArray[] = {
#include "keyCodes.h"
    {0, 0, 0}
};
static Tcl_HashTable keyCodeTable;	/* Hashed form of above structure. */

/*
 * Forward declarations of static functions.
 */

static void		TermFileProc _ANSI_ARGS_((ClientData clientData,
			    int mask));
static void		RefreshDisplay _ANSI_ARGS_((TkDisplay *dispPtr));
static void		DrawTextSpan _ANSI_ARGS_((int left, int right, int y,
			    ClientData data));
static void		FillSpan _ANSI_ARGS_((int left, int right, int y,
			    ClientData data));



/*
 *--------------------------------------------------------------
 *
 * CtkDisplayInit --
 *
 *	Opens a connection to terminal with specified name,
 *	and stores terminal information in the display
 *	structure pointed to by `dispPtr'.
 *
 * Results:
 *	Standard TCL result.
 *
 * Side effects:
 *	The screen is cleared, and all sorts of I/O options
 *	are set appropriately for a full-screen application.
 *
 *--------------------------------------------------------------
 */

int
CtkDisplayInit(interp, dispPtr, termName)
    Tcl_Interp *interp;
    TkDisplay *dispPtr;
    char *termName;
{
    char *type;
    int length;
    FILE *outPtr;

    int fd;			/* For the return value of Tcl_GetChannelHandle */
    /* The Tcl_File is no longer needed, since Channels now subsume their work */

    static int initialized = 0;

    if (!initialized) {
	register KeyCodeInfo *codePtr;
	register Tcl_HashEntry *hPtr;
	int dummy;

	initialized = 1;
	Tcl_InitHashTable(&keyCodeTable, TCL_ONE_WORD_KEYS);
	for (codePtr = keyCodeArray; codePtr->code != 0; codePtr++) {
	    hPtr = Tcl_CreateHashEntry(&keyCodeTable,
	    	    (char *) codePtr->code, &dummy);
	    Tcl_SetHashValue(hPtr, (ClientData) codePtr);
	}
    }

    type = strchr(termName, ':');
    if (type == NULL) {
    	length = strlen(termName);
    	type = getenv("CTK_TERM");
	if (!type) type = getenv("TERM");
    } else {
	length = type - termName;
	type++;
    }
    dispPtr->type = (char *) ckalloc((unsigned) strlen(type) + 1);
    strcpy(dispPtr->type, type);

    dispPtr->name = (char *) ckalloc((unsigned) (length+1));
    strncpy(dispPtr->name, termName, length);
    dispPtr->name[length] = '\0';

    if (strcmp(dispPtr->name, "tty") == 0) {
	dispPtr->chan = Tcl_GetStdChannel(TCL_STDIN);
    } else {
#ifdef HAVE_SET_TERM
	dispPtr->chan = HAVE_SET_TERM ? Tcl_OpenFileChannel(interp, dispPtr->name, 
							    "r+", 0) : NULL;
#else
	dispPtr->chan = NULL;
#endif
        if (dispPtr->chan == NULL) {
	    Tcl_AppendResult(interp, "couldn't connect to device \"",
		    dispPtr->name, "\"", (char *) NULL);
	    goto error;
	}
    }
    if ( Tcl_GetChannelHandle(dispPtr->chan, TCL_READABLE, &(dispPtr->fd) ) != TCL_OK )
    {
    	Tcl_AppendResult(interp, "couldn't get device handle for device \"",
    		dispPtr->name, "\"", (char *) NULL);
        goto error;
    }
    
    if (!isatty(dispPtr->fd)) {
	Tcl_AppendResult(interp, "display device \"", dispPtr->name,
		"\" is not a tty", (char *) NULL);
	goto error;
    }
    if (dispPtr->fd == 0) {
	dispPtr->inPtr = stdin;
	outPtr = stdout;
    } else {
	dispPtr->inPtr = fdopen(dispPtr->fd, "r+");
	outPtr = dispPtr->inPtr;
    }

    dispPtr->display =
	    (ClientData) newterm(dispPtr->type, outPtr, dispPtr->inPtr);
    SetDisplay(dispPtr);
    raw();
    nonl();
    noecho();
    keypad(stdscr, TRUE);

    Tcl_CreateChannelHandler(dispPtr->chan, TCL_READABLE,
    	    TermFileProc, (ClientData) dispPtr);
    return TCL_OK;

error:
    ckfree(dispPtr->name);
    dispPtr->name = NULL;
    ckfree(dispPtr->type);
    dispPtr->type = NULL;
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * CtkDisplayEnd --
 *
 *	Ends CTk's use of terminal.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The terminal is restored to line mode.
 *
 *--------------------------------------------------------------
 */

void
CtkDisplayEnd(dispPtr)
    TkDisplay *dispPtr;
{
    SetDisplay(dispPtr);
    curs_set(1);
    endwin();

    Tcl_DeleteChannelHandler(dispPtr->chan,
		TermFileProc, (ClientData) dispPtr);
    if (dispPtr->inPtr != stdin) {
    	fclose(dispPtr->inPtr);
    }
    ckfree(dispPtr->name);
    ckfree(dispPtr->type);
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_DisplayFlush --
 *
 *	Flushes all output to the specified display.  If dispPtr
 *	is NULL then output to all connected displays is flushed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The terminal display is updated.
 *
 *--------------------------------------------------------------
 */

void
Ctk_DisplayFlush(dispPtr)
    TkDisplay *dispPtr;
{
    if (dispPtr) {
	RefreshDisplay(dispPtr);
    } else {
    	for (dispPtr = tkDisplayList;
    		dispPtr != (TkDisplay*) NULL;
    		dispPtr = dispPtr->nextPtr) {
	    RefreshDisplay(dispPtr);
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_DisplayRedraw --
 *
 *	Force a complete redraw of the specified display.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The entire terminal display is redrawn.
 *
 *--------------------------------------------------------------
 */

void
Ctk_DisplayRedraw(dispPtr)
    TkDisplay *dispPtr;
{
    SetDisplay(dispPtr);
    clearok(stdscr, 1);
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_SetCursor --
 *
 *	Postions display cursor in window at specified (local)
 *	coordinates.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Modifies window's display structure.
 *--------------------------------------------------------------
 */

void
Ctk_SetCursor(winPtr, x, y)
    TkWindow *winPtr;
    int x, y;
{
    TkDisplay *dispPtr = Tk_Display(winPtr);
    dispPtr->cursorPtr = winPtr;
    dispPtr->cursorX = x;
    dispPtr->cursorY = y;
}

static void
RefreshDisplay(dispPtr)
    TkDisplay *dispPtr;
{
    TkWindow *winPtr = dispPtr->cursorPtr;
    int x, y;
    int visible = 0;

    SetDisplay(dispPtr);
    if (CtkIsDisplayed(winPtr)) {
	/*
	 * Convert to absolute screen coordinates
	 */
	x = dispPtr->cursorX + winPtr->absLeft;
	y = dispPtr->cursorY + winPtr->absTop;
	if (y >= winPtr->maskRect.top
		&& y < winPtr->maskRect.bottom
		&& x >= winPtr->maskRect.left
		&& x < winPtr->maskRect.right
		&& CtkPointInRegion(x, y, winPtr->clipRgn) ) {
	    Move(x, y);
	    visible = 1;
	}
    }
    curs_set(visible);
    refresh();
}

/*
 *--------------------------------------------------------------
 *
 * CtkDisplayBell --
 *
 *	Flushes all output to the terminal (otherwise drawing
 *	may be buffered).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The terminal display is updated.
 *
 *--------------------------------------------------------------
 */

void
CtkDisplayBell(dispPtr)
    TkDisplay *dispPtr;
{
    SetDisplay(dispPtr);
    beep();
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_DisplayWidth --
 * Ctk_DisplayHeight --
 *
 *	Get geometry of terminal.
 *
 * Results:
 *	Size (width/height respectively) of terminal.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

int
Ctk_DisplayWidth(dispPtr)
    TkDisplay *dispPtr;
{
    SetDisplay(dispPtr);
    return COLS;
}

int
Ctk_DisplayHeight(dispPtr)
    TkDisplay *dispPtr;
{
    SetDisplay(dispPtr);
    return LINES;
}

/*
 *--------------------------------------------------------------
 *
 * TermFileProc --
 *
 *	File handler for a terminal.
 *
 * Results:
 *	Returns TK_FILE_HANDLED if any events were processed.
 *	Otherwise returns TCL_READABLE.
 *
 * Side effects:
 *	Dispatches events (invoking event handlers).
 *
 *--------------------------------------------------------------
 */

static void
TermFileProc(clientData, mask)
    ClientData clientData;
    int mask;
{
    TkDisplay *dispPtr = (TkDisplay *) clientData;
    Ctk_Event event;
    struct tms timesBuf;
    Tcl_HashEntry *hPtr;
    KeyCodeInfo *codePtr;
    int key;

    if ((mask & TCL_READABLE) == TCL_READABLE) {
	SetDisplay(dispPtr);

	key = getch();
	hPtr = Tcl_FindHashEntry(&keyCodeTable, (char *) key);
	if (hPtr) {
	    codePtr = (KeyCodeInfo *) Tcl_GetHashValue(hPtr);
	    event.u.key.sym = codePtr->sym;
	    event.u.key.state = codePtr->modMask;
	} else {
	    event.u.key.sym = key;
	    event.u.key.state = 0;
	}
	event.type = CTK_KEY_EVENT;
	event.window = dispPtr->focusPtr;
	event.u.key.time = (unsigned long) (times(&timesBuf)*MS_PER_CLOCK);
	Tk_HandleEvent(&event);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_DrawCharacter --
 *
 *	Display a single character in a view.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Character is output to the terminal.
 *
 *--------------------------------------------------------------
 */

void
Ctk_DrawCharacter(winPtr, x, y, style, character)
    TkWindow *winPtr;		/* Window to draw into. */
    int x,y;			/* Position, relative to view, to
    				 * start draw at. */
    Ctk_Style style;		/* Style to draw character in. */
    int character;		/* Character to draw. */
{
    if (!CtkIsDisplayed(winPtr)) {
	return;
    }

    /*
     * Convert to absolute screen coordinates
     */
    y += winPtr->absTop;
    x += winPtr->absLeft;

    if (y >= winPtr->clipRect.top
    	    && y < winPtr->clipRect.bottom
    	    && x >= winPtr->clipRect.left
    	    && x < winPtr->clipRect.right
	    && CtkPointInRegion(x, y, winPtr->clipRgn) ) {
	SetDisplay(winPtr->dispPtr);
	SetStyle(style);
	Move(x, y);
	PutChar(character);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_DrawString --
 *
 *	Display `length' characters from `str' into `winPtr'
 *	at position (`x',`y') in specified `style'.  If `length'
 *	is -1 then draw till a null character is reached.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters are output to the terminal.
 *
 *--------------------------------------------------------------
 */

void
Ctk_DrawString(winPtr, x, y, style, str, length)
    TkWindow *winPtr;		/* Window to draw into. */
    int x,y;			/* Position, relative to view, to
    				 * start drawing. */
    Ctk_Style style;		/* Style to draw characters in. */
    char *str;			/* Points to characters to be drawn. */
    int length;			/* Number of characters from str
    				 * to draw, or -1 to draw till NULL
    				 * termination. */
{
    int strLeft, strRight;
    TextInfo text_info;

    if (!CtkIsDisplayed(winPtr)) {
	return;
    }

    /*
     * Convert to absolute screen coordinates
     */
    y += winPtr->absTop;
    if (y < winPtr->clipRect.top || y > winPtr->clipRect.bottom) {
	return;
    }
    x += winPtr->absLeft;

    if (length == -1) {
	length = strlen(str);
    }
    strLeft = x;
    strRight = x+length;
    CtkIntersectSpans(&strLeft, &strRight,
	    winPtr->clipRect.left, winPtr->clipRect.right);
    if (CtkSpanIsEmpty(strLeft, strRight))  return;

    SetDisplay(winPtr->dispPtr);
    SetStyle(style);
    text_info.str = str;
    text_info.left = x;
    CtkForEachIntersectingSpan(DrawTextSpan, (ClientData) &text_info,
	    strLeft, strRight, y, winPtr->clipRgn);
}

/*
 *--------------------------------------------------------------
 *
 * DrawTextSpan --
 *
 *	Called by ForEachSpan() or ForEachIntersectingSpan()
 *	to draw a segment of a string.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters are output to the terminal.
 *
 *--------------------------------------------------------------
 */

static void
DrawTextSpan(left, right, y, data)
    int left;			/* X coordinate to start drawing. */
    int right;			/* X coordinate to stop drawing (this
    				 * position is not drawn into). */
    int y;			/* Y coordinate to draw at. */
    ClientData data;		/* Points at TextInfo structure. */
{
    char *charPtr = ((TextInfo*) data)->str + left - ((TextInfo*) data)->left;
    int x;

    Move(left, y);
    for (x=left; x < right; x++) {
	PutChar(UCHAR(*charPtr++));
    }
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_ClearWindow --
 *
 *	Fill view with its background (as defined by
 *	winPtr->fillStyle and winPtr->fillChar).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters are output to the terminal.
 *
 *--------------------------------------------------------------
 */

void
Ctk_ClearWindow(winPtr)
    TkWindow * winPtr;	/* Window to clear. */
{
    int left = winPtr->clipRect.left;
    int right = winPtr->clipRect.right;
    int y;

    if (winPtr->fillStyle == CTK_INVISIBLE_STYLE || (!CtkIsDisplayed(winPtr))
    	    || CtkSpanIsEmpty(left, right)) {
	return;
    }

    SetDisplay(winPtr->dispPtr);
    SetStyle(winPtr->fillStyle);
    for (y=winPtr->clipRect.top; y < winPtr->clipRect.bottom; y++) {
	CtkForEachIntersectingSpan(
	    FillSpan, (ClientData) winPtr->fillChar,
	    left, right, y,
	    winPtr->clipRgn);
    }
}

/*
 *--------------------------------------------------------------
 *
 * CtkFillRegion --
 *
 *	Fills in a region with the specified character and style.
 *	Region is in absolute screen coordinates.  No clipping is
 *	performed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters are output to the terminal.
 *
 *--------------------------------------------------------------
 */

void
CtkFillRegion(dispPtr, rgnPtr, fillStyle, fillChar)
    TkDisplay *dispPtr;
    CtkRegion *rgnPtr;
    Ctk_Style fillStyle;
    int fillChar;
{
    SetDisplay(dispPtr);
    SetStyle(fillStyle);
    CtkForEachSpan(FillSpan, (ClientData) fillChar, rgnPtr);
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_FillRect --
 *
 *	Draw a rectangle filled with the specified character
 *	and style in `winPtr' at relative coordinates (x1,y1)
 *	to (x2-1,y2-1).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters are output to the terminal.
 *
 *--------------------------------------------------------------
 */

void
Ctk_FillRect(winPtr, x1, y1, x2, y2, fillStyle, fillChar)
    TkWindow *winPtr;
    int x1;
    int y1;
    int x2;
    int y2;
    Ctk_Style fillStyle;
    int fillChar;
{
    Ctk_Rect rect;
    int y;

    if (!CtkIsDisplayed(winPtr)) {
	return;
    }
    CtkSetRect(&rect, x1, y1, x2, y2);
    CtkMoveRect(&rect, winPtr->absLeft, winPtr->absTop);
    CtkIntersectRects(&rect, &winPtr->clipRect);
    if ( CtkSpanIsEmpty(rect.left, rect.right) ) {
	return;
    }
    SetDisplay(winPtr->dispPtr);
    SetStyle(fillStyle);
    for (y=rect.top; y < rect.bottom; y++)
    {
	CtkForEachIntersectingSpan( FillSpan, (ClientData) fillChar,
		rect.left, rect.right, y, winPtr->clipRgn);
    }
}

/*
 *--------------------------------------------------------------
 *
 * FillSpan --
 *
 *	Called by ForEachSpan() or ForEachIntersectingSpan()
 *	to fill a span with the same character.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters are output to the terminal.
 *
 *--------------------------------------------------------------
 */

static void
FillSpan(left, right, y, data)
    int left;			/* X coordinate to start filling. */
    int right;			/* X coordinate to stop filling (this
    				 * position is not draw into). */
    int y;			/* Y coordinate to draw at. */
    ClientData data;		/* Character to draw. */
{
    int x;

    Move(left, y);
    for (x=left; x < right; x++) {
	PutChar((int) data);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_DrawRect --
 *
 *	Draw outline of rectangle with line drawing characters
 *	and the specified style in `winPtr' at relative
 *	coordinates (x1,y1) to (x2,y2).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Characters are output to the terminal.
 *
 *--------------------------------------------------------------
 */

void
Ctk_DrawRect(winPtr, x1, y1, x2, y2, lineStyle)
    TkWindow *winPtr;
    int x1;
    int y1;
    int x2;
    int y2;
    Ctk_Style lineStyle;
{
    Ctk_Rect *clipRectPtr = &winPtr->clipRect;
    int left;
    int right;
    int top;
    int bottom;
    int y;

    if (!CtkIsDisplayed(winPtr) || x1 > x2 || y1 > y2) {
	return;
    }
    SetDisplay(winPtr->dispPtr);
    SetStyle(lineStyle);

    Ctk_DrawCharacter(winPtr, x1, y1, lineStyle, ACS_ULCORNER);
    Ctk_DrawCharacter(winPtr, x2, y1, lineStyle, ACS_URCORNER);
    Ctk_DrawCharacter(winPtr, x1, y2, lineStyle, ACS_LLCORNER);
    Ctk_DrawCharacter(winPtr, x2, y2, lineStyle, ACS_LRCORNER);

    /* Convert to screen coordinates */
    x1 += winPtr->absLeft;
    x2 += winPtr->absLeft;
    y1 += winPtr->absTop;
    y2 += winPtr->absTop;

    /*
     *	Draw horizontal lines.
     */
    left = x1+1;
    right = x2;
    CtkIntersectSpans(&left, &right, clipRectPtr->left, clipRectPtr->right);
    if (!CtkSpanIsEmpty(left, right)) {
	if ((clipRectPtr->top <= y1) && (clipRectPtr->bottom > y1)) {
	    CtkForEachIntersectingSpan(
		FillSpan, (ClientData) ACS_HLINE,
		left, right, y1,
		winPtr->clipRgn);
	}
	if ((clipRectPtr->top <= y2) && (clipRectPtr->bottom > y2)) {
	    CtkForEachIntersectingSpan(
		FillSpan, (ClientData) ACS_HLINE,
		left, right, y2,
		winPtr->clipRgn);
	}
    }

    /*
     *	Draw vertical lines.
     */
    top = y1 + 1;
    bottom = y2;
    CtkIntersectSpans(&top, &bottom, clipRectPtr->top, clipRectPtr->bottom);
    if ((clipRectPtr->left <= x1) && (clipRectPtr->right > x1)) {
	for (y=top; y < bottom; y++) {
	    if (CtkPointInRegion(x1, y, winPtr->clipRgn)) {
		Move(x1, y);
		PutChar(ACS_VLINE);
	    }
	}
    }
    if ((clipRectPtr->left <= x2) && (clipRectPtr->right > x2)) {
	for (y=top; y < bottom; y++) {
	    if (CtkPointInRegion(x2, y, winPtr->clipRgn)) {
		Move(x2, y);
		PutChar(ACS_VLINE);
	    }
	}
    }
}
