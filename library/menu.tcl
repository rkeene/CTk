# menu.tcl --
#
# This file defines the default bindings for Tk menus and menubuttons.
# It also implements keyboard traversal of menus and implements a few
# other utility procedures related to menus.
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
# focus -		Saves the focus during a menu selection operation.
#			Focus gets restored here when the menu is unposted.
# oldGrab -		Window that had the grab before a menu was posted.
#			Used to restore the grab state after the menu
#			is unposted.  Empty string means there was no
#			grab previously set.
# popup -		If a menu has been popped up via tk_popup, this
#			gives the name of the menu.  Otherwise this
#			value is empty.
# postedMb -		Name of the menubutton whose menu is currently
#			posted, or an empty string if nothing is posted
#			A grab is set on this widget.
#-------------------------------------------------------------------------

#-------------------------------------------------------------------------
# Overall note:
# This file is tricky because there are four different ways that menus
# can be used:
#
# 1. As a pulldown from a menubutton.  This is the most common usage.
#    In this style, the variable tkPriv(postedMb) identifies the posted
#    menubutton.
# 2. As a torn-off menu copied from some other menu.  In this style
#    tkPriv(postedMb) is empty, and the top-level menu is no
#    override-redirect.
# 3. As an option menu, triggered from an option menubutton.  In thi
#    style tkPriv(postedMb) identifies the posted menubutton.
# 4. As a popup menu.  In this style tkPriv(postedMb) is empty and
#    the top-level menu is override-redirect.
#
# The various binding procedures use the  state described above to
# distinguish the various cases and take different actions in each
# case.
#-------------------------------------------------------------------------

#-------------------------------------------------------------------------
# The code below creates the default class bindings for menus
# and menubuttons.
#-------------------------------------------------------------------------

bind Menubutton <Select> {
    tkMbPost %W
    tkMenuFirstEntry [%W cget -menu]
}
bind Menubutton <Return> {
    tkMbPost %W
    tkMenuFirstEntry [%W cget -menu]
}

# Must set focus when mouse enters a menu, in order to allow
# mixed-mode processing using both the mouse and the keyboard.

bind Menu <Select> {
    tkMenuInvoke %W
}
bind Menu <Return> {
    tkMenuInvoke %W
}
bind Menu <Menu> {
    tkMenuEscape %W
}
bind Menu <Cancel> {
    tkMenuEscape %W
}
bind Menu <Left> {
    tkMenuLeftRight %W left
}
bind Menu <Right> {
    tkMenuLeftRight %W right
}
bind Menu <Up> {
    tkMenuNextEntry %W -1
}
bind Menu <Down> {
    tkMenuNextEntry %W +1
}
bind Menu <KeyPress> {
    tkTraverseWithinMenu %W %A
}

# The following bindings apply to all windows, and are used to
# implement keyboard menu traversal.

bind all <KeyPress> {
    if [string match {[A-Za-z]} %A] {tkTraverseToMenu %W %A}
}
bind all <Menu> {
    tkFirstMenu %W
}

# tkMbPost --
# Given a menubutton, this procedure does all the work of posting
# its associated menu and unposting any other menu that is currently
# posted.
#
# Arguments:
# w -			The name of the menubutton widget whose menu
#			is to be posted.
# x, y -		Root coordinates of cursor, used for positioning
#			option menus.  If not specified, then the center
#			of the menubutton is used for an option menu.

proc tkMbPost {w {x {}} {y {}}} {
    global tkPriv
    if {([$w cget -state] == "disabled") || ($w == $tkPriv(postedMb))} {
	return
    }
    set menu [$w cget -menu]
    if {$menu == ""} {
	return
    }
    if ![string match $w.* $menu] {
	error "can't post $menu:  it isn't a descendant of $w (this is a new requirement in Tk versions 3.0 and later)"
    }
    set cur $tkPriv(postedMb)
    if {$cur != ""} {
	tkMenuUnpost {}
    }
    set tkPriv(postedMb) $w
    set tkPriv(focus) [focus]
    $menu activate none

    # If this looks like an option menubutton then post the menu so
    # that the current entry is on top of the mouse.  Otherwise post
    # the menu just below the menubutton, as for a pull-down.

    if {[$w cget -indicatoron]} {
	if {$y == ""} {
	    set x [expr [winfo rootx $w] + [winfo width $w]/2]
	    set y [expr [winfo rooty $w] + [winfo height $w]/2]
	}
	tkPostOverPoint $menu $x $y [tkMenuFindName $menu [$w cget -text]]
    } else {
	$menu post [winfo rootx $w] [expr [winfo rooty $w]+[winfo height $w]]
    }
    focus $menu
    set tkPriv(oldGrab) [grab current $w]
    grab -global $w
}

# tkMenuUnpost --
# This procedure unposts a given menu, plus all of its ancestors up
# to (and including) a menubutton, if any.  It also restores various
# values to what they were before the menu was posted, and releases
# a grab if there's a menubutton involved.  Special notes:
# 1. It's important to unpost all menus before releasing the grab, so
#    that any Enter-Leave events (e.g. from menu back to main
#    application) have mode NotifyGrab.
# 2. Be sure to enclose various groups of commands in "catch" so that
#    the procedure will complete even if the menubutton or the menu
#    or the grab window has been deleted.
#
# Arguments:
# menu -		Name of a menu to unpost.  Ignored if there
#			is a posted menubutton.

proc tkMenuUnpost menu {
    global tkPriv
    set mb $tkPriv(postedMb)

    # Restore focus right away (otherwise X will take focus away when
    # the menu is unmapped and under some window managers (e.g. olvwm)
    # we'll lose the focus completely).

    catch {focus $tkPriv(focus)}
    set tkPriv(focus) ""

    # Unpost menu(s) and restore some stuff that's dependent on
    # what was posted.

    catch {
	if {$mb != ""} {
	    set menu [$mb cget -menu]
	    $menu unpost
	    set tkPriv(postedMb) {}
	} elseif {$tkPriv(popup) != ""} {
	    $tkPriv(popup) unpost
	    set tkPriv(popup) {}
	} elseif {[wm overrideredirect $menu]} {
	    # We're in a cascaded sub-menu from a torn-off menu or popup.
	    # Unpost all the menus up to the toplevel one (but not
	    # including the top-level torn-off one) and deactivate the
	    # top-level torn off menu if there is one.

	    while 1 {
		set parent [winfo parent $menu]
		if {([winfo class $parent] != "Menu")
			|| ![winfo ismapped $parent]} {
		    break
		}
		$parent activate none
		$parent postcascade none
		if {![wm overrideredirect $parent]} {
		    break
		}
		set menu $parent
	    }
	    $menu unpost
	}
    }

    # Release grab, if any, and restore the previous grab, if there
    # was one.

    if {$menu != ""} {
	set grab [grab current $menu]
	if {$grab != ""} {
	    grab release $grab
	}
    }
    if {$tkPriv(oldGrab) != ""} {
	grab set $tkPriv(oldGrab)
	set tkPriv(oldGrab) ""
    }
}

# tkMenuInvoke --
# This procedure is invoked when button 1 is released over a menu.
# It invokes the appropriate menu action and unposts the menu if
# it came from a menubutton.
#
# Arguments:
# w -			Name of the menu widget.

proc tkMenuInvoke w {
    if {[$w type active] == "cascade"} {
	$w postcascade active
	set menu [$w entrycget active -menu]
	tkMenuFirstEntry $menu
    } elseif {[$w type active] == "tearoff"} {
	tkMenuUnpost $w
	tkTearOffMenu $w
    } else {
	tkMenuUnpost $w
	uplevel #0 [list $w invoke active]
    }
}

# tkMenuEscape --
# This procedure is invoked for the Cancel (or Escape) key.  It unposts
# the given menu and, if it is the top-level menu for a menu button,
# unposts the menu button as well.
#
# Arguments:
# menu -		Name of the menu window.

proc tkMenuEscape menu {
    if {[winfo class [winfo parent $menu]] != "Menu"} {
	tkMenuUnpost $menu
    } else {
	tkMenuLeftRight $menu -1
    }
}

# tkMenuLeftRight --
# This procedure is invoked to handle "left" and "right" traversal
# motions in menus.  It traverses to the next menu in a menu bar,
# or into or out of a cascaded menu.
#
# Arguments:
# menu -		The menu that received the keyboard
#			event.
# direction -		Direction in which to move: "left" or "right"

proc tkMenuLeftRight {menu direction} {
    global tkPriv

    # First handle traversals into and out of cascaded menus.

    if {$direction == "right"} {
	set count 1
	if {[$menu type active] == "cascade"} {
	    $menu postcascade active
	    set m2 [$menu entrycget active -menu]
	    if {$m2 != ""} {
		tkMenuFirstEntry $m2
	    }
	    return
	}
    } else {
	set count -1
	set m2 [winfo parent $menu]
	if {[winfo class $m2] == "Menu"} {
	    $menu activate none
	    focus $m2

	    # This code unposts any posted submenu in the parent.

	    set tmp [$m2 index active]
	    $m2 activate none
	    $m2 activate $tmp
	    return
	}
    }

    # Can't traverse into or out of a cascaded menu.  Go to the next
    # or previous menubutton, if that makes sense.

    set w $tkPriv(postedMb)
    if {$w == ""} {
	return
    }
    set buttons [winfo children [winfo parent $w]]
    set length [llength $buttons]
    set i [expr [lsearch -exact $buttons $w] + $count]
    while 1 {
	while {$i < 0} {
	    incr i $length
	}
	while {$i >= $length} {
	    incr i -$length
	}
	set mb [lindex $buttons $i]
	if {([winfo class $mb] == "Menubutton")
		&& ([$mb cget -state] != "disabled")
		&& ([$mb cget -menu] != "")
		&& ([[$mb cget -menu] index last] != "none")} {
	    break
	}
	if {$mb == $w} {
	    return
	}
	incr i $count
    }
    tkMbPost $mb
    tkMenuFirstEntry [$mb cget -menu]
}

# tkMenuNextEntry --
# Activate the next higher or lower entry in the posted menu,
# wrapping around at the ends.  Disabled entries are skipped.
#
# Arguments:
# menu -			Menu window that received the keystroke.
# count -			1 means go to the next lower entry,
#				-1 means go to the next higher entry.

proc tkMenuNextEntry {menu count} {
    global tkPriv
    if {[$menu index last] == "none"} {
	return
    }
    set length [expr [$menu index last]+1]
    set quitAfter $length
    set active [$menu index active]
    if {$active == "none"} {
	set i 0
    } else {
	set i [expr $active + $count]
    }
    while 1 {
	if {$quitAfter <= 0} {
	    # We've tried every entry in the menu.  Either there are
	    # none, or they're all disabled.  Just give up.

	    return
	}
	while {$i < 0} {
	    incr i $length
	}
	while {$i >= $length} {
	    incr i -$length
	}
	if {[catch {$menu entrycget $i -state} state] == 0} {
	    if {$state != "disabled"} {
		break
	    }
	}
	if {$i == $active} {
	    return
	}
	incr i $count
	incr quitAfter -1
    }
    $menu activate $i
    $menu postcascade $i
}

# tkMenuFind --
# This procedure searches the entire window hierarchy under w for
# a menubutton that isn't disabled and whose underlined character
# is "char".  It returns the name of that window, if found, or an
# empty string if no matching window was found.  If "char" is an
# empty string then the procedure returns the name of the first
# menubutton found that isn't disabled.
#
# Will not match menu buttons that have their indicator on (assumed
# to be option buttons).
#
# Arguments:
# w -				Name of window where key was typed.
# char -			Underlined character to search for;
#				may be either upper or lower case, and
#				will match either upper or lower case.

proc tkMenuFind {w char} {
    global tkPriv
    set char [string tolower $char]

    foreach child [winfo child $w] {
	switch [winfo class $child] {
	    Menubutton {
		if [$child cget -indicatoron]  continue
		set char2 [string index [$child cget -text] \
			[$child cget -underline]]
		if {([string compare $char [string tolower $char2]] == 0)
			|| ($char == "")} {
		    if {[$child cget -state] != "disabled"} {
			return $child
		    }
		}
	    }
	    Frame {
		set match [tkMenuFind $child $char]
		if {$match != ""} {
		    return $match
		}
	    }
	}
    }
    return {}
}

# tkTraverseToMenu --
# This procedure implements keyboard traversal of menus.  Given an
# ASCII character "char", it looks for a menubutton with that character
# underlined.  If one is found, it posts the menubutton's menu
#
# Arguments:
# w -				Window in which the key was typed (selects
#				a toplevel window).
# char -			Character that selects a menu.  The case
#				is ignored.  If an empty string, nothing
#				happens.

proc tkTraverseToMenu {w char} {
    if {$char == ""} {
	return
    }
    set w [tkMenuFind [winfo toplevel $w] $char]
    if {$w != ""} {
	tkMbPost $w
	tkMenuFirstEntry [$w cget -menu]
    }
}

# tkFirstMenu --
# This procedure traverses to the first menubutton in the toplevel
# for a given window, and posts that menubutton's menu.
#
# Arguments:
# w -				Name of a window.  Selects which toplevel
#				to search for menubuttons.

proc tkFirstMenu w {
    set w [tkMenuFind [winfo toplevel $w] ""]
    if {$w != ""} {
	tkMbPost $w
	tkMenuFirstEntry [$w cget -menu]
    }
}

# tkTraverseWithinMenu
# This procedure implements keyboard traversal within a menu.  It
# searches for an entry in the menu that has "char" underlined.  If
# such an entry is found, it is invoked and the menu is unposted.
#
# Arguments:
# w -				The name of the menu widget.
# char -			The character to look for;  case is
#				ignored.  If the string is empty then
#				nothing happens.

proc tkTraverseWithinMenu {w char} {
    if {$char == ""} {
	return
    }
    set char [string tolower $char]
    set last [$w index last]
    if {$last == "none"} {
	return
    }
    for {set i 0} {$i <= $last} {incr i} {
	if [catch {set char2 [string index \
		[$w entrycget $i -label] \
		[$w entrycget $i -underline]]}] {
	    continue
	}
	if {[string compare $char [string tolower $char2]] == 0} {
	    if {[$w type $i] == "cascade"} {
		$w postcascade $i
		$w activate $i
		set m2 [$w entrycget $i -menu]
		if {$m2 != ""} {
		    tkMenuFirstEntry $m2
		}
	    } else {
		tkMenuUnpost $w
		uplevel #0 [list $w invoke $i]
	    }
	    return
	}
    }
}

# tkMenuFirstEntry --
# Given a menu, this procedure finds the first entry that isn't
# disabled or a tear-off or separator, and activates that entry.
# However, if there is already an active entry in the menu (e.g.,
# because of a previous call to tkPostOverPoint) then the active
# entry isn't changed.  This procedure also sets the input focus
# to the menu.
#
# Arguments:
# menu -		Name of the menu window (possibly empty).

proc tkMenuFirstEntry menu {
    if {$menu == ""} {
	return
    }
    focus $menu
    if {[$menu index active] != "none"} {
	return
    }
    set last [$menu index last]
    if {$last == "none"} {
	return
    }
    for {set i 0} {$i <= $last} {incr i} {
	if {([catch {set state [$menu entrycget $i -state]}] == 0)
		&& ($state != "disabled") && ([$menu type $i] != "tearoff")} {
	    $menu activate $i
	    return
	}
    }
}

# tkMenuFindName --
# Given a menu and a text string, return the index of the menu entry
# that displays the string as its label.  If there is no such entry,
# return an empty string.  This procedure is tricky because some names
# like "active" have a special meaning in menu commands, so we can't
# always use the "index" widget command.
#
# Arguments:
# menu -		Name of the menu widget.
# s -			String to look for.

proc tkMenuFindName {menu s} {
    set i ""
    if {![regexp {^active$|^last$|^none$|^[0-9]|^@} $s]} {
	catch {set i [$menu index $s]}
	return $i
    }
    set last [$menu index last]
    if {$last == "none"} {
	return
    }
    for {set i 0} {$i <= $last} {incr i} {
	if ![catch {$menu entrycget $i -label} label] {
	    if {$label == $s} {
		return $i
	    }
	}
    }
    return ""
}

# tkPostOverPoint --
# This procedure posts a given menu such that a given entry in the
# menu is centered over a given point in the root window.  It also
# activates the given entry.
#
# Arguments:
# menu -		Menu to post.
# x, y -		Root coordinates of point.
# entry -		Index of entry within menu to center over (x,y).
#			If omitted or specified as {}, then the menu's
#			upper-left corner goes at (x,y).

proc tkPostOverPoint {menu x y {entry {}}}  {
    if {$entry != {}} {
	# There was a bunch of adjusting here - just seemed to goof up in CTk
	incr y [expr -[$menu yposition $entry]]
	incr x [expr -[winfo reqwidth $menu]/2]
    }
    $menu post $x $y
    if {($entry != {}) && ([$menu entrycget $entry -state] != "disabled")} {
	$menu activate $entry
    }
}

# tk_popup --
# This procedure pops up a menu and sets things up for traversing
# the menu and its submenus.
#
# Arguments:
# menu -		Name of the menu to be popped up.
# x, y -		Root coordinates at which to pop up the
#			menu.
# entry -		Index of a menu entry to center over (x,y).
#			If omitted or specified as {}, then menu's
#			upper-left corner goes at (x,y).

proc tk_popup {menu x y {entry {}}} {
    global tkPriv
    if {($tkPriv(popup) != "") || ($tkPriv(postedMb) != "")} {
	tkMenuUnpost {}
    }
    tkPostOverPoint $menu $x $y $entry
    set tkPriv(oldGrab) [grab current $menu]
    grab -global $menu
    set tkPriv(popup) $menu
    set tkPriv(focus) [focus]
    focus $menu
}
