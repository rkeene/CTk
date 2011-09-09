/*
 * keyCodes.h (CTk) --
 *
 *	This file defines the mapping from curses key codes to
 *	to X11 keysyms and modifier masks.
 *
 * Copyright (c) 1995 Cleveland Clinic Foundation
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * @(#) $Header: /usrs/andrewm/work/RCS/ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
 */

    { 0001, 0x0061, ControlMask },	/* Control-A */
    { 0002, 0x0062, ControlMask },	/* Control-B */
    { 0003, 0x0063, ControlMask },	/* Control-C */
    { 0004, 0x0064, ControlMask },	/* Control-D */
    { 0005, 0x0065, ControlMask },	/* Control-E */
    { 0006, 0x0066, ControlMask },	/* Control-F */
    { 0007, 0x0067, ControlMask },	/* Control-G */
    { 0010, 0xFF08, 0 },		/* Backspace (Control-H) */
    { 0177, 0xFF08, 0 },		/* Backspace (Control-?) */
    { 0011, 0xFF09, 0 },		/* Tab (Control-I) */
    { 0012, 0x006A, ControlMask },	/* Control-J */
    { 0013, 0x006B, ControlMask },	/* Control-K */
    { 0014, 0x006C, ControlMask },	/* Control-L */
    { 0015, 0xFF0D, 0 },		/* Carriage Return (Control-M) */
    { 0016, 0x006E, ControlMask },	/* Control-N */
    { 0017, 0x006F, ControlMask },	/* Control-O */
    { 0020, 0x0070, ControlMask },	/* Control-P */
    { 0021, 0x0071, ControlMask },	/* Control-Q */
    { 0022, 0x0072, ControlMask },	/* Control-R */
    { 0023, 0x0073, ControlMask },	/* Control-S */
    { 0024, 0x0074, ControlMask },	/* Control-T */
    { 0025, 0x0075, ControlMask },	/* Control-U */
    { 0026, 0x0076, ControlMask },	/* Control-V */
    { 0027, 0x0077, ControlMask },	/* Control-W */
    { 0030, 0x0078, ControlMask },	/* Control-X */
    { 0031, 0x0079, ControlMask },	/* Control-Y */
    { 0032, 0x007A, ControlMask },	/* Control-Z */
    { 0033, 0xFF1B, 0 },		/* Escape (deprecated) */
#ifdef KEY_BREAK
    { KEY_BREAK, 0xFF6B, 0 },		/* Break key (unreliable) */
#endif
#ifdef KEY_DOWN
    { KEY_DOWN, 0xFF54, 0 },		/* Down */
#endif
#ifdef KEY_UP
    { KEY_UP, 0xFF52, 0 },		/* Up */
#endif
#ifdef KEY_LEFT
    { KEY_LEFT, 0xFF51, 0 },		/* Left */
#endif
#ifdef KEY_RIGHT
    { KEY_RIGHT, 0xFF53, 0 },		/* Right */
#endif
#ifdef KEY_HOME
    { KEY_HOME, 0xFF50, 0 },		/* Home key (upward+left arrow) */
#endif
#ifdef KEY_BACKSPACE
    { KEY_BACKSPACE, 0xFF08, 0 },	/* backspace (unreliable) */
#endif
#ifdef KEY_F
    { KEY_F(1), 0xFFBE, 0 },		/* F1 */
    { KEY_F(2), 0xFFBF, 0 },		/* F2 */
    { KEY_F(3), 0xFFC0, 0 },		/* F3 */
    { KEY_F(4), 0xFFC1, 0 },		/* F4 */
    { KEY_F(5), 0xFFC2, 0 },		/* F5 */
    { KEY_F(6), 0xFFC3, 0 },		/* F6 */
    { KEY_F(7), 0xFFC4, 0 },		/* F7 */
    { KEY_F(8), 0xFFC5, 0 },		/* F8 */
    { KEY_F(9), 0xFFC6, 0 },		/* F9 */
    { KEY_F(10), 0xFFC7, 0 },		/* F10 */
#endif
#ifdef KEY_DL
    { KEY_DL, 0xFFFF, ShiftMask },	/* Delete line */
#endif
#ifdef KEY_IL
    { KEY_IL, 0xFF63, ShiftMask },	/* Insert line */
#endif
#ifdef KEY_DC
    { KEY_DC, 0xFFFF, 0 },		/* Delete character */
#endif
#ifdef KEY_IC
    { KEY_IC, 0xFF63, 0 },		/* Insert character/mode */
#endif
#ifdef KEY_EIC
    { KEY_EIC, 0xFF63, 0 },		/* Exit insert mode */
#endif
#ifdef KEY_CLEAR
    { KEY_CLEAR, 0xFF0B, 0 },		/* Clear screen */
#endif
#ifdef KEY_NPAGE
    { KEY_NPAGE, 0xFF56, 0 },		/* Next page */
#endif
#ifdef KEY_PPAGE
    { KEY_PPAGE, 0xFF55, 0 },		/* Previous page */
#endif
#ifdef KEY_ENTER
    { KEY_ENTER, 0xFF8D, 0 },		/* Enter or send (unreliable) */
#endif
#ifdef KEY_PRINT
    { KEY_PRINT, 0xFF61, 0 },		/* Print or copy */
#endif
#ifdef KEY_LL
    { KEY_LL, 0xFF57, ControlMask },	/* home down or bottom (lower left) */
#endif
#ifdef KEY_BTAB
    { KEY_BTAB, 0xFF09, ShiftMask },	/* Back tab */
#endif
#ifdef KEY_BEG
    { KEY_BEG, 0xFF58, 0 },		/* beg(inning) key */
#endif
#ifdef KEY_CANCEL
    { KEY_CANCEL, 0xFF69, 0 },		/* cancel key */
#endif
#ifdef KEY_COMMAND
    { KEY_COMMAND, 0xFF62, 0 },		/* cmd (command) key */
#endif
#ifdef KEY_END
    { KEY_END, 0xFF57, 0 },		/* End key */
#endif
#ifdef KEY_FIND
    { KEY_FIND, 0xFF68, 0 },		/* Find key */
#endif
#ifdef KEY_HELP
    { KEY_HELP, 0xFF6A, 0 },		/* Help key */
#endif
#ifdef KEY_NEXT
    { KEY_NEXT, 0xFF09, 0 },		/* Next object key */
#endif
#ifdef KEY_OPTIONS
    { KEY_OPTIONS, 0xFF67, 0 },		/* Options key */
#endif
#ifdef KEY_PREVIOUS
    { KEY_PREVIOUS, 0xFF09, ShiftMask },/* Previous object key */
#endif
#ifdef KEY_REDO
    { KEY_REDO, 0xFF66, 0 },		/* Redo key */
#endif
#ifdef KEY_SELECT
    { KEY_SELECT, 0xFF60, 0 },		/* Select key */
#endif
#ifdef KEY_SUSPEND
    { KEY_SUSPEND, 0xFF13, 0 },		/* Suspend key */
#endif
#ifdef KEY_UNDO
    { KEY_UNDO, 0xFF65, 0 },		/* Undo key */
#endif
