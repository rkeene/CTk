/*
 * default.h (CTk) --
 *
 *	This file defines the defaults for all options for all of
 *	the CTk widgets.
 *
 * Copyright (c) 1991-1994 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 * Copyright (c) 1994-1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
 */

#ifndef _DEFAULT
#define _DEFAULT

/*
 * Defaults for labels, buttons, checkbuttons, and radiobuttons:
 */

#define DEF_BUTTON_ANCHOR		"center"
#define DEF_BUTTON_BORDER_WIDTH		"0"
#define DEF_BUTTON_COMMAND		""
#define DEF_BUTTON_HEIGHT		"-1"
#define DEF_BUTTON_INDICATOR		"1"
#define DEF_BUTTON_JUSTIFY		"center"
#define DEF_BUTTON_OFF_VALUE		"0"
#define DEF_BUTTON_ON_VALUE		"1"
#define DEF_BUTTON_PADX			"0"
#define DEF_BUTTON_PADY			"0"
#define DEF_BUTTON_STATE		"normal"
#define DEF_LABEL_TAKE_FOCUS		"0"
#define DEF_BUTTON_TAKE_FOCUS		(char *) NULL
#define DEF_BUTTON_TEXT			" "
#define DEF_BUTTON_TEXT_VARIABLE	""
#define DEF_BUTTON_UNDERLINE		"-1"
#define DEF_BUTTON_VALUE		""
#define DEF_BUTTON_WIDTH		"-1"
#define DEF_BUTTON_WRAP_LENGTH		"0"
#define DEF_RADIOBUTTON_VARIABLE	"selectedButton"
#define DEF_CHECKBUTTON_VARIABLE	""

/*
 * Defaults for entries:
 */

#define DEF_ENTRY_BORDER_WIDTH		"0"
#define DEF_ENTRY_JUSTIFY		"left"
#define DEF_ENTRY_SCROLL_COMMAND	""
#define DEF_ENTRY_SHOW			(char *) NULL
#define DEF_ENTRY_STATE			"normal"
#define DEF_ENTRY_TAKE_FOCUS		(char *) NULL
#define DEF_ENTRY_TEXT_VARIABLE		""
#define DEF_ENTRY_WIDTH			"20"

/*
 * Defaults for frames:
 */

#define DEF_FRAME_BORDER_WIDTH		"0"
#define DEF_FRAME_CLASS			"Frame"
#define DEF_FRAME_HEIGHT		"0"
#define DEF_FRAME_TAKE_FOCUS		"0"
#define DEF_FRAME_WIDTH			"0"

/*
 * Defaults for listboxes:
 */

#define DEF_LISTBOX_BORDER_WIDTH	"1"
#define DEF_LISTBOX_HEIGHT		"10"
#define DEF_LISTBOX_SCROLL_COMMAND	""
#define DEF_LISTBOX_SELECT_MODE		"browse"
#define DEF_LISTBOX_TAKE_FOCUS		(char *) NULL
#define DEF_LISTBOX_WIDTH		"20"

/*
 * Defaults for individual entries of menus:
 */

#define DEF_MENU_ENTRY_ACCELERATOR	(char *) NULL
#define DEF_MENU_ENTRY_COMMAND		(char *) NULL
#define DEF_MENU_ENTRY_INDICATOR	"1"
#define DEF_MENU_ENTRY_LABEL		(char *) NULL
#define DEF_MENU_ENTRY_MENU		(char *) NULL
#define DEF_MENU_ENTRY_OFF_VALUE	"0"
#define DEF_MENU_ENTRY_ON_VALUE		"1"
#define DEF_MENU_ENTRY_STATE		"normal"
#define DEF_MENU_ENTRY_VALUE		(char *) NULL
#define DEF_MENU_ENTRY_CHECK_VARIABLE	(char *) NULL
#define DEF_MENU_ENTRY_RADIO_VARIABLE	"selectedButton"
#define DEF_MENU_ENTRY_UNDERLINE	"-1"

/*
 * Defaults for menus overall:
 */

#define DEF_MENU_BORDER_WIDTH		"1"
#define DEF_MENU_POST_COMMAND		""
#define DEF_MENU_TAKE_FOCUS		"0"
#define DEF_MENU_TEAROFF		"0"

/*
 * Defaults for menubuttons:
 */

#define DEF_MENUBUTTON_ANCHOR		"center"
#define DEF_MENUBUTTON_BORDER_WIDTH	"0"
#define DEF_MENUBUTTON_HEIGHT		"-1"
#define DEF_MENUBUTTON_INDICATOR	"0"
#define DEF_MENUBUTTON_JUSTIFY		"center"
#define DEF_MENUBUTTON_MENU		""
#define DEF_MENUBUTTON_PADX		"0"
#define DEF_MENUBUTTON_PADY		"0"
#define DEF_MENUBUTTON_STATE		"normal"
#define DEF_MENUBUTTON_TAKE_FOCUS	(char *) NULL
#define DEF_MENUBUTTON_TEXT		" "
#define DEF_MENUBUTTON_TEXT_VARIABLE	""
#define DEF_MENUBUTTON_UNDERLINE	"-1"
#define DEF_MENUBUTTON_WIDTH		"-1"
#define DEF_MENUBUTTON_WRAP_LENGTH	"0"

/*
 * Defaults for scrollbars:
 */

#define DEF_SCROLLBAR_BORDER_WIDTH	"0"
#define DEF_SCROLLBAR_COMMAND		""
#define DEF_SCROLLBAR_ORIENT		"vertical"
#define DEF_SCROLLBAR_TAKE_FOCUS	"0"
#define DEF_SCROLLBAR_WIDTH		"1"

/*
 * Defaults for texts:
 */

#define DEF_TEXT_BORDER_WIDTH		"1"
#define DEF_TEXT_HEIGHT			"10"
#define DEF_TEXT_PADX			"0"
#define DEF_TEXT_PADY			"0"
#define DEF_TEXT_SPACING1		"0"
#define DEF_TEXT_SPACING2		"0"
#define DEF_TEXT_SPACING3		"0"
#define DEF_TEXT_STATE			"normal"
#define DEF_TEXT_TABS			""
#define DEF_TEXT_TAKE_FOCUS		(char *) NULL
#define DEF_TEXT_WIDTH			"40"
#define DEF_TEXT_WRAP			"char"
#define DEF_TEXT_XSCROLL_COMMAND	""
#define DEF_TEXT_YSCROLL_COMMAND	""

/*
 * Defaults for toplevels (most of the defaults for frames also apply
 * to toplevels):
 */

#define DEF_TOPLEVEL_BORDER_WIDTH	"1"
#define DEF_TOPLEVEL_CLASS		"Toplevel"
#define DEF_TOPLEVEL_SCREEN		""
#define DEF_TOPLEVEL_TITLE		(char *) NULL


#endif /* _DEFAULT */
