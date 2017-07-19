#!/usr/bin/env python

import Atlantis
import sys

def setRules(game, game_name):
    if game_name != "ceran":
        print "Only ceran supported right now"
        sys.exit()

    import yaml
    import os
    f = open(os.path.dirname(__file__) + "/" + game_name + ".yaml")
    data_map = yaml.safe_load(f)
    f.close()

    cfg = data_map['config']
    if "items" in cfg:
        if "enable" in cfg["items"]:
            for i in cfg["items"]["enable"]:
                game.enableItem(i)

        if "disable" in cfg["items"]:
            for i in cfg["items"]["disable"]:
                game.enableItem(i, False)

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
        else:
            print "Unknown option %s" % sys.argv[1]
            sys.exit()

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

        print "Setting Up Turn..."
        if game.readPlayers():
            sys.exit()

        if game.isFinished():
            print "This game is finished!"
            sys.exit()

        if game.run():
            sys.exit()

        game.save()
    elif cmd == "check":
        game.dummy()
        game.checkOrders()
    elif cmd == "genrules":
        game.genRules();

