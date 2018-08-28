MISK_OBJS := $(addprefix miskatonic/obj/,$(RULESET))
MISK_DEP  := $(addprefix dep/miskatonic/,$(RULESET:.o=.d))

-include $(MISK_DEP)

all: dep/miskatonic miskatonic/obj miskatonic

miskatonic: miskatonic/miskatonic.exe

clean::
	@rm -f $(MISK_OBJS) miskatonic/miskatonic.exe

miskatonic/obj:
	@mkdir $@

dep/miskatonic:
	@mkdir $@

$(MISK_OBJS): miskatonic/obj/%.o: miskatonic/%.cpp
	@$(CXXBUILD)

miskatonic/miskatonic.exe: $(ALL_OBJS) $(MISK_OBJS)
	@$(CXX) -o $@ $^

