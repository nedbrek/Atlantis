#!/usr/bin/env tclsh
proc testGame {dir} {
	cd $dir
	set turns [lsort -integer [regsub -all {turn} [glob "turn*"] {}]]

	set otherDir [string range $dir 0 end-1]

	foreach t $turns {
		puts "turn$t"
		cd turn$t
		exec diff . ../../$otherDir/turn$t
		exec ../../ceran.exe run > /dev/null
		exec diff . ../../$otherDir/turn$t
		cd ..
	}
}

if {$argc == 0} {
	set dirs [list "game1b"]
} else {
	set dirs $argv
}

foreach d $dirs {
	testGame $d
}

