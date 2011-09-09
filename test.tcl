#! /usr/bin/env tclsh

if {![info exists tk_port]} {
	set tk_library [file join [file dirname [info script]] library]
	load ./libctk.so Tk
}

label .l -text "Password"
entry .x
button .y -text "OK" -command exit

pack .l -side left
pack .x -side right -fill x -expand 1
pack .y
