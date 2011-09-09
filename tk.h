/*
 * tk.h (installed as ctk.h) (CTk) --
 *
 *	Declarations for Tk-related things that are visible
 *	outside of the Tk module itself.
 *
 * Copyright (c) 1989-1994 The Regents of the University of California.
 * Copyright (c) 1994 The Australian National University.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 1994-1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Header: /usrs/andrewm/work/RCS/ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
 */

#ifndef _TK
#define _TK

#define TK_VERSION "8.0"
#define TK_MAJOR_VERSION 8
#define TK_MINOR_VERSION 0
#define TK_PORT_CURSES

#ifndef _TCL
#include <tcl.h>
#endif
#ifdef __STDC__
#include <stddef.h>
#endif

/*
 * Dummy types that are used by clients:
 */

typedef struct Tk_BindingTable_ *Tk_BindingTable;
typedef struct Tk_TimerToken_ *Tk_TimerToken;
typedef struct TkWindow *Tk_Window;

/*
 * Additional types exported to clients.
 */

typedef char *Tk_Uid;

/*
 * Definitions that shouldn't be used by clients, but its simpler
 * to put them here.
 */
typedef struct CtkRegion CtkRegion;
typedef struct TkMainInfo TkMainInfo;
typedef struct TkDisplay TkDisplay;
typedef struct TkEventHandler TkEventHandler;

/*
 * CTk specific definitions.
 */

typedef struct {
    int left;
    int top;
    int right;
    int bottom;
} Ctk_Rect;

typedef enum {
    CTK_INVISIBLE_STYLE, CTK_PLAIN_STYLE, CTK_UNDERLINE_STYLE,
    CTK_REVERSE_STYLE, CTK_DIM_STYLE, CTK_BOLD_STYLE,
    CTK_DISABLED_STYLE, CTK_BUTTON_STYLE, CTK_CURSOR_STYLE,
    CTK_SELECTED_STYLE
} Ctk_Style;

typedef enum {
    CTK_MAP_EVENT, CTK_UNMAP_EVENT, CTK_EXPOSE_EVENT,
    CTK_FOCUS_EVENT, CTK_UNFOCUS_EVENT, CTK_KEY_EVENT,
    CTK_DESTROY_EVENT, CTK_UNSUPPORTED_EVENT
} Ctk_EventType;

/*
 * Event groupings.
 */

#define CTK_MAP_EVENT_MASK		(1<<0)
#define CTK_EXPOSE_EVENT_MASK		(1<<1)
#define CTK_FOCUS_EVENT_MASK		(1<<2)
#define CTK_KEY_EVENT_MASK		(1<<3)
#define CTK_DESTROY_EVENT_MASK		(1<<4)
#define CTK_UNSUPPORTED_EVENT_MASK	(1<<5)

/*
 * Various X11 definitions to ease porting of Tk code.
 */

#define MapNotify	CTK_MAP_EVENT
#define ConfigureNotify	CTK_MAP_EVENT
#define UnmapNotify	CTK_UNMAP_EVENT
#define Expose		CTK_EXPOSE_EVENT
#define FocusIn		CTK_FOCUS_EVENT
#define FocusOut	CTK_UNFOCUS_EVENT
#define KeyPress	CTK_KEY_EVENT
#define DestroyNotify	CTK_DESTROY_EVENT

#define StructureNotifyMask	(CTK_MAP_EVENT_MASK|CTK_DESTROY_EVENT_MASK)

#define ShiftMask		(1<<0)
#define LockMask		(1<<1)
#define ControlMask		(1<<2)
#define Mod1Mask		(1<<3)
#define Mod2Mask		(1<<4)
#define Mod3Mask		(1<<5)
#define Mod4Mask		(1<<6)
#define Mod5Mask		(1<<7)
#define Button1Mask		(1<<8)
#define Button2Mask		(1<<9)
#define Button3Mask		(1<<10)
#define Button4Mask		(1<<11)
#define Button5Mask		(1<<12)
#define AnyModifier		(1<<15)

#define Above 0
#define Below 1

typedef unsigned long Time;
typedef unsigned long KeySym;
typedef struct {
    short x, y;
} XPoint;

/*
 * One of these structures is created for every event that occurs.
 * They are stored in a queue for the appropriate display.
 */

typedef struct Ctk_Event {
    Ctk_EventType type;			/* Type of event. */
    Tk_Window window;			/* Window where event occured. */
    unsigned long serial;		/* Assigned by Tk_HandleEvent() */
    struct Ctk_Event *nextPtr;		/* Next event in queue. */

    union {				/* Detail info according to type: */
	struct {
	    KeySym sym;			/* X-style key symbol. */
	    unsigned int state;		/* Modifier key mask. */
	    Time time;			/* When key was pressed. */
	} key;
	Ctk_Rect expose;		/* Rectangle to redraw. */
    } u;
} Ctk_Event, XEvent;

/*
 * CTk special routines.
 */

EXTERN Tk_Window    Ctk_ParentByName _ANSI_ARGS_((Tcl_Interp *interp,
			char *pathName, Tk_Window));
EXTERN int	    Ctk_Unsupported _ANSI_ARGS_((Tcl_Interp *interp,
			char *feature));
EXTERN void	    Ctk_Map _ANSI_ARGS_((Tk_Window tkwin,
			int x1, int y1, int x2, int y2));
EXTERN void	    Ctk_Unmap _ANSI_ARGS_((Tk_Window tkwin));

/*
 * Window info
 */

#define Ctk_Left(tkwin)		((tkwin)->rect.left)
#define Ctk_Top(tkwin)		((tkwin)->rect.top)
#define Ctk_Right(tkwin)	((tkwin)->rect.right)
#define Ctk_Bottom(tkwin)	((tkwin)->rect.bottom)
#define Ctk_AbsLeft(tkwin)	((tkwin)->absLeft)
#define Ctk_AbsTop(tkwin)	((tkwin)->absTop)


EXTERN Tk_Window	Ctk_PriorSibling _ANSI_ARGS_((Tk_Window tkwin));
EXTERN Tk_Window	Ctk_NextSibling _ANSI_ARGS_((Tk_Window tkwin));
EXTERN Tk_Window	Ctk_BottomChild _ANSI_ARGS_((Tk_Window tkwin));
EXTERN Tk_Window	Ctk_TopChild _ANSI_ARGS_((Tk_Window tkwin));
EXTERN Tk_Window	Ctk_TopLevel _ANSI_ARGS_((Tk_Window tkwin));


/*
 *  Display Device definitions.
 *
 *  Meant to mask curses level I/O so it could be swapped with
 *  another (DOS character I/O for example).
 */

EXTERN void         Ctk_DisplayFlush _ANSI_ARGS_((TkDisplay *dispPtr));
EXTERN int          Ctk_DisplayWidth _ANSI_ARGS_((TkDisplay *dispPtr));
EXTERN int          Ctk_DisplayHeight _ANSI_ARGS_((TkDisplay *dispPtr));
EXTERN void         Ctk_DisplayRedraw _ANSI_ARGS_((TkDisplay *dispPtr));
EXTERN void         Ctk_DrawString _ANSI_ARGS_((Tk_Window tkwin,
			int x, int y, Ctk_Style style,
			char *string, int length));
EXTERN void         Ctk_DrawCharacter _ANSI_ARGS_((Tk_Window tkwin,
			int x, int y, Ctk_Style style, int ch));
EXTERN void         Ctk_DrawRect _ANSI_ARGS_((Tk_Window tkwin,
			int x1, int y1, int x2, int y2, Ctk_Style style));
EXTERN void         Ctk_FillRect _ANSI_ARGS_((Tk_Window tkwin,
			int x1, int y1, int x2, int y2,
			Ctk_Style style, int ch));
EXTERN void         Ctk_ClearWindow _ANSI_ARGS_((Tk_Window tkwin));
EXTERN void	    Ctk_DrawBorder _ANSI_ARGS_((Tk_Window, Ctk_Style,
			char *title));
EXTERN void	    Ctk_SetCursor _ANSI_ARGS_((Tk_Window, int x, int y));

/*
 * Structure used to specify how to handle argv options.
 */

typedef struct {
    char *key;		/* The key string that flags the option in the
			 * argv array. */
    int type;		/* Indicates option type;  see below. */
    char *src;		/* Value to be used in setting dst;  usage
			 * depends on type. */
    char *dst;		/* Address of value to be modified;  usage
			 * depends on type. */
    char *help;		/* Documentation message describing this option. */
} Tk_ArgvInfo;

/*
 * Legal values for the type field of a Tk_ArgvInfo: see the user
 * documentation for details.
 */

#define TK_ARGV_CONSTANT		15
#define TK_ARGV_INT			16
#define TK_ARGV_STRING			17
#define TK_ARGV_UID			18
#define TK_ARGV_REST			19
#define TK_ARGV_FLOAT			20
#define TK_ARGV_FUNC			21
#define TK_ARGV_GENFUNC			22
#define TK_ARGV_HELP			23
#define TK_ARGV_CONST_OPTION		24
#define TK_ARGV_OPTION_VALUE		25
#define TK_ARGV_OPTION_NAME_VALUE	26
#define TK_ARGV_END			27

/*
 * Flag bits for passing to Tk_ParseArgv:
 */

#define TK_ARGV_NO_DEFAULTS		0x1
#define TK_ARGV_NO_LEFTOVERS		0x2
#define TK_ARGV_NO_ABBREV		0x4
#define TK_ARGV_DONT_SKIP_FIRST_ARG	0x8

/*
 * Structure used to describe application-specific configuration
 * options:  indicates procedures to call to parse an option and
 * to return a text string describing an option.
 */

typedef int (Tk_OptionParseProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, Tk_Window tkwin, char *value, char *widgRec,
	int offset));
typedef char *(Tk_OptionPrintProc) _ANSI_ARGS_((ClientData clientData,
	Tk_Window tkwin, char *widgRec, int offset,
	Tcl_FreeProc **freeProcPtr));

typedef struct Tk_CustomOption {
    Tk_OptionParseProc *parseProc;	/* Procedure to call to parse an
					 * option and store it in converted
					 * form. */
    Tk_OptionPrintProc *printProc;	/* Procedure to return a printable
					 * string describing an existing
					 * option. */
    ClientData clientData;		/* Arbitrary one-word value used by
					 * option parser:  passed to
					 * parseProc and printProc. */
} Tk_CustomOption;

/*
 * Structure used to specify information for Tk_ConfigureWidget.  Each
 * structure gives complete information for one option, including
 * how the option is specified on the command line, where it appears
 * in the option database, etc.
 */

typedef struct Tk_ConfigSpec {
    int type;			/* Type of option, such as TK_CONFIG_COLOR;
				 * see definitions below.  Last option in
				 * table must have type TK_CONFIG_END. */
    char *argvName;		/* Switch used to specify option in argv.
				 * NULL means this spec is part of a group. */
    char *dbName;		/* Name for option in option database. */
    char *dbClass;		/* Class for option in database. */
    char *defValue;		/* Default value for option if not
				 * specified in command line or database. */
    int offset;			/* Where in widget record to store value;
				 * use Tk_Offset macro to generate values
				 * for this. */
    int specFlags;		/* Any combination of the values defined
				 * below;  other bits are used internally
				 * by tkConfig.c. */
    Tk_CustomOption *customPtr;	/* If type is TK_CONFIG_CUSTOM then this is
				 * a pointer to info about how to parse and
				 * print the option.  Otherwise it is
				 * irrelevant. */
} Tk_ConfigSpec;

/*
 * Type values for Tk_ConfigSpec structures.  See the user
 * documentation for details.
 */

#define TK_CONFIG_BOOLEAN	1
#define TK_CONFIG_INT		2
#define TK_CONFIG_DOUBLE	3
#define TK_CONFIG_STRING	4
#define TK_CONFIG_UID		5
#define TK_CONFIG_JUSTIFY	13
#define TK_CONFIG_ANCHOR	14
#define TK_CONFIG_SYNONYM	15
#define TK_CONFIG_PIXELS	18
#define TK_CONFIG_MM		19
#define TK_CONFIG_WINDOW	20
#define TK_CONFIG_CUSTOM	21
#define TK_CONFIG_END		22

/*
 * Macro to use to fill in "offset" fields of Tk_ConfigInfos.
 * Computes number of bytes from beginning of structure to a
 * given field.
 */

#ifdef offsetof
#define Tk_Offset(type, field) ((int) offsetof(type, field))
#else
#define Tk_Offset(type, field) ((int) ((char *) &((type *) 0)->field))
#endif

/*
 * Possible values for flags argument to Tk_ConfigureWidget:
 */

#define TK_CONFIG_ARGV_ONLY	1

/*
 * Possible flag values for Tk_ConfigInfo structures.  Any bits at
 * or above TK_CONFIG_USER_BIT may be used by clients for selecting
 * certain entries.  Before changing any values here, coordinate with
 * tkConfig.c (internal-use-only flags are defined there).
 */

#define TK_CONFIG_COLOR_ONLY		1
#define TK_CONFIG_MONO_ONLY		2
#define TK_CONFIG_NULL_OK		4
#define TK_CONFIG_DONT_SET_DEFAULT	8
#define TK_CONFIG_OPTION_SPECIFIED	0x10
#define TK_CONFIG_USER_BIT		0x100

/*
 * Special return value from Tk_FileProc2 procedures indicating that
 * an event was successfully processed.
 */

#define TK_FILE_HANDLED -1

/*
 * Flag values to pass to Tk_DoOneEvent to disable searches
 * for some kinds of events:
 */

#define TK_DONT_WAIT		TCL_DONT_WAIT
#define TK_X_EVENTS		TCL_WINDOW_EVENTS
#define TK_FILE_EVENTS		TCL_FILE_EVENTS
#define TK_TIMER_EVENTS		TCL_TIMER_EVENTS
#define TK_IDLE_EVENTS		TCL_IDLE_EVENTS
#define TK_ALL_EVENTS		TCL_ALL_EVENTS

/*
 * Priority levels to pass to Tk_AddOption:
 */

#define TK_WIDGET_DEFAULT_PRIO	20
#define TK_STARTUP_FILE_PRIO	40
#define TK_USER_DEFAULT_PRIO	60
#define TK_INTERACTIVE_PRIO	80
#define TK_MAX_PRIO		100

/*
 * Enumerated type for describing a point by which to anchor something:
 */

typedef enum {
    TK_ANCHOR_N, TK_ANCHOR_NE, TK_ANCHOR_E, TK_ANCHOR_SE,
    TK_ANCHOR_S, TK_ANCHOR_SW, TK_ANCHOR_W, TK_ANCHOR_NW,
    TK_ANCHOR_CENTER
} Tk_Anchor;

/*
 * Enumerated type for describing a style of justification:
 */

typedef enum {
    TK_JUSTIFY_LEFT, TK_JUSTIFY_RIGHT, TK_JUSTIFY_CENTER
} Tk_Justify;

/*
 * Each geometry manager (the packer, the placer, etc.) is represented
 * by a structure of the following form, which indicates procedures
 * to invoke in the geometry manager to carry out certain functions.
 */

typedef void (Tk_GeomRequestProc) _ANSI_ARGS_((ClientData clientData,
	Tk_Window tkwin));
typedef void (Tk_GeomLostSlaveProc) _ANSI_ARGS_((ClientData clientData,
	Tk_Window tkwin));

typedef struct Tk_GeomMgr {
    char *name;			/* Name of the geometry manager (command
				 * used to invoke it, or name of widget
				 * class that allows embedded widgets). */
    Tk_GeomRequestProc *requestProc;
				/* Procedure to invoke when a slave's
				 * requested geometry changes. */
    Tk_GeomLostSlaveProc *lostSlaveProc;
				/* Procedure to invoke when a slave is
				 * taken away from one geometry manager
				 * by another.  NULL means geometry manager
				 * doesn't care when slaves are lost. */
} Tk_GeomMgr;

/*
 * Result values returned by Tk_GetScrollInfo:
 */

#define TK_SCROLL_MOVETO	1
#define TK_SCROLL_PAGES		2
#define TK_SCROLL_UNITS		3
#define TK_SCROLL_ERROR		4


/*
 *--------------------------------------------------------------
 *
 * Macros for querying Tk_Window structures.  See the
 * manual entries for documentation.
 *
 *--------------------------------------------------------------
 */

#define Tk_Display(tkwin)		((tkwin)->dispPtr)
#define Tk_Depth(tkwin)			1
#define Tk_WindowId(tkwin)		(tkwin)
#define Tk_PathName(tkwin) 		((tkwin)->pathName)
#define Tk_Name(tkwin) 			((tkwin)->nameUid)
#define Tk_Class(tkwin) 		((tkwin)->classUid)
#define Tk_X(tkwin)			((tkwin)->rect.left)
#define Tk_Y(tkwin)			((tkwin)->rect.top)
#define Tk_Width(tkwin) \
    ((tkwin)->rect.right - (tkwin)->rect.left)
#define Tk_Height(tkwin) \
    ((tkwin)->rect.bottom - (tkwin)->rect.top)
#define Tk_IsMapped(tkwin)		((tkwin)->flags & TK_MAPPED)
#define Tk_IsTopLevel(tkwin)		((tkwin)->flags & TK_TOP_LEVEL)
#define Tk_ReqWidth(tkwin)		((tkwin)->reqWidth)
#define Tk_ReqHeight(tkwin)		((tkwin)->reqHeight)
#define Tk_InternalBorderWidth(tkwin)	((tkwin)->borderWidth)
#define Tk_BorderWidth(tkwin)		0
#define Tk_Parent(tkwin)		((tkwin)->parentPtr)


typedef struct TkWindow {
    /*
     * Relatives
     */
    struct TkWindow *priorPtr;
    struct TkWindow *nextPtr;
    struct TkWindow *parentPtr;
    struct {
	struct TkWindow *priorPtr;	/* Top child */
	struct TkWindow *nextPtr;	/* Bottom child */
    } childList;

    char *pathName;		/* Full name of window */
    Tk_Uid nameUid;		/* Name of the window within its parent
				 * (unique within the parent). */
    Tk_Uid classUid;		/* Widget class */
    int flags;			/* Various status flags, see below */
    TkMainInfo *mainPtr;	/* Information shared by all windows
				 * associated with a particular main
				 * window. */
    TkDisplay *dispPtr;		/* Display for window. */

    /*
     * Geometry
     */
    Ctk_Rect rect;		/* Window outline, relative to parent.
				 * Undefined if window is not mapped.  */
    int absLeft, absTop;	/* Absolute screen position.  Undefined if
				 * window is not displayed.  */
    int borderWidth;		/* Internal border width.  Does not affect
				 * the window's local coordinate system,
				 * but the border area is removed from
				 * the clipRect so that widget can't draw
				 * on border. */
    Ctk_Rect maskRect;		/* In absolute coordinates.  Represents clipping
				 * by parents.  Used for computing overlap with
				 * other windows. */
    Ctk_Rect clipRect;		/* In absolute coordinates.  Represents clipping
				 * by parents and internal border.  Undefined
				 * if window is not displayed. */
    CtkRegion *clipRgn;		/* In absolute coordinates, represents clipping
				 * by siblings, shared by entire tree of
				 * a top-level window.  Undefined if window
				 * is not displayed. */

    /*
     * Background fill
     */
    Ctk_Style fillStyle;
    int fillChar;

    /*
     * Information kept by the event manager (tkEvent.c):
     */

    TkEventHandler *handlerList;/* First in list of event handlers
				 * declared for this window, or
				 * NULL if none. */

    /*
     * Information used for event bindings (see "bind" and "bindtags"
     * commands in tkCmds.c):
     */

    ClientData *tagPtr;		/* Points to array of tags used for bindings
				 * on this window.  Each tag is a Tk_Uid.
				 * Malloc'ed.  NULL means no tags. */
    int numTags;		/* Number of tags at *tagPtr. */

    /*
     * Information used by tkOption.c to manage options for the
     * window.
     */

    int optionLevel;		/* -1 means no option information is
				 * currently cached for this window.
				 * Otherwise this gives the level in
				 * the option stack at which info is
				 * cached. */
    /*
     * Information used by tkGeometry.c for geometry management.
     */

    Tk_GeomMgr *geomMgrPtr;	/* Information about geometry manager for
				 * this window. */
    ClientData geomData;	/* Argument for geometry manager procedures. */
    int reqWidth, reqHeight;	/* Arguments from last call to
				 * Tk_GeometryRequest, or 0's if
				 * Tk_GeometryRequest hasn't been
				 * called. */
} TkWindow;

typedef TkWindow Tk_FakeWin;

/*
 * Flag values for TkWindow (and Tk_FakeWin) structures are:
 *
 * TK_MAPPED		Is the window positioned in the parent window?
 *			Window has a relative position, but not necessarily
 *			an absolute one.
 *
 * TK_ALREADY_DEAD	If true, free_proc will be called during next
 *			idle period.  BEWARE:  Most of the field are
 *			undefined if this flag is set.  (Which are
 *			valid?).
 *
 * TK_TOP_LEVEL:	1 means this is a top-level window (it
 *			was or will be created as a child of
 *			a root window).
 *
 * CTK_DISPLAYED	Is window and all its ancestors mapped?  Window has an
 *			absolute position.
 *
 * CTK_HAS_TOPLEVEL_CHILD
 *			1 means this window has top-level children (which
 *			won't be in the standard linked list of children
 *			for this window - the will be found as a child
 *			of a root window and must be located by name.)
 */
#define TK_MAPPED		(1<<0)
#define TK_ALREADY_DEAD		(1<<1)
#define TK_TOP_LEVEL		(1<<2)
#define CTK_DISPLAYED		(1<<3)
#define CTK_HAS_TOPLEVEL_CHILD	(1<<4)



/*
 *--------------------------------------------------------------
 *
 * Additional procedure types defined by Tk.
 *
 *--------------------------------------------------------------
 */

typedef void (Tk_EventProc) _ANSI_ARGS_((ClientData clientData,
	XEvent *eventPtr));
typedef void (Tk_FileProc) _ANSI_ARGS_((ClientData clientData, int mask));
typedef int (Tk_FileProc2) _ANSI_ARGS_((ClientData clientData, int mask,
	int flags));
typedef void (Tk_FreeProc) _ANSI_ARGS_((ClientData clientData));
typedef int (Tk_GenericProc) _ANSI_ARGS_((ClientData clientData,
	XEvent *eventPtr));
typedef int (Tk_GetSelProc) _ANSI_ARGS_((ClientData clientData,
	Tcl_Interp *interp, char *portion));
typedef void (Tk_IdleProc) _ANSI_ARGS_((ClientData clientData));
typedef void (Tk_LostSelProc) _ANSI_ARGS_((ClientData clientData));
typedef int (Tk_SelectionProc) _ANSI_ARGS_((ClientData clientData,
	int offset, char *buffer, int maxBytes));
typedef void (Tk_TimerProc) _ANSI_ARGS_((ClientData clientData));

/*
 *--------------------------------------------------------------
 *
 * Exported procedures and variables.
 *
 *--------------------------------------------------------------
 */

EXTERN void		Tk_AddOption _ANSI_ARGS_((Tk_Window tkwin, char *name,
			    char *value, int priority));
EXTERN void		Tk_BindEvent _ANSI_ARGS_((Tk_BindingTable bindingTable,
			    XEvent *eventPtr, Tk_Window tkwin, int numObjects,
			    ClientData *objectPtr));
EXTERN int		Tk_ConfigureInfo _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_ConfigSpec *specs,
			    char *widgRec, char *argvName, int flags));
EXTERN int		Tk_ConfigureValue _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_ConfigSpec *specs,
			    char *widgRec, char *argvName, int flags));
EXTERN int		Tk_ConfigureWidget _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_ConfigSpec *specs,
			    int argc, char **argv, char *widgRec,
			    int flags));
EXTERN Tk_Window	Tk_CoordsToWindow _ANSI_ARGS_((int rootX, int rootY,
			    Tk_Window tkwin));
EXTERN unsigned long	Tk_CreateBinding _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_BindingTable bindingTable, ClientData object,
			    char *eventString, char *command, int append));
EXTERN Tk_BindingTable	Tk_CreateBindingTable _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN void		Tk_CreateEventHandler _ANSI_ARGS_((Tk_Window token,
			    unsigned long mask, Tk_EventProc *proc,
			    ClientData clientData));
EXTERN void		Tk_CreateFileHandler _ANSI_ARGS_((int fd, int mask,
			    Tk_FileProc *proc, ClientData clientData));
EXTERN void		Tk_CreateFileHandler2 _ANSI_ARGS_((int fd,
			    Tk_FileProc2 *proc, ClientData clientData));
EXTERN void		Tk_CreateGenericHandler _ANSI_ARGS_((
			    Tk_GenericProc *proc, ClientData clientData));
EXTERN Tk_Window	Tk_CreateMainWindow _ANSI_ARGS_((Tcl_Interp *interp,
			    char *screenName, char *baseName,
			    char *className));
EXTERN Tk_TimerToken	Tk_CreateTimerHandler _ANSI_ARGS_((int milliseconds,
			    Tk_TimerProc *proc, ClientData clientData));
EXTERN Tk_Window	Tk_CreateWindow _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window parent, char *name, char *screenName));
EXTERN Tk_Window	Tk_CreateWindowFromPath _ANSI_ARGS_((
			    Tcl_Interp *interp, Tk_Window tkwin,
			    char *pathName, char *screenName));
EXTERN void		Tk_DeleteAllBindings _ANSI_ARGS_((
			    Tk_BindingTable bindingTable, ClientData object));
EXTERN int		Tk_DeleteBinding _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_BindingTable bindingTable, ClientData object,
			    char *eventString));
EXTERN void		Tk_DeleteBindingTable _ANSI_ARGS_((
			    Tk_BindingTable bindingTable));
EXTERN void		Tk_DeleteEventHandler _ANSI_ARGS_((Tk_Window token,
			    unsigned long mask, Tk_EventProc *proc,
			    ClientData clientData));
EXTERN void		Tk_DeleteFileHandler _ANSI_ARGS_((int fd));
EXTERN void		Tk_DeleteGenericHandler _ANSI_ARGS_((
			    Tk_GenericProc *proc, ClientData clientData));
EXTERN void		Tk_DeleteTimerHandler _ANSI_ARGS_((
			    Tk_TimerToken token));
EXTERN void		Tk_DestroyWindow _ANSI_ARGS_((Tk_Window tkwin));
EXTERN char *		Tk_DisplayName _ANSI_ARGS_((Tk_Window tkwin));
EXTERN int		Tk_DoOneEvent _ANSI_ARGS_((int flags));
EXTERN void		Tk_EventuallyFree _ANSI_ARGS_((ClientData clientData,
			    Tk_FreeProc *freeProc));
EXTERN void		Tk_FreeOptions _ANSI_ARGS_((Tk_ConfigSpec *specs,
			    char *widgRec, int needFlags));
EXTERN void		Tk_GeometryRequest _ANSI_ARGS_((Tk_Window tkwin,
			    int reqWidth,  int reqHeight));
EXTERN void		Tk_GetAllBindings _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_BindingTable bindingTable, ClientData object));
EXTERN int		Tk_GetAnchor _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, Tk_Anchor *anchorPtr));
EXTERN char *		Tk_GetBinding _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_BindingTable bindingTable, ClientData object,
			    char *eventString));
EXTERN int		Tk_GetJustify _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, Tk_Justify *justifyPtr));
EXTERN Tk_Uid		Tk_GetOption _ANSI_ARGS_((Tk_Window tkwin, char *name,
			    char *className));
EXTERN int		Tk_GetPixels _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, char *string, int *intPtr));
EXTERN void		Tk_GetRootCoords _ANSI_ARGS_ ((Tk_Window tkwin,
			    int *xPtr, int *yPtr));
EXTERN int		Tk_GetScrollInfo _ANSI_ARGS_((Tcl_Interp *interp,
			    int argc, char **argv, double *dblPtr,
			    int *intPtr));
EXTERN int		Tk_GetScreenMM _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, char *string, double *doublePtr));
EXTERN Tk_Uid		Tk_GetUid _ANSI_ARGS_((char *string));
EXTERN void		Tk_GetVRootGeometry _ANSI_ARGS_((Tk_Window tkwin,
			    int *xPtr, int *yPtr, int *widthPtr,
			    int *heightPtr));
EXTERN int		Tk_Grab _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, int grabGlobal));
EXTERN void		Tk_HandleEvent _ANSI_ARGS_((XEvent *eventPtr));
EXTERN int		Tk_Init _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN void		Tk_Main _ANSI_ARGS_((int argc, char **argv,
			    Tcl_AppInitProc *appInitProc));
EXTERN void		Tk_MainLoop _ANSI_ARGS_((void));
EXTERN void		Tk_MaintainGeometry _ANSI_ARGS_((Tk_Window slave,
			    Tk_Window master, int x, int y, int width,
			    int height));
EXTERN Tk_Window	Tk_MainWindow _ANSI_ARGS_((Tcl_Interp *interp));
EXTERN void		Tk_MakeWindowExist _ANSI_ARGS_((Tk_Window tkwin));
EXTERN void		Tk_ManageGeometry _ANSI_ARGS_((Tk_Window tkwin,
			    Tk_GeomMgr *mgrPtr, ClientData clientData));
#define			Tk_MapWindow(tkwin) \
			    Ctk_Map(tkwin, Ctk_Left(tkwin), Ctk_Top(tkwin), \
			    Ctk_Right(tkwin), Ctk_Bottom(tkwin))
#define			Tk_MoveResizeWindow(tkwin, x, y, width, height) \
			    Ctk_Map(tkwin, x, y, (x)+(width), (y)+(height))
EXTERN char *		Tk_NameOfAnchor _ANSI_ARGS_((Tk_Anchor anchor));
EXTERN char *		Tk_NameOfJustify _ANSI_ARGS_((Tk_Justify justify));
EXTERN Tk_Window	Tk_NameToWindow _ANSI_ARGS_((Tcl_Interp *interp,
			    char *pathName, Tk_Window tkwin));
EXTERN int		Tk_ParseArgv _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, int *argcPtr, char **argv,
			    Tk_ArgvInfo *argTable, int flags));
EXTERN void		Tk_Preserve _ANSI_ARGS_((ClientData clientData));
EXTERN void		Tk_Release _ANSI_ARGS_((ClientData clientData));
EXTERN int		Tk_RestackWindow _ANSI_ARGS_((Tk_Window tkwin,
			    int aboveBelow, Tk_Window other));
EXTERN char *		Tk_SetAppName _ANSI_ARGS_((Tk_Window tkwin,
			    char *name));
EXTERN void		Tk_SetClass _ANSI_ARGS_((Tk_Window tkwin,
			    char *className));
EXTERN void		Tk_SetInternalBorder _ANSI_ARGS_((Tk_Window tkwin,
			    int width));
EXTERN void		Tk_Sleep _ANSI_ARGS_((int ms));
EXTERN int		Tk_StrictMotif _ANSI_ARGS_((Tk_Window tkwin));
EXTERN void		Tk_UnmaintainGeometry _ANSI_ARGS_((Tk_Window slave,
			    Tk_Window master));
#define			Tk_UnmapWindow(tkwin)	Ctk_Unmap(tkwin)


EXTERN int		tk_NumMainWindows;

/*
 * Tcl commands peculiar to CTk.
 */

EXTERN int		Ctk_CtkCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Ctk_CtkEventCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Ctk_TkFocusNextCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Ctk_TkFocusPrevCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Ctk_TkEntryInsertCmd _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    int argc, char **argv));
EXTERN int		Ctk_TkEntrySeeInsertCmd _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp,
			    int argc, char **argv));

/*
 * Tcl commands exported by Tk:
 */

EXTERN int		Tk_AfterCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_BellCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_BindCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_BindtagsCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_ButtonCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_CheckbuttonCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_ClipboardCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_DestroyCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_EntryCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_ExitCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_FileeventCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_FrameCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_FocusCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_GrabCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_LabelCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_ListboxCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_LowerCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_MenuCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_MenubuttonCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_MessageCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_OptionCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_PackCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_PlaceCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_RadiobuttonCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_RaiseCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_ScaleCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_ScrollbarCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_TextCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_TkCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_TkwaitCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_UpdateCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_WinfoCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
EXTERN int		Tk_WmCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));

#endif /* _TK */
