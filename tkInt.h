/*
 * tkInt.h (CTk) --
 *
 *	Declarations for things used internally by the Tk
 *	procedures but not exported outside the module.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 1994-1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Header: /usrs/andrewm/work/RCS/ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
 */

#ifndef _TKINT
#define _TKINT

#ifndef _TK
#include "tk.h"
#endif
#include <tcl.h>

/*
 * One of the following structures is maintained for each display
 * containing a window managed by Tk:
 */

struct TkDisplay {
    /*
     * Maintained by ctkDisplay.c
     */
    char *name;			/* Name of display device. Malloc-ed. */
    char *type;			/* Device type. Malloc-ed. */
    ClientData display;		/* Curses's info about display. */
    Tcl_Channel chan;		/* Input channel for the device */
    int fd;			/* Input file descriptor for device. */
    FILE *inPtr;		/* Input file pointer for device. */
    TkWindow *cursorPtr;	/* Window to display cursor in. */
    int cursorX, cursorY;	/* Position in `cursWinPtr' to display
				 * cursor. */

    /*
     * Maintained by tkWindow.c
     */
    int numWindows;		/* Windows currently existing in display
    				 * (including root). */
    TkWindow *rootPtr;		/* Root window of display. */
    TkWindow *focusPtr;		/* Window that has the keyboard focus. */
    struct TkDisplay *nextPtr;	/* Next in list of all displays. */
};

/*
 * One of the following structures exists for each event handler
 * created by calling Tk_CreateEventHandler.  This information
 * is used by tkEvent.c only.
 */

struct TkEventHandler {
    unsigned long mask;		/* Events for which to invoke
				 * proc. */
    Tk_EventProc *proc;		/* Procedure to invoke when an event
				 * in mask occurs. */
    ClientData clientData;	/* Argument to pass to proc. */
    struct TkEventHandler *nextPtr;
				/* Next in list of handlers
				 * associated with window (NULL means
				 * end of list). */
};

/*
 * Tk keeps one of the following data structures for each main
 * window (created by a call to Tk_CreateMainWindow).  It stores
 * information that is shared by all of the windows associated
 * with a particular main window.
 */

struct TkMainInfo {
    int refCount;		/* Number of windows whose "mainPtr" fields
				 * point here.  When this becomes zero, can
				 * free up the structure (the reference
				 * count is zero because windows can get
				 * deleted in almost any order;  the main
				 * window isn't necessarily the last one
				 * deleted). */
    struct TkWindow *winPtr;	/* Pointer to main window. */
    Tcl_Interp *interp;		/* Interpreter associated with application. */
    Tcl_HashTable nameTable;	/* Hash table mapping path names to TkWindow
				 * structs for all windows related to this
				 * main window.  Managed by tkWindow.c. */
    Tk_BindingTable bindingTable;
				/* Used in conjunction with "bind" command
				 * to bind events to Tcl commands. */
    TkDisplay *curDispPtr;	/* Display for last binding command invoked
				 * in this application;  used only  by
				 * tkBind.c. */
    int bindingDepth;		/* Number of active instances of Tk_BindEvent
				 * in this application.  Used only by
				 * tkBind.c. */
    struct ElArray *optionRootPtr;
				/* Top level of option hierarchy for this
				 * main window.  NULL means uninitialized.
				 * Managed by tkOption.c. */
    struct TkMainInfo *nextPtr;	/* Next in list of all main windows managed by
				 * this process. */
};

/*
 * Pointer to first entry in list of all displays currently known.
 */

extern TkDisplay *tkDisplayList;

/*
 * Flags passed to TkMeasureChars:
 */

#define TK_WHOLE_WORDS		 1
#define TK_AT_LEAST_ONE		 2
#define TK_PARTIAL_OK		 4
#define TK_NEWLINES_NOT_SPECIAL	 8
#define TK_IGNORE_TABS		16

/*
 * Location of library directory containing Tk scripts.  This value
 * is put in the $tkLibrary variable for each application.
 */

#ifndef CTK_LIBRARY
#define CTK_LIBRARY "/usr/local/lib/ctk"
#endif

/*
 * Special flag to pass to Tk_CreateFileHandler to indicate that
 * the file descriptor is actually for a display, not a file, and
 * should be treated specially.  Make sure that this value doesn't
 * conflict with TK_READABLE, TK_WRITABLE, or TK_EXCEPTION from tk.h.
 */

#define TK_IS_DISPLAY	32

/*
 * The macro below is used to modify a "char" value (e.g. by casting
 * it to an unsigned character) so that it can be used safely with
 * macros such as isspace.
 */

#define UCHAR(c) ((unsigned char) (c))

/*
 * Miscellaneous variables shared among Tk modules but not exported
 * to the outside world:
 */

extern Tk_Uid			tkActiveUid;
extern void			(*tkDelayedEventProc) _ANSI_ARGS_((void));
extern Tk_Uid			tkDisabledUid;
extern TkMainInfo		*tkMainWindowList;
extern Tk_Uid			tkNormalUid;

/*
 * Internal procedures shared among Tk modules but not exported
 * to the outside world:
 */

extern void		TkBindEventProc _ANSI_ARGS_((TkWindow *winPtr,
			    XEvent *eventPtr));
extern void		TkComputeTextGeometry _ANSI_ARGS_((
			    char *string,
			    int numChars, int wrapLength, int *widthPtr,
			    int *heightPtr));
extern int		TkCopyAndGlobalEval _ANSI_ARGS_((Tcl_Interp *interp,
			    char *script));
extern Time		TkCurrentTime _ANSI_ARGS_((void));
extern int		TkDeadAppCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
extern void		TkDisplayChars _ANSI_ARGS_((TkWindow *winPtr,
			    Ctk_Style style, char *string,
			    int numChars, int x, int y, int tabOrigin,
			    int flags));
extern void		TkDisplayText _ANSI_ARGS_((TkWindow *winPtr,
			    Ctk_Style style,
			    char *string, int numChars, int x, int y,
			    int length, Tk_Justify justify, int underline));
extern void		TkEventCleanupProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp));
extern void		TkEventDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkFocusDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern int		TkFocusFilterEvent _ANSI_ARGS_((TkWindow *winPtr,
			    XEvent *eventPtr));
extern void		TkFreeBindingTags _ANSI_ARGS_((TkWindow *winPtr));
extern TkWindow *	TkGetFocus _ANSI_ARGS_((TkWindow *winPtr));
extern int		TkGetInterpNames _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin));
extern char *		TkInitFrame _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, int toplevel, int argc,
			    char *argv[]));
extern int		TkMeasureChars _ANSI_ARGS_((
			    char *source, int maxChars, int startX, int maxX,
			    int tabOrigin, int flags, int *nextXPtr));
extern void		TkOptionClassChanged _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkOptionDeadWindow _ANSI_ARGS_((TkWindow *winPtr));
extern void		TkQueueEvent _ANSI_ARGS_((TkDisplay *dispPtr,
			    XEvent *eventPtr));

extern void		TkDeleteMain _ANSI_ARGS_((TkMainInfo *mainPtr));

#define CtkIsDisplayed(tkwin)	(((tkwin)->flags)& CTK_DISPLAYED)

typedef void (CtkSpanProc) _ANSI_ARGS_((int left, int right, int y,
	ClientData data));

#define CtkSpanIsEmpty(left, right) ((left) >= (right))
#define CtkCopyRect(dr,sr) (memcpy((dr), (sr), sizeof(Ctk_Rect)))
#define CtkMoveRect(rect,x,y) \
	((rect)->left += (x), (rect)->top += (y), \
	(rect)->right += (x), (rect)->bottom += (y))
#define CtkSetRect(rect,l,t,r,b) \
	((rect)->left = (l), (rect)->top = (t), \
	(rect)->right = (r), (rect)->bottom = (b))


EXTERN int		CtkDisplayInit _ANSI_ARGS_((Tcl_Interp *interp,
			    TkDisplay *dispPtr, char *displayName));
EXTERN void		CtkDisplayEnd _ANSI_ARGS_((TkDisplay *dispPtr));
EXTERN void		CtkDisplayBell _ANSI_ARGS_((TkDisplay *dispPtr));

EXTERN void         CtkSetFocus _ANSI_ARGS_((TkWindow *winPtr));
EXTERN void	    Ctk_Forget _ANSI_ARGS_((Tk_Window tkwin));

EXTERN void	    CtkFillRegion _ANSI_ARGS_((TkDisplay *dispPtr,
			CtkRegion *rgn_ptr, Ctk_Style style, int ch));

EXTERN void	    CtkIntersectSpans _ANSI_ARGS_((int *left_ptr,
			int *right_ptr, int left2, int right2));
EXTERN void	    CtkIntersectRects _ANSI_ARGS_((Ctk_Rect *r1_ptr,
			CONST Ctk_Rect *r2_ptr));
EXTERN CtkRegion * CtkCreateRegion _ANSI_ARGS_((Ctk_Rect *rect));
EXTERN void	    CtkDestroyRegion _ANSI_ARGS_((CtkRegion *rgn));
EXTERN void	    CtkForEachIntersectingSpan _ANSI_ARGS_((
			CtkSpanProc *func,
			ClientData func_data, int left, int right, int y,
			CtkRegion *rgn));
EXTERN void	    CtkForEachSpan _ANSI_ARGS_((CtkSpanProc *func,
			ClientData func_data, CtkRegion *rgn));
EXTERN CtkRegion * CtkRegionMinusRect _ANSI_ARGS_((CtkRegion *rgn_id,
			Ctk_Rect *rect, int want_inter));
EXTERN void	    CtkUnionRegions _ANSI_ARGS_((CtkRegion *rgn1,
			CtkRegion *rgn2));
EXTERN void	    CtkRegionGetRect _ANSI_ARGS_((CtkRegion *rgn,
			Ctk_Rect *rect_ptr));
EXTERN int	    CtkPointInRegion _ANSI_ARGS_((int x, int y,
			CtkRegion *rgn));

#endif  /* _TKINT */
