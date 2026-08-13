appname := autotester
headless_appname := cemu-headless

CXX := g++

CXXFLAGS := -std=c++11 -O2 -Wall -Wextra
CXXFLAGS += -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=date-time -Werror=return-type -Werror=pointer-arith -Winit-self

CPPFLAGS += -DGLOB_SUPPORT

# Add these flags if your compiler supports it
#CFLAGS += -Wstack-protector -fstack-protector-strong --param=ssp-buffer-size=1 -fsanitize=address,bounds -fsanitize-undefined-trap-on-error

LDLIBS  := -L../../core/ -lcemucore
CORELIB := ../../core/libcemucore.a

ifeq ($(OS),Windows_NT)
	LDLIBS += -static-libgcc -static-libstdc++ -static
endif

srcfiles := autotester.cpp autotester_cli.cpp headless_cli.cpp
objects  := $(patsubst %.cpp, %.o, $(srcfiles))

all: $(appname) $(headless_appname)

$(appname): autotester.o autotester_cli.o $(CORELIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ autotester.o autotester_cli.o $(LDLIBS)

$(headless_appname): autotester.o headless_cli.o $(CORELIB)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ autotester.o headless_cli.o $(LDLIBS)

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(objects) $(appname) $(headless_appname)
