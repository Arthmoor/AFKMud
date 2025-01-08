/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2025 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
 *                                                                          *
 * External contributions from Remcon, Quixadhal, Zarius, and many others.  *
 *                                                                          *
 * Original SMAUG 1.8b written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, Edmond, Conran, and Nivek.                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *         Slay V2.0 - Online editable configurable slay options            *
 ****************************************************************************/

#ifndef __SLAY_H__
#define __SLAY_H__

/* Capability to create, edit and delete slaytypes added to original code by Samson 8-3-98 */

#define SLAY_FILE SYSTEM_DIR "slay.dat"  /* Slay data file for online editing - Samson 8-3-98 */

/* Improved data structure for online slay editing - Samson 8-3-98 */
class slay_data
{
 private:
   slay_data( const slay_data & s );
     slay_data & operator=( const slay_data & );

 public:
     slay_data(  );
    ~slay_data(  );

   void set_owner( const string & name )
   {
      owner = name;
   }
   string get_owner(  )
   {
      return owner;
   }

   void set_type( const string & name )
   {
      type = name;
   }
   string get_type(  )
   {
      return type;
   }

   void set_cmsg( const string & msg )
   {
      cmsg = msg;
   }
   string get_cmsg(  )
   {
      return cmsg;
   }

   void set_vmsg( const string & msg )
   {
      vmsg = msg;
   }
   string get_vmsg(  )
   {
      return vmsg;
   }

   void set_rmsg( const string & msg )
   {
      rmsg = msg;
   }
   string get_rmsg(  )
   {
      return rmsg;
   }

   void set_color( int clr )
   {
      color = clr;
   }
   int get_color(  )
   {
      return color;
   }

 private:
   string owner;
   string type;
   string cmsg;
   string vmsg;
   string rmsg;
   int color;
};
#endif
