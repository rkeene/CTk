#! /usr/bin/env tclsh

load ./libctk.so Tk

namespace eval ::pkcs11_getfile {}
unset -nocomplain ::pkcs11_getfile::pkcs11file

# Curses Tk lacks a "tk_getOpenFile" dialog, we create a simple input dialog
toplevel .pkcs11_getfile
label .pkcs11_getfile.lblInput -text "Please enter the pathname to a working PKCS#11 Module"
entry .pkcs11_getfile.entInput
button .pkcs11_getfile.btnOK -text "OK" -command {
	set ::pkcs11_getfile::pkcs11file [.pkcs11_getfile.entInput get]

	destroy .pkcs11_getfile
}
button .pkcs11_getfile.btnCancel -text "Cancel" -command {
	destroy .pkcs11_getfile

	set ::pkcs11_getfile::pkcs11file ""
}

pack .pkcs11_getfile.lblInput
pack .pkcs11_getfile.entInput
pack .pkcs11_getfile.btnOK .pkcs11_getfile.btnCancel

focus .pkcs11_getfile.entInput

tkwait variable ::pkcs11_getfile::pkcs11file
set pkcs11file $::pkcs11_getfile::pkcs11file

