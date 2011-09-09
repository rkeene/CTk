/* 
 * tkFocus.c (CTk) --
 *
 *	This file contains procedures that manage the input
 *	focus for Tk.
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

#include "tkInt.h"
#include "tkPort.h"

/*
 * Hash table mapping top-level windows to their local focus (a descendant
 * window).  Both key and values are window pointers.  There is an
 * entry for every top-level window that has ever recieved the focus.
 */

static Tcl_HashTable focusTable;

/*
 * Has files static data been initialized?
 */

static int initialized = 0;


/*
 *--------------------------------------------------------------
 *
 * Tk_FocusCmd --
 *
 *	This procedure is invoked to process the "focus" Tcl command.
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

int
Tk_FocusCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr = (TkWindow *) clientData;
    TkWindow *newPtr, *focusWinPtr, *topLevelPtr;
    char c;
    size_t length;
    Tcl_HashEntry *hPtr;

    /*
     * If invoked with no arguments, just return the current focus window.
     */

    if (argc == 1) {
	focusWinPtr = TkGetFocus(winPtr);
	if (focusWinPtr != NULL) {
	    Tcl_SetResult(interp,focusWinPtr->pathName,TCL_VOLATILE);
	}
	return TCL_OK;
    }

    /*
     * If invoked with a single argument beginning with "." then focus
     * on that window.
     */

    if (argc == 2) {
	if (argv[1][0] == 0) {
	    return TCL_OK;
	}
	if (argv[1][0] == '.') {
	    newPtr = (TkWindow *) Tk_NameToWindow(interp, argv[1], tkwin);
	    if (newPtr == NULL) {
		return TCL_ERROR;
	    }
	    if (!(newPtr->flags & TK_ALREADY_DEAD)) {
		CtkSetFocus(newPtr);
	    }
	    return TCL_OK;
	}
    }

    length = strlen(argv[1]);
    c = argv[1][1];
    if ((c == 'd') && (strncmp(argv[1], "-displayof", length) == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " -displayof window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	newPtr = (TkWindow *) Tk_NameToWindow(interp, argv[2], tkwin);
	if (newPtr == NULL) {
	    return TCL_ERROR;
	}
	newPtr = TkGetFocus(newPtr);
	if (newPtr != NULL) {
	    Tcl_SetResult(interp,newPtr->pathName,TCL_VOLATILE);
	}
    } else if ((c == 'f') && (strncmp(argv[1], "-force", length) == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " -force window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	if (argv[2][0] == 0) {
	    return TCL_OK;
	}
	newPtr = (TkWindow *) Tk_NameToWindow(interp, argv[1], tkwin);
	if (newPtr == NULL) {
	    return TCL_ERROR;
	}
	CtkSetFocus(newPtr);
    } else if ((c == 'l') && (strncmp(argv[1], "-lastfor", length) == 0)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " -lastfor window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	newPtr = (TkWindow *) Tk_NameToWindow(interp, argv[2], tkwin);
	if (newPtr == NULL) {
	    return TCL_ERROR;
	}
	topLevelPtr = Ctk_TopLevel(newPtr);
	hPtr = Tcl_FindHashEntry(&focusTable, (char *) topLevelPtr);
	if (hPtr && (newPtr = (TkWindow *) Tcl_GetHashValue(hPtr))) {
	    Tcl_SetResult(interp,topLevelPtr->pathName,TCL_VOLATILE);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be -displayof, -force, or -lastfor", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CtkSetFocus --
 *
 *	This procedure is invoked to change the focus window for a
 *	given display in a given application.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Event handlers may be invoked to process the change of
 *	focus.
 *
 *----------------------------------------------------------------------
 */

void
CtkSetFocus(winPtr)
    TkWindow *winPtr;
{
    TkWindow *focusPtr = winPtr->dispPtr->focusPtr;
    Ctk_Event event;
    Tcl_HashEntry *hPtr;
    int new;

    if (!initialized) {
	Tcl_InitHashTable(&focusTable, TCL_ONE_WORD_KEYS);
	initialized = 1;
    }
    if (winPtr == (TkWindow *)NULL || (winPtr->flags & TK_ALREADY_DEAD)) {
    	panic("Attempt to set focus to null/dead window");
    }

    if (Tk_IsTopLevel(winPtr)) {
	/*
	 * Window is a top-level.
	 * Change focus destination to local focus of top-level.
	 */
	hPtr = Tcl_FindHashEntry(&focusTable, (char *) winPtr);
	if (hPtr && Tcl_GetHashValue(hPtr)) {
	    winPtr = (TkWindow *) Tcl_GetHashValue(hPtr);
	}
    } else {
	/*
	 * Set local focus of winPtr's top-level to winPtr.
	 */
	hPtr = Tcl_CreateHashEntry(&focusTable, (char *) Ctk_TopLevel(winPtr),
		&new);
	Tcl_SetHashValue(hPtr, (ClientData) winPtr);
    }

    if (winPtr != focusPtr) {
        if (focusPtr && !(focusPtr->flags & TK_ALREADY_DEAD)) {
	    event.type = CTK_UNFOCUS_EVENT;
	    event.window = focusPtr;
	    Tk_HandleEvent(&event);
	}
	winPtr->dispPtr->focusPtr = winPtr;
	Ctk_SetCursor(winPtr, 0, 0);
	event.type = CTK_FOCUS_EVENT;
	event.window = winPtr;
	Tk_HandleEvent(&event);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetFocus --
 *
 *	Given a window, this procedure returns the current focus
 *	window for its application and display.
 *
 * Results:
 *	The return value is a pointer to the window that currently
 *	has the input focus for the specified application and
 *	display, or NULL if none.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

TkWindow *
TkGetFocus(winPtr)
    TkWindow *winPtr;
{
    return winPtr->dispPtr->focusPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFocusDeadWindow --
 *
 *	This procedure is invoked when it is determined that
 *	a window is dead.  It cleans up focus-related information
 *	about the window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The input focus for the window's display may change.
 *
 *----------------------------------------------------------------------
 */

void
TkFocusDeadWindow(winPtr)
    TkWindow *winPtr;
{
    if (initialized) {
	/*
	 * Remove window from focusTable.  Delete hash entry if winPtr
	 * is a top-level.  Clear hash entry value if winPtr has a local
	 * focus.
	 */
	Tcl_HashEntry *hPtr;
	TkWindow *focusPtr;

	if (Tk_IsTopLevel(winPtr)) {
	    hPtr = Tcl_FindHashEntry(&focusTable, (char *) winPtr);
	    if (hPtr)  Tcl_DeleteHashEntry(hPtr);
	} else {
	    hPtr = Tcl_FindHashEntry(&focusTable,
		    (char *) Ctk_TopLevel(winPtr));
	    if (hPtr && winPtr == (TkWindow *) Tcl_GetHashValue(hPtr)) {
		Tcl_SetHashValue(hPtr, (ClientData) (TkWindow *) NULL);
	    }
	}
    }

    if (winPtr == winPtr->dispPtr->focusPtr) {
    	/*
    	 * This window has the focus, try to pass focus first to
    	 * window's top-level, then to topmost visible top-level,
	 * then to main top-level.  If none of these exist
    	 * then give up - the application will have exited
    	 * before any more key events will be processed).
    	 */
    	TkWindow *newFocusPtr = Ctk_TopLevel(winPtr);

	if (!(newFocusPtr->flags & TK_ALREADY_DEAD))  goto gotfocus;
	for (newFocusPtr = Ctk_TopChild(winPtr->dispPtr->rootPtr);
		 newFocusPtr != NULL;
		 newFocusPtr = Ctk_PriorSibling(newFocusPtr)) {
	    if (!(newFocusPtr->flags & TK_ALREADY_DEAD)
		    && (newFocusPtr->flags & CTK_DISPLAYED)) {
		goto gotfocus;
	    }
	}
	newFocusPtr = Ctk_BottomChild(winPtr->dispPtr->rootPtr);
	if (newFocusPtr && !(newFocusPtr->flags & TK_ALREADY_DEAD)) {
gotfocus:
	    CtkSetFocus(newFocusPtr);
	}
    }
}
