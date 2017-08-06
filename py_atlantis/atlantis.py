#!/usr/bin/env python

import Atlantis
import sys

class CAtlantis:
    def __init__(self):
        self.game = Atlantis.PyAtlantis()

    def new(self, seed):
        self.game.new(seed)

    def save(self):
        return self.game.save()

    def open(self):
        return self.game.open()

    def readPlayers(self):
        return self.game.readPlayers()

    def isFinished(self):
        return self.game.isFinished()

    def parseOrders(self, fac_num, fname):
        self.game.parseOrders(fac_num, fname)

    def defaultWorkOrder(self):
        self.game.defaultWorkOrder()

    def removeInactiveFactions(self):
        self.game.removeInactiveFactions()

    def runFindOrders(self):
        self.game.runFindOrders()

    def runEnterOrders(self):
        self.game.runEnterOrders()

    def runPromoteOrders(self):
        self.game.runPromoteOrders()

    def doAttackOrders(self):
        self.game.doAttackOrders()

    def doAutoAttacksRegion(self, r):
        self.game.doAutoAttacksRegion(r)

    def runStealOrders(self):
        self.game.runStealOrders()

    def doGiveOrders(self):
        self.game.doGiveOrders()

    def doExchangeOrders(self):
        self.game.doExchangeOrders()

    def runDestroyOrders(self):
        self.game.runDestroyOrders()

    def runPillageOrders(self):
        self.game.runPillageOrders()

    def runTaxOrders(self):
        self.game.runTaxOrders()

    def doGuard1Orders(self):
        self.game.doGuard1Orders()

    def runCastOrders(self):
        self.game.runCastOrders()

    def runSellOrders(self):
        self.game.runSellOrders()

    def runBuyOrders(self):
        self.game.runBuyOrders()

    def runForgetOrders(self):
        self.game.runForgetOrders()

    def midProcessTurn(self):
        self.game.midProcessTurn()

    def runQuitOrders(self):
        self.game.runQuitOrders()

    def deleteEmptyUnits(self):
        self.game.deleteEmptyUnits()

    def sinkUncrewedShips(self):
        self.game.sinkUncrewedShips()

    def drownUnits(self):
        self.game.drownUnits()

    def doWithdrawOrders(self):
        self.game.doWithdrawOrders()

    def runSailOrders(self):
        self.game.runSailOrders()

    def runMoveOrders(self):
        self.game.runMoveOrders()

    def findDeadFactions(self):
        self.game.findDeadFactions()

    def runTeachOrders(self):
        self.game.runTeachOrders()

    def runMonthOrders(self):
        self.game.runMonthOrders()

    def runTeleportOrders(self):
        self.game.runTeleportOrders()

    def assessMaintenance(self):
        self.game.assessMaintenance()

    def postProcessTurn(self):
        self.game.postProcessTurn()

    def writeReports(self):
        self.game.writeReports()

    def writePlayers(self):
        self.game.writePlayers()

    def cleanup(self):
        self.game.cleanup()

    def dummy(self):
        self.game.dummy()

    def checkOrders(self):
        return self.game.checkOrders()

    def genRules(self):
        return self.game.genRules()

    def enableItem(self, item_name, val=True):
        self.game.enableItem(item_name, val)

    def factions(self):
        return self.game.factions()

    def regions(self):
        return self.game.regions()
#end

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

#end setRules


if __name__ == '__main__':
    # check arguments
    prog_name = sys.argv[0]
    if len(sys.argv) < 2:
        Atlantis.usage()
        sys.exit()

    game = CAtlantis()

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
        game.writePlayers()
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

        print "Reading the Orders File..."
        facs = game.factions()
        for f in facs:
            if not f.isNpc():
                game.parseOrders(f.num(), "orders.%d" % f.num())

        print "Running the Turn..."
        game.defaultWorkOrder()
        game.removeInactiveFactions()

        # Form and instant orders are handled during parsing
        print "Running FIND Orders..."
        game.runFindOrders()

        print "Running ENTER/LEAVE Orders..."
        game.runEnterOrders()

        print "Running PROMOTE/EVICT Orders..."
        game.runPromoteOrders()

        print "Running Combat..."
        game.doAttackOrders()
        for r in game.regions():
            game.doAutoAttacksRegion(r)

        print "Running STEAL/ASSASSINATE Orders..."
        game.runStealOrders()

        print "Running GIVE/PAY/TRANSFER Orders..."
        game.doGiveOrders()

        print "Running EXCHANGE Orders..."
        game.doExchangeOrders()

        print "Running DESTROY Orders..."
        game.runDestroyOrders()

        print "Running PILLAGE Orders..."
        game.runPillageOrders()

        print "Running TAX Orders..."
        game.runTaxOrders()

        print "Running GUARD 1 Orders..."
        game.doGuard1Orders()

        print "Running Magic Orders..."
        game.runCastOrders()

        print "Running SELL Orders..."
        game.runSellOrders()

        print "Running BUY Orders..."
        game.runBuyOrders()

        print "Running FORGET Orders..."
        game.runForgetOrders()

        print "Mid-Turn Processing..."
        game.midProcessTurn()

        print "Running QUIT Orders..."
        game.runQuitOrders()

        print "Removing Empty Units..."
        game.deleteEmptyUnits()
        game.sinkUncrewedShips()
        game.drownUnits()

        game.doWithdrawOrders()

        print "Running Sail Orders..."
        game.runSailOrders()

        print "Running Move Orders..."
        game.runMoveOrders()
        game.sinkUncrewedShips()
        game.drownUnits()

        game.findDeadFactions()

        print "Running Teach Orders..."
        game.runTeachOrders()

        print "Running Month-long Orders..."
        game.runMonthOrders()
        game.runTeleportOrders()

        print "Assessing Maintenance costs..."
        game.assessMaintenance()

        print "Post-Turn Processing..."
        game.postProcessTurn()

        print "Writing the Report File..."
        game.writeReports()

        print ""

        print "Writing Playerinfo File..."
        game.writePlayers()

        print "Removing Dead Factions..."
        game.cleanup()

        print "done"

        game.save()
    elif cmd == "check":
        game.dummy()
        game.checkOrders()
    elif cmd == "genrules":
        game.genRules();

