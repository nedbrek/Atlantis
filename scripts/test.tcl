#!/usr/bin/env tclsh
proc testGame {dir exe} {
	cd $dir
	set turns [lsort -integer [regsub -all {turn} [glob "turn*"] {}]]

	set otherDir [string range $dir 0 end-1]

	foreach t $turns {
		puts "turn$t"
		cd turn$t
		exec diff . ../../$otherDir/turn$t
		exec ../../$exe run > /dev/null
		exec diff . ../../$otherDir/turn$t
		cd ..
	}
	cd ..
}

set exe "ceran.exe"

if {$argc == 0} {
	set dirs [list "game1b"]
} else {
	if {[string index [lindex $argv 0] 0] eq "-"} {
		set opt [string range [lindex $argv 0] 1 end]
		if {$opt eq "exe"} {
			set exe [lindex $argv 1]
		}
		set argv [lrange $argv 2 end]
	}
	set dirs $argv
}

foreach d $dirs {
	testGame $d $exe
}

