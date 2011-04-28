set s [read [open creport.3]]

proc dGet {dict key} {
	return [string trim [dict get $dict $key]]
}

proc printRegion {r} {
	puts -nonewline "[dict get $r Terrain] ("
	set loc [dict get $r Location]
	puts -nonewline "[lindex $loc 0],[lindex $loc 1]) in [dGet $r Region]"

	set city [dict get $r Town]
	if {[llength $city]} {
		puts -nonewline ", contains [string trim [lindex $city 0]]"
		puts -nonewline " \[[string trim [lindex $city 1]]\]"
	}

	puts ", [dict get $r Population] peasants"
	puts "([dGet $r Race]), \$[dict get $r MaxTax]."
	puts "------------------------------------------------------------"
	puts -nonewline "The weather was [dict get $r WeatherOld] last month; "
	puts "it will be [dict get $r WeatherNew] next month."

	set exits [dict get $r Exits]
	if {[llength $exits]} {

		puts "Exits:"

		foreach {dir des} $exits {
			puts -nonewline "$dir : [dGet $des Terrain]"
			set loc [dict get $des Location]
			puts -nonewline "([lindex $loc 0],[lindex $loc 1]) in "
			puts -nonewline "[dGet $des Region]"

			set city [dict get $des Town]
			if {[llength $city]} {
				puts -nonewline ", contains [string trim [lindex $city 0]]"
				puts -nonewline "\[[string trim [lindex $city 1]]\]"
			}
			puts ""
		}
	}
}

puts "Atlantis Report For:"
puts "[dict get $s Name] [dict get $s FactionType]"
puts "[dict get $s Month], Year [dict get $s Year]"
puts ""
puts "Atlantis Engine Version: [dict get $s VerString]"
puts "[dict get $s Rulesetname], Version: [dict get $s Rulesetversion]"
puts ""

if {![dict get $s Newssheet]} {
	puts "Note: The Times is not being sent to you."
}

if {[dict get $s Password] eq "none"} {
	puts "REMINDER: You have not set a password for your faction!"
}

set turnCountdown [dict get $s TurnCountdown]
if {$turnCountdown != -1} {
	puts "WARNING: You have $turnCountdown turns until your faction is\
 automatically removed due to inactivity!"
}

switch {[dict get $s Quit]} {
	restart	{puts "You restarted your faction this turn. This faction \
has been removed, and a new faction has been started \
for you. (Your new faction report will come in a \
separate message.)"
	}

	over	{puts "I'm sorry, the game has ended. Better luck in \
the next game you play!"
	}

	won	{puts "Congratulations, you have won the game!"}

	eliminated	{
		puts "I'm sorry, your faction has been eliminated."
		puts "If you wish to restart, please let the \
Gamemaster know, and you will be restarted for \
the next available turn."
	}
}

puts "Faction Status:"
puts "Tax Regions: [dict get $s TaxRegion] ([dict get $s MaxTax])"
puts "Trade Regions: [dict get $s TradeRegion] ([dict get $s MaxTrade])"
puts "Mages: [dict get $s NumMage] ([dict get $s MaxMage])"
if {[dict exists $s NumAppr]} {
	puts "Apprentices: [dict get $s NumAppr] ([dict get $s MaxAppr])"
}
puts ""

set errors [dict get $s Errors]
if {[llength $errors]} {
	puts "Errors during turn:"

	foreach err $errors {
		puts $err
	}
}

set battles [dict get $s Battles]
if {[llength $battles]} {
	puts "Battles during turn:"
	foreach battle $battles {
		printBattle $battle
	}
}

set events [dict get $s Events]
if {[llength $events]} {
	puts "Events during turn:"
	foreach event $events {
		puts $event
	}
}

set newSkills [dict get $s NewSkills]
if {[llength $newSkills]} {
	puts "Skill reports:"
	foreach skill $newSkills {
		printSkill $skill
	}
}

set newItems [dict get $s NewItems]
if {[llength $newItems]} {
	puts "Item reports:"
	foreach item $newItems {
		puts "\n$item"
	}
}

set newObjects [dict get $s NewObjects]
if {[llength $newObjects]} {
	puts "Object reports:"
	foreach object $newObjects {
		puts "\n$object"
	}
}

puts "Declared Attitudes (default [dict get $s DefaultAttitude]):"
puts ""

puts "Unclaimed silver: [dict get $s Silver].\n"

set regions [dict get $s Regions]
foreach region $regions {
	printRegion $region
}

puts ""
puts "#end"
puts ""

