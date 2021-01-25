SRCDIR := src

SRC:=$(wildcard  $(SRCDIR)/*.cpp)
OBJ = $(SRC:.cpp=.o)
DEPS := $(OBJ:.o=.d)

CXXFLAGS += -std=c++14 -O3 -MMD -MP -Wno-write-strings

LIBS = -lpthread  -lncurses 

all: netgdif

verbose: CXXFLAGS += -DVERBOSE
verbose: clean
verbose: netgdif

netgdif: $(OBJ)
	$(CXX) $(LDFLAGS)  $^ -o $@ $(LIBS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -f netgdif
	rm -f $(DEPS)
	
-include $(DEPS)

.PHONY: verbose