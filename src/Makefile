CXXFLAGS = -Wno-deprecated -g -O2 -std=c++11 -I../deps/include -D_REENTRANT -D_GNU_SOURCE
#CXXFLAGS = -Wno-deprecated -g -std=c++11 -I../deps/include -D_REENTRANT -D_GNU_SOURCE 

DEPS := Makefile.dep
DEP_FLAGS := -std=c++11 -MM

Target = liblei.a
SRCS = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp, %.o, $(SRCS))

all : $(Target)

$(Target) : $(OBJS)
	@$(RM) $(Target)
	ar cru $(Target) $(OBJS)
	@ranlib $(Target)
	mv $(Target) ../
	cp *.h ../include

$(DEPS): $(SRCS)
	$(CXX) $(DEP_FLAGS) $(SRCS) > $(DEPS)

-include $(DEPS)

clean :
	-rm $(OBJS)
	-rm $(Target)

install :
	cp *.h ../include
	cp $(Target) ../

distclean :
	-rm $(OBJS)
	-rm $(Target)
	-rm $(DEPS)
