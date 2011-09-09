/* 
 * tkCmds.c (CTk) --
 *
 *	This file contains a collection of Tk-related Tcl commands
 *	that didn't fit in any particular file of the toolkit.
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
#include <errno.h>

/*
 * Forward declarations for procedures defined later in this file:
 */

static char *		WaitVariableProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));
static void		WaitWindowProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));

static int		GetFocusOk _ANSI_ARGS_((Tcl_Interp *interp,
			    TkWindow *winPtr, int *flagPtr));
static char error_buffer[200];

/*
 *----------------------------------------------------------------------
 *
 * Tk_BellCmd --
 *
 *	This procedure is invoked to process the "bell" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_BellCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    size_t length;

    if ((argc != 1) && (argc != 3)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" ?-displayof window?\"", (char *) NULL);
	return TCL_ERROR;
    }

    if (argc == 3) {
	length = strlen(argv[1]);
	if ((length < 2) || (strncmp(argv[1], "-displayof", length) != 0)) {
	    Tcl_AppendResult(interp, "bad option \"", argv[1],
		    "\": must be -displayof", (char *) NULL);
	    return TCL_ERROR;
	}
	tkwin = Tk_NameToWindow(interp, argv[2], tkwin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
    }
    CtkDisplayBell(Tk_Display(tkwin));
    Ctk_DisplayFlush(Tk_Display(tkwin));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_BindCmd --
 *
 *	This procedure is invoked to process the "bind" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_BindCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr;
    ClientData object;

    if ((argc < 2) || (argc > 4)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" window ?pattern? ?command?\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (argv[1][0] == '.') {
	winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[1], tkwin);
	if (winPtr == NULL) {
	    return TCL_ERROR;
	}
	object = (ClientData) winPtr->pathName;
    } else {
	winPtr = (TkWindow *) clientData;
	object = (ClientData) Tk_GetUid(argv[1]);
    }

    if (argc == 4) {
	int append = 0;
	unsigned long mask;

	if (argv[3][0] == 0) {
	    return Tk_DeleteBinding(interp, winPtr->mainPtr->bindingTable,
		    object, argv[2]);
	}
	if (argv[3][0] == '+') {
	    argv[3]++;
	    append = 1;
	}
	mask = Tk_CreateBinding(interp, winPtr->mainPtr->bindingTable,
		object, argv[2], argv[3], append);
	if (mask == 0) {
	    return TCL_ERROR;
	}
    } else if (argc == 3) {
	char *command;

	command = Tk_GetBinding(interp, winPtr->mainPtr->bindingTable,
		object, argv[2]);
	if (command == NULL) {
	    Tcl_ResetResult(interp);
	    return TCL_OK;
	}
	Tcl_SetResult(interp, command, TCL_VOLATILE);
    } else {
	Tk_GetAllBindings(interp, winPtr->mainPtr->bindingTable, object);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkBindEventProc --
 *
 *	This procedure is invoked by Tk_HandleEvent for each event;  it
 *	causes any appropriate bindings for that event to be invoked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on what bindings have been established with the "bind"
 *	command.
 *
 *----------------------------------------------------------------------
 */

void
TkBindEventProc(winPtr, eventPtr)
    TkWindow *winPtr;			/* Pointer to info about window. */
    XEvent *eventPtr;			/* Information about event. */
{
#define MAX_OBJS 20
    ClientData objects[MAX_OBJS], *objPtr;
    static Tk_Uid allUid = NULL;
    TkWindow *topLevPtr;
    int i, count;
    char *p;
    Tcl_HashEntry *hPtr;

    if ((winPtr->mainPtr == NULL) || (winPtr->mainPtr->bindingTable == NULL)) {
	return;
    }

    objPtr = objects;
    if (winPtr->numTags != 0) {
	/*
	 * Make a copy of the tags for the window, replacing window names
	 * with pointers to the pathName from the appropriate window.
	 */

	if (winPtr->numTags > MAX_OBJS) {
	    objPtr = (ClientData *) ckalloc((unsigned)
		    (winPtr->numTags * sizeof(ClientData)));
	}
	for (i = 0; i < winPtr->numTags; i++) {
	    p = (char *) winPtr->tagPtr[i];
	    if (*p == '.') {
		hPtr = Tcl_FindHashEntry(&winPtr->mainPtr->nameTable, p);
		if (hPtr != NULL) {
		    p = ((TkWindow *) Tcl_GetHashValue(hPtr))->pathName;
		} else {
		    p = NULL;
		}
	    }
	    objPtr[i] = (ClientData) p;
	}
	count = winPtr->numTags;
    } else {
	objPtr[0] = (ClientData) winPtr->pathName;
	objPtr[1] = (ClientData) winPtr->classUid;
	for (topLevPtr = winPtr;
		(topLevPtr != NULL) && !(topLevPtr->flags & TK_TOP_LEVEL);
		topLevPtr = topLevPtr->parentPtr) {
	    /* Empty loop body. */
	}
	if ((winPtr != topLevPtr) && (topLevPtr != NULL)) {
	    count = 4;
	    objPtr[2] = (ClientData) topLevPtr->pathName;
	} else {
	    count = 3;
	}
	if (allUid == NULL) {
	    allUid = Tk_GetUid("all");
	}
	objPtr[count-1] = (ClientData) allUid;
    }
    Tk_BindEvent(winPtr->mainPtr->bindingTable, eventPtr, (Tk_Window) winPtr,
	    count, objPtr);
    if (objPtr != objects) {
	ckfree((char *) objPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_BindtagsCmd --
 *
 *	This procedure is invoked to process the "bindtags" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_BindtagsCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr, *winPtr2;
    int i, tagArgc;
    char *p, **tagArgv;

    if ((argc < 2) || (argc > 3)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" window ?tags?\"", (char *) NULL);
	return TCL_ERROR;
    }
    winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[1], tkwin);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    if (argc == 2) {
	if (winPtr->numTags == 0) {
	    Tcl_AppendElement(interp, winPtr->pathName);
	    Tcl_AppendElement(interp, winPtr->classUid);
	    for (winPtr2 = winPtr;
		    (winPtr2 != NULL) && !(winPtr2->flags & TK_TOP_LEVEL);
		    winPtr2 = winPtr2->parentPtr) {
		/* Empty loop body. */
	    }
	    if ((winPtr != winPtr2) && (winPtr2 != NULL)) {
		Tcl_AppendElement(interp, winPtr2->pathName);
	    }
	    Tcl_AppendElement(interp, "all");
	} else {
	    for (i = 0; i < winPtr->numTags; i++) {
		Tcl_AppendElement(interp, (char *) winPtr->tagPtr[i]);
	    }
	}
	return TCL_OK;
    }
    if (winPtr->tagPtr != NULL) {
	TkFreeBindingTags(winPtr);
    }
    if (argv[2][0] == 0) {
	return TCL_OK;
    }
    if (Tcl_SplitList(interp, argv[2], &tagArgc, &tagArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    winPtr->numTags = tagArgc;
    winPtr->tagPtr = (ClientData *) ckalloc((unsigned)
	    (tagArgc * sizeof(ClientData)));
    for (i = 0; i < tagArgc; i++) {
	p = tagArgv[i];
	if (p[0] == '.') {
	    char *copy;

	    /*
	     * Handle names starting with "." specially: store a malloc'ed
	     * string, rather than a Uid;  at event time we'll look up the
	     * name in the window table and use the corresponding window,
	     * if there is one.
	     */

	    copy = (char *) ckalloc((unsigned) (strlen(p) + 1));
	    strcpy(copy, p);
	    winPtr->tagPtr[i] = (ClientData) copy;
	} else {
	    winPtr->tagPtr[i] = (ClientData) Tk_GetUid(p);
	}
    }
    ckfree((char *) tagArgv);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkFreeBindingTags --
 *
 *	This procedure is called to free all of the binding tags
 *	associated with a window;  typically it is only invoked where
 *	there are window-specific tags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any binding tags for winPtr are freed.
 *
 *----------------------------------------------------------------------
 */

void
TkFreeBindingTags(winPtr)
    TkWindow *winPtr;		/* Window whose tags are to be released. */
{
    int i;
    char *p;

    for (i = 0; i < winPtr->numTags; i++) {
	p = (char *) (winPtr->tagPtr[i]);
	if (*p == '.') {
	    /*
	     * Names starting with "." are malloced rather than Uids, so
	     * they have to be freed.
	     */
    
	    ckfree(p);
	}
    }
    ckfree((char *) winPtr->tagPtr);
    winPtr->numTags = 0;
    winPtr->tagPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_DestroyCmd --
 *
 *	This procedure is invoked to process the "destroy" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_DestroyCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window window;
    Tk_Window tkwin = (Tk_Window) clientData;
    int i;

    for (i = 1; i < argc; i++) {
	window = Tk_NameToWindow(interp, argv[i], tkwin);
	if (window == NULL) {
	    return TCL_ERROR;
	}
	Tk_DestroyWindow(window);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ExitCmd --
 *
 *	This procedure is invoked to process the "exit" Tcl command.
 *	See the user documentation for details on what it does.
 *	Note: this command replaces the Tcl "exit" command in order
 *	to properly destroy all windows.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/*ARGSUSED*/
int
Tk_ExitCmd(clientData, interp, argc, argv)
    ClientData clientData;		/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    int value;

    if ((argc != 1) && (argc != 2)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" ?returnCode?\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (argc == 1) {
	value = 0;
    } else {
	if (Tcl_GetInt(interp, argv[1], &value) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    while (tkMainWindowList != NULL) {
	Tk_DestroyWindow((Tk_Window) tkMainWindowList->winPtr);
    }
    exit(value);
    /* NOTREACHED */
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_LowerCmd --
 *
 *	This procedure is invoked to process the "lower" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_LowerCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window main = (Tk_Window) clientData;
    Tk_Window tkwin, other;

    if ((argc != 2) && (argc != 3)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " window ?belowThis?\"", (char *) NULL);
	return TCL_ERROR;
    }

    tkwin = Tk_NameToWindow(interp, argv[1], main);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (argc == 2) {
	other = NULL;
    } else {
	other = Tk_NameToWindow(interp, argv[2], main);
	if (other == NULL) {
	    return TCL_ERROR;
	}
    }
    if (Tk_RestackWindow(tkwin, Below, other) != TCL_OK) {
	Tcl_AppendResult(interp, "can't lower \"", argv[1], "\" above \"",
		argv[2], "\"", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_RaiseCmd --
 *
 *	This procedure is invoked to process the "raise" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_RaiseCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window main = (Tk_Window) clientData;
    Tk_Window tkwin, other;

    if ((argc != 2) && (argc != 3)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " window ?aboveThis?\"", (char *) NULL);
	return TCL_ERROR;
    }

    tkwin = Tk_NameToWindow(interp, argv[1], main);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }
    if (argc == 2) {
	other = NULL;
    } else {
	other = Tk_NameToWindow(interp, argv[2], main);
	if (other == NULL) {
	    return TCL_ERROR;
	}
    }
    if (Tk_RestackWindow(tkwin, Above, other) != TCL_OK) {
	Tcl_AppendResult(interp, "can't raise \"", argv[1], "\" above \"",
		argv[2], "\"", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_TkCmd --
 *
 *	This procedure is invoked to process the "tk" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_TkCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    char c;
    size_t length;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg?\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'a') && (strncmp(argv[1], "appname", length) == 0)) {
        return Ctk_Unsupported(interp, "tk appname");
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be appname", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_TkwaitCmd --
 *
 *	This procedure is invoked to process the "tkwait" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_TkwaitCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    int c, done;
    size_t length;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " variable|visible|window name\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'v') && (strncmp(argv[1], "variable", length) == 0)
	    && (length >= 2)) {
	if (Tcl_TraceVar(interp, argv[2],
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		WaitVariableProc, (ClientData) &done) != TCL_OK) {
	    return TCL_ERROR;
	}
	done = 0;
	while (!done) {
	    Tk_DoOneEvent(0);
	}
	Tcl_UntraceVar(interp, argv[2],
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		WaitVariableProc, (ClientData) &done);
    } else if ((c == 'v') && (strncmp(argv[1], "visibility", length) == 0)
	    && (length >= 2)) {
	Tk_Window window;

	window = Tk_NameToWindow(interp, argv[2], tkwin);
	if (window == NULL) {
	    return TCL_ERROR;
	}
	Tk_CreateEventHandler(window, CTK_MAP_EVENT_MASK|CTK_DESTROY_EVENT_MASK,
	    WaitWindowProc, (ClientData) &done);
	done = 0;
	while (!done) {
	    Tk_DoOneEvent(0);
	}
	Tk_DeleteEventHandler(window, CTK_MAP_EVENT_MASK|CTK_DESTROY_EVENT_MASK,
	    WaitWindowProc, (ClientData) &done);
    } else if ((c == 'w') && (strncmp(argv[1], "window", length) == 0)) {
	Tk_Window window;

	window = Tk_NameToWindow(interp, argv[2], tkwin);
	if (window == NULL) {
	    return TCL_ERROR;
	}
	Tk_CreateEventHandler(window, CTK_DESTROY_EVENT_MASK,
	    WaitWindowProc, (ClientData) &done);
	done = 0;
	while (!done) {
	    Tk_DoOneEvent(0);
	}
	/*
	 * Note:  there's no need to delete the event handler.  It was
	 * deleted automatically when the window was destroyed.
	 */
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be variable, visibility, or window", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Clear out the interpreter's result, since it may have been set
     * by event handlers.
     */

    Tcl_ResetResult(interp);
    return TCL_OK;
}

	/* ARGSUSED */
static char *
WaitVariableProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Pointer to integer to set to 1. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* Name of variable. */
    char *name2;		/* Second part of variable name. */
    int flags;			/* Information about what happened. */
{
    int *donePtr = (int *) clientData;

    *donePtr = 1;
    return (char *) NULL;
}

	/*ARGSUSED*/
static void
WaitWindowProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to integer to set to 1. */
    XEvent *eventPtr;		/* Information about event. */
{
    int *donePtr = (int *) clientData;
    *donePtr = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_UpdateCmd --
 *
 *	This procedure is invoked to process the "update" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tk_UpdateCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    int flags;

    if (argc == 1) {
	flags = TK_DONT_WAIT;
    } else if (argc == 2) {
	if (strncmp(argv[1], "idletasks", strlen(argv[1])) != 0) {
	    Tcl_AppendResult(interp, "bad argument \"", argv[1],
		    "\": must be idletasks", (char *) NULL);
	    return TCL_ERROR;
	}
	flags = TK_IDLE_EVENTS;
    } else {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " ?idletasks?\"", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Handle all pending events.
     */

    while (Tk_DoOneEvent(flags) != 0) {
	/* Empty loop body */
    }

    /*
     * Must clear the interpreter's result because event handlers could
     * have executed commands.
     */

    Tcl_ResetResult(interp);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_WinfoCmd --
 *
 *	This procedure is invoked to process the "winfo" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_WinfoCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    size_t length;
    char c, *argName;
    Tk_Window window;
    register TkWindow *winPtr;
    int result = TCL_OK;

#define SETUP(name) \
    if (argc != 3) {\
	argName = name; \
	goto wrongArgs; \
    } \
    window = Tk_NameToWindow(interp, argv[2], tkwin); \
    if (window == NULL) { \
	return TCL_ERROR; \
    }

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg?\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'a') && (strcmp(argv[1], "atom") == 0)) {
    	result = Ctk_Unsupported(interp, "winfo atom");
    } else if ((c == 'a') && (strncmp(argv[1], "atomname", length) == 0)
	    && (length >= 5)) {
    	result = Ctk_Unsupported(interp, "winfo atomname");
    } else if ((c == 'c') && (strncmp(argv[1], "cells", length) == 0)
	    && (length >= 2)) {
	Tcl_SetResult(interp,"2", TCL_STATIC);
    } else if ((c == 'c') && (strncmp(argv[1], "children", length) == 0)
	    && (length >= 2)) {
	SETUP("children");
	for (winPtr = Ctk_BottomChild(window); winPtr != NULL;
		winPtr = Ctk_NextSibling(winPtr)) {
	    Tcl_AppendElement(interp, winPtr->pathName);
	}
	if (window->flags & CTK_HAS_TOPLEVEL_CHILD) {
	    /*
	     * This window has toplevel children, which are not stored
	     * in the child list.  Check all the children of all root
	     * windows to see if their name is an extension of this
	     * windows name - if so append path name to result.
	     */
	    char *path = Tk_PathName(window);
	    int length = strlen(path);
	    TkDisplay *dispPtr;
	    char *childPath;
	    int len2;

	    for (dispPtr = tkDisplayList; dispPtr != NULL;
		    dispPtr = dispPtr->nextPtr) {
		for (winPtr = Ctk_BottomChild(dispPtr->rootPtr); winPtr != NULL;
			winPtr = Ctk_NextSibling(winPtr)) {
		    childPath = Tk_PathName(winPtr);
		    if (strncmp(childPath, path, length) == 0) {
			len2 = strrchr(childPath, '.') - childPath;
			if ((length == 1 && len2 == 0 && winPtr != window)
				|| length == len2) {
			    Tcl_AppendElement(interp, childPath);
			}
		    }
		}
	    }
	}
    } else if ((c == 'c') && (strncmp(argv[1], "class", length) == 0)
	    && (length >= 2)) {
	SETUP("class");
	Tcl_SetResult(interp,Tk_Class(window), TCL_VOLATILE);
    } else if ((c == 'c') && (strncmp(argv[1], "colormapfull", length) == 0)
	    && (length >= 3)) {
	Tcl_SetResult(interp,"0", TCL_STATIC);
    } else if ((c == 'c') && (strncmp(argv[1], "containing", length) == 0)
	    && (length >= 2)) {
	/*
	 * This one could be implemented...
	 */
    	result = Ctk_Unsupported(interp, "winfo containing");
    } else if ((c == 'd') && (strncmp(argv[1], "depth", length) == 0)) {
	Tcl_SetResult(interp,"1", TCL_STATIC);
    } else if ((c == 'e') && (strncmp(argv[1], "exists", length) == 0)) {
	if (argc != 3) {
	    argName = "exists";
	    goto wrongArgs;
	}
	winPtr = Tk_NameToWindow(interp, argv[2], tkwin);
	if ((winPtr == (TkWindow *)NULL) || (winPtr->flags & TK_ALREADY_DEAD)) {
	    Tcl_SetResult(interp,"0",TCL_STATIC);
	} else {
	    Tcl_SetResult(interp,"1",TCL_STATIC);
	}
    } else if ((c == 'f') && (strncmp(argv[1], "fpixels", length) == 0)
	    && (length >= 2)) {
	/*
	 * This one could be implemented...
	 */
    	result = Ctk_Unsupported(interp, "winfo fpixels");
    } else if ((c == 'g') && (strncmp(argv[1], "geometry", length) == 0)) {
	SETUP("geometry");
	sprintf(error_buffer, "%dx%d+%d+%d",
		Tk_Width(window), Tk_Height(window),
		Tk_X(window), Tk_Y(window));
	Tcl_SetResult(interp, error_buffer, TCL_VOLATILE);
    } else if ((c == 'h') && (strncmp(argv[1], "height", length) == 0)) {
	SETUP("height");
	sprintf(error_buffer, "%d", Tk_Height(window));
	Tcl_SetResult(interp, error_buffer, TCL_VOLATILE);
    } else if ((c == 'i') && (strcmp(argv[1], "id") == 0)) {
    	result = Ctk_Unsupported(interp, "winfo id");
    } else if ((c == 'i') && (strncmp(argv[1], "interps", length) == 0)
	    && (length >= 2)) {
    	result = Ctk_Unsupported(interp, "winfo interps");
    } else if ((c == 'i') && (strncmp(argv[1], "ismapped", length) == 0)
	    && (length >= 2)) {
	SETUP("ismapped");
	Tcl_SetResult(interp, Tk_IsMapped(window) ? "1" : "0", TCL_STATIC);
    } else if ((c == 'm') && (strncmp(argv[1], "manager", length) == 0)) {
	SETUP("manager");
	winPtr = (TkWindow *) window;
	if (winPtr->geomMgrPtr != NULL) {
	    Tcl_SetResult(interp,winPtr->geomMgrPtr->name, TCL_VOLATILE);
	}
    } else if ((c == 'n') && (strncmp(argv[1], "name", length) == 0)) {
	SETUP("name");
	Tcl_SetResult(interp, Tk_Name(window), TCL_VOLATILE);
    } else if ((c == 'p') && (strncmp(argv[1], "parent", length) == 0)) {
	SETUP("parent");
	winPtr = Ctk_ParentByName(interp, Tk_PathName(window), window);
	if (winPtr) {
	    Tcl_SetResult(interp,winPtr->pathName,TCL_VOLATILE);
	} else {
	    return TCL_ERROR;
	}
    } else if ((c == 'p') && (strncmp(argv[1], "pathname", length) == 0)
	    && (length >= 2)) {
    	result = Ctk_Unsupported(interp, "winfo pathname");
    } else if ((c == 'p') && (strncmp(argv[1], "pixels", length) == 0)
	    && (length >= 2)) {
	int pixels;

	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " pixels window number\"", (char *) NULL);
	    return TCL_ERROR;
	}
	window = Tk_NameToWindow(interp, argv[2], tkwin);
	if (window == NULL) {
	    return TCL_ERROR;
	}
	if (Tk_GetPixels(interp, window, argv[3], &pixels) != TCL_OK) {
	    return TCL_ERROR;
	}
	sprintf(error_buffer, "%d", pixels);
	Tcl_SetResult(interp, error_buffer, TCL_VOLATILE);
    } else if ((c == 'p') && (strcmp(argv[1], "pointerx") == 0)) {
    	result = Ctk_Unsupported(interp, "winfo pointerx");
    } else if ((c == 'p') && (strcmp(argv[1], "pointerxy") == 0)) {
    	result = Ctk_Unsupported(interp, "winfo pointerxy");
    } else if ((c == 'p') && (strcmp(argv[1], "pointery") == 0)) {
    	result = Ctk_Unsupported(interp, "winfo pointery");
    } else if ((c == 'r') && (strncmp(argv[1], "reqheight", length) == 0)
	    && (length >= 4)) {
	SETUP("reqheight");
	sprintf(error_buffer, "%d", Tk_ReqHeight(window));
	Tcl_SetResult(interp, error_buffer, TCL_VOLATILE);
    } else if ((c == 'r') && (strncmp(argv[1], "reqwidth", length) == 0)
	    && (length >= 4)) {
	SETUP("reqwidth");
	sprintf(error_buffer, "%d", Tk_ReqWidth(window));
	Tcl_SetResult(interp, error_buffer, TCL_VOLATILE);
    } else if ((c == 'r') && (strncmp(argv[1], "rgb", length) == 0)
	    && (length >= 2)) {
    	result = Ctk_Unsupported(interp, "winfo rgb");
    } else if ((c == 'r') && (strcmp(argv[1], "rootx") == 0)) {
	SETUP("rootx");
	sprintf(error_buffer, "%d", Ctk_AbsLeft(window));
	Tcl_SetResult(interp, error_buffer, TCL_VOLATILE);
    } else if ((c == 'r') && (strcmp(argv[1], "rooty") == 0)) {
	SETUP("rooty");
	sprintf(error_buffer, "%d", Ctk_AbsTop(window));
	Tcl_SetResult(interp, error_buffer, TCL_VOLATILE);
    } else if ((c == 's') && (strcmp(argv[1], "screen") == 0)) {
	SETUP("screen");
	Tcl_AppendResult(interp, Tk_Display(window)->name, ".", (char *) NULL);
    } else if ((c == 's') && (strncmp(argv[1], "screencells", length) == 0)
	    && (length >= 7)) {
    	Tcl_SetResult(interp,"2",TCL_STATIC);
    } else if ((c == 's') && (strncmp(argv[1], "screendepth", length) == 0)
	    && (length >= 7)) {
    	Tcl_SetResult(interp,"1",TCL_STATIC);
    } else if ((c == 's') && (strncmp(argv[1], "screenheight", length) == 0)
	    && (length >= 7)) {
	SETUP("screenheight");
	sprintf(error_buffer, "%d", Ctk_DisplayHeight(Tk_Display(window)));
	Tcl_SetResult(interp,error_buffer,TCL_VOLATILE);
    } else if ((c == 's') && (strncmp(argv[1], "screenmmheight", length) == 0)
	    && (length >= 9)) {
    	result = Ctk_Unsupported(interp, "winfo screenmmheight");
    } else if ((c == 's') && (strncmp(argv[1], "screenmmwidth", length) == 0)
	    && (length >= 9)) {
    	result = Ctk_Unsupported(interp, "winfo screenmmheight");
    } else if ((c == 's') && (strncmp(argv[1], "screenvisual", length) == 0)
	    && (length >= 7)) {
	Tcl_SetResult(interp,"staticgray",TCL_STATIC);
    } else if ((c == 's') && (strncmp(argv[1], "screenwidth", length) == 0)
	    && (length >= 7)) {
	SETUP("screenwidth");
	sprintf(error_buffer, "%d", Ctk_DisplayWidth(Tk_Display(window)));
	Tcl_SetResult(interp,error_buffer,TCL_VOLATILE);
    } else if ((c == 's') && (strncmp(argv[1], "server", length) == 0)
	    && (length >= 2)) {
    	result = Ctk_Unsupported(interp, "winfo server");
    } else if ((c == 't') && (strncmp(argv[1], "toplevel", length) == 0)) {
	SETUP("toplevel");
	Tcl_SetResult(interp,Tk_PathName(Ctk_TopLevel(window)), TCL_STATIC);
    } else if ((c == 'v') && (strncmp(argv[1], "viewable", length) == 0)
	    && (length >= 3)) {
	SETUP("viewable");
	Tcl_SetResult(interp,(window->flags & CTK_DISPLAYED) ? "1" : "0",
                      TCL_STATIC);
    } else if ((c == 'v') && (strncmp(argv[1], "visual", length) == 0)) {
	Tcl_SetResult(interp,"staticgray",TCL_STATIC);
    } else if ((c == 'v') && (strncmp(argv[1], "visualsavailable", length) == 0)
	    && (length >= 7)) {
	Tcl_SetResult(interp,"staticgray 1",TCL_STATIC);
    } else if ((c == 'v') && (strncmp(argv[1], "vrootheight", length) == 0)
	    && (length >= 6)) {
	SETUP("vrootheight");
	sprintf(error_buffer, "%d", Ctk_DisplayHeight(Tk_Display(window)));
	Tcl_SetResult(interp,error_buffer,TCL_VOLATILE);
    } else if ((c == 'v') && (strncmp(argv[1], "vrootwidth", length) == 0)
	    && (length >= 6)) {
	SETUP("vrootwidth");
	sprintf(error_buffer, "%d", Ctk_DisplayWidth(Tk_Display(window)));
	Tcl_SetResult(interp,error_buffer,TCL_VOLATILE);
    } else if ((c == 'v') && (strcmp(argv[1], "vrootx") == 0)) {
	Tcl_SetResult(interp,"0",TCL_STATIC);
    } else if ((c == 'v') && (strcmp(argv[1], "vrooty") == 0)) {
	Tcl_SetResult(interp,"0",TCL_STATIC);
    } else if ((c == 'w') && (strncmp(argv[1], "width", length) == 0)) {
	SETUP("width");
	sprintf(error_buffer, "%d", Tk_Width(window));
	Tcl_SetResult(interp,error_buffer,TCL_VOLATILE);
    } else if ((c == 'x') && (argv[1][1] == '\0')) {
	SETUP("x");
	sprintf(error_buffer, "%d", Tk_X(window));
	Tcl_SetResult(interp,error_buffer,TCL_VOLATILE);
    } else if ((c == 'y') && (argv[1][1] == '\0')) {
	SETUP("y");
	sprintf(error_buffer, "%d", Tk_Y(window));
	Tcl_SetResult(interp,error_buffer,TCL_VOLATILE);
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be atom, atomname, cells, children, ",
		"class, colormapfull, containing, depth, exists, fpixels, ",
		"geometry, height, ",
		"id, interps, ismapped, manager, name, parent, pathname, ",
		"pixels, pointerx, pointerxy, pointery, reqheight, ",
		"reqwidth, rgb, ",
		"rootx, rooty, ",
		"screen, screencells, screendepth, screenheight, ",
		"screenmmheight, screenmmwidth, screenvisual, ",
		"screenwidth, server, ",
		"toplevel, viewable, visual, visualsavailable, ",
		"vrootheight, vrootwidth, vrootx, vrooty, ",
		"width, x, or y", (char *) NULL);
	return TCL_ERROR;
    }
    return result;

    wrongArgs:
    Tcl_AppendResult(interp, "wrong # arguments: must be \"",
	    argv[0], " ", argName, " window\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TkDeadAppCmd --
 *
 *	If an application has been deleted then all Tk commands will be
 *	re-bound to this procedure.
 *
 * Results:
 *	A standard Tcl error is reported to let the user know that
 *	the application is dead.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TkDeadAppCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Dummy. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tcl_AppendResult(interp, "can't invoke \"", argv[0],
	    "\" command:  application has been destroyed", (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Ctk_TkFocusNextCmd --
 *
 *	Get the next window in "focus order" after specified window
 *	(the window that should receive the focus next if Tab is typed).
 *	"Next" is defined by a pre-order search of a top-level and its
 *	non-top-level descendants, with the stacking order determining
 *	the order of siblings.  The "-takefocus" options on windows
 *	determine whether or not they should be skipped.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May execute arbitrary commands specified by the "-takefocus"
 *	options of the widgets.
 *
 *----------------------------------------------------------------------
 */

int
Ctk_TkFocusNextCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window mainWin = (Tk_Window) clientData;
    Tk_Window win, startWin, nextWin;
    int flag;

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " window\"", (char *) NULL);
	return TCL_ERROR;
    }
    startWin = Tk_NameToWindow(interp, argv[1], mainWin);
    if (!startWin)  return TCL_ERROR;

    win = startWin;
    do {
	/*
	 * First try to traverse to first child.  If that fails,
	 * find the first ancestor that has a next sibling and
	 * traverse to that sibling.  If the top-level is reached,
	 * stop there.
	 */

	nextWin = Ctk_BottomChild(win);
	while (!nextWin) {
	    if (Tk_IsTopLevel(win))  goto gotit;
	    nextWin = Ctk_NextSibling(win);
	    win = Tk_Parent(win);
	}
	win = nextWin;

gotit:
	/*
	 * Stop traversing if we have gone full circle or
	 * this window can get the focus.
	 */

	if (win == startWin) break;
	if (GetFocusOk(interp, win, &flag) != TCL_OK)  return TCL_ERROR;
    } while (!flag);
    Tcl_SetResult(interp, Tk_PathName(win), TCL_STATIC);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Ctk_TkFocusPrevCmd --
 *
 *	Get the previous window in "focus order" before specified window
 *	(the window that should receive the focus next if Shift-Tab is
 *	typed).  "Previous" is defined by a pre-order search of a top-level
 *	and its non-top-level descendants, with the stacking order
 *	determining the order of siblings.  The "-takefocus" options
 *	on windows determine whether or not they should be skipped.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May execute arbitrary commands specified by the "-takefocus"
 *	options of the widgets.
 *
 *----------------------------------------------------------------------
 */

int
Ctk_TkFocusPrevCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window mainWin = (Tk_Window) clientData;
    Tk_Window win, startWin, nextWin;
    int flag;

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " window\"", (char *) NULL);
	return TCL_ERROR;
    }
    startWin = Tk_NameToWindow(interp, argv[1], mainWin);
    if (!startWin)  return TCL_ERROR;

    win = startWin;
    do {
	/*
	 * If window is a top-level, repeatedly traverse to topmost
	 * (last) children till a leaf is reached.  Otherwise, to
	 * prior sibling and then traverse to topmost descendant.
	 * If there is no prior sibling (and this is not a top-level).
	 * Traverse to parent.
	 */

	if (Tk_IsTopLevel(win)) {
	    nextWin = win;
	} else {
	    nextWin = Ctk_PriorSibling(win);
	    if (!nextWin)  win = Tk_Parent(win);
	}

	/*
	 * Stop traversing if we have gone full circle or
	 * this window can get the focus.
	 */

	while (nextWin) {
	    win = nextWin;
	    nextWin = Ctk_TopChild(win);
	}
	if (win == startWin) break;
	if (GetFocusOk(interp, win, &flag) != TCL_OK)  return TCL_ERROR;
    } while (!flag);
    Tcl_SetResult(interp, Tk_PathName(win), TCL_STATIC);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetFocusOk --
 *
 *	Get the previous window in "focus order" before specified window
 *	(the window that should receive the focus next if Shift-Tab is
 *	typed).  "Previous" is defined by a pre-order search of a top-level
 *	and its non-top-level descendants, with the stacking order
 *	determining the order of siblings.  The "-takefocus" options
 *	on windows determine whether or not they should be skipped.
 *
 * Results:
 *	If succesful, returns TCL_OK and stores 1 in *flagPtr if
 *	the window should get the focus and 0 if it shouldn't.  If
 *	an error occurs while trying to determine focusability,
 *	returns TCL_ERROR and stores an error message in the interpreter
 *	result.
 *
 * Side effects:
 *	May execute an arbitrary command specified by the "-takefocus"
 *	options of the widget.
 *
 *----------------------------------------------------------------------
 */

static int
GetFocusOk(interp, winPtr, flagPtr)
    Tcl_Interp *interp;
    TkWindow *winPtr;
    int *flagPtr;
{
    /*
     * If window is not viewable, don't focus.
     */

    if (! (winPtr->flags & CTK_DISPLAYED))  goto nofocus;

    /*
     * Check widget's -takefocus option.
     */

    if (Tcl_VarEval(interp, Tk_PathName(winPtr), " cget -takefocus",
	    (char *) NULL) == TCL_OK  && Tcl_GetStringResult(interp)[0] != '\0') {
	
	/*
	 * Try to interpret option value as simple 1 or 0.
	 */

	if (Tcl_GetStringResult(interp)[1] == '\0') {
	    if (Tcl_GetStringResult(interp)[0] == '1') {
		goto focus;
	    } else if (Tcl_GetStringResult(interp)[0] == '0') {
		goto nofocus;
	    }
	}
	{
	
	    /*
	     * The -takefocus option is not 1 or 0, append window
	     * pathname to the option value and evaluate as script.
	     * Interpret result as boolean.
	     */

	    Tcl_DString dStr;
	    int result;

	    Tcl_DStringInit(&dStr);
	    Tcl_DStringGetResult(interp, &dStr);
	    Tcl_DStringAppend(&dStr, " ", 1);
	    Tcl_DStringAppend(&dStr, Tk_PathName(winPtr), -1);
	    Tcl_GlobalEval(interp, Tcl_DStringValue(&dStr));

	    Tcl_DStringGetResult(interp, &dStr);
	    result = Tcl_GetBoolean(interp, Tcl_DStringValue(&dStr), flagPtr);
	    Tcl_DStringFree(&dStr);
	    if (result == TCL_ERROR) {
		Tcl_AddErrorInfo(interp, "\n    (-takefocus script)");
	    }
	    return result;
	}
    }

    /*
     * Check widget's -state option.  If value is "disaabled",
     * don't focus.
     */

    if (Tcl_VarEval(interp, Tk_PathName(winPtr), " cget -state",
	    (char *) NULL) == TCL_OK) {
	if (Tcl_GetStringResult(interp)[0] == 'd'
		&& strcmp(Tcl_GetStringResult(interp), "disabled") == 0) goto nofocus;
    }

    /*
     * Check if widget has any Keyboard related bindings (check
     * individual widget tag and its class tag).
     */

    if (Tcl_VarEval(interp, "bind ", Tk_PathName(winPtr), (char *) NULL)
	    != TCL_OK)  return TCL_ERROR;
    if (strstr(Tcl_GetStringResult(interp), "Key"))  goto focus;
    if (strstr(Tcl_GetStringResult(interp), "Focus"))  goto focus;

    if (Tcl_VarEval(interp, "bind ", Tk_Class(winPtr), (char *) NULL)
	    != TCL_OK)  return TCL_ERROR;
    if (strstr(Tcl_GetStringResult(interp), "Key"))  goto focus;
    if (strstr(Tcl_GetStringResult(interp), "Focus"))  goto focus;

nofocus:
    *flagPtr = 0;
    return TCL_OK;

focus:
    *flagPtr = 1;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Ctk_CtkCmd --
 *
 *	This procedure is invoked to process the "ctk" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Ctk_CtkCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window mainWin = (Tk_Window) clientData;
    char c;
    size_t length;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg?\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'r') && (strncmp(argv[1], "redraw", length) == 0)) {
	Tk_Window tkwin;
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " redraw window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	tkwin = Tk_NameToWindow(interp, argv[2], mainWin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
        Ctk_DisplayRedraw(Tk_Display(tkwin));
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be redraw", (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_TkEntryInsertCmd --
 *
 *	This procedure is invoked to process the "tkEntryInsert"
 *	Tcl command.  Insert a string into an entry at the point
 *	of the insertion cursor.  If there is a selection in the
 *	entry, and it covers the point of the insertion cursor,
 *	then delete the selection before inserting.
 *
 *	First thought about letting this function use the entry
 *	widget internals - but that would not work with the
 *	object widget systems (like mine, and [incr Tk]).
 *
 *	The payoff for these entry commands is not nearly
 *	as high as the focus processing ones above.  Is it
 * 	worth it?
 *
 * Results:
 *	A standard Tcl result.  Sets result to "1" if characters
 *	are inserted, and "0" otherwise.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Ctk_TkEntryInsertCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tcl_DString dStr;
    Tcl_CmdInfo cmdInfo;
    char *widgetArgv[5];
    int insert, first, last;
    int result = TCL_ERROR;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " window string\"", (char *) NULL);
	return TCL_ERROR;
    }
    if (!Tcl_GetCommandInfo(interp, argv[1], &cmdInfo)) {
	Tcl_AppendResult(interp, "widget command \"", argv[1],
		"\" is not defined", (char *) NULL);
	return TCL_ERROR;
    }
    if (argv[2][0] == '\0') {
	Tcl_SetResult(interp, "0", TCL_STATIC);
	return TCL_OK;
    }
    Tcl_DStringInit(&dStr);
    widgetArgv[0] = argv[1];

    /*
     * Check if insertion point is in the selection region.
     */

    widgetArgv[1] = "index";
    widgetArgv[2] = "insert";
    widgetArgv[3] = NULL;
    if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 3, widgetArgv) ) goto doit;
    Tcl_DStringGetResult(interp, &dStr);
    if (Tcl_GetInt(interp, Tcl_DStringValue(&dStr), &insert))  goto done;

    widgetArgv[1] = "index";
    widgetArgv[2] = "sel.first";
    widgetArgv[3] = NULL;
    if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 3, widgetArgv) ) goto doit;
    Tcl_DStringGetResult(interp, &dStr);
    if (Tcl_GetInt(interp, Tcl_DStringValue(&dStr), &first))  goto done;

    if (first <= insert) {
	widgetArgv[1] = "index";
	widgetArgv[2] = "sel.last";
	widgetArgv[3] = NULL;
	if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 3, widgetArgv) )
		goto doit;
	Tcl_DStringGetResult(interp, &dStr);
	if (Tcl_GetInt(interp, Tcl_DStringValue(&dStr), &last))  goto done;

	if (last >= insert) {
	    widgetArgv[1] = "delete";
	    widgetArgv[2] = "sel.first";
	    widgetArgv[3] = "sel.last";
	    widgetArgv[4] = NULL;
	    if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 4, widgetArgv) )
		    goto doit;
	}
    }

    /*
     * Perform the insertion, then update view to contain the new
     * insertion point.
     */

doit:
    widgetArgv[1] = "insert";
    widgetArgv[2] = "insert";
    widgetArgv[3] = argv[2];
    widgetArgv[4] = NULL;
    Tcl_ResetResult(interp);
    if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 4, widgetArgv) ) goto done;

    widgetArgv[1] = argv[1];
    widgetArgv[2] = NULL;
    Tcl_ResetResult(interp);
    result = Ctk_TkEntrySeeInsertCmd(clientData, interp, 2, widgetArgv);
    if (result == TCL_OK) {
	Tcl_SetResult(interp, "1", TCL_STATIC);
    }

done:
    Tcl_DStringFree(&dStr);
    return result;
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_TkEntrySeeInsertCmd --
 *
 *	This procedure is invoked to process the "tkEntrySeeInsert"
 *	Tcl command.  Makes sure that the insertion cursor is
 *	visible in the entry window.  If not, adjust the view so
 *	that it is.
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
Ctk_TkEntrySeeInsertCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window mainWin = (Tk_Window) clientData;
    Tk_Window tkwin;
    Tcl_DString dStr;
    int result = TCL_ERROR;
    Tcl_CmdInfo cmdInfo;
    char *widgetArgv[4];
    int c, left, x, i;
    char buf[50];

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " window\"", (char *) NULL);
	return TCL_ERROR;
    }
    tkwin = Tk_NameToWindow(interp, argv[1], mainWin);
    if (!tkwin)  return TCL_ERROR;
    if (!Tcl_GetCommandInfo(interp, argv[1], &cmdInfo)) {
	Tcl_AppendResult(interp, "widget command \"", argv[1],
		"\" is not defined", (char *) NULL);
	return TCL_ERROR;
    }
    Tcl_DStringInit(&dStr);
    widgetArgv[0] = argv[1];
    widgetArgv[3] = NULL;

    widgetArgv[1] = "index";
    widgetArgv[2] = "insert";
    if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 3, widgetArgv) ) goto done;
    Tcl_DStringGetResult(interp, &dStr);
    if (Tcl_GetInt(interp, Tcl_DStringValue(&dStr), &c))  goto done;

    widgetArgv[1] = "index";
    widgetArgv[2] = "@0";
    if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 3, widgetArgv) ) goto done;
    Tcl_DStringGetResult(interp, &dStr);
    if (Tcl_GetInt(interp, Tcl_DStringValue(&dStr), &left))  goto done;

    if (left > c) {
	sprintf(buf, "%d", c);
	widgetArgv[1] = "xview";
	widgetArgv[2] = buf;
	result = (cmdInfo.proc)(cmdInfo.clientData, interp, 3, widgetArgv);
	goto done;
    }

    x = Tk_Width(tkwin);
    while (1) {
	sprintf(buf, "@%d", x);
	widgetArgv[1] = "index";
	widgetArgv[2] = buf;
	if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 3, widgetArgv) )
		goto done;
	Tcl_DStringGetResult(interp, &dStr);
	if (Tcl_GetInt(interp, Tcl_DStringValue(&dStr), &i))  goto done;
	if (i > c || left >= c)  break;

	left++;
	sprintf(buf, "%d", left);
	widgetArgv[1] = "xview";
	widgetArgv[2] = buf;
	if ( (cmdInfo.proc)(cmdInfo.clientData, interp, 3, widgetArgv) )
		goto done;
	Tcl_ResetResult(interp);
    }
    result = TCL_OK;
done:
    Tcl_DStringFree(&dStr);
    return result;
}
