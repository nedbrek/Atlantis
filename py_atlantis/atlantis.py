#!/usr/bin/env python

import Atlantis
import sys

def setRules(game, game_name):
    if game_name != "ceran":
        print "Only ceran supported right now"
        sys.exit()

    game.enableItem("PICK")
    game.enableItem("SPEAR");
    game.enableItem("AXE");
    game.enableItem("HAMMER");
    game.enableItem("MXBO");
    game.enableItem("MWAG");
    game.enableItem("GLIDER");
    game.enableItem("NET");
    game.enableItem("LASSO");
    game.enableItem("BAG");
    game.enableItem("SPIN");
    game.enableItem("LEATHER ARMOR");
    game.enableItem("CLOTH ARMOR");
    game.enableItem("BOOT");

    game.enableItem("PIRATES");
    game.enableItem("KRAKEN");
    game.enableItem("MERFOLK");
    game.enableItem("ELEMENTAL");

    game.enableItem("FAIRY");
    game.enableItem("LIZA");
    game.enableItem("URUK");
    game.enableItem("GBLN");
    game.enableItem("HALFLING");
    game.enableItem("GNOLL");
    game.enableItem("DRLF");
    game.enableItem("MERC");
    game.enableItem("TITAN");
    game.enableItem("AMAZON");
    game.enableItem("OGER");
    game.enableItem("GNOME");
    game.enableItem("MOUNTAINMAN");
    game.enableItem("MINOTAUR");
    game.enableItem("GREY ELF");

    game.enableItem("LANCE");
    game.enableItem("MUSHROOM");

    game.enableItem("RRAT");
    game.enableItem("NOOGLE");
    game.enableItem("MUTANT");

    game.enableItem("BAXE");
    game.enableItem("MBAX");
    game.enableItem("ADMT");
    game.enableItem("ASWR");
    game.enableItem("ABAX");
    game.enableItem("IMTH");
    game.enableItem("ARNG");
    game.enableItem("AARM");
    game.enableItem("CAMEL");

    game.enableItem("DROW");
    game.enableItem("HYDR");
    game.enableItem("STORM GIANT");
    game.enableItem("CLOUD GIANT");
    game.enableItem("ILLYRTHID");
    game.enableItem("EVIL SORCERER");
    game.enableItem("EVIL MAGICIAN");
    game.enableItem("DARK MAGE");
    game.enableItem("EVIL WARRIORS");
    game.enableItem("ICE DRAGON");

    game.enableItem("HEALING POTION");
    game.enableItem("ROUGH GEM");
    game.enableItem("GEMS");
    game.enableItem("JAVELIN");
    game.enableItem("PIKE");
    game.enableItem("MWOL");
    game.enableItem("MSPI");
    game.enableItem("MOLE");
    game.enableItem("BPLA");
    game.enableItem("FSWO");
    game.enableItem("MCAR");
    game.enableItem("QUAR");
    game.enableItem("SABRE");
    game.enableItem("MACE");
    game.enableItem("MSTA");
    game.enableItem("DAGGER");
    game.enableItem("PDAG");
    game.enableItem("BHAM");
    game.enableItem("SHORTBOW");
    game.enableItem("BOW");
    game.enableItem("HEAVY CROSSBOW");
    game.enableItem("HARP");

    game.enableItem("TAROT CARDS", False);
    game.enableItem("SUPER BOW", False);
    game.enableItem("DOUBLE BOW", False);
    # end

if __name__ == '__main__':
    # check arguments
    prog_name = sys.argv[0]
    if len(sys.argv) < 2:
        Atlantis.usage()
        sys.exit()

    game = Atlantis.PyAtlantis()

    game_type = "ceran"
    if sys.argv[1][0] == '-':
        opt = sys.argv[1][1:]
        if opt == "type":
            game_type = sys.argv[2]
            sys.argv = sys.argv[3:]

    setRules(game, game_type)

    cmd = sys.argv[1]
    sys.argv = sys.argv[2:]

    if cmd == "new":
        if len(sys.argv) > 0:
            seed = int(sys.argv[0])
        else:
            import random
            seed = random.randint(0, 2147483647)
            print "Seed is %d" % seed

        game.new(seed)
        game.save()
    elif cmd == "run":
        if game.open():
            sys.exit()

        if game.run():
            sys.exit()

        game.save()
    elif cmd == "check":
        game.dummy()
        game.checkOrders()
    elif cmd == "genrules":
        game.genRules();

