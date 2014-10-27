/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2015 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
 *                                                                          *
 * External contributions from Remcon, Quixadhal, Zarius, and many others.  *
 *                                                                          *
 * Original SMAUG 1.4a written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, and Nivek.                                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                        Finger and Wizinfo Module                         *
 ****************************************************************************/

#ifndef __FINGER_H__
#define __FINGER_H__

class wizinfo_data
{
 private:
   wizinfo_data( const wizinfo_data & w );
     wizinfo_data & operator=( const wizinfo_data & );

 public:
     wizinfo_data(  );
    ~wizinfo_data(  );

   void set_name( const string & newname )
   {
      name = newname;
   }
   string get_name(  )
   {
      return name;
   }

   void set_email( const string & newmail )
   {
      email = newmail;
   }
   const string get_email(  )
   {
      return email;
   }

   void set_icq( int num )
   {
      icq = num;
   }
   int get_icq(  )
   {
      return icq;
   }

   void set_level( short num )
   {
      level = num;
   }
   short get_level(  )
   {
      return level;
   }

 private:
   string name;
   string email;
   int icq;
   short level;
};
#endif
