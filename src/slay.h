/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2026 by Roger Libiez (Samson),                     *
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

#pragma once

/* Capability to create, edit and delete slaytypes added to original code by Samson 8-3-98 */

inline constexpr std::string_view SLAY_FILE = "../system/slay.dat";  /* Slay data file for online editing - Samson 8-3-98 */

/* Improved data structure for online slay editing - Samson 8-3-98 */
class slay_data
{
 public:
    slay_data( const slay_data & ) = delete;
    slay_data & operator=( const slay_data & ) = delete;
    slay_data(  );

   void set_owner( std::string_view name )
   {
      owner = name;
   }
   std::string get_owner(  )
   {
      return owner;
   }

   void set_type( std::string_view name )
   {
      type = name;
   }
   std::string get_type(  )
   {
      return type;
   }

   void set_cmsg( std::string_view msg )
   {
      cmsg = msg;
   }
   std::string get_cmsg(  )
   {
      return cmsg;
   }

   void set_vmsg( std::string_view msg )
   {
      vmsg = msg;
   }
   std::string get_vmsg(  )
   {
      return vmsg;
   }

   void set_rmsg( std::string_view msg )
   {
      rmsg = msg;
   }
   std::string get_rmsg(  )
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
   std::string owner;   // Name of the immortal who created it.
   std::string type;    // Name of the slay being used.
   std::string cmsg;    // Message seen by the player when the slay is used.
   std::string vmsg;    // Message the victim sees before they die.
   std::string rmsg;    // Message other people in the room see when the victim dies.
   int color;           // Default color setting for the displayed content of the slay.
};
