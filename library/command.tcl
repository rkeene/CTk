# command.tcl --
#
# This file defines the CTk command dialog procedure.
#
# @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
#
# Copyright (c) 1995 Cleveland Clinic Foundation
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

proc ctk_menu {} {
    if ![winfo exists .ctkMenu] {
	menu .ctkMenu
	.ctkMenu add command -label "Command.." -underline 0 -command ctkDialog
	.ctkMenu add command -label "Next" -underline 0\
		-command {ctkNextTop [focus]}
	.ctkMenu add command -label "Redraw" -underline 0\
		-command {ctk redraw .ctkMenu}
	.ctkMenu add command -label "Exit" -underline 1 -command exit
    }
    tk_popup .ctkMenu 0 0 0
}

proc ctkDialog {} {ctkCommand .ctkDlg}

# ctkCommand --
#
proc ctkCommand w {
    if [winfo exists $w] {
    	wm deiconify $w
	raise $w
    } else {
	toplevel $w -class Dialog -width 30 -height 10
	wm title $w Command
	entry $w.entry
	text $w.output -state disabled -takefocus 1
	button $w.close -command "destroy $w" -text Close

	pack $w.entry -side top -fill x
	pack $w.output -side top -fill both -expand 1
	pack $w.close -side bottom

	bind $w.entry <Return> "ctkCommandRun $w; break"
	bind $w <Cancel> "destroy $w"
    }
    focus $w.entry
    tkwait window $w
}

proc ctkCommandRun w {
    global errorInfo
    set code [catch {uplevel #0 [$w.entry get]} result]
    $w.output configure -state normal
    $w.output delete 1.0 end
    $w.output insert 1.0 $result
    if $code  { $w.output insert end "\n----\n$errorInfo" }
    $w.output mark set insert 1.0
    $w.output configure -state disabled
    $w.entry delete 0 end
}
