# text.tcl --
#
# This file defines the default bindings for Tk text widgets and provides
# procedures that help in implementing the bindings.
#
# @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
#
# Copyright (c) 1992-1994 The Regents of the University of California.
# Copyright (c) 1994-1995 Sun Microsystems, Inc.
# Copyright (c) 1995 Cleveland Clinic Foundation
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#-------------------------------------------------------------------------
# Elements of tkPriv that are used in this file:
#
# char -		Character position on the line;  kept in order
#			to allow moving up or down past short lines while
#			still remembering the desired position.
# prevPos -		Used when moving up or down lines via the keyboard.
#			Keeps track of the previous insert position, so
#			we can distinguish a series of ups and downs, all
#			in a row, from a new up or down.
# selectMode -		The style of selection currently underway:
#			char, word, or line.
#-------------------------------------------------------------------------


#-------------------------------------------------------------------------
# The code below creates the default class bindings for entries.
#-------------------------------------------------------------------------

# Standard Motif bindings:

bind Text <Left> {
    tkTextSetCursor %W [%W index {insert - 1c}]
}
bind Text <Right> {
    tkTextSetCursor %W [%W index {insert + 1c}]
}
bind Text <Up> {
    tkTextSetCursor %W [tkTextUpDownLine %W -1]
}
bind Text <Down> {
    tkTextSetCursor %W [tkTextUpDownLine %W 1]
}
bind Text <Prior> {
    tkTextSetCursor %W [tkTextScrollPages %W -1]
}
bind Text <Next> {
    tkTextSetCursor %W [tkTextScrollPages %W 1]
}

bind Text <Home> {
    tkTextSetCursor %W {insert linestart}
}
bind Text <End> {
    tkTextSetCursor %W {insert lineend}
}

#    bind Text <Tab> {
#	tkTextInsert %W \t
#	focus %W
#	break
#    }
#    bind Text <Shift-Tab> {
#	# Needed only to keep <Tab> binding from triggering;  doesn't
#	# have to actually do anything.
#    }

bind Text <Control-i> {
    tkTextInsert %W \t
}
bind Text <Return> {
    tkTextInsert %W \n
}
bind Text <Delete> {
    if {[%W tag nextrange sel 1.0 end] != ""} {
	%W delete sel.first sel.last
    } else {
	%W delete insert
	%W see insert
    }
}
bind Text <BackSpace> {
    if {[%W tag nextrange sel 1.0 end] != ""} {
	%W delete sel.first sel.last
    } elseif [%W compare insert != 1.0] {
	%W delete insert-1c
	%W see insert
    }
}

bind Text <Select> {
    %W mark set anchor insert
}
bind Text <Insert> {
    catch {tkTextInsert %W [selection get -displayof %W]}
}
bind Text <KeyPress> {
    if [tkTextInsert %W %A] break
}

# Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for <Escape> and <Tab>.

bind Text <Alt-KeyPress> {# nothing }
bind Text <Meta-KeyPress> {# nothing}
bind Text <Control-KeyPress> {# nothing}
bind Text <Escape> {# nothing}
bind Text <Tab> {# nothing}
bind Text <KP_Enter> {# nothing}

# Additional emacs-like bindings:

if !$tk_strictMotif {
    bind Text <Control-a> {
	tkTextSetCursor %W {insert linestart}
    }
    bind Text <Control-b> {
	tkTextSetCursor %W insert-1c
    }
    bind Text <Control-d> {
	%W delete insert
    }
    bind Text <Control-e> {
	tkTextSetCursor %W {insert lineend}
    }
    bind Text <Control-f> {
	tkTextSetCursor %W insert+1c
    }
    bind Text <Control-k> {
	if [%W compare insert == {insert lineend}] {
	    %W delete insert
	} else {
	    %W delete insert {insert lineend}
	}
    }
    bind Text <Control-n> {
	tkTextSetCursor %W [tkTextUpDownLine %W 1]
    }
    bind Text <Control-o> {
	%W insert insert \n
	%W mark set insert insert-1c
    }
    bind Text <Control-p> {
	tkTextSetCursor %W [tkTextUpDownLine %W -1]
    }
    bind Text <Control-t> {
	tkTextTranspose %W
    }
}
set tkPriv(prevPos) {}

# tkTextKeyExtend --
# This procedure handles extending the selection from the keyboard,
# where the point to extend to is really the boundary between two
# characters rather than a particular character.
#
# Arguments:
# w -		The text window.
# index -	The point to which the selection is to be extended.

proc tkTextKeyExtend {w index} {
    global tkPriv

    set cur [$w index $index]
    if [catch {$w index anchor}] {
	$w mark set anchor $cur
    }
    set anchor [$w index anchor]
    if [$w compare $cur < anchor] {
	set first $cur
	set last anchor
    } else {
	set first anchor
	set last $cur
    }
    $w tag remove sel 0.0 $first
    $w tag add sel $first $last
    $w tag remove sel $last end
}

# tkTextSetCursor
# Move the insertion cursor to a given position in a text.  Also
# clears the selection, if there is one in the text, and makes sure
# that the insertion cursor is visible.  Also, don't let the insertion
# cursor appear on the dummy last line of the text.
#
# Arguments:
# w -		The text window.
# pos -		The desired new position for the cursor in the window.

proc tkTextSetCursor {w pos} {
    global tkPriv

    if [$w compare $pos == end] {
	set pos {end - 1 chars}
    }
    $w mark set insert $pos
    $w tag remove sel 1.0 end
    $w see insert
}

# tkTextKeySelect
# This procedure is invoked when stroking out selections using the
# keyboard.  It moves the cursor to a new position, then extends
# the selection to that position.
#
# Arguments:
# w -		The text window.
# new -		A new position for the insertion cursor (the cursor hasn't
#		actually been moved to this position yet).

proc tkTextKeySelect {w new} {
    global tkPriv

    if {[$w tag nextrange sel 1.0 end] == ""} {
	if [$w compare $new < insert] {
	    $w tag add sel $new insert
	} else {
	    $w tag add sel insert $new
	}
	$w mark set anchor insert
    } else {
	if [$w compare $new < anchor] {
	    set first $new
	    set last anchor
	} else {
	    set first anchor
	    set last $new
	}
	$w tag remove sel 1.0 $first
	$w tag add sel $first $last
	$w tag remove sel $last end
    }
    $w mark set insert $new
    $w see insert
    update idletasks
}

# tkTextInsert --
# Insert a string into a text at the point of the insertion cursor.
# If there is a selection in the text, and it covers the point of the
# insertion cursor, then delete the selection before inserting.
#
# Arguments:
# w -		The text window in which to insert the string
# s -		The string to insert (usually just a single character)
#
# Results:
#	Returns 1 if any characters are inserted, 0 otherwise.

proc tkTextInsert {w s} {
    if {$s == "" || ([$w cget -state] == "disabled")} {return 0}
    catch {
	if {[$w compare sel.first <= insert]
		&& [$w compare sel.last >= insert]} {
	    $w delete sel.first sel.last
	}
    }
    $w insert insert $s
    $w see insert
    return 1
}

# tkTextUpDownLine --
# Returns the index of the character one line above or below the
# insertion cursor.  There are two tricky things here.  First,
# we want to maintain the original column across repeated operations,
# even though some lines that will get passed through don't have
# enough characters to cover the original column.  Second, don't
# try to scroll past the beginning or end of the text.
#
# Arguments:
# w -		The text window in which the cursor is to move.
# n -		The number of lines to move: -1 for up one line,
#		+1 for down one line.

proc tkTextUpDownLine {w n} {
    global tkPriv

    set i [$w index insert]
    scan $i "%d.%d" line char
    if {[string compare $tkPriv(prevPos) $i] != 0} {
	set tkPriv(char) $char
    }
    set new [$w index [expr $line + $n].$tkPriv(char)]
    if {[$w compare $new == end] || [$w compare $new == "insert linestart"]} {
	set new $i
    }
    set tkPriv(prevPos) $new
    return $new
}

# tkTextPrevPara --
# Returns the index of the beginning of the paragraph just before a given
# position in the text (the beginning of a paragraph is the first non-blank
# character after a blank line).
#
# Arguments:
# w -		The text window in which the cursor is to move.
# pos -		Position at which to start search.

proc tkTextPrevPara {w pos} {
    set pos [$w index "$pos linestart"]
    while 1 {
	if {(([$w get "$pos - 1 line"] == "\n") && ([$w get $pos] != "\n"))
		|| ($pos == "1.0")} {
	    if [regexp -indices {^[ 	]+(.)} [$w get $pos "$pos lineend"] \
		    dummy index] {
		set pos [$w index "$pos + [lindex $index 0] chars"]
	    }
	    if {[$w compare $pos != insert] || ($pos == "1.0")} {
		return $pos
	    }
	}
	set pos [$w index "$pos - 1 line"]
    }
}

# tkTextNextPara --
# Returns the index of the beginning of the paragraph just after a given
# position in the text (the beginning of a paragraph is the first non-blank
# character after a blank line).
#
# Arguments:
# w -		The text window in which the cursor is to move.
# start -	Position at which to start search.

proc tkTextNextPara {w start} {
    set pos [$w index "$start linestart + 1 line"]
    while {[$w get $pos] != "\n"} {
	if [$w compare $pos == end] {
	    return [$w index "end - 1c"]
	}
	set pos [$w index "$pos + 1 line"]
    }
    while {[$w get $pos] == "\n"} {
	set pos [$w index "$pos + 1 line"]
	if [$w compare $pos == end] {
	    return [$w index "end - 1c"]
	}
    }
    if [regexp -indices {^[ 	]+(.)} [$w get $pos "$pos lineend"] \
	    dummy index] {
	return [$w index "$pos + [lindex $index 0] chars"]
    }
    return $pos
}

# tkTextScrollPages --
# This is a utility procedure used in bindings for moving up and down
# pages and possibly extending the selection along the way.  It scrolls
# the view in the widget by the number of pages, and it returns the
# index of the character that is at the same position in the new view
# as the insertion cursor used to be in the old view.
#
# Arguments:
# w -		The text window in which the cursor is to move.
# count -	Number of pages forward to scroll;  may be negative
#		to scroll backwards.

proc tkTextScrollPages {w count} {
    set bbox [$w bbox insert]
    $w yview scroll $count pages
    if {$bbox == ""} {
	return [$w index @[expr [winfo height $w]/2],0]
    }
    return [$w index @[lindex $bbox 0],[lindex $bbox 1]]
}

# tkTextTranspose --
# This procedure implements the "transpose" function for text widgets.
# It tranposes the characters on either side of the insertion cursor,
# unless the cursor is at the end of the line.  In this case it
# transposes the two characters to the left of the cursor.  In either
# case, the cursor ends up to the right of the transposed characters.
#
# Arguments:
# w -		Text window in which to transpose.

proc tkTextTranspose w {
    set pos insert
    if [$w compare $pos != "$pos lineend"] {
	set pos [$w index "$pos + 1 char"]
    }
    set new [$w get "$pos - 1 char"][$w get  "$pos - 2 char"]
    if [$w compare "$pos - 1 char" == 1.0] {
	return
    }
    $w delete "$pos - 2 char" $pos
    $w insert insert $new
    $w see insert
}
