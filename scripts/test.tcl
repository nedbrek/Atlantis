#!/usr/bin/env tclsh
cd game1b
set turns [lsort -integer [regsub -all {turn} [glob "turn*"] {}]]

foreach t $turns {
	puts "turn$t"
	cd turn$t
	exec diff . ../../game1/turn$t
	exec ../../ceran.exe run > /dev/null
	exec diff . ../../game1/turn$t
	cd ..
}

