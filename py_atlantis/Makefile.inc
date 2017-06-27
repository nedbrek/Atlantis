CFLAGS += -fPIC
CXXFLAGS += -fPIC $(shell pkg-config --cflags python-2.7)

PYATL_OBJS := $(addprefix py_atlantis/obj/,$(RULESET)) py_atlantis/obj/py_atlantis.o
PYATL_DEP  := $(addprefix dep/py_atlantis/,$(RULESET:.o=.d))

-include $(PYATL_DEP)

all: dep/py_atlantis py_atlantis/obj py_atlantis

py_atlantis: py_atlantis/Atlantis.so

clean::
	@rm -f $(PYATL_OBJS) py_atlantis/py_atlantis.so

py_atlantis/obj:
	@mkdir $@

dep/py_atlantis:
	@mkdir $@

$(PYATL_OBJS): py_atlantis/obj/%.o: py_atlantis/%.cpp
	@$(CXXBUILD)

py_atlantis/Atlantis.so: $(ALL_OBJS) $(PYATL_OBJS)
	@$(CXX) -shared -o $@ $^ $(LDFLAGS) -lboost_python $(shell pkg-config --libs python-2.7)

