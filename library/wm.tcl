# wm.tcl --
#
# Partial implementation of the Tk wm and grab commands for CTk.
#
# @(#) $Id: ctk.shar,v 1.50 1996/01/15 14:47:16 andrewm Exp andrewm $
#
# Copyright (c) 1995 Cleveland Clinic Foundation
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.


# wm --
#	Cheap simulation of Tk's wm.

proc wm {option window args} {
    global tkPriv

    switch -glob -- $option {
	deiconify {
	    ctkWmPlace $window
	}
	geom* {
	    if {$args == ""} {
		return [winfo geometry $window]
	    }
	    set geom [string trim $args =]
	    set w {}
	    set h {}
	    set xsign {}
	    set x {}
	    set ysign {}
	    set y {}
	    if {[scan $geom {%d x %d %[+-] %d %[+-] %d} w h xsign x ysign y]
		    < 2} {
		set w {}
	        scan $geom {%[+-] %d %[+-] %d} xsign x ysign y
	    }
	    set tkPriv(wm,$window) [list $w $h $xsign$x $ysign$y]
	    ctkWmPlace $window
	}
	title {
	    if {$args == ""} {
		return $window cget -title
	    }
	    $window configure -title [lindex $args 0]
	}
	transient {
	    switch [llength $args] {
		0 {
		    if [info exists tkPriv(wm-transient,$window)] {
			return $tkPriv(wm-transient,$window)
		    }
		}
		1 {set tkPriv(wm-transient,$window) [lindex $args 0]}
		default {error {wrong # args}}
	    }
	}
	overrideredirect {return 0 }
	iconify -
	withdraw { place forget $window }
    }
}

# ctkWmPlace --
#	Place toplevel window `w' according to window manager settings.

proc ctkWmPlace w {
    global tkPriv

    if [info exists tkPriv(wm,$w)] {
	set width [lindex $tkPriv(wm,$w) 0]
	set height [lindex $tkPriv(wm,$w) 1]
	set x [lindex $tkPriv(wm,$w) 2]
	set y [lindex $tkPriv(wm,$w) 3]
	set placeArgs [list -width $width -height $height]
	switch -glob -- $x {
	    "" { }
	    -* {lappend placeArgs -x [expr [winfo screenwidth $w]+$x] -relx 0}
	    default {lappend placeArgs -x [expr $x] -relx 0}
	}
	switch -glob -- $y {
	    "" { }
	    -* {lappend placeArgs -y [expr [winfo screenheight $w]+$y] -rely 0}
	    default {lappend placeArgs -y [expr $y] -rely 0}
	}
    } else {
	set placeArgs {-relx 0.5 -rely 0.5 -width {} -height {} -anchor center}
    }
    eval place $w $placeArgs
}

# grab --
#	Cheap simulation of Tk's grab.  Currently - a grab has
#	no effect on CTk - this will change if I add a real
#	window manager.

proc grab {option args} {
    global tkPriv

    if {! [info exists tkPriv(grab)]} {
	set tkPriv(grab) {}
    }
    switch -exact -- $option {
	current { return $tkPriv(grab) }
	release {
	    if {$args == $tkPriv(grab)}  {
	    	set tkPriv(grab) {}
	    }
	    return {}
	}
	status  {
	    if {$args == $tkPriv(grab)} {
		return $tkPriv(grabType)
	    } else {
		return none
	    }
	}
	set     {
	    set option [lindex $args 0]
	    set args [lrange $args 1 end]
	    # Falls through ...
	}
    }
    if {$option == "-global"} {
	set tkPriv(grab) $args
	set tkPriv(grabType) global
    } else {
	set tkPriv(grab) $option
	set tkPriv(grabType) local
    }
}

# ctkNextTop
#	Pass focus to the next toplevel window after w's toplevel.

proc ctkNextTop w {
    global tkPriv

    if {$tkPriv(grab) != ""}  {bell; return}
    set cur [winfo toplevel $w]
    set tops [lsort ". [winfo children .]"]
    set i [lsearch -exact $tops $cur]
    set tops "[lrange $tops [expr $i+1] end] [lrange $tops 0 [expr $i-1]]"
    foreach top $tops {
	if {[winfo toplevel $top] == $top
		&& ![info exists tkPriv(wm-transient,$top)]} {
	    wm deiconify $top
	    raise $top
	    focus $top
	    return
	}
    }
}

