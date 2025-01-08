AFKMud: Alsherok Game Code Release
==================================

Space. The final frontier.... no wait. That was someone else's 5 year mission.

Sine our initial public release back in 2002, we've spent many years developing this codebase into what you see today. Sure, it's not the prettiest code out there. But it does work, and we'd like to think it's better than most. The last 2-3 years were devoted to a C++ conversion which never seemed to get close to "finished". There was always something new to try. Something old to convert. Then there just wasn't enough time. We'd still like to think we did good though. Pushed things past expectations. Did things nobody else has. Hopefully we've left you with a nice, clean, stable, and reliable foundation for you to build your world upon.

This document should help explain some of the mundane details such as how much memory it eats. How much drive space does it need. What features can be compiled out during initial setup.

Either way, have fun. Enjoy yourself and the new world you're about to create.

--- Samson, Implementor of Alsherok. Home site for the AFKMud codebase.

Homepage: https://smaugmuds.afkmods.com/

Licensing
=========

See the file 'license.txt' in your afkmud/doc directory. Read, obey. Downloading and installing this code implies your agreement to the terms.

Installation Notes
==================

There are some things that you need to be aware of prior to installing the game. Please read this stuff BEFORE you try and run the game. You'll avoid alot of hassle, and we'll be asking if you checked this stuff anyway.

Release Frequency
=================

Codebase releases will not be following any kind of set schedule. This is usually done on the fly based on the number of bug reports received between releases. If there are a lot of them, then it could be a relatively short period. If there are none, then it will depend on how confident we feel about new features which might be added. If you absolutely cannot wait, there's always the GitHub repository.

Documentation
=============

Documentation on various features of AFKMud can be found in the areadocs directory. For information on area construction, see Area.txt.
Adding races and classes are covered by Addrace.txt and Addclass.txt respectively.
Overland editing is covered by Newcontinent.txt and RGB.txt.

We know the existing documentation is sparse. Hopefully it won't stay that way forever, but it's been given very low priority. User submitted docs are always welcome though.

Older SMAUG and Merc documentation can be found in the /doc directory. Most of these older docs have been superseded by newer AFK docs, but  there may still be useful elements in them. One that you should most likely read anyway is hacker.txt - it offers some basic thoughts on how to be a mud admin.

If anything seems lacking or there is a feature you want to know about that isn't documented here, please let us know.

Cygwin Support
==============

Cygwin support is largely touch and go. The C++ conversion leaves it somewhat up in the air as to whether it will decide to work properly or not. It has not been heavily tested, so don't be too surprised if after it's compiled it just refuses to behave.

SQL, Multiport, and interport channels are not available in Cygwin. It simply lacks the proper tools to handle it. You also need to make sure the NEED_DL line in the Makefile is commented out.

FreeBSD Support
===============

There should be no real reason why AFKMud 2.0 should not work in FreeBSD. It might require some tweaking of the Makefile, and you'll need to be sure to use the gmake compiler, but aside from that the code itself should compile hassle free.

Comment out the NEED_DL define in the Makefile to compile.

OpenBSD Support
===============

OpenBSD support has not been verified with 2.0. It was shaky at best in 1.x. If you have any luck getting it to work, let us know. If it needed special attention,
let us know what changes to include and we'll try and see to it they show up in a future maintenance release.

Comment out the NEED_DL define, and the EXPORT_SYMBOLS define in the Makefile to compile.

CPU, Memory, and Hard Drive Requirements
========================================

AFKMud is on the large side due to the sheer number of features included. You will need to take this into consideration first, before ever even thinking about where to host it. Count on somewhere around 30MB in drive space usage once the code is compiled. Preliminary results for 64bit systems suggests you'll need almost twice that. Don't ask us why because we don't know. It's probably one of those weird gcc things only GNU knows about.

RAM usage should expect to hover around 30MB on a moderately sized world. This is assuming approximately 9,000 rooms and 3 1000x1000 overland maps.

CPU usage should hover at or near 0.0% during idle, and around 0.5% during moderate load. We don't have any high stress usage figures to offer, but it shouldn't cause you any grief.

First Immortal
==============

A pfile named "Admin" is included, with password "admin". Use this to get your first immortal advanced. Then promptly delete him!

The Makefile
============

The Makefile has some suggestions on what to comment out if you get certain errors during your compile. If you encounter any, refer there to see if there's a fix for it. The usual 'make clean' command will do its usual thing, then proceed to recompile the code. Don't panic, this is normal. If all you want to do is remove the *.o files, use the 'make purge' option instead.

Stock Areas
===========

First of all: No. There are no stock areas included, other than our rendition of the Astral Plane. The purpose of releasing AFKMud was not to provide people with cookie-cutter ready made worlds. It was to provide people the tools to create their own unique worlds for people to enjoy. Sure, this means we don't have 500 muds using the code, but those who do use it end up creating some great stuff with it.

Not satisfied with that? All is not lost. Though the AFKMud area file format has been significantly altered from Smaug, we included a conversion routine to allow loading of stock Smaug material. It's not perfect, but if you want your ready made stuff, you've got to work for it. This can be accomplished one of two ways:

1. Leave your areas and area.lst file alone and try to boot the mud with them as is. Your logs will look like a train wreck, but if you're lucky they'll load and you can then execute a "fold all" command once you've logged in. This would be advisable ASAP after an event of this magnitude. If any of the zones crash the mud, you'll need to take them out of the area.lst file and hold them over for method 2 below. 

2. If you come across a stock zone that won't convert and you need to try it again later, or if you decide to load one you've downloaded at some point, drop it into your area directory and type "areaconvert filename.are". If it fails to load, your mud has probably crashed. Examine the logs, core dumps, etc to find out why, correct it, and try again.

This functionality allows you to load Smaug 1.02a, 1.4a, SmaugWiz, ResortMUD, CalareyMUD, Chronicles, Dark Rifts, DOTDII, and SWR areas. Other formats are considered non-standard and will not be supported.

Keep in mind that you may also need to do some fine tuning once the area is converted - subtle differences in each version may produce unpredictable stats on the areas.

Adding Commands, Skills, and spec_funs
======================================

When you find yourself wanting to add a new command to the code, either from a snippet you've downloaded and decided to use, or from something you've written yourself, if you've been working with Smaug for long you know all about the tables.c file and the places you need to insert things to make it work.

With AFKMud, you no longer need to worry about that. The code used the dlsym functionality to handle the required lookups. It isn't even necessary to have DECLARE_DO_FUN statements anywhere. Just insert, compile, and create the command online. It's that easy now.

In the unlikely event your system does not have the right library support, you'll need to have that fixed by your sysadmin. Most linux installs have this natively.

Overland, and libgd-devel
=========================

AFKMud comes with a wilderness ANSI map system we've dubbed "The overland". Basically it is an ANSI color representation of the surrounding terrain around your character when he's out in the wilderness, away from normal areas. This is meant to be used as a filler for what are ordinarily boring connector zones, like large forests, hills, mountains, etc. It lets your builders concentrate more on the interesting places like castles, cities, troll dens, etc. The maps are stored statically in memory after being loaded from png graphic files. Because of this, you will need to be sure your host has the GD development library installed. The compiler will alert you to this in the form of errors trying to compile overland.cpp if the library is not installed.

Multiple Port Support
=====================

The code now has ways to check which port the mud is running on. This allows for things to behave differently based on which port the game is running on. This requires that some defines be set prior to startup or things will not behave properly. The defines are located in mud.h and are called MAINPORT, BUILDPORT, and CODEPORT. These values MUST be different from each other or the mud will not compile. Change them to fit your hosting situation. These should only be used for additional testing ports and will require an additional password for newly created characters to enter. This password can be set using cset, and ships with the default password of "blocked". Change this ASAP if you intend to use it.

This feature is disabled by default in the Makefile.

Shell Code
==========

The mud has an internal shell processor that will allow some limited functions to make life easier. It is not recommended that these commands be given to anyone but administrators. Also, some mud hosts may not allow this kind of access, check with them first if you have any doubts.

This feature will not be available if Multiport support has been disabled in the Makefile. Command entries will need to be created for this code if the support is activated. Help for this will not be given. Expect to be ignored if you ask, this is dangerous code in the wrong hands and we refuse to be responsible for any damage that could be done.

MSP - Mud Sound Protocol
========================

AFKMud includes support for the MUD Sound Protocol, a system which allows the mud to generate sound for players with clients that support it. For now this seems largely restricted to Zmud users.

Further information on the protocol can be found at: https://www.zuggsoft.com/zmud/msp.htm

MXP - Mud eXtension Protocol
============================

MXP is one of those things that sounds good on paper, looks good when the specs are written, but end up being badly implemented by the people who cooked it up. Still, we have basic support included. It probably doesn't work 100% right, and definitely won't work right in Zmud, ironically enough. But when the spec author disobeys his own standards, that happens.

This feature is being considered for removal. If you want to save it, there's a poll pinned to the forum. Go vote. If you want to guarantee that we don't remove it anyway, despite the poll, then send us a patch to fix it.
