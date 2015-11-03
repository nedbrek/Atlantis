#!/usr/bin/env tclsh
set turns [lsort -integer [regsub -all {turn} [glob turn*] {}]]
set lastTurn [lindex $turns end]

set nextTurn [expr {$lastTurn + 1}]
set nextDir "turn$nextTurn"

file mkdir $nextDir

file copy "turn$lastTurn/game.out" "$nextDir/game.in"
file copy "turn$lastTurn/players.out" "$nextDir/players.in"

