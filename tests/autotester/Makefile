appname := autotester

CXX := g++
CXXFLAGS := -std=c++11 -O3 -g3 -Wall -flto
CXXFLAGS += -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=date-time -Werror=return-type -Werror=pointer-arith -Winit-self
LDFLAGS := -flto -L../../core/ -lcemucore

srcfiles := autotester.cpp autotester_cli.cpp
objects  := $(patsubst %.cpp, %.o, $(srcfiles))

all: $(appname)

$(appname): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname) $(objects) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(objects) $(appname)