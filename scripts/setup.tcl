package require json

if {$argc < 1} {
	puts "Usage $argv0 <setup.json>"
	exit
}

proc dGet {d k} {
	if {![dict exists $d $k]} { return "" }

	return [dict get $d $k]
}

set ifile [open "players.in"]
set itext [read $ifile]
close $ifile

set ofile [open "players.in.new" "w"]
puts $ofile $itext

set cfg_file [open "setup.json"]
set cfg_text [read $cfg_file]
close $cfg_file
set cfg [::json::json2dict $cfg_text]

set factions [dGet $cfg "Factions"]
foreach f $factions {
	puts $ofile "Faction: new"
	if {[dict exists $f "Name"]} {
		puts $ofile "Name: [dict get $f Name]"
	}
	if {[dict exists $f "StartLoc"]} {
		puts $ofile "StartLoc: [dict get $f StartLoc]"
	}

	set units [dGet $f "Units"]
	set unit_num 0
	foreach u $units {
		incr unit_num

		puts $ofile "NewUnit: $unit_num"

		set items [dGet $u Items]
		foreach i $items {
			set id [dGet $i Id]
			set ct [dGet $i Count]
			puts $ofile "Item: gm$unit_num $ct $id"
		}

		set skills [dGet $u Skills]
		foreach s $skills {
			set abbr [dGet $s Name]
			set days [dGet $s Days]
			puts $ofile "Skill: gm$unit_num $abbr $days"
		}

		set orders [dGet $u Orders]
		foreach o $orders {
			puts $ofile "Order: gm$unit_num $o"
		}
	}
}


close $ofile

