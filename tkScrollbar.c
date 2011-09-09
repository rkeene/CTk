/* 
 * tkScrollbar.c (CTk) --
 *
 *	This module implements a scrollbar widgets for the CTk
 *	toolkit.  A scrollbar displays a slider and two arrows;
 *	mouse clicks on features within the scrollbar cause
 *	scrolling commands to be invoked.
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
#include "default.h"
#include "tkInt.h"

/*
 * A data structure of the following type is kept for each scrollbar
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the scrollbar.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up.*/
    Tcl_Interp *interp;		/* Interpreter associated with scrollbar. */
    Tcl_Command widgetCmd;	/* Token for scrollbar's widget command. */
    Tk_Uid orientUid;		/* Orientation for window ("vertical" or
				 * "horizontal"). */
    int vertical;		/* Non-zero means vertical orientation
				 * requested, zero means horizontal. */
    int width;			/* Desired narrow dimension of scrollbar,
				 * in pixels. */
    char *command;		/* Command prefix to use when invoking
				 * scrolling commands.  NULL means don't
				 * invoke commands.  Malloc'ed. */
    int commandSize;		/* Number of non-NULL bytes in command. */

    /*
     * Information used when displaying widget:
     */

    int borderWidth;		/* Width of border in pixels. */
    int sliderFirst;		/* Character coordinate of top or left edge
				 * of slider area, including border. */
    int sliderLast;		/* Coordinate of character just after bottom
				 * or right edge of slider area, including
				 * border. */

    /*
     * Information describing the application related to the scrollbar.
     * This information is provided by the application by invoking the
     * "set" widget command.  This information can now be provided in
     * two ways:  the "old" form (totalUnits, windowUnits, firstUnit,
     * and lastUnit), or the "new" form (firstFraction and lastFraction).
     * FirstFraction and lastFraction will always be valid, but
     * the old-style information is only valid if the NEW_STYLE_COMMANDS
     * flag is 0.
     */

    int totalUnits;		/* Total dimension of application, in
				 * units.  Valid only if the NEW_STYLE_COMMANDS
				 * flag isn't set. */
    int windowUnits;		/* Maximum number of units that can be
				 * displayed in the window at once.  Valid
				 * only if the NEW_STYLE_COMMANDS flag isn't
				 * set. */
    int firstUnit;		/* Number of last unit visible in
				 * application's window.  Valid only if the
				 * NEW_STYLE_COMMANDS flag isn't set. */
    int lastUnit;		/* Index of last unit visible in window.
				 * Valid only if the NEW_STYLE_COMMANDS
				 * flag isn't set. */
    double firstFraction;	/* Position of first visible thing in window,
				 * specified as a fraction between 0 and
				 * 1.0. */
    double lastFraction;	/* Position of last visible thing in window,
				 * specified as a fraction between 0 and
				 * 1.0. */

    /*
     * Miscellaneous information:
     */

    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
} Scrollbar;

/*
 * Flag bits for scrollbars:
 * 
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler
 *				has already been queued to redraw
 *				this window.
 * NEW_STYLE_COMMANDS:		Non-zero means the new style of commands
 *				should be used to communicate with the
 *				widget:  ".t yview scroll 2 lines", instead
 *				of ".t yview 40", for example.
 * GOT_FOCUS:			Non-zero means this window has the input
 *				focus.
 */

#define REDRAW_PENDING		1
#define NEW_STYLE_COMMANDS	2
#define GOT_FOCUS		4

/*
 * Minimum slider length and (fixed) arrow length, in characters.
 */
#define MIN_SLIDER_LENGTH	1
#define ARROW_LENGTH		1

/*
 * Information used for argv parsing.
 */
static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_SCROLLBAR_BORDER_WIDTH, Tk_Offset(Scrollbar, borderWidth), 0},
    {TK_CONFIG_STRING, "-command", "command", "Command",
	DEF_SCROLLBAR_COMMAND, Tk_Offset(Scrollbar, command),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_UID, "-orient", "orient", "Orient",
	DEF_SCROLLBAR_ORIENT, Tk_Offset(Scrollbar, orientUid), 0},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_SCROLLBAR_TAKE_FOCUS, Tk_Offset(Scrollbar, takeFocus),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-width", "width", "Width",
	DEF_SCROLLBAR_WIDTH, Tk_Offset(Scrollbar, width), 0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		ComputeScrollbarGeometry _ANSI_ARGS_((
			    Scrollbar *scrollPtr));
static int		ConfigureScrollbar _ANSI_ARGS_((Tcl_Interp *interp,
			    Scrollbar *scrollPtr, int argc, char **argv,
			    int flags));
static void		DestroyScrollbar _ANSI_ARGS_((ClientData clientData));
static void		DisplayScrollbar _ANSI_ARGS_((ClientData clientData));
static void		EventuallyRedraw _ANSI_ARGS_((Scrollbar *scrollPtr));
static void		ScrollbarCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		ScrollbarEventProc _ANSI_ARGS_((ClientData clientData,
			    Ctk_Event *eventPtr));
static int		ScrollbarWidgetCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *, int argc, char **argv));

/*
 *--------------------------------------------------------------
 *
 * Tk_ScrollbarCmd --
 *
 *	This procedure is invoked to process the "scrollbar" Tcl
 *	command.  See the user documentation for details on what
 *	it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_ScrollbarCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    register Scrollbar *scrollPtr;
    Tk_Window new;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args:  should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    new = Tk_CreateWindowFromPath(interp, tkwin, argv[1], (char *) NULL);
    if (new == NULL) {
	return TCL_ERROR;
    }

    /*
     * Initialize fields that won't be initialized by ConfigureScrollbar,
     * or which ConfigureScrollbar expects to have reasonable values
     * (e.g. resource pointers).
     */

    scrollPtr = (Scrollbar *) ckalloc(sizeof(Scrollbar));
    scrollPtr->tkwin = new;
    scrollPtr->interp = interp;
    scrollPtr->widgetCmd = Tcl_CreateCommand(interp,
	    Tk_PathName(scrollPtr->tkwin), ScrollbarWidgetCmd,
	    (ClientData) scrollPtr, ScrollbarCmdDeletedProc);
    scrollPtr->orientUid = NULL;
    scrollPtr->vertical = 0;
    scrollPtr->width = 0;
    scrollPtr->command = NULL;
    scrollPtr->commandSize = 0;
    scrollPtr->borderWidth = 0;
    scrollPtr->sliderFirst = 0;
    scrollPtr->sliderLast = 0;
    scrollPtr->totalUnits = 0;
    scrollPtr->windowUnits = 0;
    scrollPtr->firstUnit = 0;
    scrollPtr->lastUnit = 0;
    scrollPtr->firstFraction = 0.0;
    scrollPtr->lastFraction = 0.0;
    scrollPtr->takeFocus = NULL;
    scrollPtr->flags = 0;

    Tk_SetClass(scrollPtr->tkwin, "Scrollbar");
    Tk_CreateEventHandler(scrollPtr->tkwin,
    	    CTK_EXPOSE_EVENT_MASK|CTK_FOCUS_EVENT_MASK|CTK_MAP_EVENT_MASK
    	    |CTK_DESTROY_EVENT_MASK,
	    ScrollbarEventProc, (ClientData) scrollPtr);
    if (ConfigureScrollbar(interp, scrollPtr, argc-2, argv+2, 0) != TCL_OK) {
	goto error;
    }

    Tcl_SetResult(interp,Tk_PathName(scrollPtr->tkwin),TCL_VOLATILE);
    return TCL_OK;

    error:
    Tk_DestroyWindow(scrollPtr->tkwin);
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarWidgetCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
ScrollbarWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Information about scrollbar
					 * widget. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    char **argv;			/* Argument strings. */
{
    register Scrollbar *scrollPtr = (Scrollbar *) clientData;
    int result = TCL_OK;
    size_t length;
    int c;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    Tk_Preserve((ClientData) scrollPtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'a') && (strncmp(argv[1], "activate", length) == 0)) {
        result = Ctk_Unsupported(interp, "scrollbar activate");
    } else if ((c == 'c') && (strncmp(argv[1], "cget", length) == 0)
	    && (length >= 2)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " cget option\"",
		    (char *) NULL);
	    goto error;
	}
	result = Tk_ConfigureValue(interp, scrollPtr->tkwin, configSpecs,
		(char *) scrollPtr, argv[2], 0);
    } else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)
	    && (length >= 2)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, scrollPtr->tkwin, configSpecs,
		    (char *) scrollPtr, (char *) NULL, 0);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, scrollPtr->tkwin, configSpecs,
		    (char *) scrollPtr, argv[2], 0);
	} else {
	    result = ConfigureScrollbar(interp, scrollPtr, argc-2, argv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    } else if ((c == 'd') && (strncmp(argv[1], "delta", length) == 0)) {
	int xDelta, yDelta, pixels, length;
	double fraction;

	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " delta xDelta yDelta\"", (char *) NULL);
	    goto error;
	}
	if ((Tcl_GetInt(interp, argv[2], &xDelta) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[3], &yDelta) != TCL_OK)) {
	    goto error;
	}
	if (scrollPtr->vertical) {
	    pixels = yDelta;
	    length = Tk_Height(scrollPtr->tkwin) - 1
		    - 2*(ARROW_LENGTH + scrollPtr->borderWidth);
	} else {
	    pixels = xDelta;
	    length = Tk_Width(scrollPtr->tkwin) - 1
		    - 2*(ARROW_LENGTH + scrollPtr->borderWidth);
	}
	if (length == 0) {
	    fraction = 0.0;
	} else {
	    fraction = ((double) pixels / (double) length);
	}
	{
	  char buffer[30];
	  sprintf(buffer, "%g", fraction);
	  Tcl_SetResult(interp,buffer,TCL_VOLATILE);
	}
    } else if ((c == 'f') && (strncmp(argv[1], "fraction", length) == 0)) {
	int x, y, pos, length;
	double fraction;

	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " fraction x y\"", (char *) NULL);
	    goto error;
	}
	if ((Tcl_GetInt(interp, argv[2], &x) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[3], &y) != TCL_OK)) {
	    goto error;
	}
	if (scrollPtr->vertical) {
	    pos = y - (ARROW_LENGTH + scrollPtr->borderWidth);
	    length = Tk_Height(scrollPtr->tkwin) - 1
		    - 2*(ARROW_LENGTH + scrollPtr->borderWidth);
	} else {
	    pos = x - (ARROW_LENGTH + scrollPtr->borderWidth);
	    length = Tk_Width(scrollPtr->tkwin) - 1
		    - 2*(ARROW_LENGTH + scrollPtr->borderWidth);
	}
	if (length == 0) {
	    fraction = 0.0;
	} else {
	    fraction = ((double) pos / (double) length);
	}
	if (fraction < 0) {
	    fraction = 0;
	} else if (fraction > 1.0) {
	    fraction = 1.0;
	}
	{
	  char buffer[30];
	  sprintf(buffer, "%g", fraction);
	  Tcl_SetResult(interp,buffer,TCL_VOLATILE);
	}
    } else if ((c == 'g') && (strncmp(argv[1], "get", length) == 0)) {
	if (argc != 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " get\"", (char *) NULL);
	    goto error;
	}
	if (scrollPtr->flags & NEW_STYLE_COMMANDS) {
	    char first[TCL_DOUBLE_SPACE], last[TCL_DOUBLE_SPACE];

	    Tcl_PrintDouble(interp, scrollPtr->firstFraction, first);
	    Tcl_PrintDouble(interp, scrollPtr->lastFraction, last);
	    Tcl_AppendResult(interp, first, " ", last, (char *) NULL);
	} else {
	    char buffer[100];
	    sprintf(buffer, "%d %d %d %d", scrollPtr->totalUnits,
		    scrollPtr->windowUnits, scrollPtr->firstUnit,
		    scrollPtr->lastUnit);
	    Tcl_SetResult(interp, buffer, TCL_VOLATILE);
	}
    } else if ((c == 'i') && (strncmp(argv[1], "identify", length) == 0)) {
        result = Ctk_Unsupported(interp, "scrollbar identify");
    } else if ((c == 's') && (strncmp(argv[1], "set", length) == 0)) {
	int totalUnits, windowUnits, firstUnit, lastUnit;

	if (argc == 4) {
	    double first, last;

	    if (Tcl_GetDouble(interp, argv[2], &first) != TCL_OK) {
		goto error;
	    }
	    if (Tcl_GetDouble(interp, argv[3], &last) != TCL_OK) {
		goto error;
	    }
	    if (first < 0) {
		scrollPtr->firstFraction = 0;
	    } else if (first > 1.0) {
		scrollPtr->firstFraction = 1.0;
	    } else {
		scrollPtr->firstFraction = first;
	    }
	    if (last < scrollPtr->firstFraction) {
		scrollPtr->lastFraction = scrollPtr->firstFraction;
	    } else if (last > 1.0) {
		scrollPtr->lastFraction = 1.0;
	    } else {
		scrollPtr->lastFraction = last;
	    }
	    scrollPtr->flags |= NEW_STYLE_COMMANDS;
	} else if (argc == 6) {
	    if (Tcl_GetInt(interp, argv[2], &totalUnits) != TCL_OK) {
		goto error;
	    }
	    if (totalUnits < 0) {
		totalUnits = 0;
	    }
	    if (Tcl_GetInt(interp, argv[3], &windowUnits) != TCL_OK) {
		goto error;
	    }
	    if (windowUnits < 0) {
		windowUnits = 0;
	    }
	    if (Tcl_GetInt(interp, argv[4], &firstUnit) != TCL_OK) {
		goto error;
	    }
	    if (Tcl_GetInt(interp, argv[5], &lastUnit) != TCL_OK) {
		goto error;
	    }
	    if (totalUnits > 0) {
		if (lastUnit < firstUnit) {
		    lastUnit = firstUnit;
		}
	    } else {
		firstUnit = lastUnit = 0;
	    }
	    scrollPtr->totalUnits = totalUnits;
	    scrollPtr->windowUnits = windowUnits;
	    scrollPtr->firstUnit = firstUnit;
	    scrollPtr->lastUnit = lastUnit;
	    if (scrollPtr->totalUnits == 0) {
		scrollPtr->firstFraction = 0.0;
		scrollPtr->lastFraction = 1.0;
	    } else {
		scrollPtr->firstFraction = ((double) firstUnit)/totalUnits;
		scrollPtr->lastFraction = ((double) (lastUnit+1))/totalUnits;
	    }
	    scrollPtr->flags &= ~NEW_STYLE_COMMANDS;
	} else {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " set firstFraction lastFraction\" or \"",
		    argv[0],
		    " set totalUnits windowUnits firstUnit lastUnit\"",
		    (char *) NULL);
	    goto error;
	}
	ComputeScrollbarGeometry(scrollPtr);
	EventuallyRedraw(scrollPtr);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be activate, cget, configure, delta, fraction, ",
		"get, identify, or set", (char *) NULL);
	goto error;
    }
    Tk_Release((ClientData) scrollPtr);
    return result;

    error:
    Tk_Release((ClientData) scrollPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyScrollbar --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a scrollbar at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the scrollbar is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyScrollbar(clientData)
    ClientData clientData;	/* Info about scrollbar widget. */
{
    register Scrollbar *scrollPtr = (Scrollbar *) clientData;

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */
    Tk_FreeOptions(configSpecs, (char *) scrollPtr, 0);
    ckfree((char *) scrollPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureScrollbar --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a scrollbar widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, border width,
 *	etc. get set for scrollPtr;  old resources get freed,
 *	if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureScrollbar(interp, scrollPtr, argc, argv, flags)
    Tcl_Interp *interp;			/* Used for error reporting. */
    register Scrollbar *scrollPtr;	/* Information about widget;  may or
					 * may not already have values for
					 * some fields. */
    int argc;				/* Number of valid entries in argv. */
    char **argv;			/* Arguments. */
    int flags;				/* Flags to pass to
					 * Tk_ConfigureWidget. */
{
    size_t length;

    if (Tk_ConfigureWidget(interp, scrollPtr->tkwin, configSpecs,
	    argc, argv, (char *) scrollPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * A few options need special processing, such as parsing the
     * orientation or setting the background from a 3-D border.
     */

    if (scrollPtr->width < 1) {
    	scrollPtr->width = 1;
    }
    length = strlen(scrollPtr->orientUid);
    if (strncmp(scrollPtr->orientUid, "vertical", length) == 0) {
	scrollPtr->vertical = 1;
    } else if (strncmp(scrollPtr->orientUid, "horizontal", length) == 0) {
	scrollPtr->vertical = 0;
    } else {
	Tcl_AppendResult(interp, "bad orientation \"", scrollPtr->orientUid,
		"\": must be vertical or horizontal", (char *) NULL);
	return TCL_ERROR;
    }

    if (scrollPtr->command != NULL) {
	scrollPtr->commandSize = strlen(scrollPtr->command);
    } else {
	scrollPtr->commandSize = 0;
    }

    /*
     * Register the desired geometry for the window (leave enough space
     * for the two arrows plus a minimum-size slider, plus border around
     * the whole window, if any).  Then arrange for the window to be
     * redisplayed.
     */

    Tk_SetInternalBorder(scrollPtr->tkwin, scrollPtr->borderWidth);
    ComputeScrollbarGeometry(scrollPtr);
    EventuallyRedraw(scrollPtr);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * DisplayScrollbar --
 *
 *	This procedure redraws the contents of a scrollbar window.
 *	It is invoked as a do-when-idle handler, so it only runs
 *	when there's nothing else for the application to do.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information appears on the screen.
 *
 *--------------------------------------------------------------
 */

static void
DisplayScrollbar(clientData)
    ClientData clientData;	/* Information about window. */
{
    register Scrollbar *scrollPtr = (Scrollbar *) clientData;
    register Tk_Window tkwin = scrollPtr->tkwin;
    int bd = scrollPtr->borderWidth;
    int xBound = Tk_Width(tkwin) - bd;
    int yBound = Tk_Height(tkwin) - bd;
    int middle;

    if ((scrollPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	goto done;
    }

    if (scrollPtr->vertical) {
	if (Tk_Width(tkwin) > 1 + 2*bd) {
	    Ctk_FillRect(tkwin, bd, bd, xBound, yBound, CTK_PLAIN_STYLE, ' ');
	}
	middle = Tk_Width(tkwin)/2;
	Ctk_DrawCharacter(tkwin, middle, bd, CTK_PLAIN_STYLE, '^');
	Ctk_FillRect(tkwin, middle, bd+1, middle+1,  yBound-1,
		CTK_PLAIN_STYLE, '|');
	Ctk_DrawCharacter(tkwin, middle, yBound-1, CTK_PLAIN_STYLE, 'V');
	Ctk_FillRect(scrollPtr->tkwin,
		middle, scrollPtr->sliderFirst, middle+1, scrollPtr->sliderLast,
		CTK_PLAIN_STYLE, '#');
	if (scrollPtr->flags & GOT_FOCUS) {
	    Ctk_SetCursor(tkwin, middle, scrollPtr->sliderFirst);
	}
    } else {
	if (Tk_Height(tkwin) > 1 + 2*bd) {
	    Ctk_FillRect(tkwin, bd, bd, xBound, yBound, CTK_PLAIN_STYLE, ' ');
	}
	middle = Tk_Height(tkwin)/2;
	Ctk_DrawCharacter(tkwin, bd, middle, CTK_PLAIN_STYLE, '<');
	Ctk_FillRect(tkwin, bd+1, middle, xBound-1, middle+1, 
		CTK_PLAIN_STYLE, '-');
	Ctk_DrawCharacter(tkwin, xBound-1, middle, CTK_PLAIN_STYLE, '>');
	Ctk_FillRect(scrollPtr->tkwin,
		scrollPtr->sliderFirst, middle, scrollPtr->sliderLast, middle+1,
		CTK_PLAIN_STYLE, '#');
	if (scrollPtr->flags & GOT_FOCUS) {
	    Ctk_SetCursor(tkwin, scrollPtr->sliderFirst, middle);
	}
    }
    Ctk_DrawBorder(tkwin, CTK_PLAIN_STYLE, (char *)NULL);

    done:
    scrollPtr->flags &= ~REDRAW_PENDING;
}

/*
 *--------------------------------------------------------------
 *
 * ScrollbarEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on scrollbars.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
ScrollbarEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    Ctk_Event *eventPtr;	/* Information about event. */
{
    Scrollbar *scrollPtr = (Scrollbar *) clientData;

    if (eventPtr->type == CTK_EXPOSE_EVENT) {
	EventuallyRedraw(scrollPtr);
    } else if (eventPtr->type == CTK_DESTROY_EVENT) {
	if (scrollPtr->tkwin != NULL) {
	    scrollPtr->tkwin = NULL;
	    Tcl_DeleteCommand(scrollPtr->interp,
		    Tcl_GetCommandName(scrollPtr->interp,
		    scrollPtr->widgetCmd));
	}
	if (scrollPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayScrollbar, (ClientData) scrollPtr);
	}
	Tk_EventuallyFree((ClientData) scrollPtr, DestroyScrollbar);
    } else if (eventPtr->type == CTK_FOCUS_EVENT) {
	scrollPtr->flags |= GOT_FOCUS;
    } else if (eventPtr->type == CTK_UNFOCUS_EVENT) {
	scrollPtr->flags &= ~GOT_FOCUS;
    } else if (eventPtr->type == CTK_MAP_EVENT) {
	ComputeScrollbarGeometry(scrollPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ScrollbarCmdDeletedProc --
 *
 *	This procedure is invoked when a widget command is deleted.  If
 *	the widget isn't already in the process of being destroyed,
 *	this command destroys it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The widget is destroyed.
 *
 *----------------------------------------------------------------------
 */

static void
ScrollbarCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    Scrollbar *scrollPtr = (Scrollbar *) clientData;
    Tk_Window tkwin = scrollPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	scrollPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeScrollbarGeometry --
 *
 *	After changes in a scrollbar's size or configuration, this
 *	procedure recomputes various geometry information used in
 *	displaying the scrollbar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The scrollbar will be displayed differently.
 *
 *----------------------------------------------------------------------
 */

static void
ComputeScrollbarGeometry(scrollPtr)
    register Scrollbar *scrollPtr;	/* Scrollbar whose geometry may
					 * have changed. */
{
    int width, fieldLength;

    width = (scrollPtr->vertical)
    	    ? Tk_Width(scrollPtr->tkwin) : Tk_Height(scrollPtr->tkwin);
    fieldLength = ( scrollPtr->vertical
    	    ? (Tk_Height(scrollPtr->tkwin) - 2*scrollPtr->borderWidth)
	    : (Tk_Width(scrollPtr->tkwin) - 2*scrollPtr->borderWidth) )
	    - 2*ARROW_LENGTH;
    if (fieldLength < 0) {
	fieldLength = 0;
    }
    scrollPtr->sliderFirst = fieldLength*scrollPtr->firstFraction;
    scrollPtr->sliderLast = fieldLength*scrollPtr->lastFraction;

    /*
     * Adjust the slider so that some piece of it is always
     * displayed in the scrollbar and so that it has at least
     * a minimal width (so it can be grabbed with the mouse).
     */
    if (scrollPtr->sliderFirst > (fieldLength - 1)) {
	scrollPtr->sliderFirst = fieldLength - 1;
    }
    if (scrollPtr->sliderFirst < 0) {
	scrollPtr->sliderFirst = 0;
    }
    if (scrollPtr->sliderLast < (scrollPtr->sliderFirst + MIN_SLIDER_LENGTH)) {
	scrollPtr->sliderLast = scrollPtr->sliderFirst + MIN_SLIDER_LENGTH;
    }
    if (scrollPtr->sliderLast > fieldLength) {
	scrollPtr->sliderLast = fieldLength;
    }
    if (scrollPtr->vertical) {
	scrollPtr->sliderFirst += ARROW_LENGTH + scrollPtr->borderWidth;
	scrollPtr->sliderLast += ARROW_LENGTH + scrollPtr->borderWidth;
    } else {
	scrollPtr->sliderFirst += ARROW_LENGTH + scrollPtr->borderWidth;
	scrollPtr->sliderLast += ARROW_LENGTH + scrollPtr->borderWidth;
    }

    /*
     * Register the desired geometry for the window (leave enough space
     * for the two arrows plus a minimum-size slider, plus border around
     * the whole window, if any).  Then arrange for the window to be
     * redisplayed.
     */
    if (scrollPtr->vertical) {
	Tk_GeometryRequest(scrollPtr->tkwin,
		scrollPtr->width + 2*scrollPtr->borderWidth,
		MIN_SLIDER_LENGTH + 2*(ARROW_LENGTH + scrollPtr->borderWidth));
    } else {
	Tk_GeometryRequest(scrollPtr->tkwin,
		MIN_SLIDER_LENGTH + 2*(ARROW_LENGTH + scrollPtr->borderWidth),
		scrollPtr->width + 2*scrollPtr->borderWidth);
    }
}

/*
 *--------------------------------------------------------------
 *
 * EventuallyRedraw --
 *
 *	Arrange for one or more of the fields of a scrollbar
 *	to be redrawn.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
EventuallyRedraw(scrollPtr)
    register Scrollbar *scrollPtr;	/* Information about widget. */
{
    if ((scrollPtr->tkwin == NULL) || (!Tk_IsMapped(scrollPtr->tkwin))) {
	return;
    }
    if ((scrollPtr->flags & REDRAW_PENDING) == 0) {
	Tcl_DoWhenIdle(DisplayScrollbar, (ClientData) scrollPtr);
	scrollPtr->flags |= REDRAW_PENDING;
    }
}
