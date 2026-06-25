#This is the AFKMud Makefile.
#This should be usable on pretty much any Linux system.
#Refer to your 'man gcc' for explanations of these options
#To compile in FreeBSD or OpenBSD, use the gmake compiler.

#Change this to reflect whichever compiler you want to use.
CC      = g++
#CC      = clang++

# Only use ccache if it's found
CCACHE := $(shell command -v ccache 2>/dev/null)
# This creates a safe command for compiling, but not for linking
CXX_COMPILE = $(CCACHE) $(CC)

# Automatically detect if we are running in a Cygwin environment
ifeq ($(findstring CYGWIN, $(shell uname -s)), CYGWIN)
    IS_CYGWIN = 1
endif

# Check if libdl is available on the system
HAS_DL := $(shell echo 'int main(){}' | $(CC) -x c++ - -ldl -o /dev/null 2>/dev/null && echo yes)

ifeq ($(HAS_DL),yes)
    NEED_DL = -ldl
else
    NEED_DL =
endif

#OS Detection. Used to check for MacOS below.
UNAME_S := $(shell uname -s)

# Set EXPORT_SYMBOLS only if not on Darwin (MacOS)
ifneq ($(UNAME_S), Darwin)
    EXPORT_SYMBOLS = -export-dynamic
endif

#Multiport support. Comment out to disable this feature.
#MULTIPORT = 1

#MySQL support. Need to specify the proper path for your system.
#Uncomment this to enable SQL support.
#The link argument should be either -lmysqlclient or -lmariadb depending on which DB solution the server is using.
#LIB_MYSQL = -lmariadb

#Miscellaneous compiler options.
OPT_FLAG = -pipe -O3
DEBUG_FLAG = -g2

#GCC 4.8+ memory checking option. https://github.com/google/sanitizers Does not work in Cygwin.
#These should only be enabled for testing environments. Disable for a live environment.
ifndef IS_CYGWIN
   MEM_FLAG = -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer
endif

# These protect against stack smashing and buffer overflows in production.
# We add -D_FORTIFY_SOURCE=3 to perform runtime checks on string/memory functions.
HARDENING_FLAGS = -fstack-protector-strong -D_FORTIFY_SOURCE=3

#AFKMud now requires at least C++23 to be enabled. This will work in GCC 13.1 or later, which should be more than sufficient.
#You can try out any *later* standard, but not an earlier one since the code uses many C++23 features.
C_STD = -std=c++23

#Comment out for any system it complains this isn't available on. It will never work in Cygwin.
ifndef IS_CYGWIN
   LIB_TRACE = -lstdc++_libbacktrace
endif

ifneq ($(CC), clang++)
   W_FLAGS = -Wall -Wshadow -Wpedantic -Wmissing-format-attribute -Wpointer-arith -Wcast-align -Wredundant-decls -Wswitch-default -Wuseless-cast -Wwrite-strings -Wformat=2 -Wformat-security -Wformat-signedness -Wtype-limits
else
   W_FLAGS = -Wall -Wshadow -Wpedantic -Wmissing-format-attribute -Wpointer-arith -Wcast-align -Wredundant-decls -Wswitch-default -Wwrite-strings -Wformat=2 -Wformat-security -Wformat-signedness -Wtype-limits
endif

ifndef IS_CYGWIN
   W_FLAGS := $(W_FLAGS) #-Werror
   ifeq ($(CC), g++)
      OPT_FLAG := -rdynamic $(OPT_FLAG)
   endif
endif

C_FLAGS = $(C_STD) $(W_FLAGS) $(HARDENING_FLAGS) $(DEBUG_FLAG) $(OPT_FLAG) $(MEM_FLAG) $(EXPORT_SYMBOLS)

ifeq ($(CC), g++)
   L_FLAGS = $(DEBUG_FLAG) $(OPT_FLAG) $(MEM_FLAG) $(EXPORT_SYMBOLS) -lz -lgd $(LIB_TRACE) $(LIB_MYSQL) $(NEED_DL)
else
   L_FLAGS = $(DEBUG_FLAG) $(OPT_FLAG) $(MEM_FLAG) $(EXPORT_SYMBOLS) -lz -lgd $(LIB_MYSQL) $(NEED_DL) -Wl, --export-dynamic
endif

#D_FLAGS : For the DNS Resolver process.
D_FLAGS = $(C_STD) $(W_FLAGS) $(HARDENING_FLAGS) $(DEBUG_FLAG) $(OPT_FLAG) $(MEM_FLAG)

C_FILES = act_comm.cpp act_info.cpp act_move.cpp act_obj.cpp act_wiz.cpp \
          archery.cpp area.cpp areaconvert.cpp area_fussconvert.cpp area_oldafk.cpp \
          auction.cpp ban.cpp bits.cpp boards.cpp build.cpp calendar.cpp channels.cpp \
          character.cpp chess.cpp clans.cpp color.cpp comm.cpp commands.cpp comments.cpp \
          connhist.cpp const.cpp db.cpp deity.cpp descriptor.cpp editor.cpp environment.cpp \
          event.cpp event_handler.cpp features.cpp fight.cpp finger.cpp handler.cpp \
          help.cpp hotboot.cpp imm_host.cpp liquids.cpp magic.cpp \
          mapout.cpp mapper.cpp misc.cpp mobindex.cpp \
          mspecial.cpp mssp.cpp mudcfg.cpp mud_comm.cpp mud_prog.cpp new_auth.cpp \
          object.cpp objindex.cpp olcmob.cpp olcobj.cpp olcroom.cpp overland.cpp \
          pfiles.cpp player.cpp polymorph.cpp rare.cpp realms.cpp renumber.cpp reset.cpp \
          roomindex.cpp save.cpp sha256.cpp sha512.cpp ships.cpp shops.cpp \
          skills.cpp skyship.cpp slay.cpp string_tools.cpp tables.cpp track.cpp treasure.cpp \
          update.cpp variables.cpp weather.cpp web.cpp

ifdef LIB_MYSQL
	C_FILES := sql.cpp $(C_FILES)
	C_FLAGS := $(C_FLAGS) -DSQL
endif

ifdef MULTIPORT
	C_FILES := shell.cpp $(C_FILES)
	C_FLAGS := $(C_FLAGS) -DMULTIPORT
	#Sorry folks, this doesn't work in Cygwin
   ifndef IS_CYGWIN
	C_FILES := mudmsg.cpp $(C_FILES)
   endif
endif

O_FILES := $(patsubst %.cpp,o/%.o,$(C_FILES))

H_FILES = $(wildcard *.h) 

ifdef IS_CYGWIN
	AFKMUD = afkmud.exe
	RESOLVER = resolver.exe
else
	AFKMUD = afkmud
	RESOLVER = resolver
endif

all:
	@echo "Building AFKMud...";
	$(info    Using C-Flags: $(C_FLAGS))
	$(info    Using L-Flags: $(L_FLAGS))
	$(MAKE) -j2 -s afkmud
	@echo "Buidling DNS Resolver...";
	$(info    Resolver using C-Flags: $(D_FLAGS))
	$(MAKE) -s resolver

# pull in dependency info for *existing* .o files
-include dependencies.d

afkmud: $(O_FILES)
	@rm -f $(AFKMUD)
ifdef IS_CYGWIN
	@dlltool --export-all --output-def afkmud.def $(O_FILES)
	@dlltool --dllname $(AFKMUD) --output-exp afkmud.exp --def afkmud.def
	$(CC) -o $(AFKMUD) $(O_FILES) afkmud.exp $(L_FLAGS)
else
	$(CC) -o $(AFKMUD) $(O_FILES) $(L_FLAGS)
endif
	@echo "Generating dependency file ...";
	@$(CC) -MM $(C_FLAGS) $(C_FILES) > dependencies.d
	@perl -pi -e 's.^([a-z]).o/$$1.g' dependencies.d
	@echo "Done building AFKMud.";
	@chmod g+w $(AFKMUD)
	@chmod a+x $(AFKMUD)
	@chmod g+w $(O_FILES)

indent:
	@indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l175 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(C_FILES) resolver.cpp
	@indent -ts3 -nut -nsaf -nsai -nsaw -npcs -npsl -ncs -nbc -bls -prs -bap -cbi0 -cli3 -bli0 -l175 -lp -i3 -cdb -c1 -cd1 -sc -pmt $(H_FILES)

indentclean:
	@rm *.cpp~ *.h~

resolver: resolver.o
	@rm -f $(RESOLVER) 
	$(CC) $(D_FLAGS) -o $(RESOLVER) resolver.o
	@echo "Done buidling DNS Resolver.";
	@chmod g+w $(RESOLVER)
	@chmod a+x $(RESOLVER)
	@chmod g+w resolver.o

clean:
	@rm -f o/*.o $(AFKMUD) afkmud.def afkmud.exp core dependencies.d $(RESOLVER) resolver.o
	$(MAKE) all

purge:
	@rm -f o/*.o $(AFKMUD) afkmud.def afkmud.exp core dependencies.d $(RESOLVER) resolver.o

o/%.o: %.cpp
	@echo "  Compiling $@...";
	@$(CXX_COMPILE) -c $(C_FLAGS) $< -o $@
