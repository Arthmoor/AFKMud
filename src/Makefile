#This is the AFKMud Makefile.
#This should be usable on pretty much any Linux system.
#Refer to your 'man gcc' for explanations of these options
#To compile in FreeBSD or OpenBSD, use the gmake compiler.

#Change this to reflect whichever compiler you want to use.
CC      = g++

#Type of machine to compile for. Athlon works just as well on Duron too.
#The march option needs to match the general family of CPU.
#If you don't know what to set for these options, and your system administrator doesn't either, comment this line out
#MACHINE = -mtune=athlon64 -march=athlon64

#Uncomment if compiling in Windows under Cygwin
#CYGWIN = 1

# Uncomment the two lines below if compiling on a Solaris box
#SOLARIS_FLAG = -Dsun -DSYSV -Wno-char-subscripts
#SOLARIS_LINK = -lnsl -lsocket -lresolv

#Uncomment the line below if you are getting undefined references to dlsym, dlopen, and dlclose.
#Comment it out if you get errors about ldl not being found.
NEED_DL = -ldl

#Some systems need this for dynamic linking to work.
EXPORT_SYMBOLS = -export-dynamic

#Math functions - uncomment if you get errors or warnings about math functions.
MATH_LIB = -lm

#MySQL support. Need to specify the proper path for your system.
#Uncomment this to enable SQL support.
#DO NOT ENABLE if running in Cygwin. Cygwin has no library support!
#LIB_MYSQL = -L/usr/lib/mysql/ -lmysqlclient

#IMC2 - Comment out to disable IMC2 support
IMC = 1

#Multiport support. Comment out to disable this feature.
#MULTIPORT = 1

#Miscellaneous compiler options.
OPT_FLAG = -pipe -Os
DEBUG_FLAG = -g2
#PROF_FLAG = -pg

W_FLAGS = -std=c++0x -Wall -Wshadow -Wformat=2 -Wmissing-format-attribute -Wpointer-arith -Wcast-align -Wredundant-decls -Wswitch-default -Wuseless-cast -Wwrite-strings -Wstrict-null-sentinel

ifndef CYGWIN
   W_FLAGS := $(W_FLAGS) #-Werror
   OPT_FLAG := -rdynamic $(OPT_FLAG)
endif

C_FLAGS = $(MACHINE) $(W_FLAGS) $(DEBUG_FLAG) $(OPT_FLAG) $(PROF_FLAG) $(SOLARIS_FLAG) $(EXPORT_SYMBOLS)
L_FLAGS = $(MACHINE) $(DEBUG_FLAG) $(OPT_FLAG) $(PROF_FLAG) $(EXPORT_SYMBOLS) $(SOLARIS_LINK) $(MATH_LIB) -lz -lgd $(LIB_MYSQL) $(NEED_DL)
#D_FLAGS : For the DNS Resolver process. No need in linking all the extra libs for this.
D_FLAGS = $(MACHINE) $(W_FLAGS) $(DEBUG_FLAG) $(OPT_FLAG) $(PROF_FLAG) $(SOLARIS_LINK)

C_FILES = act_comm.cpp act_info.cpp act_move.cpp act_obj.cpp act_wiz.cpp \
          archery.cpp area.cpp areaconvert.cpp auction.cpp ban.cpp bits.cpp boards.cpp \
          build.cpp calendar.cpp channels.cpp character.cpp chess.cpp clans.cpp color.cpp \
          comm.cpp commands.cpp comments.cpp connhist.cpp const.cpp db.cpp \
          deity.cpp descriptor.cpp editor.cpp environment.cpp event.cpp \
          event_handler.cpp features.cpp fight.cpp finger.cpp handler.cpp \
          hashstr.cpp help.cpp hotboot.cpp imm_host.cpp liquids.cpp magic.cpp \
          mapout.cpp mapper.cpp misc.cpp mobindex.cpp modules.cpp \
          mspecial.cpp mssp.cpp mudcfg.cpp mud_comm.cpp mud_prog.cpp new_auth.cpp \
          object.cpp objindex.cpp olcmob.cpp olcobj.cpp olcroom.cpp overland.cpp \
          pfiles.cpp player.cpp polymorph.cpp rare.cpp realms.cpp renumber.cpp reset.cpp \
          roomindex.cpp save.cpp sha256.cpp ships.cpp shops.cpp \
          skills.cpp skyship.cpp slay.cpp tables.cpp track.cpp treasure.cpp \
          update.cpp variables.cpp web.cpp

ifdef LIB_MYSQL
   C_FILES := sql.cpp $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DSQL
endif

ifdef IMC
   C_FILES := imc.cpp $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DIMC
endif

ifdef MULTIPORT
   C_FILES := shell.cpp $(C_FILES)
   C_FLAGS := $(C_FLAGS) -DMULTIPORT
   #Sorry folks, this doesn't work in Cygwin
   ifndef CYGWIN
      C_FILES := mudmsg.cpp $(C_FILES)
   endif
endif

O_FILES := $(patsubst %.cpp,o/%.o,$(C_FILES))

H_FILES = $(wildcard *.h) 

ifdef CYGWIN
   AFKMUD = afkmud.exe
   RESOLVER = resolver.exe
else
   AFKMUD = afkmud
   RESOLVER = resolver
endif

all:
	@echo "Building AFKMud....";
	$(MAKE) -j2 -s afkmud
	@echo "Buidling DNS Resolver...";
	$(MAKE) -s resolver
#	@echo "Building modules...";
#	$(MAKE) -s -C modules

# pull in dependency info for *existing* .o files
-include dependencies.d

afkmud: $(O_FILES)
	@rm -f $(AFKMUD)
ifdef CYGWIN
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

modsclean:
	$(MAKE) -C modules clean

purge:
	@rm -f o/*.o $(AFKMUD) afkmud.def afkmud.exp core dependencies.d $(RESOLVER) resolver.o

modules:
	$(MAKE) -C modules

modspurge:
	$(MAKE) -C modules purge

allclean:
	$(MAKE) modsclean
	$(MAKE) clean

o/%.o: %.cpp
	@echo "  Compiling $@....";
	$(CC) -c $(C_FLAGS) $< -o $@

.cpp.o: mud.h
	$(CC) -c $(C_FLAGS) $<
