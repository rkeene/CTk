/* 
 * tkMenubutton.c (CTk) --
 *
 *	This module implements button-like widgets that are used
 *	to invoke pull-down menus.
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
 * A data structure of the following type is kept for each
 * widget managed by this file:
 */

typedef struct {
    Tk_Window tkwin;		/* Window that embodies the widget.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up.*/
    Tcl_Interp *interp;		/* Interpreter associated with menubutton. */
    Tcl_Command widgetCmd;	/* Token for menubutton's widget command. */
    char *menuName;		/* Name of menu associated with widget.
				 * Malloc-ed. */

    /*
     * Information about what's displayed in the menu button:
     */

    char *text;			/* Text to display in button (malloc'ed)
				 * or NULL. */
    int numChars;		/* # of characters in text. */
    int underline;		/* Index of character to underline. */
    char *textVarName;		/* Name of variable (malloc'ed) or NULL.
				 * If non-NULL, button displays the contents
				 * of this variable. */

    /*
     * Information used when displaying widget:
     */

    Tk_Uid state;		/* State of button for display purposes:
				 * normal, active, or disabled. */
    int borderWidth;		/* Width of border. */
    int width, height;		/* If > 0, these specify dimensions to request
				 * for window, in characters for text and in
				 * pixels for bitmaps.  In this case the actual
				 * size of the text string or bitmap is
				 * ignored in computing desired window size. */
    int wrapLength;		/* Line length (in pixels) at which to wrap
				 * onto next line.  <= 0 means don't wrap
				 * except at newlines. */
    int padX, padY;		/* Extra space around text or bitmap (pixels
				 * on each side). */
    Tk_Anchor anchor;		/* Where text/bitmap should be displayed
				 * inside window region. */
    Tk_Justify justify;		/* Justification to use for multi-line text. */
    int textWidth;		/* Width needed to display text as requested,
				 * in pixels. */
    int textHeight;		/* Height needed to display text as requested,
				 * in pixels. */
    int indicatorOn;		/* Non-zero means display indicator;  0 means
				 * don't display. */

    /*
     * Miscellaneous information:
     */

    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
} MenuButton;

/*
 * Flag bits for buttons:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler
 *				has already been queued to redraw
 *				this window.
 * POSTED:			Non-zero means that the menu associated
 *				with this button has been posted (typically
 *				because of an active button press).
 * GOT_FOCUS:			Non-zero means this button currently
 *				has the input focus.
 */

#define REDRAW_PENDING		1
#define POSTED			2
#define GOT_FOCUS		4

/*
 * The following constants define the dimensions of the cascade indicator,
 * which is displayed if the "-indicatoron" option is true.  The units for
 * these options are pixels (characters).
 */

#define INDICATOR_WIDTH		1

/*
 * Information used for parsing configuration specs:
 */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
	DEF_MENUBUTTON_ANCHOR, Tk_Offset(MenuButton, anchor), 0},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_MENUBUTTON_BORDER_WIDTH, Tk_Offset(MenuButton, borderWidth), 0},
    {TK_CONFIG_INT, "-height", "height", "Height",
	DEF_MENUBUTTON_HEIGHT, Tk_Offset(MenuButton, height), 0},
    {TK_CONFIG_BOOLEAN, "-indicatoron", "indicatorOn", "IndicatorOn",
	DEF_MENUBUTTON_INDICATOR, Tk_Offset(MenuButton, indicatorOn), 0},
    {TK_CONFIG_JUSTIFY, "-justify", "justify", "Justify",
	DEF_MENUBUTTON_JUSTIFY, Tk_Offset(MenuButton, justify), 0},
    {TK_CONFIG_STRING, "-menu", "menu", "Menu",
	DEF_MENUBUTTON_MENU, Tk_Offset(MenuButton, menuName),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
	DEF_MENUBUTTON_PADX, Tk_Offset(MenuButton, padX), 0},
    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
	DEF_MENUBUTTON_PADY, Tk_Offset(MenuButton, padY), 0},
    {TK_CONFIG_UID, "-state", "state", "State",
	DEF_MENUBUTTON_STATE, Tk_Offset(MenuButton, state), 0},
    {TK_CONFIG_STRING, "-takefocus", "takeFocus", "TakeFocus",
	DEF_MENUBUTTON_TAKE_FOCUS, Tk_Offset(MenuButton, takeFocus),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-text", "text", "Text",
	DEF_MENUBUTTON_TEXT, Tk_Offset(MenuButton, text), 0},
    {TK_CONFIG_STRING, "-textvariable", "textVariable", "Variable",
	DEF_MENUBUTTON_TEXT_VARIABLE, Tk_Offset(MenuButton, textVarName),
	TK_CONFIG_NULL_OK},
    {TK_CONFIG_INT, "-underline", "underline", "Underline",
	DEF_MENUBUTTON_UNDERLINE, Tk_Offset(MenuButton, underline), 0},
    {TK_CONFIG_INT, "-width", "width", "Width",
	DEF_MENUBUTTON_WIDTH, Tk_Offset(MenuButton, width), 0},
    {TK_CONFIG_PIXELS, "-wraplength", "wrapLength", "WrapLength",
	DEF_MENUBUTTON_WRAP_LENGTH, Tk_Offset(MenuButton, wrapLength), 0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		ComputeMenuButtonGeometry _ANSI_ARGS_((
			    MenuButton *mbPtr));
static void		MenuButtonCmdDeletedProc _ANSI_ARGS_((
			    ClientData clientData));
static void		MenuButtonEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static char *		MenuButtonTextVarProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    char *name1, char *name2, int flags));
static int		MenuButtonWidgetCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
static int		ConfigureMenuButton _ANSI_ARGS_((Tcl_Interp *interp,
			    MenuButton *mbPtr, int argc, char **argv,
			    int flags));
static void		DestroyMenuButton _ANSI_ARGS_((ClientData clientData));
static void		DisplayMenuButton _ANSI_ARGS_((ClientData clientData));

/*
 *--------------------------------------------------------------
 *
 * Tk_MenubuttonCmd --
 *
 *	This procedure is invoked to process the "button", "label",
 *	"radiobutton", and "checkbutton" Tcl commands.  See the
 *	user documentation for details on what it does.
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
Tk_MenubuttonCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    register MenuButton *mbPtr;
    Tk_Window tkwin = (Tk_Window) clientData;
    Tk_Window new;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args:  should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Create the new window.
     */

    new = Tk_CreateWindowFromPath(interp, tkwin, argv[1], (char *) NULL);
    if (new == NULL) {
	return TCL_ERROR;
    }

    /*
     * Initialize the data structure for the button.
     */

    mbPtr = (MenuButton *) ckalloc(sizeof(MenuButton));
    mbPtr->tkwin = new;
    mbPtr->interp = interp;
    mbPtr->widgetCmd = Tcl_CreateCommand(interp, Tk_PathName(mbPtr->tkwin),
	    MenuButtonWidgetCmd, (ClientData) mbPtr, MenuButtonCmdDeletedProc);
    mbPtr->menuName = NULL;
    mbPtr->text = NULL;
    mbPtr->numChars = 0;
    mbPtr->underline = -1;
    mbPtr->textVarName = NULL;
    mbPtr->state = tkNormalUid;
    mbPtr->borderWidth = 0;
    mbPtr->width = 0;
    mbPtr->height = 0;
    mbPtr->wrapLength = 0;
    mbPtr->padX = 0;
    mbPtr->padY = 0;
    mbPtr->anchor = TK_ANCHOR_CENTER;
    mbPtr->justify = TK_JUSTIFY_CENTER;
    mbPtr->indicatorOn = 0;
    mbPtr->takeFocus = NULL;
    mbPtr->flags = 0;

    Tk_SetClass(mbPtr->tkwin, "Menubutton");
    Tk_CreateEventHandler(mbPtr->tkwin,
    	    CTK_EXPOSE_EVENT_MASK|CTK_DESTROY_EVENT_MASK|CTK_FOCUS_EVENT_MASK,
	    MenuButtonEventProc, (ClientData) mbPtr);
    if (ConfigureMenuButton(interp, mbPtr, argc-2, argv+2, 0) != TCL_OK) {
	Tk_DestroyWindow(mbPtr->tkwin);
	return TCL_ERROR;
    }

    Tcl_SetResult(interp,Tk_PathName(mbPtr->tkwin),TCL_VOLATILE);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * MenuButtonWidgetCmd --
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
MenuButtonWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Information about button widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    register MenuButton *mbPtr = (MenuButton *) clientData;
    int result = TCL_OK;
    size_t length;
    int c;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    Tk_Preserve((ClientData) mbPtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'c') && (strncmp(argv[1], "cget", length) == 0)
	    && (length >= 2)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " cget option\"",
		    (char *) NULL);
	    goto error;
	}
	result = Tk_ConfigureValue(interp, mbPtr->tkwin, configSpecs,
		(char *) mbPtr, argv[2], 0);
    } else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)
	    && (length >= 2)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, mbPtr->tkwin, configSpecs,
		    (char *) mbPtr, (char *) NULL, 0);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, mbPtr->tkwin, configSpecs,
		    (char *) mbPtr, argv[2], 0);
	} else {
	    result = ConfigureMenuButton(interp, mbPtr, argc-2, argv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\":  must be cget or configure",
		(char *) NULL);
	goto error;
    }
    Tk_Release((ClientData) mbPtr);
    return result;

    error:
    Tk_Release((ClientData) mbPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyMenuButton --
 *
 *	This procedure is invoked to recycle all of the resources
 *	associated with a button widget.  It is invoked as a
 *	when-idle handler in order to make sure that there is no
 *	other use of the button pending at the time of the deletion.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the widget is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyMenuButton(clientData)
    ClientData clientData;	/* Info about button widget. */
{
    register MenuButton *mbPtr = (MenuButton *) clientData;

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */

    if (mbPtr->textVarName != NULL) {
	Tcl_UntraceVar(mbPtr->interp, mbPtr->textVarName,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuButtonTextVarProc, (ClientData) mbPtr);
    }
    Tk_FreeOptions(configSpecs, (char *) mbPtr, 0);
    ckfree((char *) mbPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMenuButton --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a menubutton widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as text string, colors, font,
 *	etc. get set for mbPtr;  old resources get freed, if there
 *	were any.  The menubutton is redisplayed.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureMenuButton(interp, mbPtr, argc, argv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register MenuButton *mbPtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int argc;			/* Number of valid entries in argv. */
    char **argv;		/* Arguments. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    int result;

    /*
     * Eliminate any existing trace on variables monitored by the menubutton.
     */

    if (mbPtr->textVarName != NULL) {
	Tcl_UntraceVar(interp, mbPtr->textVarName,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuButtonTextVarProc, (ClientData) mbPtr);
    }

    result = Tk_ConfigureWidget(interp, mbPtr->tkwin, configSpecs,
	    argc, argv, (char *) mbPtr, flags);
    if (result != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * A few options need special processing.
     */

    if ((mbPtr->state != tkNormalUid) && (mbPtr->state != tkActiveUid)
	    && (mbPtr->state != tkDisabledUid)) {
	Tcl_AppendResult(interp, "bad state value \"", mbPtr->state,
		"\":  must be normal, active, or disabled", (char *) NULL);
	mbPtr->state = tkNormalUid;
	return TCL_ERROR;
    }

    if (mbPtr->padX < 0) {
	mbPtr->padX = 0;
    }
    if (mbPtr->padY < 0) {
	mbPtr->padY = 0;
    }

    if (mbPtr->textVarName != NULL) {
	/*
	 * The menubutton displays a variable.  Set up a trace to watch
	 * for any changes in it.
	 */

	char *value;

	value = Tcl_GetVar(interp, mbPtr->textVarName, TCL_GLOBAL_ONLY);
	if (value == NULL) {
	    Tcl_SetVar(interp, mbPtr->textVarName, mbPtr->text,
		    TCL_GLOBAL_ONLY);
	} else {
	    if (mbPtr->text != NULL) {
		ckfree(mbPtr->text);
	    }
	    mbPtr->text = (char *) ckalloc((unsigned) (strlen(value) + 1));
	    strcpy(mbPtr->text, value);
	}
	Tcl_TraceVar(interp, mbPtr->textVarName,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuButtonTextVarProc, (ClientData) mbPtr);
    }

    /*
     * Recompute the geometry for the button.
     */
    ComputeMenuButtonGeometry(mbPtr);

    /*
     * Lastly, arrange for the button to be redisplayed.
     */

    if (Tk_IsMapped(mbPtr->tkwin) && !(mbPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayMenuButton, (ClientData) mbPtr);
	mbPtr->flags |= REDRAW_PENDING;
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayMenuButton --
 *
 *	This procedure is invoked to display a menubutton widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menubutton in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayMenuButton(clientData)
    ClientData clientData;	/* Information about widget. */
{
    register MenuButton *mbPtr = (MenuButton *) clientData;
    register Tk_Window tkwin = mbPtr->tkwin;
    Ctk_Style style;
    int x, y;
    unsigned int width = Tk_Width(tkwin);
    unsigned int height = Tk_Height(tkwin);
    int indicatorSpace = (mbPtr->indicatorOn) ? INDICATOR_WIDTH : 0;

    mbPtr->flags &= ~REDRAW_PENDING;
    if ((tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    style = (mbPtr->state == tkDisabledUid)
	    ? CTK_DISABLED_STYLE : CTK_BUTTON_STYLE;

    switch (mbPtr->anchor) {
    case TK_ANCHOR_NW: case TK_ANCHOR_W: case TK_ANCHOR_SW:
	x = mbPtr->borderWidth + mbPtr->padX;
	break;
    case TK_ANCHOR_N: case TK_ANCHOR_CENTER: case TK_ANCHOR_S:
	x = (width - indicatorSpace - mbPtr->textWidth)/2;
	break;
    default:
	x = width - mbPtr->borderWidth - mbPtr->padX - indicatorSpace
		- mbPtr->textWidth;
	break;
    }
    switch (mbPtr->anchor) {
    case TK_ANCHOR_NW: case TK_ANCHOR_N: case TK_ANCHOR_NE:
	y = mbPtr->borderWidth + mbPtr->padY;
	break;
    case TK_ANCHOR_W: case TK_ANCHOR_CENTER: case TK_ANCHOR_E:
	y = (height - mbPtr->textHeight)/2;
	break;
    default:
	y = height - mbPtr->borderWidth - mbPtr->padY - mbPtr->textHeight;
	break;
    }

    /*
     * Clear rect.
     */
    Ctk_FillRect(tkwin, 0, 0, width, height, style, ' ');

    /*
     * Draw text.
     */
    TkDisplayText(tkwin, style, mbPtr->text, mbPtr->numChars,
	    x, y, mbPtr->textWidth, mbPtr->justify, mbPtr->underline);

    /*
     * Draw Indicator.
     */
    if (indicatorSpace) {
	x = width - mbPtr->borderWidth - mbPtr->padX - indicatorSpace;
	y += mbPtr->textHeight/2;
	Ctk_DrawCharacter(tkwin, x, y, style, '^');
    }

    /*
     * Draw border.
     */
    Ctk_DrawBorder(tkwin, style, (char *)NULL);

    /*
     * Position cursor.
     */
    if (mbPtr->flags & GOT_FOCUS) {
	Ctk_SetCursor(tkwin, mbPtr->borderWidth, mbPtr->borderWidth);
    }
}

/*
 *--------------------------------------------------------------
 *
 * MenuButtonEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on buttons.
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
MenuButtonEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    MenuButton *mbPtr = (MenuButton *) clientData;
    if (eventPtr->type == CTK_EXPOSE_EVENT) {
	if ((mbPtr->tkwin != NULL) && !(mbPtr->flags & REDRAW_PENDING)) {
	    Tcl_DoWhenIdle(DisplayMenuButton, (ClientData) mbPtr);
	    mbPtr->flags |= REDRAW_PENDING;
	}
    } else if (eventPtr->type == CTK_DESTROY_EVENT) {
	if (mbPtr->tkwin != NULL) {
	    mbPtr->tkwin = NULL;
	    Tcl_DeleteCommand(mbPtr->interp,
		Tcl_GetCommandName(mbPtr->interp, mbPtr->widgetCmd));
	}
	if (mbPtr->flags & REDRAW_PENDING) {
	    Tcl_CancelIdleCall(DisplayMenuButton, (ClientData) mbPtr);
	}
	Tk_EventuallyFree((ClientData) mbPtr, DestroyMenuButton);
    } else if (eventPtr->type == CTK_FOCUS_EVENT) {
	mbPtr->flags |= GOT_FOCUS;
	Ctk_SetCursor(mbPtr->tkwin, mbPtr->borderWidth, mbPtr->borderWidth);
    } else if (eventPtr->type == CTK_UNFOCUS_EVENT) {
	mbPtr->flags &= ~GOT_FOCUS;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MenuButtonCmdDeletedProc --
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
MenuButtonCmdDeletedProc(clientData)
    ClientData clientData;	/* Pointer to widget record for widget. */
{
    MenuButton *mbPtr = (MenuButton *) clientData;
    Tk_Window tkwin = mbPtr->tkwin;

    /*
     * This procedure could be invoked either because the window was
     * destroyed and the command was then deleted (in which case tkwin
     * is NULL) or because the command was deleted, and then this procedure
     * destroys the widget.
     */

    if (tkwin != NULL) {
	mbPtr->tkwin = NULL;
	Tk_DestroyWindow(tkwin);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeMenuButtonGeometry --
 *
 *	After changes in a menu button's text or bitmap, this procedure
 *	recomputes the menu button's geometry and passes this information
 *	along to the geometry manager for the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menu button's window may change size.
 *
 *----------------------------------------------------------------------
 */

static void
ComputeMenuButtonGeometry(mbPtr)
    register MenuButton *mbPtr;		/* Widget record for menu button. */
{
    int width, height;
    int indicatorSpace = (mbPtr->indicatorOn) ? INDICATOR_WIDTH : 0;

    mbPtr->numChars = strlen(mbPtr->text);
    TkComputeTextGeometry(mbPtr->text, mbPtr->numChars,
    	    mbPtr->wrapLength,
    	    &mbPtr->textWidth, &mbPtr->textHeight);
    width = mbPtr->width;
    if (width < 0) {
	width = mbPtr->textWidth;
    }
    height = mbPtr->height;
    if (height < 0) {
	height = mbPtr->textHeight;
    }
    width += 2*mbPtr->padX;
    height += 2*mbPtr->padY;

    Tk_GeometryRequest(mbPtr->tkwin,
    	    width + indicatorSpace + 2*mbPtr->borderWidth,
    	    height + 2*mbPtr->borderWidth);
}

/*
 *--------------------------------------------------------------
 *
 * MenuButtonTextVarProc --
 *
 *	This procedure is invoked when someone changes the variable
 *	whose contents are to be displayed in a menu button.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The text displayed in the menu button will change to match the
 *	variable.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static char *
MenuButtonTextVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about button. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable. */
    char *name2;		/* Second part of variable name. */
    int flags;			/* Information about what happened. */
{
    register MenuButton *mbPtr = (MenuButton *) clientData;
    char *value;

    /*
     * If the variable is unset, then immediately recreate it unless
     * the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_SetVar(interp, mbPtr->textVarName, mbPtr->text,
		    TCL_GLOBAL_ONLY);
	    Tcl_TraceVar(interp, mbPtr->textVarName,
		    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    MenuButtonTextVarProc, clientData);
	}
	return (char *) NULL;
    }

    value = Tcl_GetVar(interp, mbPtr->textVarName, TCL_GLOBAL_ONLY);
    if (value == NULL) {
	value = "";
    }
    if (mbPtr->text != NULL) {
	ckfree(mbPtr->text);
    }
    mbPtr->text = (char *) ckalloc((unsigned) (strlen(value) + 1));
    strcpy(mbPtr->text, value);
    ComputeMenuButtonGeometry(mbPtr);

    if ((mbPtr->tkwin != NULL) && Tk_IsMapped(mbPtr->tkwin)
	    && !(mbPtr->flags & REDRAW_PENDING)) {
	Tcl_DoWhenIdle(DisplayMenuButton, (ClientData) mbPtr);
	mbPtr->flags |= REDRAW_PENDING;
    }
    return (char *) NULL;
}
