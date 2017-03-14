NAME = vega
include config.mk
 
INSTALL ?= install

CC = gcc


CXX = g++

CPPFLAGS ?= -g -D_DEBUG

TAU = deps/tau
include $(TAU)/config.mk
LUAJIT = deps/luajit

INCLUDES = -I$(TAU)/include  -Ideps/luajit/src  -I$(TAU)/deps/libevent/include 
LDFLAGS = -L$(TAU) -L$(LUAJIT)/src -L$(TAU)/deps/libevent/.libs  $(TAU_LIBS) -lluajit

PLATFORM_LDFLAGS ?= 

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
PLATFORM_LDFLAGS = -pthread  -lrt -ldl -lm -export-dynamic
endif

ifeq ($(UNAME), Darwin)
PLATFORM_LDFLAGS = -pagezero_size 10000 -image_base 100000000 -framework CoreServices
endif

prefix = $(DEST)

CPPDEPS = -MT$@ -MF`echo $@ | sed -e 's,\.o$$,.d,'` -MD -MP
CXXFLAGS = -std=c++0x $(INCLUDES) -D_THREAD_SAFE -pthread    $(CPPFLAGS)
		
SOURCES = $(wildcard src/*.cpp wildcard src/lua/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)	

TARGET = $(NAME)

all: $(TARGET)

install: install_vega

uninstall: uninstall_vega

clean_vega:
	find src -name *.o -o -name *.d | xargs rm -rf
	rm -f $(TARGET)
	
rebuild: clean_vega
	make -j	
	
rebuild_tau: clean_tau
	make -j	

clean: 	clean_vega
#	 -(cd $(TAU) && $(MAKE) clean_all)
#	-(cd deps/luajit && $(MAKE) clean)

clean_tau: 
	-(cd $(TAU) && $(MAKE) clean)
	
	
libs: 
	cd $(TAU) && $(MAKE)
#	cd deps/luajit && $(MAKE) && rm src/libluajit.so

$(TARGET): libs $(OBJECTS) 
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) $(PLATFORM_LDFLAGS) 
    
install_vega: 

    #
    # install -c $(TARGET) $(prefix)/bin
    # rsync -a --exclude='test' --exclude 'test.lua' lib/* $(prefix)/lib/lua/5.1
    # chmod -R a+r $(prefix)/lib/lua/5.1/


		
test:
	test/run.sh	

%.o: %.cpp
	$(CXX) -c -o $@ $(CXXFLAGS) $(CPPDEPS) $<


.PHONY: all install uninstall clean install_vega  test 


-include src/*.d