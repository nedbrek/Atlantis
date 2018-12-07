.PHONY: all clean lib

CFLAGS := -g -I. -I.. -Wall -MP -MMD
CXXFLAGS := $(CFLAGS)

CXXBUILD = $(CXX) $(CXXFLAGS) -MF $(patsubst %.cpp,dep/%.d,$<) -c -o $@ $<

# objects shared by all rule sets
OBJ := alist.o aregion.o army.o astring.o battle.o faction.o \
  fileio.o game.o gamedefs.o gameio.o genrules.o items.o main.o \
  market.o modify.o monthorders.o npc.o object.o orders.o parseorders.o \
  production.o runorders.o shields.o skills.o skillshows.o specials.o \
  spells.o template.o unit.o
ALL_OBJ := $(OBJ) rand.o

# objects per rule set
RULESET := extra.o monsters.o rules.o world.o gamedata.o

# sub games
GAMES := ceran conquest miskatonic realms standard wyreth

DEP  := $(addprefix dep/,$(OBJ:.o=.d))
OBJS := $(addprefix obj/,$(OBJ))
ALL_OBJS := $(addprefix obj/,$(ALL_OBJ))

### targets
all: dep obj

obj:
	@mkdir $@

dep:
	@mkdir $@

-include $(DEP)
-include $(addsuffix /Makefile.inc, $(GAMES))

clean::
	@rm -f $(ALL_OBJS)

$(OBJS): obj/%.o: %.cpp
	@$(CXXBUILD)

obj/rand.o: i_rand.c
	@$(CC) $(CFLAGS) -MF $(patsubst %.c,dep/%.d,$<) -c -o $@ $<

