#!/usr/bin/env python3
import json
import re
import sys
import textwrap

def decodeEvent(i):
    if i["type"] == "Tax":
        r = i["region"]
        xy = r["location"]
        if len(xy) == 2:
            xytxt = "{},{}".format(xy[0], xy[1])
        else:
            xytxt = "{},{},{} <{}>".format(xy[0], xy[1], xy[2], r["underworld"])
        loc = "{} ({}) in {}".format(r["terrain"], xytxt, r["province"])
        return "{}: Collects ${} in taxes in {}.".format(i["name"], i["amount"], loc)

    if i["type"] == "Give" or i["type"] == "Receive":
        verb = "Gives"
        prep = "to"
        if i["type"] == "Receive":
            verb = "Receives"
            prep = "from"
            u2 = i["giver"]
            u1 = i["receiver"]
        else:
            u1 = i["giver"]
            u2 = i["receiver"]

        abbr = i["abbr"]
        if i["amount"] != 1:
            name = "{} {} [{}]".format(i["amount"], global_items[abbr]["names"], abbr)
        else:
            name = "{} [{}]".format(global_items[abbr]["name"], abbr)

        return "{}: {} {} {} {}.".format(u1, verb, name, prep, u2)

    if i["type"] == "Produce":
        r = i["region"]
        xy = r["location"]
        if len(xy) == 2:
            xytxt = "{},{}".format(xy[0], xy[1])
        else:
            xytxt = "{},{},{} <{}>".format(xy[0], xy[1], xy[2], r["underworld"])
        loc = "{} ({}) in {}".format(r["terrain"], xytxt, r["province"])

        amt = i["amount"]
        abbr = i["abbr"]
        if amt != 1:
            prod = "{} {} [{}]".format(amt, global_items[abbr]["names"], abbr)
        else:
            prod = "{} [{}]".format(global_items[abbr]["name"], abbr)

        return "{}: Produces {} in {}.".format(i["name"], prod, loc)

    return "Unknown event type {}".format(i["type"])

def dumpTemplateHeader(tw, i):
    loc = i["location"]
    if len(loc) == 2:
        loc_txt = "{},{}".format(loc[0], loc[1])
    else:
        loc_txt = "{},{},{} <{}>".format(loc[0], loc[1], loc[2], i["underworld"])

    print("")
    old_indent = tw.subsequent_indent
    tw.subsequent_indent = ";  "
    if "settlement" in i:
        print(tw.fill(";*** {} ({}) in {}, contains {} [{}] ***".format(i["terrain"], loc_txt, i["province"], i["settlement"]["name"], i["settlement"]["size"])))
    else:
        print(tw.fill(";*** {} ({}) in {} ***".format(i["terrain"], loc_txt, i["province"])))
    tw.subsequent_indent = old_indent

def dumpTemplate(tw, d):
    fmt = d["template"]
    if fmt != "short" and fmt != "long":
        print("Unsupported template format")
        sys.exit

    print("\nOrders Template ({} Format):\n".format(fmt.capitalize()))
    print('#atlantis {} "{}"'.format(d["faction"]["number"], d["password"]))
    if "regions" not in jsdata:
        print("#end")
        return

    pull_unum = re.compile("[^(]* \(([0-9]+)\)")
    for i in d["regions"]:
        dump_header = True

        if "units" in i:
            for u in i["units"]:
                if u["report"] != "own":
                    continue
                if dump_header:
                    dumpTemplateHeader(tw, i)
                    dump_header = False

                unum = pull_unum.search(u["name"]).group(1)
                print("\nunit {}".format(unum))
                if "orders" in u:
                    for o in u["orders"]:
                        print(o)

        if "objects" in i:
            for obj in i["objects"]:
                if "units" not in obj:
                    continue
                for u in obj["units"]:
                    if u["report"] != "own":
                        continue
                    if dump_header:
                        dumpTemplateHeader(tw, i)
                        dump_header = False

                    unum = pull_unum.search(u["name"]).group(1)
                    print("\nunit {}".format(unum))
                    if "orders" in u:
                        for o in u["orders"]:
                            print(o)

    print("\n#end\n")

# return number of items "abbr" in items list
def itemCount(items, abbr):
    for i in items:
        if i["abbr"] == abbr:
            return i["amount"]
    return 0

def findPullers(items, item_def, max_load):
    tot_ct = 0
    tot_inc = 0
    for k,v in item_def.items():
        ct = itemCount(items, k)
        if ct > max_load:
            ct = max_load
        tot_ct = tot_ct + ct
        increase = v
        tot_inc = tot_inc + increase * ct
        max_load = max_load - ct

    return [tot_ct, tot_inc]

def printUnits(tw, d, f):
    for u in d:
        mine = u["report"] == "own"
        if (mine):
            txt = "*"
        else:
            txt = "-"
        txt = txt + " {}".format(u["name"])

        flags = u["flags"]
        if "GUARD" in flags and flags["GUARD"] == 1:
            txt = txt + ", on guard"

        if mine:
            txt = txt + f", {f}"
        elif "faction" in u:
            txt = txt + ", {}".format(u["faction"])

        if "AVOID" in flags and flags["AVOID"] == 1:
            txt = txt + ", avoiding"
        if "BEHIND" in flags and flags["BEHIND"] == 1:
            txt = txt + ", behind"
        if "REVEAL" in flags and flags["REVEAL"] == "FACTION":
            txt = txt + ", revealing faction"
        if "REVEAL" in flags and flags["REVEAL"] == "UNIT":
            txt = txt + ", revealing unit"
        if "HOLD" in flags and flags["HOLD"] == 1:
            txt = txt + ", holding"
        if "AUTOTAX" in flags and flags["AUTOTAX"] == 1:
            txt = txt + ", taxing"
        if "NOAID" in flags and flags["NOAID"] == 1:
            txt = txt + ", receiving no aid"
        if "CONSUME" in flags and flags["CONSUME"] == "FACTION":
            txt = txt + ", consuming faction's food"
        if "CONSUME" in flags and flags["CONSUME"] == "UNIT":
            txt = txt + ", consuming unit's food"
        if "NOCROSS" in flags and flags["NOCROSS"] == 1:
            txt = txt + ", won't cross water"
        if "SHARE" in flags and flags["SHARE"] == 1:
            txt = txt + ", sharing"
        if "SPOILS" in flags and flags["SPOILS"] == "NONE":
            txt = txt + ", weightless battle spoils"
        if "SPOILS" in flags and flags["SPOILS"] == "WALK":
            txt = txt + ", walking battle spoils"
        if "SPOILS" in flags and flags["SPOILS"] == "RIDE":
            txt = txt + ", riding battle spoils"
        if "SPOILS" in flags and flags["SPOILS"] == "FLY":
            txt = txt + ", flying battle spoils"

        items = u["items"]
        tot_wt = 0
        cap = {}
        cap["Walk"] = 0
        cap["Ride"] = 0
        cap["Fly"] = 0
        cap["Swim"] = 0

        for i in items:
            abbr = i["abbr"]
            pabbr = abbr
            ct = i["amount"]
            illusion = False
            if "illusion" in i:
                illusion = True
                abbr = "I" + abbr
            tot_wt = tot_wt + ct * global_items[abbr]["weight"]

            if "capacity" in global_items[abbr]:
                caps = global_items[abbr]["capacity"]
                if "walking" in caps:
                    cap["Walk"] = cap["Walk"] + ct * caps["walking"]
                if "riding" in caps:
                    cap["Ride"] = cap["Ride"] + ct * caps["riding"]
                if "flying" in caps:
                    cap["Fly"] = cap["Fly"] + ct * caps["flying"]
                if "swimming" in caps:
                    cap["Swim"] = cap["Swim"] + ct * caps["swimming"]

            if "hitchItems" in global_items[abbr]:
                pullers = findPullers(items, global_items[abbr]["hitchItems"], ct)
                cap["Walk"] = cap["Walk"] + pullers[1]

            if ct > 1:
                name = global_items[abbr]["names"]
                txt = txt + ", {} {} [{}]".format(ct, name, pabbr)
            else:
                name = global_items[abbr]["name"]
                txt = txt + ", {} [{}]".format(name, pabbr)

            if illusion:
                txt = txt + " (illusion)"

        if mine:
            txt = txt + f". Weight: {tot_wt}"
            if cap:
                cap_txt = ". Capacity: {}/{}/{}/{}".format(cap["Fly"], cap["Ride"], cap["Walk"], cap["Swim"])
                txt = txt + cap_txt

        if "upkeep" in u:
            txt = txt + ". Upkeep: ${}".format(u["upkeep"])

        if "skills" in u:
            txt = txt + ". Skills:"
            first = True
            for s in u["skills"]:
                abbr = s["abbr"]
                name = global_skills[abbr]["name"]
                level = s["level"]
                days = s["days"]
                if first:
                    first = False
                else:
                    txt = txt + ","
                txt = txt + " {} [{}] {} ({})".format(name, abbr, level, days)
            #end for
        elif mine:
            txt = txt + ". Skills: none"

        if "combatSpell" in u:
            txt = txt + ". Combat spell: {}".format(u["combatSpell"])

        if "readyWeapons" in u:
            first = True
            for abbr in u["readyWeapons"]:
                if first:
                    first = False
                    txt = txt + ". Ready weapon"
                    if len(u["readyWeapons"]) > 1:
                        txt = txt + "s"
                    txt = txt + ": "
                else:
                    txt = txt + ", "
                txt = txt + "{} [{}]".format(global_items[abbr]["name"], abbr)

        if "readyArmor" in u:
            first = True
            for abbr in u["readyArmor"]:
                if first:
                    first = False
                    txt = txt + ". Ready armor: "
                else:
                    txt = txt + ", "
                txt = txt + "{} [{}]".format(global_items[abbr]["name"], abbr)

        if "canStudy" in u:
            txt = txt + ". Can Study: "
            first = True
            for s in u["canStudy"]:
                if first:
                    first = False
                else:
                    txt = txt + ", "
                txt = txt + "{} [{}]".format(global_skills[s]["name"], s)

        if "description" in u and u["description"]:
            desc = u["description"]
            if "captain" in u:
                txt = txt + "; Captain of {}".format(u["captain"])
                if desc != " ":
                    txt = txt + ". {}".format(desc)
            elif "inObj" in u:
                txt = txt + "; Sailing in {}".format(u["inObj"])
                if desc != " ":
                    txt = txt + ". {}".format(desc)
            elif desc == " ":
                txt = txt + ";" + desc
            else:
                txt = txt + "; " + desc
        txt = txt + "."

        print(tw.fill(txt))

def printRegion(tw, d, f):
    loc = d["location"]

    txt = "{} ({},{}".format(d["terrain"], loc[0], loc[1]);
    if (len(loc) > 2):
        txt = txt + ",{} <{}>".format(loc[2], d["underworld"])

    txt = txt + ") in {}".format(d["province"])

    if "settlement" in d:
        txt = txt + ", contains {} [{}]".format(d["settlement"]["name"], d["settlement"]["size"])

    if "race" in d:
        txt = txt + ", {} peasants ({})".format(d["population"], d["race"])

    if "tax" in d:
        txt = txt + ", ${}".format(d["tax"])
    txt = txt + "."

    print(tw.fill(txt))
    print("------------------------------------------------------------")
    tw.initial_indent = "  "
    tw.subsequent_indent = " " * 4

    if d["terrain"] == "nexus":
        print("  ")
        print(tw.fill("""Cykranosh Nexus is a magical place; the entryway to the world of
Cykranosh. Enjoy your stay, the city guards should keep you safe
as long as you should choose to stay. However, rumor has it that
once you have left the Nexus, you can never return. You will need
to 'CAST Gate_Lore RANDOM' to leave the Nexus. Or, you can DECLARE
ALIGNMENT <GOOD|EVIL> and CAST Gate_Lore ALIGNMENT to land in an
unoccupied hex matching your alignment.
        """))
        print("  ")

    if "wages" in d and d["wages"]:
        print(tw.fill("Wages: ${} (Max: ${}).".format(d["wages"], d["totalWages"])))
    else:
        print(tw.fill("Wages: $0."))

    if "wanted" in d:
        txt = "Wanted: "
        first = True
        for w in d["wanted"]:
            amt = w["amount"]
            abbr = w["abbr"]
            price = w["price"]

            if first:
                first = False
            else:
                txt = txt + ", "

            if amt > 1:
                name = global_items[abbr]["names"]
                txt = txt + "{} {} [{}] at ${}".format(amt, name, abbr, price)
            else:
                name = global_items[abbr]["name"]
                txt = txt + "{} [{}] at ${}".format(name, abbr, price)

        #end for
        txt = txt + "."
        print(tw.fill(txt))
    else:
        print(tw.fill("Wanted: none."))

    if "forSale" in d:
        txt = "For Sale: "
        first = True
        for w in d["forSale"]:
            amt = w["amount"]
            abbr = w["abbr"]
            price = w["price"]

            if first:
                first = False
            else:
                txt = txt + ", "

            if amt > 1:
                name = global_items[abbr]["names"]
                txt = txt + "{} {} [{}] at ${}".format(amt, name, abbr, price)
            else:
                name = global_items[abbr]["name"]
                txt = txt + "{} [{}] at ${}".format(name, abbr, price)
        #end for

        txt = txt + "."
        print(tw.fill(txt))
    else:
        print(tw.fill("For Sale: none."))

    if "entertainment" in d:
        print(tw.fill("Entertainment available: ${}.".format(d["entertainment"])))

    if "products" in d:
        products = d["products"]
        # Products
        txt = "Products: "
        first = True
        for k,v in products.items():
            if first:
                first = False
            else:
                txt = txt + ", "

            if v > 1:
                name = global_items[k]["names"]
            else:
                name = global_items[k]["name"]
            txt = txt + "{} {} [{}]".format(v, name, k)
        #end for

        txt = txt + "."
        print(tw.fill(txt))
        print("")

    print("Exits:")
    if "exits" in d:
        exits = d["exits"]
        for k,v in exits.items():
            loc_arr = v["location"]
            loc = "({},{}".format(loc_arr[0], loc_arr[1])
            if (len(loc_arr) > 2):
                loc = loc + ",{} <{}>".format(loc_arr[2], v["underworld"])
            loc = loc + ")"
            txt = "{} : {} {} in {}".format(k, v["terrain"], loc, v["province"])
            if "settlement" in v:
                txt = txt + ", contains {} [{}]".format(v["settlement"]["name"], v["settlement"]["size"])
            txt = txt + "."
            print(tw.fill(txt))
    else:
        print("  none")
    print("")

    if "gate" in d:
        print("There is a Gate here (Gate {} of {}).".format(d["gate"]["number"], d["gate"]["max"]))
        print("")

    # Units
    tw.initial_indent = ""
    tw.subsequent_indent = " " * 2
    if "units" in d:
        printUnits(tw, d["units"], f)

    # Objects
    if "objects" in d:
        for o in d["objects"]:
            print("")
            txt = o["name"]
            print(tw.fill(txt))

            if "units" in o:
                # indent units
                tw.initial_indent = " " * 2
                tw.subsequent_indent = " " * 4
                printUnits(tw, o["units"], f)

                # put tw back to default
                tw.initial_indent = ""
                tw.subsequent_indent = " " * 2

    print("")

with open("creport.json") as gfd:
    global_data = json.load(gfd)
    global_items = global_data["items"]
    global_skills = global_data["skills"]

with open(sys.argv[1]) as fd:
    jsdata = json.load(fd)
    print("Atlantis Report For:")

    txt = ""
    first = True
    faction_name = "{} ({})".format(jsdata["faction"]["name"], jsdata["faction"]["number"])
    for k,v in jsdata["faction"]["type"].items():
        if first:
            first = False
        else:
            txt = txt + ", "
        txt = txt + "{} {}".format(k, v)
    print("{} ({})".format(faction_name, txt))

    print("{}, Year {}\n".format(jsdata["date"]["month"], jsdata["date"]["year"]))
    print("Atlantis Engine Version: {}".format(jsdata["engine"]["version"]))
    print("{}, Version: {}\n".format(jsdata["engine"]["ruleset"]["name"], jsdata["engine"]["ruleset"]["version"]))

    tw = textwrap.TextWrapper()
    tw.width = 70
    tw.break_on_hyphens = False
    tw.subsequent_indent = "  "

    if "turnCountdown" in jsdata and jsdata["turnCountdown"] != -1:
        print(tw.fill("WARNING: You have {} turns until your faction is automatically removed due to inactivity!".format(jsdata["turnCountdown"])))
        print("")

    if "quit" in jsdata and jsdata["quit"] > 0:
        print("I'm sorry, your faction has been eliminated.")
        print(tw.fill("""If you wish to restart, please let the Gamemaster know, and you will
be restarted for the next available turn.
        """))
        print("")

    print("Faction Status:")
    for i in jsdata["factionStatus"]:
        print("{}: {} ({})".format(i["key"], i["amount"], i["max"]))
    print("")

    print("Your faction is {}.\n".format(jsdata["alignment"]))

    if "errors" in jsdata:
        print("Errors during turn:")
        for i in jsdata["errors"]:
            print(tw.fill(i))
        print("")

    if "battles" in jsdata:
        print("Battles during turn:")
        for i in jsdata["battles"]:
            for l in i:
                print(tw.fill(l))

    if "events" in jsdata:
        print("Events during turn:")
        for i in jsdata["events"]:
            if isinstance(i, str):
                print(tw.fill(i))
            else:
                print(tw.fill(decodeEvent(i)))
        print("")

    if "skillReports" in jsdata:
        print("Skill reports:\n")
        for i in jsdata["skillReports"]:
            print(tw.fill(i))
            print("")

    if "itemReports" in jsdata:
        print("Item reports:\n")
        for i in jsdata["itemReports"]:
            print(tw.fill(i))
            print("")

    if "objectReports" in jsdata:
        print("Object reports:\n")
        for i in jsdata["objectReports"]:
            print(tw.fill(i))
            print("")

    # Attitudes
    attitudes = jsdata["attitudes"]

    print("Declared Attitudes (default {}):".format(attitudes["default"]))

    for k in ["Hostile", "Unfriendly", "Neutral", "Friendly", "Ally"]:
        txt = "{} : ".format(k)
        first_fac = True
        v = []
        if k in attitudes:
            v = attitudes[k]

        for f in v:
            if first_fac:
                first_fac = False
            else:
                txt = txt + ", "
            txt = txt + f

        if not v:
            txt = txt + "none"

        txt = txt + "."
        print(tw.fill(txt))

    print("")

    print("Unclaimed silver: {}.".format(jsdata["unclaimedSilver"]))
    print("")

    if "regions" in jsdata:
        for i in jsdata["regions"]:
            printRegion(tw, i, faction_name)
            print("")

    # Orders template
    if "template" in jsdata:
        dumpTemplate(tw, jsdata)

