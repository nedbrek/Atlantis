#!/usr/bin/env python

import Atlantis

if __name__ == '__main__':
    # check arguments
    import sys
    prog_name = sys.argv[0]
    if len(sys.argv) < 2:
        Atlantis.usage()
        sys.exit()

    game = Atlantis.PyAtlantis()

    cmd = sys.argv[1]
    if cmd == "new":
        if len(sys.argv) > 2:
            seed = int(sys.argv[2])
        else:
            import random
            seed = random.randint(0, 2147483647)
            print "Seed is %d" % seed

        game.new(seed)
        game.save()
    elif cmd == "run":
        game.open()
        game.run()
        game.save()
    elif cmd == "check":
        game.dummy()
        game.checkOrders()
    elif cmd == "genrules":
        game.genRules();

