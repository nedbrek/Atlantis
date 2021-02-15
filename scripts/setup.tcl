
set ifile [open "players.in"]
set itext [read $ifile]
close $ifile

set ofile [open "players.in.new" "w"]
puts $ofile $itext

puts $ofile "Faction: new"
puts $ofile "Name: Neddites"
puts $ofile "StartLoc: 1 1 1"

puts $ofile "NewUnit: 1"
puts $ofile "Item: gm1 25 GNOM"
puts $ofile "Item: gm1 25 WOOD"
puts $ofile "Skill: gm1 SHIP 30"
puts $ofile "Skill: gm1 SAIL 30"
puts $ofile "Order: gm1 BUILD LONGBOAT"

puts $ofile "NewUnit: 2"
puts $ofile "Item: gm2 25 GNOM"
puts $ofile "Item: gm2 25 WOOD"
puts $ofile "Skill: gm2 SHIP 30"
puts $ofile "Skill: gm2 SAIL 30"
puts $ofile "Order: gm2 BUILD LONGBOAT"

puts $ofile "NewUnit: 3"
puts $ofile "Item: gm3 5 MDWA"

puts $ofile "NewUnit: 4"
puts $ofile "Item: gm4 5 MDWA"

puts $ofile "NewUnit: 5"
puts $ofile "Item: gm3 5 HLNG"

puts $ofile "NewUnit: 6"
puts $ofile "Item: gm4 5 MDWA"

close $ofile

