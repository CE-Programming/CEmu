CC = gcc
CXX = g++

# If you want no debug/symbols info, remove -g3
# If you need debug support, add -DDEBUG_SUPPORT
CFLAGS = -Wall -W -fPIC -flto -O3 -g3 -static

OBJS = $(patsubst %.c, %.o, $(shell find . -name \*.c))
OBJS += $(patsubst %.cpp, %.o, $(shell find . -name \*.cpp))

STATICLIB = libcemucore.a

.PHONY: all
all: lib

lib: $(OBJS)
	ar rcs $(STATICLIB) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -std=gnu11 -c $< -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -std=c++11 -c $< -o $@

clean:
	rm -f $(OBJS) $(STATICLIB)
