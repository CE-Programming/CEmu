GCC = gcc
GXX = g++

# If you want no debug/symbols info, remove -g
# If you need debug support, add -DDEBUG_SUPPORT
GCCFLAGS = -Wall -W -fPIC -flto -O3 -g -static

OBJS = $(patsubst %.c, %.o, $(shell find . -name \*.c))
OBJS += $(patsubst %.cpp, %.o, $(shell find . -name \*.cpp))

STATICLIB = libcemucore.a

.PHONY: all
all: lib

lib: $(OBJS)
	ar rcs $(STATICLIB) $(OBJS)

%.o: %.c
	$(GCC) $(GCCFLAGS) -std=gnu11 -c $< -o $@

%.o: %.cpp
	$(GXX) $(GCCFLAGS) -std=c++11 -c $< -o $@

clean:
	rm -f $(OBJS) $(STATICLIB)
