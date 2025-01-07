Adding a New Class
------------------

Last Modified by Samson, Nov. 26, 2014

1. In mudcfg.h, find MAX_CLASS. You will need to increase this if your
   MUD has filled all 20 playable class slots. If this is the case,
   it'll be a big undertaking since there are some NPC-only classes
   listed in the tables.

2. Add your new class to the class_types enum list in mudcfg.h.
   In stock code, this list ends with CLASS_PC19 as the last member.
   If all of the PC_* slots are gone, you'll need to do some major organizing
   to address this.

3. In const.cpp, find the npc_class table. Change the first empty slot to 
   the name of your new class. You should be changing something like "pc13".
   Pay attention to the comments in the code there, they are important.

4. At this point, your done with the code. Make clean, and recompile.
   Don't reboot yet though.

5. Using an existing class file, copy it's contents to a new file.

6. Set the class# in the file to the number that matches the slot you put it
   in in mudcfg.h. Zero is the first value. Stock code ends with 19.

7. At this point, you're ready to upload the class file.
   Don't reboot just yet.

8. In entry.are, you need to add your new class to the appropriate speech_prog
   lists for the races you are going to allow to use it. The speech_prog should
   look similar to:

#MUDPROG
Progtype  speech_prog~
Arglist   p mage~
Comlist   if level($n) == 1
mpmset 0.$n class mage
mptrlook 0.$n 120
mpat 0.$n mpforce 0.$n bounce
endif
~
#ENDPROG

   It should be tacked on to the end of the existing list.

9. After the reboot, use the setclass command to fine-tune the settings
   for the new class.

10. You should then be ready to do skill setup for this new class.
    Have fun, skill setting could well be a document all its own :)

That should be it. Your new class should be ready for use.
