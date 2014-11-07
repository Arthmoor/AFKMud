#AFKMud dynamically loaded commands.
#This makefile can be invoked directly, or via the makefile in the src directory.
#
#Takes any existing cpp files in this directory and makes dynamically loadable .so files out of them.

#Type of machine to compile for. Athlon works just as well on Duron too.
#The march option needs to match the general family of CPU.
#If you don't know what to set for these options, and your system administrator doesn't either, comment this line out
#MACHINE = -mtune=athlon64 -march=athlon64

# Uncomment the two lines below if compiling on a Solaris box
#SOLARIS_FLAG = -Dsun -DSYSV -Wno-char-subscripts
#SOLARIS_LINK = -lnsl -lsocket -lresolv

#IMC2 - Comment out to disable IMC2 support
IMC = 1

#Multiport support. Comment out to disable this feature.
MULTIPORT = 1

#Miscellaneous compiler options.
OPT_FLAG = -fpic -pipe -Os
DEBUG_FLAG = -g2
#PROF_FLAG = -pg

W_FLAGS = -Wall -Wformat=2 -Wshadow -Wpointer-arith -Wcast-align -Wcast-qual -Wredundant-decls -Wconversion

C_FLAGS = $(MACHINE) $(W_FLAGS) $(DEBUG_FLAG) $(OPT_FLAG) $(PROF_FLAG) $(SOLARIS_FLAG)
C_FILES := $(wildcard *.cpp)
O_FILES := $(patsubst %.cpp,o/%.o,$(C_FILES))
SO_FILES := $(patsubst o/%.o,so/%.so,$(O_FILES))

ifdef IMC
   C_FLAGS := $(C_FLAGS) -DIMC
endif

ifdef MULTIPORT
   C_FLAGS := $(C_FLAGS) -DMULTIPORT
endif

all:
	$(MAKE) -s obj
	$(MAKE) -s sobj
	@echo "Done building modules.";

obj:	$(O_FILES)

sobj:	$(SO_FILES)

clean:
	@rm -f o/*.o so/*.so
	$(MAKE) all

purge:
	@rm -f o/*.o so/*.so

o/%.o: %.cpp
	@echo "  Compiling $@....";
	g++ -c $(C_FLAGS) $< -o $@

so/%.so: o/%.o
	@echo "  Linking $@....";
	g++ -shared $< -o $@
