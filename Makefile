SRCDIR := src

SRC:=$(wildcard  $(SRCDIR)/*.cpp)
OBJ = $(SRC:.cpp=.o)
DEPS := $(OBJ:.o=.d)

CXXFLAGS += -MMD -MP -Wno-write-strings

all: CXXFLAGS += -g
all: netgdif

netgdif: $(OBJ)
	$(CXX) $(LDFLAGS)  $^ -o $@ $(LIBS)

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)
	rm -f netgdif
	rm -f $(DEPS)
	
-include $(DEPS)