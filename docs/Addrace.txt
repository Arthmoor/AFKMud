Adding a New Race, With Language
--------------------------------

Last Modified by Samson, Nov. 26, 2014

Skip the language portions if your new race doesn't need one.

1. In mudcfg.h, find MAX_RACE. You will need to increase this if your
   MUD has filled all 20 playable class slots. If this is the case,
   it'll be a big undertaking since there are some NPC-only classes
   listed in the tables.

2. In const.cpp, find the npc_race table. Change the first empty slot to 
   the name of your new race. You should be changing something like "r9".
   Stock AFKMud code only has one free slot - keep this in mind. You may
   simply want to change an existing race into a new one instead. Only
   the first 19 races in this table can be used for PCs unless you
   bumped up MAX_RACE and do a bunch of ugly surgery on the NPC race
   values.

3. In build.cpp, find:

   const char *lang_names[] = {

   In that, add LANG_<newrace> where <newrace> is the name of the
   race your adding. In the table right below it, add your race name
   to the list, just before the empty double quotes.

4. in mudcfg.h again, find LANG_UNKNOWN, you should be looking at a list
   of capitalized values. Add your race to the list, right before LANG_UNKNOWN.
   Then add it to the list of VALID_LANGS as well, after the last listed
   language.

5. Add your new race to the race_types enum list in mudcfg.h.
   In stock code, this list ends with RACE_19 as the last member.

6. At this point, your done with the code. Make clean, and recompile.
   Don't reboot yet though.

7. Using an existing race file, copy it's contents to a new file.

8. Set the race# in the file to the number that matches the slot you put it
   in in mudcfg.h. Zero is the first value. Stock code ends with 19.

9. Set the language line to the proper string value for that race.
   This should match the name you used when editing the lang_names
   array in build.cpp.

10. At this point, you're ready to upload the race file.
    Don't reboot just yet.

11. In entry.are, you need to add a speech_prog to the race rooms. There
    are two of them, one is used with the name auth system, one is used
    without. Your speech_prog should look similar to:

#MUDPROG
Progtype  speech_prog~
Arglist   p human~
Comlist   if level($n) == 1
mpmset 0.$n race human
mpat 0.$n mpapplyb 0.$n
mptrlook 0.$n 104
mpat 0.$n mpforce 0.$n bounce
endif
~
#ENDPROG

   It should be tacked on to the end of the existing list.

12. After the reboot, use the setrace command to fine-tune the settings
    for the new race.

13. Now you need to create the language skill. Using the sset command,
    execute the following:

    sset create skill (language)
    sset (lang) type tongue
    sset (lang) guild -1   (very important)
    sset (lang) code spell_null
    sset save skill table

    'language' being the name of the language, and 'lang' being the sn#
    of the new language. You can get that by doing an slookup on the
    language.

14. You'll then need to adjust the levels that each class gets to learn
    the language at. You can use the ssset command for this, but I find
    it to be much faster to do this kind of thing offline. Just add a line
    similar to this at the end of the skills list for each class:
 
    Skill 'trollese' 1 99

    1 is the level at which the class learns the language, 99 is the adept
    level.

That should be it. Your new race and language should be ready for use.
