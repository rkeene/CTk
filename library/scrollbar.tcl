# scrollbar.tcl --
#
# This file defines the default bindings for Tk scrollbar widgets.
#
# @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
#
# Copyright (c) 1994 The Regents of the University of California.
# Copyright (c) 1994-1995 Sun Microsystems, Inc.
# Copyright (c) 1995 Cleveland Clinic Foundation
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#-------------------------------------------------------------------------
# The code below creates the default class bindings for scrollbars.
#-------------------------------------------------------------------------

# Standard Motif bindings:

bind Scrollbar <Up> {
    tkScrollByUnits %W v -1
}
bind Scrollbar <Down> {
    tkScrollByUnits %W v 1
}
bind Scrollbar <Left> {
    tkScrollByUnits %W h -1
}
bind Scrollbar <Right> {
    tkScrollByUnits %W h 1
}
bind Scrollbar <Prior> {
    tkScrollByPages %W hv -1
}
bind Scrollbar <Next> {
    tkScrollByPages %W hv 1
}
bind Scrollbar <Home> {
    tkScrollToPos %W 0
}
bind Scrollbar <End> {
    tkScrollToPos %W 1
}

# tkScrollByUnits --
# This procedure tells the scrollbar's associated widget to scroll up
# or down by a given number of units.  It notifies the associated widget
# in different ways for old and new command syntaxes.
#
# Arguments:
# w -		The scrollbar widget.
# orient -	Which kinds of scrollbars this applies to:  "h" for
#		horizontal, "v" for vertical, "hv" for both.
# amount -	How many units to scroll:  typically 1 or -1.

proc tkScrollByUnits {w orient amount} {
    set cmd [$w cget -command]
    if {($cmd == "") || ([string first \
	    [string index [$w cget -orient] 0] $orient] < 0)} {
	return
    }
    set info [$w get]
    if {[llength $info] == 2} {
	uplevel #0 $cmd scroll $amount units
    } else {
	uplevel #0 $cmd [expr [lindex $info 2] + $amount]
    }
}

# tkScrollByPages --
# This procedure tells the scrollbar's associated widget to scroll up
# or down by a given number of screenfuls.  It notifies the associated
# widget in different ways for old and new command syntaxes.
#
# Arguments:
# w -		The scrollbar widget.
# orient -	Which kinds of scrollbars this applies to:  "h" for
#		horizontal, "v" for vertical, "hv" for both.
# amount -	How many screens to scroll:  typically 1 or -1.

proc tkScrollByPages {w orient amount} {
    set cmd [$w cget -command]
    if {($cmd == "") || ([string first \
	    [string index [$w cget -orient] 0] $orient] < 0)} {
	return
    }
    set info [$w get]
    if {[llength $info] == 2} {
	uplevel #0 $cmd scroll $amount pages
    } else {
	uplevel #0 $cmd [expr [lindex $info 2] + $amount*([lindex $info 1] - 1)]
    }
}

# tkScrollToPos --
# This procedure tells the scrollbar's associated widget to scroll to
# a particular location, given by a fraction between 0 and 1.  It notifies
# the associated widget in different ways for old and new command syntaxes.
#
# Arguments:
# w -		The scrollbar widget.
# pos -		A fraction between 0 and 1 indicating a desired position
#		in the document.

proc tkScrollToPos {w pos} {
    set cmd [$w cget -command]
    if {($cmd == "")} {
	return
    }
    set info [$w get]
    if {[llength $info] == 2} {
	uplevel #0 $cmd moveto $pos
    } else {
	uplevel #0 $cmd [expr round([lindex $info 0]*$pos)]
    }
}

