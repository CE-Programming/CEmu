GCC = gcc
GXX = g++

GCCFLAGS = -Wall -W -fPIC -flto -O3 -g3

OBJS = $(patsubst %.c, %.o, $(shell find . -name \*.c))
OBJS += $(patsubst %.cpp, %.o, $(shell find . -name \*.cpp))
EXE = cemu_core
DISTDIR = .

all: $(OBJS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -std=gnu11 -c $< -o $@

%.o: %.cpp
	$(GXX) $(GCCFLAGS) -std=c++11 -c $< -o $@

clean:
	rm -f $(OBJS) $(DISTDIR)/$(EXE)
