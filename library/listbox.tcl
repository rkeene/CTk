# listbox.tcl --
#
# This file defines the default bindings for Tk listbox widgets
# and provides procedures that help in implementing those bindings.
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
# The code below creates the default class bindings for listboxes.
#-------------------------------------------------------------------------

#--------------------------------------------------------------------------
# tkPriv elements used in this file:
#
# listboxPrev -		The last element to be selected or deselected
#			during a selection operation.
# listboxSelection -	All of the items that were selected before the
#			current selection operation (such as a mouse
#			drag) started;  used to cancel an operation.
#--------------------------------------------------------------------------

bind Listbox <Up> {
    tkListboxUpDown %W -1
}
bind Listbox <Down> {
    tkListboxUpDown %W 1
}
bind Listbox <Left> {
    %W xview scroll -1 pages
}
bind Listbox <Right> {
    %W xview scroll 1 pages
}
bind Listbox <Prior> {
    %W yview scroll -1 pages
    tkListboxGoto %W @0,0
}
bind Listbox <Next> {
    %W yview scroll 1 pages
    tkListboxGoto %W @0,9999999
}
bind Listbox <Home> {
    tkListboxGoto %W 0
}
bind Listbox <End> {
    tkListboxGoto %W end
}
#bind Listbox <space> {
#    tkListboxBeginSelect %W [%W index active]
#}
bind Listbox <Return> {
    tkListboxBeginSelect %W [%W index active]
    focus [tk_focusNext %W]
}
bind Listbox <Select> {
    tkListboxBeginSelect %W [%W index active]
}
bind Listbox <Cancel> {
    tkListboxCancel %W
}

# tkListboxBeginSelect --
#
# This procedure is typically invoked on button-1 presses.  It begins
# the process of making a selection in the listbox.  Its exact behavior
# depends on the selection mode currently in effect for the listbox;
# see the Motif documentation for details.
#
# Arguments:
# w -		The listbox widget.
# el -		The element for the selection operation (typically the
#		one under the pointer).  Must be in numerical form.

proc tkListboxBeginSelect {w el} {
    global tkPriv
    if {[$w cget -selectmode]  == "multiple"} {
	if [$w selection includes $el] {
	    $w selection clear $el
	} else {
	    $w selection set $el
	}
    } else {
	$w selection clear 0 end
	$w selection set $el
	$w selection anchor $el
	set tkPriv(listboxSelection) {}
	set tkPriv(listboxPrev) $el
    }
}

# tkListboxMotion --
#
# This procedure is called to process mouse motion events while
# button 1 is down.  It may move or extend the selection, depending
# on the listbox's selection mode.
#
# Arguments:
# w -		The listbox widget.
# el -		The element under the pointer (must be a number).

proc tkListboxMotion {w el} {
    global tkPriv
    if {$el == $tkPriv(listboxPrev)} {
	return
    }
    set anchor [$w index anchor]
    switch [$w cget -selectmode] {
	browse {
	    $w selection clear 0 end
	    $w selection set $el
	    set tkPriv(listboxPrev) $el
	}
	extended {
	    set i $tkPriv(listboxPrev)
	    if [$w selection includes anchor] {
		$w selection clear $i $el
		$w selection set anchor $el
	    } else {
		$w selection clear $i $el
		$w selection clear anchor $el
	    }
	    while {($i < $el) && ($i < $anchor)} {
		if {[lsearch $tkPriv(listboxSelection) $i] >= 0} {
		    $w selection set $i
		}
		incr i
	    }
	    while {($i > $el) && ($i > $anchor)} {
		if {[lsearch $tkPriv(listboxSelection) $i] >= 0} {
		    $w selection set $i
		}
		incr i -1
	    }
	    set tkPriv(listboxPrev) $el
	}
    }
}

# tkListboxBeginExtend --
#
# This procedure is typically invoked on shift-button-1 presses.  It
# begins the process of extending a selection in the listbox.  Its
# exact behavior depends on the selection mode currently in effect
# for the listbox;  see the Motif documentation for details.
#
# Arguments:
# w -		The listbox widget.
# el -		The element for the selection operation (typically the
#		one under the pointer).  Must be in numerical form.

proc tkListboxBeginExtend {w el} {
    if {([$w cget -selectmode] == "extended")
	    && [$w selection includes anchor]} {
	tkListboxMotion $w $el
    }
}

# tkListboxGoto --
#
# Moves the location cursor (active element) to a specifc element,
# and changes the selection if we're in browse or extended selection
# mode.
#
# Arguments:
# w -		The listbox widget.
# index -	Any valid listbox index

proc tkListboxGoto {w index} {
    global tkPriv
    $w activate $index
    $w see active
    switch [$w cget -selectmode] {
	browse {
	    $w selection clear 0 end
	    $w selection set active
	}
	extended {
	    $w selection clear 0 end
	    $w selection set active
	    $w selection anchor active
	    set tkPriv(listboxPrev) [$w index active]
	    set tkPriv(listboxSelection) {}
	}
    }
}

# tkListboxUpDown --
#
# Moves the location cursor (active element) up or down by one element,
# and changes the selection if we're in browse or extended selection
# mode.
#
# Arguments:
# w -		The listbox widget.
# amount -	+1 to move down one item, -1 to move back one item.

proc tkListboxUpDown {w amount} {
    global tkPriv
    $w activate [expr [$w index active] + $amount]
    $w see active
    switch [$w cget -selectmode] {
	browse {
	    $w selection clear 0 end
	    $w selection set active
	}
	extended {
	    $w selection clear 0 end
	    $w selection set active
	    $w selection anchor active
	    set tkPriv(listboxPrev) [$w index active]
	    set tkPriv(listboxSelection) {}
	}
    }
}

# tkListboxExtendUpDown --
#
# Does nothing unless we're in extended selection mode;  in this
# case it moves the location cursor (active element) up or down by
# one element, and extends the selection to that point.
#
# Arguments:
# w -		The listbox widget.
# amount -	+1 to move down one item, -1 to move back one item.

proc tkListboxExtendUpDown {w amount} {
    if {[$w cget -selectmode] != "extended"} {
	return
    }
    $w activate [expr [$w index active] + $amount]
    $w see active
    tkListboxMotion $w [$w index active]
}

# tkListboxDataExtend
#
# This procedure is called for key-presses such as Shift-KEndData.
# If the selection mode isn't multiple or extend then it does nothing.
# Otherwise it moves the active element to el and, if we're in
# extended mode, extends the selection to that point.
#
# Arguments:
# w -		The listbox widget.
# el -		An integer element number.

proc tkListboxDataExtend {w el} {
    set mode [$w cget -selectmode]
    if {$mode == "extended"} {
	$w activate $el
	$w see $el
        if [$w selection includes anchor] {
	    tkListboxMotion $w $el
	}
    } elseif {$mode == "multiple"} {
	$w activate $el
	$w see $el
    }
}

# tkListboxCancel
#
# This procedure is invoked to cancel an extended selection in
# progress.  If there is an extended selection in progress, it
# restores all of the items between the active one and the anchor
# to their previous selection state.
#
# Arguments:
# w -		The listbox widget.

proc tkListboxCancel w {
    global tkPriv
    if {[$w cget -selectmode] != "extended"} {
	return
    }
    set first [$w index anchor]
    set last $tkPriv(listboxPrev)
    if {$first > $last} {
	set tmp $first
	set first $last
	set last $tmp
    }
    $w selection clear $first $last
    while {$first <= $last} {
	if {[lsearch $tkPriv(listboxSelection) $first] >= 0} {
	    $w selection set $first
	}
	incr first
    }
}

# tkListboxSelectAll
#
# This procedure is invoked to handle the "select all" operation.
# For single and browse mode, it just selects the active element.
# Otherwise it selects everything in the widget.
#
# Arguments:
# w -		The listbox widget.

proc tkListboxSelectAll w {
    set mode [$w cget -selectmode]
    if {($mode == "single") || ($mode == "browse")} {
	$w selection clear 0 end
	$w selection set active
    } else {
	$w selection set 0 end
    }
}
