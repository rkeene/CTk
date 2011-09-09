# button.tcl --
#
# This file defines the default bindings for Tk label, button,
# checkbutton, and radiobutton widgets and provides procedures
# that help in implementing those bindings.
#
# @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
#
# Copyright (c) 1992-1994 The Regents of the University of California.
# Copyright (c) 1994 Sun Microsystems, Inc.
# Copyright (c) 1995 Cleveland Clinic Foundation
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#-------------------------------------------------------------------------
# The code below creates the default class bindings for buttons.
#-------------------------------------------------------------------------

bind Button <Select> {
    tkButtonInvoke %W
}
bind Button <Return> {
    if !$tk_strictMotif {
	tkButtonInvoke %W
    }
}

bind Checkbutton <Select> {
    tkCheckRadioInvoke %W
}
bind Button <Return> {
    if !$tk_strictMotif {
	tkCheckRadioInvoke %W
    }
}

bind Radiobutton <Select> {
    tkCheckRadioInvoke %W
}
bind Button <Return> {
    if !$tk_strictMotif {
	tkCheckRadioInvoke %W
    }
}

# tkButtonInvoke --
# The procedure below is called when a button is invoked through
# the keyboard.  It simulate a press of the button via the mouse.
#
# Arguments:
# w -		The name of the widget.

proc tkButtonInvoke w {
    if {[$w cget -state] != "disabled"} {
	set oldState [$w cget -state]
	$w configure -state active
	update idletasks
	after 100
	$w configure -state $oldState
	uplevel #0 [list $w invoke]
    }
}

# tkCheckRadioInvoke --
# The procedure below is invoked when the mouse button is pressed in
# a checkbutton or radiobutton widget, or when the widget is invoked
# through the keyboard.  It invokes the widget if it
# isn't disabled.
#
# Arguments:
# w -		The name of the widget.

proc tkCheckRadioInvoke w {
    if {[$w cget -state] != "disabled"} {
	uplevel #0 [list $w invoke]
    }
}
