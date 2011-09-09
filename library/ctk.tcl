# ctk.tcl --
#
# Initialization script normally executed in the interpreter for each
# CTk-based application.  Arranges class bindings for widgets.
#
# @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
#
# Copyright (c) 1992-1994 The Regents of the University of California.
# Copyright (c) 1994-1995 Sun Microsystems, Inc.
# Copyright (c) 1995 Cleveland Clinic Foundation
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

# Insist on running with compatible versions of Tcl and Tk.

scan [info tclversion] "%d.%d" a b
if {$a != 8} {
    error "wrong version of Tcl loaded ([info tclversion]): need 8.x"
}
scan $tk_version "%d.%d" a b
if {($a != 8) || ($b < 0)} {
    error "wrong version of Tk loaded ($tk_version): need 8.x"
}
if {$tk_port != "curses"} {
    error "wrong port of Tk loaded ($tk_port): need curses"
}
unset a b

# Add Tk's directory to the end of the auto-load search path:

lappend auto_path $tk_library

# Turn off strict Motif look and feel as a default.

set tk_strictMotif 0

# ctk_unsupported --
# This procedure is invoked when a Tk command that is not implemented
# by Ctk is invoked.
#
# Arguments:
# cmd -		command that was invoked

proc ctk_unsupported cmd {
    catch {tkerror "Unsupported Tk command: $cmd"}
}

# tkScreenChanged --
# This procedure is invoked by the binding mechanism whenever the
# "current" screen is changing.  The procedure does two things.
# First, it uses "upvar" to make global variable "tkPriv" point at an
# array variable that holds state for the current display.  Second,
# it initializes the array if it didn't already exist.
#
# Arguments:
# screen -		The name of the new screen.

proc tkScreenChanged screen {
    set disp [file rootname $screen]
    uplevel #0 upvar #0 tkPriv.$disp tkPriv
    global tkPriv
    if [info exists tkPriv] {
	set tkPriv(screen) $screen
	return
    }
    set tkPriv(afterId) {}
    set tkPriv(buttons) 0
    set tkPriv(buttonWindow) {}
    set tkPriv(dragging) 0
    set tkPriv(focus) {}
    set tkPriv(grab) {}
    set tkPriv(grabType) {}
    set tkPriv(inMenubutton) {}
    set tkPriv(initMouse) {}
    set tkPriv(listboxPrev) {}
    set tkPriv(mouseMoved) 0
    set tkPriv(oldGrab) {}
    set tkPriv(popup) {}
    set tkPriv(postedMb) {}
    set tkPriv(screen) $screen
    set tkPriv(selectMode) char
    set tkPriv(window) {}
}

# Do initial setup for tkPriv, so that it is always bound to something
# (otherwise, if someone references it, it may get set to a non-upvar-ed
# value, which will cause trouble later).

tkScreenChanged [winfo screen .]

# ----------------------------------------------------------------------
# Read in files that define all of the class bindings.
# ----------------------------------------------------------------------

source $tk_library/button.tcl
source $tk_library/entry.tcl
source $tk_library/listbox.tcl
source $tk_library/menu.tcl
source $tk_library/scrollbar.tcl
source $tk_library/text.tcl

# ----------------------------------------------------------------------
# Default bindings for keyboard traversal.
# ----------------------------------------------------------------------

bind all <Tab> {focus [tk_focusNext %W]}
bind all <Shift-Tab> {focus [tk_focusPrev %W]}

bind all <Control-c> ctk_menu
bind all <Escape> {ctk_event %W KeyPress -key Cancel}
bind all <KP_Enter> {ctk_event %W KeyPress -key Execute}
bind all <space> {ctk_event %W KeyPress -key Select}

bind all <F1> {ctk_event %W KeyPress -key Help}
bind all <F2> {ctk_event %W KeyPress -key Execute}
bind all <F3> {ctk_event %W KeyPress -key Menu}
bind all <F4> {ctk_event %W KeyPress -key Cancel}
