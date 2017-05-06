CC  = gcc
CXX = g++

# If you want no debug/symbols info, remove -g3
# If you need debug support, add -DDEBUG_SUPPORT (but that requires C++...)
CFLAGS = -Wall -W -fPIC -O3 -g3 -static

# Add these flags if your compiler supports it
#CFLAGS += -Wstack-protector -fstack-protector-strong --param=ssp-buffer-size=1 -fsanitize=address,bounds -fsanitize-undefined-trap-on-error

OBJS  = $(patsubst %.c,   %.o, $(shell find . -name \*.c))
OBJS += $(patsubst %.cpp, %.o, $(shell find . -name \*.cpp))

STATICLIB = libcemucore.a

all: lib

lib: $(OBJS)
	ar rcs $(STATICLIB) $(OBJS)

%.o: %.c
	$(CC)  $(CFLAGS) -std=gnu11 -c $< -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -std=c++11 -c $< -o $@

clean:
	rm -f $(OBJS) $(STATICLIB)

.PHONY: clean all lib
