CERAN_OBJS := $(addprefix ceran/obj/,$(RULESET))
CERAN_DEP  := $(addprefix dep/ceran/,$(RULESET:.o=.d))

-include $(CERAN_DEP)

all: dep/ceran ceran/obj ceran

ceran: ceran/ceran.exe

clean::
	@rm -f $(CERAN_OBJS) ceran/ceran.exe

ceran/obj:
	@mkdir $@

dep/ceran:
	@mkdir $@

$(CERAN_OBJS): ceran/obj/%.o: ceran/%.cpp
	@$(CXXBUILD)

ceran/ceran.exe: $(ALL_OBJS) $(CERAN_OBJS)
	@$(CXX) -o $@ $^ $(LDFLAGS)

