appname := cemu-sdl

CXX := g++

CXXFLAGS := -std=c++11 -O2 -g3 -Wall -Wextra
CXXFLAGS += -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=date-time -Werror=return-type -Werror=pointer-arith -Winit-self

# Add these flags if your compiler supports it
#CFLAGS += -Wstack-protector -fstack-protector-strong --param=ssp-buffer-size=1 -fsanitize=address,bounds -fsanitize-undefined-trap-on-error

LDLIBS  := -L../../core/ -lcemucore -lSDL2

srcfiles := main.cpp
objects  := $(patsubst %.cpp, %.o, $(srcfiles))

all: $(appname)

$(appname): $(objects)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname) $(objects) $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(objects) $(appname)
