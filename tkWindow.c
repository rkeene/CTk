/*
 * tkWindow.c (CTk) --
 *
 *	CTk window manipulation functions.
 *
 * Copyright (c) 1989-1994 The Regents of the University of California.
 * Copyright (c) 1994-1995 Sun Microsystems, Inc.
 * Copyright (c) 1994-1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

static char rcsid[] = "@(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $";


#include "tkPort.h"
#include "tkInt.h"
#include "patchlevel.h"


#define HEAD_CHILD(winPtr)	((TkWindow *) &((winPtr)->childList))
#define TOP_CHILD(winPtr)	((winPtr)->childList.priorPtr)
#define BOTTOM_CHILD(winPtr)	((winPtr)->childList.nextPtr)

/*
 * Count of number of main windows currently open in this process.
 */

int tk_NumMainWindows;

/*
 * First in list of all main windows managed by this process.
 */

TkMainInfo *tkMainWindowList = NULL;

/*
 * List of all displays currently in use.
 */

TkDisplay *tkDisplayList = NULL;

/*
 * Have statics in this module been initialized?
 */

static int initialized = 0;

/*
 * The variables below hold several uid's that are used in many places
 * in the toolkit.
 */

Tk_Uid tkDisabledUid = NULL;
Tk_Uid tkActiveUid = NULL;
Tk_Uid tkNormalUid = NULL;

/*
 * The following structure defines all of the commands supported by
 * CTk, and the C procedures that execute them.
 */

typedef struct {
    char *name;			/* Name of command. */
    int (*cmdProc) _ANSI_ARGS_((ClientData clientData, Tcl_Interp *interp,
	    int argc, char **argv));
				/* Command procedure. */
} TkCmd;

static TkCmd commands[] = {
    /*
     * Commands that are part of the intrinsics:
     */

    {"bell",		Tk_BellCmd},
    {"bind",		Tk_BindCmd},
    {"bindtags",	Tk_BindtagsCmd},
    /* {"clipboard",		Tk_ClipboardCmd}, */
    {"ctk",		Ctk_CtkCmd},
    {"ctk_event",	Ctk_CtkEventCmd},
    {"destroy",		Tk_DestroyCmd},
    {"exit",		Tk_ExitCmd},
    {"focus",		Tk_FocusCmd},
    /* {"grab",		Tk_GrabCmd}, */
    /* {"image",		Tk_ImageCmd}, */
    {"lower",		Tk_LowerCmd},
    {"option",		Tk_OptionCmd},
    {"pack",		Tk_PackCmd},
    {"place",		Tk_PlaceCmd},
    {"raise",		Tk_RaiseCmd},
    /* {"selection",	Tk_SelectionCmd}, */
    {"tk",		Tk_TkCmd},
    {"tk_focusNext",	Ctk_TkFocusNextCmd},
    {"tk_focusPrev",	Ctk_TkFocusPrevCmd},
    {"tkEntryInsert", Ctk_TkEntryInsertCmd},
    {"tkEntrySeeInsert", Ctk_TkEntrySeeInsertCmd},
    {"tkwait",		Tk_TkwaitCmd},
    {"update",		Tk_UpdateCmd},
    {"winfo",		Tk_WinfoCmd},
    /* {"wm",		Tk_WmCmd}, */

    /*
     * Widget-creation commands.
     */
    {"button",		Tk_ButtonCmd},
    /* {"canvas",		Tk_CanvasCmd}, */
    {"checkbutton",	Tk_CheckbuttonCmd},
    {"entry",		Tk_EntryCmd},
    {"frame",		Tk_FrameCmd},
    {"label",		Tk_LabelCmd},
    {"listbox",		Tk_ListboxCmd},
    {"menu",		Tk_MenuCmd},
    {"menubutton",	Tk_MenubuttonCmd},
    /* {"message",		Tk_MessageCmd}, */
    {"radiobutton",	Tk_RadiobuttonCmd},
    /* {"scale",		Tk_ScaleCmd}, */
    {"scrollbar",	Tk_ScrollbarCmd},
    {"text",		Tk_TextCmd},
    {"toplevel",	Tk_FrameCmd},
    {(char *) NULL,	(int (*)()) NULL}
};

/*
 * Forward declarations to procedures defined later in this file:
 */

static TkDisplay *	GetScreen _ANSI_ARGS_((Tcl_Interp *interp,
			    char *screenName));
static TkWindow *	CreateRoot _ANSI_ARGS_((Tcl_Interp *interp,
			    TkDisplay *dispPtr));
static TkWindow *	NewWindow _ANSI_ARGS_((TkDisplay *dispPtr));
static void		DisplayWindow _ANSI_ARGS_((TkWindow *winPtr));
static void		UndisplayWindow _ANSI_ARGS_((TkWindow *winPtr));
static void		InsertWindow _ANSI_ARGS_((TkWindow *winPtr,
			    TkWindow *sibling));
static void		UnlinkWindow _ANSI_ARGS_((TkWindow *winPtr));
static void		Unoverlap _ANSI_ARGS_((TkWindow *underPtr,
			    TkWindow *overPtr));
static void		UnoverlapHierarchy _ANSI_ARGS_((TkWindow *underPtr,
			    TkWindow * overPtr));
static void		ExposeWindow _ANSI_ARGS_((TkWindow *winPtr,
			    CtkRegion *rgn));
static void		ComputeClipRect _ANSI_ARGS_((TkWindow *winPtr));



/*
 *----------------------------------------------------------------------
 *
 * Tk_Init --
 *
 *	This procedure is typically invoked by Tcl_AppInit procedures
 *	to perform additional Tk initialization for a Tcl interpreter,
 *	such as sourcing the "ctk.tcl" script.
 *
 * Results:
 *	Returns a standard Tcl completion code and sets interp->result
 *	if there is an error.
 *
 * Side effects:
 *	Depends on what's in the ctk.tcl script.
 *
 *----------------------------------------------------------------------
 */

int
Tk_Init(interp)
    Tcl_Interp *interp;		/* Interpreter to initialize. */
{
#ifdef USE_TCL_STUBS
    Tk_Window winPtr;
#endif
    static char initCmd[] =
	"if [file exists $tk_library/ctk.tcl] {\n\
	    source $tk_library/ctk.tcl\n\
	} else {\n\
	    set msg \"can't find $tk_library/ctk.tcl; perhaps you \"\n\
	    append msg \"need to\\ninstall CTk or set your CTK_LIBRARY \"\n\
	    append msg \"environment variable?\"\n\
	    error $msg\n\
	}";
    int retval;

#ifdef USE_TCL_STUBS
    Tcl_InitStubs(interp, "8", 0);
#endif

#ifdef USE_TCL_STUBS
    winPtr = Tk_CreateMainWindow(interp, NULL, "ctk", "ctk");
    if (winPtr == NULL) {
        return(TCL_ERROR);
    }
#endif

    retval = Tcl_Eval(interp, initCmd);
    if (retval != TCL_OK) {
        return(retval);
    }

#ifdef USE_TCL_STUBS
    Tcl_SetMainLoop(Tk_MainLoop);
#endif

    return(retval);
}

/*
 *----------------------------------------------------------------------
 *
 * GetScreen --
 *
 *	Given a string name for a terminal device-plus-type, find the
 *	TkDisplay structure for the display.
 *
 * Results:
 *	The return value is a pointer to information about the display,
 *	or NULL if the display couldn't be opened.  In this case, an
 *	error message is left in interp->result.
 *
 * Side effects:
 *	A new stream is opened to the device if there is no
 *	connection already.  A new TkDisplay data structure is also
 *	setup, if necessary.
 *
 *----------------------------------------------------------------------
 */

static TkDisplay *
GetScreen(interp, screenName)
    Tcl_Interp *interp;		/* Place to leave error message. */
    char *screenName;		/* Name for screen.  NULL or empty means
				 * use CTK_DISPLAY environment variable. */
{
    register TkDisplay *dispPtr;
    char *p;
    size_t length;

    /*
     * Separate the terminal type from the rest of the display
     * name.  ScreenName is assumed to have the syntax
     * <device>:<type> with the colon and the type being
     * optional.
     */

    if (screenName == NULL || screenName[0] == '\0') {
	screenName = Tcl_GetVar2(interp, "env", "CTK_DISPLAY", TCL_GLOBAL_ONLY);
	if (screenName == NULL) {
	    /*
	     * For backwards compatibility, check CWISH_DISPLAY -
	     * this feature will eventually be removed.
	     */
	    screenName = Tcl_GetVar2(interp, "env", "CWISH_DISPLAY",
	    	    TCL_GLOBAL_ONLY);
	}
	if (screenName == NULL) {
	    screenName = "tty";
	}
    }
    p = strchr(screenName, ':');
    if (p == NULL) {
	length = strlen(screenName);
    } else {
	length = p - screenName;
    }

    /*
     * See if we already have a connection to this display.
     */

    for (dispPtr = tkDisplayList; dispPtr != NULL; dispPtr = dispPtr->nextPtr) {
	if ((strncmp(dispPtr->name, screenName, length) == 0)
		&& (dispPtr->name[length] == '\0')) {
	    return dispPtr;
	}
    }

    /*
     * Create entry for new display.
     */

    dispPtr = (TkDisplay *) ckalloc(sizeof(TkDisplay));
    if (CtkDisplayInit(interp, dispPtr, screenName) != TCL_OK) {
	return (TkDisplay *) NULL;
    }
    dispPtr->numWindows = 0;
    dispPtr->rootPtr = CreateRoot(interp, dispPtr);
    if (dispPtr->rootPtr == NULL) {
    	CtkDisplayEnd(dispPtr);
	return (TkDisplay *) NULL;
    }
    dispPtr->focusPtr = dispPtr->rootPtr;
    dispPtr->cursorPtr = dispPtr->rootPtr;
    dispPtr->cursorX = 0;
    dispPtr->cursorY = 0;
    dispPtr->nextPtr = tkDisplayList;
    tkDisplayList = dispPtr;

    return dispPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateMainWindow --
 *
 *	Make a new main window.  A main window is a special kind of
 *	top-level window used as the outermost window in an
 *	application.
 *
 * Results:
 *	The return value is a token for the new window, or NULL if
 *	an error prevented the new window from being created.  If
 *	NULL is returned, an error message will be left in
 *	interp->result.
 *
 * Side effects:
 *	A new window structure is allocated locally;  "interp" is
 *	associated with the window and registered for "send" commands
 *	under "baseName".  BaseName may be extended with an instance
 *	number in the form "#2" if necessary to make it globally
 *	unique.  Tk-related commands are bound into interp.  The main
 *	window becomes a "toplevel" widget and its X window will be
 *	created and mapped as an idle handler.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_CreateMainWindow(interp, screenName, baseName, className)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. */
    char *screenName;		/* "device:term-type" on which to create
				 * window.  Empty or NULL string means
				 * use stdin/stdout. */
    char *baseName;		/* Base name for application;  usually of the
				 * form "prog instance". */
    char *className;		/* Class to use for application (same as class
				 * for main window). */
{
    int dummy;
    Tcl_HashEntry *hPtr;
    register TkMainInfo *mainPtr;
    register TkWindow *winPtr;
    register TkDisplay *dispPtr;
    register TkCmd *cmdPtr;
    char *libDir;
    char *argv[1];

    if (!initialized) {
    	initialized = 1;
	tkNormalUid = Tk_GetUid("normal");
	tkDisabledUid = Tk_GetUid("disabled");
	tkActiveUid = Tk_GetUid("active");
    }

    /*
     * Create the TkMainInfo structure for this application.
     */

    mainPtr = (TkMainInfo *) ckalloc(sizeof(TkMainInfo));
    mainPtr->winPtr = NULL;
    mainPtr->refCount = 0;
    mainPtr->interp = interp;
    Tcl_InitHashTable(&mainPtr->nameTable, TCL_STRING_KEYS);
    mainPtr->bindingTable = Tk_CreateBindingTable(interp);
    mainPtr->curDispPtr = NULL;
    mainPtr->bindingDepth = 0;
    mainPtr->optionRootPtr = NULL;
    mainPtr->nextPtr = tkMainWindowList;
    tkMainWindowList = mainPtr;

    /*
     * Create the basic TkWindow structure.
     *
     * Temporarily put root window into the application's name table
     * and set root windows mainPtr to the new main structure,
     * so that Tk_TopLevelCmd() will use the new main structure for
     * the window it creates.
     */

    if (screenName == (char *) NULL) {
	screenName = "";
    }
    dispPtr = GetScreen(interp, screenName);
    if (dispPtr == NULL) {
	return (Tk_Window) NULL;
    }
    dispPtr->rootPtr->mainPtr = mainPtr;
    hPtr = Tcl_CreateHashEntry(&mainPtr->nameTable, "", &dummy);
    Tcl_SetHashValue(hPtr, dispPtr->rootPtr);
    winPtr = Tk_CreateWindowFromPath(interp, dispPtr->rootPtr, ".", screenName);
    Tcl_DeleteHashEntry(hPtr);
    dispPtr->rootPtr->mainPtr = NULL;
    if (winPtr == NULL) {
	return (Tk_Window) NULL;
    }
    mainPtr->winPtr = winPtr;

    /*
     * Bind in Tk's commands.
     */

    for (cmdPtr = commands; cmdPtr->name != NULL; cmdPtr++) {
	Tcl_CreateCommand(interp, cmdPtr->name, cmdPtr->cmdProc,
		(ClientData) winPtr, (void (*)()) NULL);
    }

    /*
     * Set variables for the intepreter.
     */

    if (Tcl_GetVar(interp, "tk_library", TCL_GLOBAL_ONLY) == NULL) {
	/*
	 * A library directory hasn't already been set, so figure out
	 * which one to use.
	 */

	libDir = getenv("CTK_LIBRARY");
	if (libDir == NULL) {
	    libDir = CTK_LIBRARY;
	}
	Tcl_SetVar(interp, "tk_library", libDir, TCL_GLOBAL_ONLY);
    }
    Tcl_SetVar(interp, "ctk_patchLevel", CTK_PATCH_LEVEL, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tk_version", TK_VERSION, TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tk_port", "curses", TCL_GLOBAL_ONLY);

    /*
     * Make the main window into a toplevel widget, and give it an initial
     * requested size.
     */
    
    Tk_SetClass(winPtr, className);
    argv[0] = NULL;
    if (TkInitFrame(interp, winPtr, 1, 0, argv) == NULL) {
	return NULL;
    }
    Tk_GeometryRequest(winPtr, 20, 10);

    CtkSetFocus(winPtr);
    tk_NumMainWindows++;
    return winPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Ctk_Unsupported --
 *
 *	This procedure is invoked when a Tk feature that is not
 *	supported by CTk is requested.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	Sets the interpreter result, if "ctk_unsupported" is defined,
 *	could do anything.
 *
 *----------------------------------------------------------------------
 */

int
Ctk_Unsupported(interp, feature)
    Tcl_Interp *interp;		/* Interpreter in which unsupported
    				 * feature has been requested. */
    char *feature;		/* Description of requested feature. */
{
    Tcl_CmdInfo info;
    char *argv[3];

    Tcl_ResetResult(interp);
    if (Tcl_GetCommandInfo(interp, "ctk_unsupported", &info)) {
	argv[0] = "ctk_unsupported";
	argv[1] = feature;
	argv[2] = NULL;
	return (*info.proc)(info.clientData, interp, 2, argv);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_CreateWindowFromPath --
 *
 *	This procedure is similar to Tk_CreateWindow except that
 *	it uses a path name to create the window, rather than a
 *	parent and a child name.
 *
 * Results:
 *	The return value is a token for the new window.  If an error
 *	occurred in creating the window (e.g. no such display or
 *	screen), then an error message is left in interp->result and
 *	NULL is returned.
 *
 * Side effects:
 *	A new window structure is allocated.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_CreateWindowFromPath(interp, winPtr, pathName, screenName)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting.
				 * Interp->result is assumed to be
				 * initialized by the caller. */
    TkWindow *winPtr;		/* Token for any window in application
				 * that is to contain new window. */
    char *pathName;		/* Path name for new window within the
				 * application of tkwin.  The parent of
				 * this window must already exist, but
				 * the window itself must not exist. */
    char *screenName;		/* If NULL, new window will be on same
				 * screen as its parent.  If non-NULL,
				 * gives name of screen on which to create
				 * new window;  window will be a top-level
				 * window. */
{
    TkWindow *parentPtr;
    TkDisplay *dispPtr;
    Tcl_HashEntry *hPtr;
    int new;
    char *name;

    name = strrchr(pathName, '.');
    if (name) {
    	name++;
    }
    parentPtr = Ctk_ParentByName(interp, pathName, winPtr);
    if (parentPtr == (TkWindow *) NULL) {
    	return (TkWindow *) NULL;
    }
    if (screenName == NULL) {
    	dispPtr = parentPtr->dispPtr;
    } else {
	dispPtr = GetScreen(interp, screenName);
    }

    /*
     * Get entry for new name.
     */
    hPtr = Tcl_CreateHashEntry(&winPtr->mainPtr->nameTable, pathName, &new);
    if (!new) {
	Tcl_AppendResult(interp, "window name \"", pathName,
		"\" already exists", (char *) NULL);
	return NULL;
    }

    /*
     * Create the window.
     */
    winPtr = NewWindow(dispPtr);
    if (screenName) {
	winPtr->parentPtr = dispPtr->rootPtr;
	winPtr->flags |= TK_TOP_LEVEL;
	parentPtr->flags |= CTK_HAS_TOPLEVEL_CHILD;
    } else {
	winPtr->parentPtr = parentPtr;
    }
    winPtr->mainPtr = parentPtr->mainPtr;
    winPtr->mainPtr->refCount++;
    InsertWindow(winPtr, HEAD_CHILD(winPtr->parentPtr));
    Tcl_SetHashValue(hPtr, winPtr);
    winPtr->pathName = Tcl_GetHashKey(&winPtr->mainPtr->nameTable, hPtr);
    winPtr->nameUid = Tk_GetUid(name);
    return winPtr;
}

/*
 *----------------------------------------------------------------------
 * Ctk_ParentByName --
 *	Determine parent of window based on path name.  This is necessary
 *	for top level windows because Ctk_Parent() will always return
 *	the root window for them.
 *
 *  Results:
 *	Returns pointer to new window if successful.  Returns
 *	NULL if the parent can't be found, and stores an error
 *	message in interp->result.
 *
 *  Side Effects:
 *----------------------------------------------------------------------
 */

TkWindow *
Ctk_ParentByName(interp, pathName, tkwin)
    Tcl_Interp *interp;
    char *pathName;
    Tk_Window tkwin;
{
#define FIXED_SPACE 50
    char fixedSpace[FIXED_SPACE+1];
    char *p;
    int numChars;
    Tk_Window parent;

    /*
     * Strip the parent's name out of pathName (it's everything up
     * to the last dot).  There are two tricky parts: (a) must
     * copy the parent's name somewhere else to avoid modifying
     * the pathName string (for large names, space for the copy
     * will have to be malloc'ed);  (b) must special-case the
     * situations where the parent is "" or ".".
     */
    p = strrchr(pathName, '.');
    if (p == NULL) {
	Tcl_AppendResult(interp, "bad window path name \"", pathName,
		"\"", (char *) NULL);
	return NULL;
    }
    numChars = p-pathName;
    if (numChars > FIXED_SPACE) {
	p = (char *) ckalloc((unsigned) (numChars+1));
    } else {
	p = fixedSpace;
    }
    if (pathName[1] == '\0') {
    	/*
    	 * Parent is root: ""
    	 */
    	*p = '\0';
    } else if (numChars == 0) {
    	/*
    	 * Parent is main: "."
    	 */
	*p = '.';
	p[1] = '\0';
    } else {
	strncpy(p, pathName, numChars);
	p[numChars] = '\0';
    }

    /*
     * Find the parent window.
     */
    parent = Tk_NameToWindow(interp, p, tkwin);
    if (p != fixedSpace) {
	ckfree(p);
    }
    return parent;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetClass --
 *
 *	This procedure is used to give a window a class.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A new class is stored for tkwin, replacing any existing
 *	class for it.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetClass(tkwin, className)
    Tk_Window tkwin;		/* Token for window to assign class. */
    char *className;		/* New class for tkwin. */
{
    register TkWindow *winPtr = (TkWindow *) tkwin;

    winPtr->classUid = Tk_GetUid(className);
    TkOptionClassChanged(winPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_NameToWindow --
 *
 *	Given a string name for a window, this procedure
 *	returns the token for the window, if there exists a
 *	window corresponding to the given name.
 *
 * Results:
 *	The return result is either a token for the window corresponding
 *	to "name", or else NULL to indicate that there is no such
 *	window.  In this case, an error message is left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
Tk_Window
Tk_NameToWindow(interp, pathName, winPtr)
    Tcl_Interp *interp;		/* Where to report errors. */
    char *pathName;		/* Path name of window. */
    TkWindow *winPtr;		/* Token for window, name is assumed to
    				 * belong to the same main window as winPtr. */
{
    Tcl_HashEntry *hPtr;

    hPtr = Tcl_FindHashEntry(&winPtr->mainPtr->nameTable, pathName);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "bad window path name \"",
		pathName, "\"", (char *) NULL);
	return NULL;
    }
    return (Tk_Window) Tcl_GetHashValue(hPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CreateRoot --
 *
 *  Creates the root window (whole screen, no parent).  The window
 *  is mapped and displayed.
 *
 *  Results:
 *	A new window pointer.
 *
 *  Side Effects:
 *	Screen is cleared.
 *
 *----------------------------------------------------------------------
 */
static TkWindow *
CreateRoot(interp, dispPtr)
    Tcl_Interp *interp;
    TkDisplay *dispPtr;
{
    TkWindow *winPtr = NewWindow(dispPtr);

    winPtr->mainPtr = NULL;
    winPtr->parentPtr = NULL;
    winPtr->nextPtr = NULL;
    winPtr->priorPtr = NULL;
    CtkSetRect(&winPtr->rect, 0, 0,
    	    Ctk_DisplayWidth(dispPtr), Ctk_DisplayHeight(dispPtr));
    CtkCopyRect(&winPtr->maskRect, &winPtr->rect);
    CtkCopyRect(&winPtr->clipRect, &winPtr->rect);
    winPtr->clipRgn = CtkCreateRegion(&(winPtr->maskRect));
    winPtr->absLeft = 0;
    winPtr->absTop = 0;
    winPtr->flags |= TK_MAPPED|CTK_DISPLAYED|TK_TOP_LEVEL;
    winPtr->classUid = Tk_GetUid("Root");
    Ctk_ClearWindow(winPtr);

    return winPtr;
}

/*
 *----------------------------------------------------------------------
 * NewWindow --
 *	Allocate a window structure and initialize contents.
 *
 * Results:
 *	Returns pointer to window.
 *
 * Side Effects:
 *
 *----------------------------------------------------------------------
 */
static TkWindow *
NewWindow(dispPtr)
    TkDisplay *dispPtr;
{
    TkWindow *winPtr = (TkWindow *) ckalloc(sizeof(TkWindow));

    winPtr->dispPtr = dispPtr;
    winPtr->pathName = NULL;
    winPtr->classUid = NULL;
    winPtr->mainPtr = NULL;
    winPtr->flags = 0;
    winPtr->handlerList = NULL;
    winPtr->numTags = 0;
    winPtr->optionLevel = -1;
    winPtr->tagPtr = NULL;
    winPtr->childList.nextPtr = HEAD_CHILD(winPtr);
    winPtr->childList.priorPtr = HEAD_CHILD(winPtr);
    winPtr->borderWidth = 0;
    winPtr->fillChar = ' ';
    winPtr->fillStyle = CTK_PLAIN_STYLE;
    winPtr->clipRgn = NULL;
    winPtr->reqWidth = 1;
    winPtr->reqHeight = 1;
    winPtr->geomMgrPtr = NULL;
    winPtr->geomData = NULL;

    dispPtr->numWindows++;
    return winPtr;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_DestroyWindow --
 *
 *	Destroy an existing window.  After this call, the caller
 *	should never again use the token.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window is deleted, along with all of its children.
 *	Relevant callback procedures are invoked.
 *
 *--------------------------------------------------------------
 */

void
Tk_DestroyWindow(winPtr)
    TkWindow *winPtr;
{
    TkWindow *child;
    TkWindow *nextPtr;
    Ctk_Event event;

    if (winPtr->flags & TK_ALREADY_DEAD) {
	/*
	 * A destroy event binding caused the window to be destroyed
	 * again.  Ignore the request.
	 */

	return;
    }
    winPtr->flags |= TK_ALREADY_DEAD;

    Ctk_Unmap(winPtr);

    /*
     * If this is a main window, remove it from the list of main
     * windows.  This needs to be done now (rather than later with
     * all the other main window cleanup) to handle situations where
     * a destroy binding for a window calls "exit".  In this case
     * the child window cleanup isn't complete when exit is called,
     * so the reference count of its application doesn't go to zero
     * when exit calls Tk_DestroyWindow on ".", so the main window
     * doesn't get removed from the list and exit loops infinitely.
     * Even worse, if "destroy ." is called by the destroy binding
     * before calling "exit", "exit" will attempt to destroy
     * mainPtr->winPtr, which no longer exists, and there may be a
     * core dump.
     */

    if (winPtr->mainPtr->winPtr == winPtr) {
	if (tkMainWindowList == winPtr->mainPtr) {
	    tkMainWindowList = winPtr->mainPtr->nextPtr;
	} else {
	    TkMainInfo *prevPtr;

	    for (prevPtr = tkMainWindowList;
		    prevPtr->nextPtr != winPtr->mainPtr;
		    prevPtr = prevPtr->nextPtr) {
		/* Empty loop body. */
	    }
	    prevPtr->nextPtr = winPtr->mainPtr->nextPtr;
	}
	tk_NumMainWindows--;
    }

    /*
     * Recursively destroy children.
     */

    for (child = BOTTOM_CHILD(winPtr);
	    child != HEAD_CHILD(winPtr);
	    child = nextPtr) {
	nextPtr = child->nextPtr;
	Tk_DestroyWindow(child);
    }
    if (winPtr->flags & CTK_HAS_TOPLEVEL_CHILD) {
	/*
	 * This window has toplevel children, which are not stored
	 * in the child list.  Check all the children of all root
	 * windows to see if their name is an extension of this
	 * windows name - if so destroy the top level window.
	 */
	char *path = Tk_PathName(winPtr);
	char *childPath;
	int length = strlen(path);
	TkWindow *priorPtr;
	TkDisplay *dispPtr;

	for (dispPtr = tkDisplayList;
		dispPtr != NULL;
	    	dispPtr = dispPtr->nextPtr) {
	    priorPtr = HEAD_CHILD(dispPtr->rootPtr);
	    child = BOTTOM_CHILD(dispPtr->rootPtr);
	    while (child != HEAD_CHILD(dispPtr->rootPtr)) {
	        childPath = Tk_PathName(child);
		if (strncmp(childPath, path, length) == 0
			&& (childPath[length] == '.'
			|| (length == 1 && childPath[1] != '\0'))) {
		    Tk_DestroyWindow(child);
		} else {
		    priorPtr = child;
		}
		child = priorPtr->nextPtr;
	    }
	}
    }

    /*
     * Generate a Destroy event.
     *
     * Note: if the window's pathName is NULL it means that the window
     * was not successfully initialized in the first place, so we should
     * not make the window exist or generate the event.
     */

    if (winPtr->pathName != NULL) {
	event.type = CTK_DESTROY_EVENT;
	event.window = winPtr;
	Tk_HandleEvent(&event);
    }

    UnlinkWindow(winPtr);
    TkEventDeadWindow(winPtr);
    if (winPtr->tagPtr != NULL) {
	TkFreeBindingTags(winPtr);
    }
    TkOptionDeadWindow(winPtr);
    TkFocusDeadWindow(winPtr);

    if (winPtr->mainPtr != NULL) {
	if (winPtr->pathName != NULL) {
	    Tk_DeleteAllBindings(winPtr->mainPtr->bindingTable,
		    (ClientData) winPtr->pathName);
	    Tcl_DeleteHashEntry(Tcl_FindHashEntry(&winPtr->mainPtr->nameTable,
		    winPtr->pathName));
	}
	winPtr->mainPtr->refCount--;
	if (winPtr->mainPtr->refCount == 0) {
	    register TkCmd *cmdPtr;

	    /*
	     * We just deleted the last window in the application.  Delete
	     * the TkMainInfo structure too and replace all of Tk's commands
	     * with dummy commands that return errors (except don't replace
	     * the "exit" command, since it may be needed for the application
	     * to exit).
	     */

	    for (cmdPtr = commands; cmdPtr->name != NULL; cmdPtr++) {
		if (cmdPtr->cmdProc != Tk_ExitCmd) {
		    Tcl_CreateCommand(winPtr->mainPtr->interp, cmdPtr->name,
			    TkDeadAppCmd, (ClientData) NULL,
			    (void (*)()) NULL);
		}
	    }
	    if (winPtr->mainPtr->bindingDepth == 0) {
		TkDeleteMain(winPtr->mainPtr);
	    }
	}
    }

    if ((--(winPtr->dispPtr->numWindows)) == 1) {
	TkDisplay *dispPtr = winPtr->dispPtr;

    	CtkDisplayEnd(dispPtr);
    	if (tkDisplayList == dispPtr) {
    	    tkDisplayList = dispPtr->nextPtr;
	} else {
	    TkDisplay *prevDispPtr;
	    for (prevDispPtr = tkDisplayList;
	    	    prevDispPtr != NULL;
	    	    prevDispPtr = prevDispPtr->nextPtr) {
		if (prevDispPtr->nextPtr == dispPtr) {
		    prevDispPtr->nextPtr = dispPtr->nextPtr;
		    break;
		}
	    }
	}
    	ckfree((char *) dispPtr->rootPtr);
    	ckfree((char *) dispPtr);
    }

    ckfree((char *) winPtr);
}

/*
 *------------------------------------------------------------
 * TkDeleteMain --
 *
 *	Release resources for a TkMainInfo structure.
 *	All windows for this main should already have
 *	been destroyed.  The pointer should no be referenced
 *	again.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *------------------------------------------------------------
 */

void
TkDeleteMain(mainPtr)
    TkMainInfo *mainPtr;
{
    Tcl_DeleteHashTable(&mainPtr->nameTable);
    Tk_DeleteBindingTable(mainPtr->bindingTable);
    ckfree((char *) mainPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_RestackWindow --
 *
 *      Change a window's position in the stacking order.
 *
 * Results:
 *      TCL_OK is normally returned.  If other is not a descendant
 *      of tkwin's parent then TCL_ERROR is returned and tkwin is
 *      not repositioned.
 *
 * Side effects:
 *      Tkwin is repositioned in the stacking order.
 *
 *----------------------------------------------------------------------
 */

int
Tk_RestackWindow(winPtr, aboveBelow, otherPtr)
    TkWindow *winPtr;
    int aboveBelow;
    TkWindow *otherPtr;
{
    int redisplay = 0;

    if (otherPtr) {
	/*
	 * Find ancestor of otherPtr (or otherPtr itself) that is a
	 * sibling of winPtr.
	 */
	while (otherPtr->parentPtr != winPtr->parentPtr) {
	    otherPtr = otherPtr->parentPtr;
	    if (!otherPtr) {
		return TCL_ERROR;
	    }
	}
    }
    if (otherPtr == winPtr) {
	return TCL_OK;
    }

    if (CtkIsDisplayed(winPtr)) {
	UndisplayWindow(winPtr);
	redisplay = 1;
    }
    UnlinkWindow(winPtr);
    if (aboveBelow == Above) {
	if (otherPtr) {
	    otherPtr = otherPtr->nextPtr;
	} else {
	    otherPtr = HEAD_CHILD(winPtr->parentPtr);
	}
    } else {
	if (!otherPtr) {
	    otherPtr = BOTTOM_CHILD(winPtr->parentPtr);
	}
    }
    InsertWindow(winPtr, otherPtr);
    if (redisplay) {
	DisplayWindow(winPtr);
    }
    return TCL_OK;
}

/*
 *------------------------------------------------------------
 * Ctk_Map --
 *
 *	Position a window within its parent.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Generates a map event for the window.
 *	If parent is displayed, then the window will be displayed.
 *
 *------------------------------------------------------------
 */

void
Ctk_Map(winPtr, left, top, right, bottom)
    TkWindow *winPtr;
    int left;
    int top;
    int right;
    int bottom;
{
    TkWindow *parentPtr = winPtr->parentPtr;
    Ctk_Event event;

    /*
     * Keep top-levels within the bounds of the screen.
     */
    if (winPtr->flags & TK_TOP_LEVEL) {
    	int width = right - left;
    	int height = bottom - top;
    	int screenWidth = Tk_Width(parentPtr);
    	int screenHeight = Tk_Height(parentPtr);

    	if (width > screenWidth) {
	    width = screenWidth;
	}
    	if (height > screenHeight) {
	    height = screenHeight;
	}
	if (left < 0) {
	    left = 0;
	} else if (left + width > screenWidth) {
	    left = screenWidth - width ;
	}
	if (top < 0) {
	    top = 0;
	} else if (top + height > screenHeight) {
	    top = screenHeight - height;
	}
	right = left + width;
	bottom = top + height;
    }

    if ( !Tk_IsMapped(winPtr)
    	    || (winPtr->rect.left != left)
	    || (winPtr->rect.top != top)
	    || (winPtr->rect.right != right)
	    || (winPtr->rect.bottom != bottom)) {
	/*
	 * Window position changed (or window was not mapped
	 * before).  Undisplay window, re-position it, and then
	 * display it if parent is displayed.
	 */

	if (CtkIsDisplayed(winPtr)) {
	    UndisplayWindow(winPtr);
	}
	CtkSetRect(&(winPtr->rect), left, top, right, bottom);
	winPtr->flags |= TK_MAPPED;
	if (CtkIsDisplayed(parentPtr)) {
	    DisplayWindow(winPtr);
	}
    }
    event.type = CTK_MAP_EVENT;
    event.window = winPtr;
    Tk_HandleEvent(&event);
}

/*
 *------------------------------------------------------------
 * Ctk_Unmap --
 *
 *	Remove positioning for a window.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	If window is displayed, it and all its descendants are
 *	undisplayed.
 *
 *------------------------------------------------------------
 */

void
Ctk_Unmap(winPtr)
    TkWindow *winPtr;
{
    Ctk_Event event;

    if (Tk_IsMapped(winPtr)) {
	/*
	 *  Window is mapped, unmap it.
	 */
	if (CtkIsDisplayed(winPtr)) {
	    UndisplayWindow(winPtr);
	}
	winPtr->flags &= ~TK_MAPPED;
	event.type = CTK_UNMAP_EVENT;
	event.window = winPtr;
	Tk_HandleEvent(&event);
    }
}

/*
 *------------------------------------------------------------
 * Ctk_BottomChild --
 * Ctk_TopChild --
 * Ctk_PriorSibling --
 * Ctk_NextSibling --
 * Ctk_TopLevel --
 *
 *	Get window relative.
 *
 * Results:
 *	Pointer to window, or NULL if window has no such relative.
 *
 * Side Effects:
 *	None.
 *
 *------------------------------------------------------------
 */

TkWindow *
Ctk_BottomChild(winPtr)
    TkWindow *winPtr;
{
    TkWindow * child = BOTTOM_CHILD(winPtr);

    if (child == HEAD_CHILD(winPtr)) {
	return (TkWindow *) NULL;
    }
    else {
	return child;
    }
}

TkWindow *
Ctk_TopChild(winPtr)
    TkWindow *winPtr;
{
    TkWindow * child = TOP_CHILD(winPtr);

    if (child == HEAD_CHILD(winPtr)) {
	return (TkWindow *) NULL;
    }
    else {
	return child;
    }
}

TkWindow *
Ctk_NextSibling(winPtr)
    TkWindow *winPtr;
{
    TkWindow * sibling = winPtr->nextPtr;

    if (sibling == HEAD_CHILD(winPtr->parentPtr)) {
	return (TkWindow *) NULL;
    }
    else {
	return sibling;
    }
}

TkWindow *
Ctk_PriorSibling(winPtr)
    TkWindow *winPtr;
{
    TkWindow * sibling = winPtr->priorPtr;

    if (sibling == HEAD_CHILD(winPtr->parentPtr)) {
	return (TkWindow *) NULL;
    }
    else {
	return sibling;
    }
}

TkWindow *
Ctk_TopLevel(winPtr)
    TkWindow *winPtr;
{
    while (!Tk_IsTopLevel(winPtr)) {
    	winPtr = winPtr->parentPtr;
    }
    return winPtr;
}

/*
 *------------------------------------------------------------
 * Tk_SetInternalBorder --
 *
 *	Set window's internal border width.  The standard drawing
 *	routines will not draw on the internal border (only
 *	Ctk_DrawBorder() will) and the geometry managers should
 *	not place child windows there.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The border width is recorded for the window, and a map
 *	event is synthesized so that all geometry managers of all
 *	children are notified to re-layout, if necessary.
 *
 *------------------------------------------------------------
 */

void
Tk_SetInternalBorder(winPtr, width)
    TkWindow *winPtr;
    int width;
{
    Ctk_Event event;

    if (winPtr->borderWidth != width) {
	winPtr->borderWidth = width;
	ComputeClipRect(winPtr);
	event.type = CTK_MAP_EVENT;
	event.window = winPtr;
	Tk_HandleEvent(&event);
    }
}

/*
 *--------------------------------------------------------------
 *
 * Ctk_DrawBorder --
 *
 *	Draw border for a window in specified style.
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
Ctk_DrawBorder(winPtr, style, title)
    TkWindow *winPtr;
    Ctk_Style style;
    char *title;
{
    int borderWidth = winPtr->borderWidth;

    if (borderWidth > 0) {
    	Ctk_Rect saveClip;

	/*
	 * Temporarily set clipRect to maskRect so that we can
	 * draw within the border area.
	 */
    	CtkCopyRect(&saveClip, &winPtr->clipRect);
    	CtkCopyRect(&winPtr->clipRect, &winPtr->maskRect);
    	Ctk_DrawRect(winPtr, 0, 0, Tk_Width(winPtr)-1, Tk_Height(winPtr)-1,
		style);
	if (title) {
	    Ctk_DrawString(winPtr, 1, 0, style, title, -1);
	}
    	CtkCopyRect(&winPtr->clipRect, &saveClip);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MainWindow --
 *
 *	Returns the main window for an application.
 *
 * Results:
 *	If interp has a Tk application associated with it, the main
 *	window for the application is returned.  Otherwise NULL is
 *	returned and an error message is left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tk_Window
Tk_MainWindow(interp)
    Tcl_Interp *interp;                 /* Interpreter that embodies the
					 * application.  Used for error
					 * reporting also. */
{
    TkMainInfo *mainPtr;
    for (mainPtr = tkMainWindowList; mainPtr != NULL;
	    mainPtr = mainPtr->nextPtr) {
	if (mainPtr->interp == interp) {
	    return (Tk_Window) mainPtr->winPtr;
	}
    }
    Tcl_SetResult(interp,"this isn't a Tk application",TCL_STATIC);
    return NULL;
}

/*
 *------------------------------------------------------------
 * DisplayWindow --
 *
 *	Display window and all its mapped descendants.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *------------------------------------------------------------
 */

static void
DisplayWindow(winPtr)
    TkWindow *winPtr;
{
    TkWindow *parentPtr = winPtr->parentPtr;
    TkWindow *sibling;
    TkWindow *child;

    if (CtkIsDisplayed(winPtr)) {
    	panic("Attempt to display already displayed window");
    }

    winPtr->flags |= CTK_DISPLAYED;

    winPtr->absLeft = parentPtr->absLeft + winPtr->rect.left;
    winPtr->absTop = parentPtr->absTop + winPtr->rect.top;
    winPtr->maskRect.top = winPtr->absTop;
    winPtr->maskRect.left = winPtr->absLeft;
    winPtr->maskRect.bottom = parentPtr->absTop + winPtr->rect.bottom;
    winPtr->maskRect.right = parentPtr->absLeft + winPtr->rect.right;
    CtkIntersectRects(&(winPtr->maskRect), &(parentPtr->clipRect));
    ComputeClipRect(winPtr);

    if (winPtr->flags & TK_TOP_LEVEL) {
	/*
	 *  This is a top level window, compute clipping by siblings.
	 *  Start with a clipping region equal to `maskRect', then
	 *  remove overlaps with siblings above this window.
	 */
	winPtr->clipRgn = CtkCreateRegion(&(winPtr->maskRect));
	for (sibling = winPtr->nextPtr;
		sibling != HEAD_CHILD(parentPtr);
		sibling = sibling->nextPtr)
	{
	    if (CtkIsDisplayed(sibling)) {
		CtkRegionMinusRect(winPtr->clipRgn, &(sibling->maskRect), 0);
	    }
	}

	/*
	 *  For each sibling below this window (and the root),
	 *  subtract the overlap between this window and the sibling
	 *  from the sibling's clipping region.
	 */
	for (sibling = winPtr->priorPtr;
		sibling != HEAD_CHILD(parentPtr);
		sibling = sibling->priorPtr)
	{
	    if (CtkIsDisplayed(sibling)) {
		CtkRegionMinusRect(sibling->clipRgn, &(winPtr->maskRect), 0);
	    }
	}
	CtkRegionMinusRect(parentPtr->clipRgn, &(winPtr->maskRect), 0);
    } else {
	winPtr->clipRgn = parentPtr->clipRgn;
    }

    Ctk_ClearWindow(winPtr);
    ExposeWindow(winPtr, winPtr->clipRgn);

    for (child = BOTTOM_CHILD(winPtr);
	    child != HEAD_CHILD(winPtr);
	    child = child->nextPtr) {
	if (Tk_IsMapped(child)) {
	    DisplayWindow(child);
	}
    }
}

/*
 *------------------------------------------------------------
 * ComputeClipRect --
 *
 *	Set the clipping rectangle for a window according
 *	to it's position, border-width, and parent's clipping
 *	rectangle.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Stores new values in winPtr->clipRect.
 *
 *------------------------------------------------------------
 */

static void
ComputeClipRect(winPtr)
    register TkWindow *winPtr;
{
    register TkWindow *parentPtr = winPtr->parentPtr;

    winPtr->clipRect.top = winPtr->absTop + winPtr->borderWidth;
    winPtr->clipRect.left = winPtr->absLeft + winPtr->borderWidth;
    winPtr->clipRect.bottom =
    	    parentPtr->absTop + winPtr->rect.bottom - winPtr->borderWidth;
    winPtr->clipRect.right =
    	    parentPtr->absLeft + winPtr->rect.right - winPtr->borderWidth;
    CtkIntersectRects(&(winPtr->clipRect), &(parentPtr->clipRect));
}

/*
 *------------------------------------------------------------
 * ExposeWindow --
 *
 *	Send expose event(s) to window for specified region
 *
 * Results:
 *	Pointer to sibling window, or NULL if window
 *	does not have a sibling that is displayed and enabled.
 *
 * Side Effects:
 *	None.
 *
 *------------------------------------------------------------
 */

static void
ExposeWindow(winPtr, rgnPtr)
    TkWindow *winPtr;
    CtkRegion *rgnPtr;
{
    Ctk_Event event;

    /*
     * Compute intersection of rgnPtr and winPtr->maskRect.
     */
    CtkRegionGetRect(rgnPtr, &event.u.expose);
    CtkIntersectRects(&event.u.expose, &(winPtr->maskRect));
    CtkMoveRect(&event.u.expose, -winPtr->absLeft, -winPtr->absTop);

    event.type = CTK_EXPOSE_EVENT;
    event.window = winPtr;
    Tk_HandleEvent(&event);
}

/*
 *------------------------------------------------------------
 * UndisplayWindow --
 *
 *	Stop displaying window and all its descendants.
 *	Window must currently be displayed.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *------------------------------------------------------------
 */

static void
UndisplayWindow(winPtr)
    TkWindow *winPtr;
{
    TkWindow *child;
    TkWindow *sibling;
    TkWindow *parentPtr = winPtr->parentPtr;
    Ctk_Event event;

    if (!CtkIsDisplayed(winPtr)) {
    	panic("Attempt to undisplay window that isn't displayed");
    }

    /*
     *	Stop displaying the descendants of `winPtr'.
     */
    for (child = BOTTOM_CHILD(winPtr);
	    child != HEAD_CHILD(winPtr);
	    child = child->nextPtr)
    {
	if (CtkIsDisplayed(child)) {
	    UndisplayWindow(child);
	}
    }

    winPtr->flags &= ~CTK_DISPLAYED;

    if (parentPtr == NULL) {
	CtkDestroyRegion(winPtr->clipRgn);
    } else if (winPtr->flags & TK_TOP_LEVEL) {
	/*
	 * This is a top level window,
	 * maintain the clipping regions.
	 */

	/*
	 * For each (displayed) sibling below this window
	 * (and the root) add the overlap between the window and the
	 * sibling to the siblings clipping region.
	 */
	for (sibling = winPtr->priorPtr;
		sibling != HEAD_CHILD(parentPtr);
		sibling = sibling->priorPtr) {
	    if (CtkIsDisplayed(sibling)) {
		UnoverlapHierarchy(sibling, winPtr);
	    }
	}
	Unoverlap(parentPtr, winPtr);
	CtkDestroyRegion(winPtr->clipRgn);
    } else if (winPtr->fillStyle != CTK_INVISIBLE_STYLE) {
	Ctk_FillRect(parentPtr,
		winPtr->rect.left, winPtr->rect.top,
		winPtr->rect.right, winPtr->rect.bottom,
		parentPtr->fillStyle, parentPtr->fillChar);
	event.type = CTK_EXPOSE_EVENT;
	event.window = parentPtr;
	CtkCopyRect(&event.u.expose, &parentPtr->rect);
	Tk_HandleEvent(&event);
    } else if (winPtr->borderWidth) {
    	int borderWidth = winPtr->borderWidth;

        /*
         * Blank out the border area.
         * This would be much easier if we could pass a character to
         * Ctk_DrawRect().
         */
	Ctk_FillRect(parentPtr,
		winPtr->rect.left, winPtr->rect.top,
		winPtr->rect.right, winPtr->rect.top+borderWidth,
		parentPtr->fillStyle, parentPtr->fillChar);
	Ctk_FillRect(parentPtr,
		winPtr->rect.left, winPtr->rect.bottom-borderWidth,
		winPtr->rect.right, winPtr->rect.bottom,
		parentPtr->fillStyle, parentPtr->fillChar);
	Ctk_FillRect(parentPtr,
		winPtr->rect.left, winPtr->rect.top+borderWidth,
		winPtr->rect.left+borderWidth, winPtr->rect.bottom-borderWidth,
		parentPtr->fillStyle, parentPtr->fillChar);
	Ctk_FillRect(parentPtr,
		winPtr->rect.right-borderWidth, winPtr->rect.top+borderWidth,
		winPtr->rect.right, winPtr->rect.bottom-borderWidth,
		parentPtr->fillStyle, parentPtr->fillChar);
    }

    winPtr->clipRgn = NULL;
}

/*
 *------------------------------------------------------------
 * UnoverlapHierarchy --
 *
 *	Restore overlapping region to underlying window tree.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *------------------------------------------------------------
 */

static void
UnoverlapHierarchy(underWinPtr, overWinPtr)
    TkWindow *underWinPtr;
    TkWindow *overWinPtr;
{
    TkWindow *child;

    for (child = TOP_CHILD(underWinPtr);
	    child != HEAD_CHILD(underWinPtr);
	    child = child->priorPtr) {
	if (CtkIsDisplayed(child)) {
	    UnoverlapHierarchy(child, overWinPtr);
	}
    }
    Unoverlap(underWinPtr, overWinPtr);
}

/*
 *------------------------------------------------------------
 * Unoverlap --
 *
 *	Restore overlapping region to underlying window.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *------------------------------------------------------------
 */

static void
Unoverlap(underWinPtr, overWinPtr)
    TkWindow *underWinPtr;
    TkWindow *overWinPtr;
{
    CtkRegion *overlap;

    if (underWinPtr->fillStyle != CTK_INVISIBLE_STYLE) {
	overlap = CtkRegionMinusRect(
		overWinPtr->clipRgn,
		&(underWinPtr->maskRect),
		1);
	CtkUnionRegions(underWinPtr->clipRgn, overlap);
	CtkFillRegion(underWinPtr->dispPtr, overlap,
		underWinPtr->fillStyle, underWinPtr->fillChar);
	ExposeWindow(underWinPtr, overlap);
	CtkDestroyRegion(overlap);
    } else {
    	if (underWinPtr->borderWidth) {
    	    /*
    	     * Ok - this is a hack:
    	     * Invisible windows can have (visible) borders, so
    	     * must send an expose event to the window.  Ideally,
    	     * I would remove the border area from the overlying
    	     * clip region, but that would take a lot of work.
    	     * Since I know that the window will not redraw until
    	     * idle time, I can send expose now, and let the parent
    	     * clear the border area.  Later, at idle, the invisible
    	     * window will draw the border.
    	     */
	    ExposeWindow(underWinPtr, overWinPtr->clipRgn);
	}
    }
}

/*
 *------------------------------------------------------------
 * InsertWindow --
 *
 *	Insert window into list in front of `sibling'.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *------------------------------------------------------------
 */

static void
InsertWindow(winPtr, sibling)
    TkWindow *winPtr;
    TkWindow *sibling;
{
    winPtr->nextPtr = sibling;
    winPtr->priorPtr = sibling->priorPtr;
    sibling->priorPtr->nextPtr = winPtr;
    sibling->priorPtr = winPtr;
}

/*
 *------------------------------------------------------------
 * UnlinkWindow --
 *
 *	Detachs the window from its parent's list of children.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *------------------------------------------------------------
 */

static void
UnlinkWindow(winPtr)
    TkWindow *winPtr;
{
    winPtr->nextPtr->priorPtr = winPtr->priorPtr;
    winPtr->priorPtr->nextPtr = winPtr->nextPtr;
}

