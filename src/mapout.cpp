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
 *                          Map Reading OLC Module                          *
 ****************************************************************************/

/*
 * This code takes a note written by a builder that parses the following type of layout into a linked set of rooms:
 *
 *                 [ ]         [ ]
 *                  =           =
 * [ ]-[ ]     [ ]-[ ]     [ ]:[ ]:[ ]
 *  |   |           |         / =
 * [ ]-[ ]:[ ]-----[ ]-----[ ] [ ]
 *        /                   \
 * [ ]:[ ]:[ ]     [ ]     [ ]:[ ]:[ ]
 *      |           |           |
 * [ ]:[ ]:[ ] [ ]-[ ]-[ ] [ ]:[ ]:[ ]
 *      |         / = \         |
 * [ ]:[ ]:[ ] [ ] [ ] [ ] [ ]:[ ]:[ ]:[ ]
 *      |           |           |       |
 * [ ]:[ ] [ ] [ ] [ ]-[ ]-[ ]-[ ]:[ ] [ ]
 *        \ =   =   |   |   |   |
 * [ ]-[ ]:[ ]-[ ]-[ ] [ ]-[ ] [ ]-[ ]
 *        /     |     \       /     |
 *     [ ] [ ]:[ ]:[ ] [ ]-[ ] [ ] [ ]-[ ]-[ ]
 *              =   |   |     = =
 *         [ ]-[ ]-[ ] [ ]:[ ]:[ ]
 *
 * The above example is taken from one of Alsherok's maps.
 *
 * Some limitations:
 *
 * The note (and therefore the room grid) is restricted to 48 x 80. This is the size of the MUD's internal edit buffer. If you need a larger set of dimensions, you'll need to do it the old fashioned way.
 * Room generation must stay within the builder's assigned VNUM range. This should already be guardrailed by the add_new_room_to_map() function.
 * Exits must be linkable as normal room links. It cannot deal with going through an exit that warps the player, or uses a portal.
 * It cannot link a room into another area. This is enforced in the link_map_exit() function. Allowing this would make a pretty big mess of things.
 */

#include "mud.h"
#include "area.h"
#include "boards.h"   // Not obvious, but this is what provides the note buffer functionality.
#include "objindex.h"
#include "roomindex.h"

struct map_data   /* contains per-room data */
{
   int vnum;   // Which map this room belongs to
   int x;      // Horizontal coordinate.
   int y;      // Vertical coordinate/
   char entry; // Code that shows up on map.
};

struct map_index
{
   int vnum = 0;              // Vnum of the map.
   int map_of_vnums[49][81];  // Room vnums arranged as a map. This matches the dimensions of a note that's written in the game's internal editor.
};

struct map_stuff
{
   int vnum;
   int proto_vnum;
   int exits;
   int index;
   char code;
};

std::list<map_index *> map_list;

/* Lets make it check the map and make sure it uses [ ] instead of [] */
std::string check_map( std::string_view str )
{
   std::string result;
   result.reserve( str.size() + 10 );

   for( size_t i = 0; i < str.size(); ++i )
   {
      if( i > 0 && str[i - 1] == '[' && str[i] == ']' )
      {
         result.push_back( ' ' );
      }
      result.push_back( str[i] );
   }
   return result;
}

/*
 * Be careful not to give this an existing map_index.
 */
map_index *make_new_map_index( int vnum )
{
   map_index *mindex;
   int i, j;

   mindex = new map_index;
   mindex->vnum = vnum;
   for( i = 0; i < 49; ++i )
   {
      for( j = 0; j < 78; ++j )
         mindex->map_of_vnums[i][j] = -1;
   }
   map_list.push_back( mindex );
   return mindex;
}

char count_lines( char *txt )
{
   int i;
   char *c;
   std::string buf;

   if( !txt )
      return '0';

   i = 1;
   for( c = txt; *c != '\0'; ++c )
      if( *c == '\n' )
         ++i;

   if( i > 9 )
      return '+';

   buf = std::format( "{}", i );
   return ( buf[0] );
}

map_index *get_map_index( int vnum )
{
   for( auto* map : map_list )
   {
      if( map->vnum == vnum )
         return map;
   }
   return nullptr;
}

void map_stats( char_data* ch, int* rooms, int* rows, int* cols )
{
   if( ch->pcdata->map_buffer.empty() )
   {
      bug("{}: ch->pcdata->map_buffer is empty!", __func__);
      return;
   }

   const std::string & text = ch->pcdata->map_buffer;

   int row = 0;
   int col = 0;
   int n = 0;
   int leftmost = 0;
   int rightmost = 0;

   for( size_t i = 0; i < text.size(); ++i )
   {
      char c = text[i];

      switch( c )
      {
         case '\r':
            col = 0;
            ++row;
            continue;

         default:
         case '\n':
            continue;

         case ' ':
            ++col;
            break;
      }

      if( c == '[' )
      {
         col += 2;
         ++n;

         if( i + 1 < text.size() && text[i + 1] == ']' )
         {
            ++col;
         }

         leftmost = umin( leftmost, col );
         rightmost = umax( rightmost, col );
      }
      else if( c == ' ' || c == '-' || c == '|' || c == '=' || c == '\\' || c == '/' || c == '^' || c == ':' )
      {
         ++col;
      }
   }

   *cols = ( rightmost - leftmost );
   *rows = row;
   *rooms = n;
}

int add_new_room_to_map( char_data * ch, char code )
{
   int i;
   room_index *location;

   /*
    * Get an unused room to copy to
    */
   for( i = ch->pcdata->low_vnum; i <= ch->pcdata->hi_vnum; ++i )
   {
      if( get_room_index( i ) == nullptr )
      {
         if( !( location = make_room( i, ch->pcdata->area ) ) )
         {
            bug( "{}: make_room failed", __func__ );
            return -1;
         }
         /*
          * Clones current room  (quietly)
          */
         location->area = ch->pcdata->area;
         location->name = "Newly Created Room";
         location->roomdesc = "Newly Created Room\r\n";
         location->flags.reset(  );
         location->flags.set( ROOM_PROTOTYPE );
         location->light = 0;
         if( code == 'I' )
            location->sector_type = SECT_INDOORS;
         else if( code == 'C' )
            location->sector_type = SECT_CITY;
         else if( code == 'f' )
            location->sector_type = SECT_FIELD;
         else if( code == 'F' )
            location->sector_type = SECT_FOREST;
         else if( code == 'H' )
            location->sector_type = SECT_HILLS;
         else if( code == 'M' )
            location->sector_type = SECT_MOUNTAIN;
         else if( code == 's' )
            location->sector_type = SECT_WATER_SWIM;
         else if( code == 'S' )
            location->sector_type = SECT_WATER_NOSWIM;
         else if( code == 'A' )
            location->sector_type = SECT_AIR;
         else if( code == 'U' )
            location->sector_type = SECT_UNDERWATER;
         else if( code == 'D' )
            location->sector_type = SECT_DESERT;
         else if( code == 'R' )
            location->sector_type = SECT_RIVER;
         else if( code == 'O' )
            location->sector_type = SECT_OCEANFLOOR;
         else if( code == 'u' )
            location->sector_type = SECT_UNDERGROUND;
         else if( code == 'J' )
            location->sector_type = SECT_JUNGLE;
         else if( code == 'W' )
            location->sector_type = SECT_SWAMP;
         else if( code == 'T' )
            location->sector_type = SECT_TUNDRA;
         else if( code == 'i' )
            location->sector_type = SECT_ICE;
         else if( code == 'o' )
            location->sector_type = SECT_OCEAN;
         else if( code == 'L' )
            location->sector_type = SECT_LAVA;
         else if( code == 't' )
            location->sector_type = SECT_TRAIL;
         else if( code == 'G' )
            location->sector_type = SECT_GRASSLAND;
         else if( code == 'c' )
            location->sector_type = SECT_SCRUB;
         else
            location->sector_type = SECT_INDOORS;
         return i;
      }
   }
   ch->print( "No available room in your vnums!" );
   return -1;
}

void link_map_exit( char_data * ch, room_index * from, room_index * to, int direction, bool is_door )
{
   // Don't link to itself.
   if( !from || !to || from == to )
      return;

   if( from->area != to->area )
   {
      ch->print_fmt( "Room {}, for area {}, is trying to link an exit in another area: {}. Skipping this link. You will need to add this manually.\r\n", from->vnum, from->area->name, to->area->name );
      return;
   }

   auto* xit = from->make_exit( to, direction );
   xit->key = -1;

   if( is_door )
   {
      SET_EXIT_FLAG( xit, EX_ISDOOR );
      SET_EXIT_FLAG( xit, EX_CLOSED );
   }
   else
   {
      xit->flags.reset();
   }
}

struct DirRule
{
   int dir;
   int dy, dx;
   int start_dy, start_dx;
   std::string symbols;

   // Checks if the immediate connector is a door
   bool is_door( const map_stuff & m ) const { return m.code == ':' || m.code == '='; }
};

const std::vector<DirRule> dir_rules = {
   { DIR_NORTH,     -1,  0, -1,  0, "|:=" },
   { DIR_SOUTH,      1,  0,  1,  0, "|:=" },
   { DIR_EAST,       0,  1,  0,  1, "-:=" },
   { DIR_WEST,       0, -1,  0, -1, "-:=" },
   { DIR_UP,        -1,  0, -1,  0, "^" },
   { DIR_DOWN,       1,  0,  1,  0, "^" },
   { DIR_NORTHEAST, -1,  1, -1,  2, "/:=" },
   { DIR_NORTHWEST, -1, -1, -1, -2, "\\:=" },
   { DIR_SOUTHEAST,  1,  1,  1,  2, "\\:=" },
   { DIR_SOUTHWEST,  1, -1,  1, -2, "/:=" }
};

/*
 * This function takes the character string in ch->pcdata->map_buffer and
 *  creates rooms laid out in the appropriate configuration.
 */
void map_to_rooms( char_data* ch, map_index* m_index )
{
   map_index* m_ptr = nullptr;
   map_index* tmp;
   static map_stuff rmap[49][78];

   if( ch->pcdata->map_buffer.empty() )
   {
      bug( "{}: ch->pcdata->map_buffer is empty!", __func__ );
      return;
   }

   ch->pcdata->map_buffer = check_map( ch->pcdata->map_buffer );
   const std::string & text = ch->pcdata->map_buffer;

   if( !m_index )
   {
      for( int i = ch->pcdata->low_vnum; i <= ch->pcdata->hi_vnum; ++i )
      {
         if( !( tmp = get_map_index(i) ) )
         {
            m_ptr = make_new_map_index(i);
            break;
         }
      }
   }
   else
   {
      m_ptr = m_index;
   }

   if( !m_ptr )
   {
      ch->print( "Couldn't find or make a map_index for you!\r\n" );
      return;
   }

   for( int x = 0; x < 49; ++x )
   {
      for( int y = 0; y < 78; ++y )
      {
         rmap[x][y].vnum = 0;
         rmap[x][y].proto_vnum = 0;
         rmap[x][y].exits = 0;
         rmap[x][y].index = 0;
      }
   }

   int row = 0, col = 0;
   bool getroomnext = false;

   for( char c : text )
   {
      if( c == '\r' )
      {
         col = 0;
         ++row;
         continue;
      }

      if( c == '\n' )
      {
         ++row;
         col = 0;
         continue;
      }

      bool is_symbol = ( c == ' ' || c == '-' || c == '|' || c == '=' || c == '\\' || c == '/' || c == '^' || c == ':' || c == '[' || c == ']' );

      if( !is_symbol && !getroomnext )
         continue;

      if( getroomnext )
      {
         rmap[row][col].vnum = add_new_room_to_map( ch, c );
         m_ptr->map_of_vnums[row][col] = rmap[row][col].vnum;
         getroomnext = false;
      }
      else
      {
         m_ptr->map_of_vnums[row][col] = 0;
         rmap[row][col].vnum = 0;
      }

      rmap[row][col].code = c;
      if( c == '[' )
         getroomnext = true;
      col++;
   }

   for( int y = 0; y <= row; ++y )
   {
      for( int x = 0; x < 78; ++x )
      {
         if( rmap[y][x].vnum == 0 )
            continue;

         room_index* src = get_room_index( rmap[y][x].vnum );
         if( !src )
            continue;

         for( const auto& rule : dir_rules )
         {
            int ny = y + rule.start_dy;
            int nx = x + rule.start_dx;

            while( ny >= 0 && ny < 49 && nx >= 0 && nx < 78 && rule.symbols.find( rmap[ny][nx].code ) != std::string::npos )
            {
               ny += rule.dy;
               nx += rule.dx;
            }

            // If we found a target room, link it.
            if( ny >= 0 && ny < 49 && nx >= 0 && nx < 78 && rmap[ny][nx].vnum != 0 )
            {
               // Don't link to itself.
               if( rmap[y][x].vnum == rmap[ny][nx].vnum )
                  continue;

               bool door = rule.is_door( rmap[y + rule.start_dy][x + rule.start_dx] );

               link_map_exit( ch, src, get_room_index(rmap[ny][nx].vnum), rule.dir, door );
            }
         }
      }
   }
}

int num_rooms_avail( char_data * ch )
{
   int i, n;

   n = 0;
   for( i = ch->pcdata->low_vnum; i <= ch->pcdata->hi_vnum; ++i )
      if( !get_room_index( i ) )
         ++n;
   return n;
}

void restore_map_buffer( char_data * ch )
{
   ch->note_attach( NOTE_OLCMAP );
   ch->pcdata->pnote->text = ch->pcdata->map_buffer;
   ch->print( "You have a pending map from your last session. Use 'mapout show' to view it.\r\n" );
}

CMDF( do_mapout )
{
   std::string arg;
   obj_data *map_obj;   /* an obj made with map as an ed */
   obj_index *map_obj_index;  /*    obj_index for previous     */
   extra_descr_data *ed;   /*    the ed for it to go in     */
   int rooms, rows, cols, avail_rooms;

   if( !ch )
   {
      bug( "{}: null ch", __func__ );
      return;
   }
   if( ch->isnpc(  ) )
   {
      ch->print( "Not in mobs.\r\n" );
      return;
   }
   if( !ch->desc )
   {
      bug( "{}: no descriptor", __func__ );
      return;
   }
   if( !ch->pcdata->area )
   {
      ch->print( "You have no assigned area.\r\n" );
      return;
   }

   switch( ch->substate )
   {
      default:
         break;
      case SUB_WRITING_NOTE:
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "{}: sub_writing_map: ch->pcdata->dest_buf != ch->pcdata->pnote", __func__ );
         ch->pcdata->pnote->text = ch->copy_buffer( );
         ch->stop_editing(  );
         ch->pcdata->map_buffer = ch->pcdata->pnote->text; // Cause it would suck if the buffer got trashed because you started writing a note or something.
         return;
   }

   ch->set_color( AT_NOTE );
   argument = one_argument( argument, arg );
   smash_tilde( argument );

   if( !str_cmp( arg, "stat" ) )
   {
      if( ch->pcdata->map_buffer.empty() )
      {
         ch->print( "You have no map in progress.\r\n" );
         return;
      }
      map_stats( ch, &rooms, &rows, &cols );
      ch->print_fmt( "Map represents {} rooms, {} rows, and {} columns\r\n", rooms, rows, cols );
      avail_rooms = num_rooms_avail( ch );
      ch->print_fmt( "You currently have {} unused rooms.\r\n", avail_rooms );
      act( AT_ACTION, "$n glances at an ethereal map.", ch, nullptr, nullptr, TO_ROOM );
      return;
   }

   if( !str_cmp( arg, "write" ) )
   {
      ch->note_attach( NOTE_OLCMAP );
      ch->substate = SUB_WRITING_NOTE;
      ch->pcdata->dest_buf = ch->pcdata->pnote;
      if( !ch->pcdata->map_buffer.empty() )
         ch->pcdata->pnote->text = ch->pcdata->map_buffer;
      ch->start_editing( ch->pcdata->pnote->text );
      return;
   }

   if( !str_cmp( arg, "clear" ) )
   {
      if( ch->pcdata->map_buffer.empty() )
      {
         ch->print( "You have no map in progress\r\n" );
         return;
      }
      ch->pcdata->map_buffer.clear();
      ch->print( "Map buffer cleared.\r\n" );
      return;
   }

   if( !str_cmp( arg, "show" ) )
   {
      if( ch->pcdata->map_buffer.empty() )
      {
         ch->print( "You have no map in progress.\r\n" );
         return;
      }
      ch->print( ch->pcdata->map_buffer );
      do_mapout( ch, "stat" );
      return;
   }

   if( !str_cmp( arg, "create" ) )
   {
      if( ch->pcdata->map_buffer.empty() )
      {
         ch->print( "You have no map in progress.\r\n" );
         return;
      }
      map_stats( ch, &rooms, &rows, &cols );
      avail_rooms = num_rooms_avail( ch );

      /*
       * check for not enough rooms 
       */
      if( rooms > avail_rooms )
      {
         ch->print( "You don't have enough unused rooms allocated!\r\n" );
         return;
      }
      act( AT_ACTION, "$n warps the very dimensions of space!", ch, nullptr, nullptr, TO_ROOM );

      map_to_rooms( ch, nullptr );  /* this does the grunt work */

      map_obj_index = get_obj_index( OBJ_VNUM_MAPS );
      if( map_obj_index )
      {
         if( !( map_obj = map_obj_index->create_object( 1 ) ) )
            log_printf( "create_object: {}:{}, line {}.", __FILE__, __func__, __LINE__ );
         ed = set_extra_descr( map_obj, "runes map scrawls" );
         ed->desc = ch->pcdata->map_buffer;
         map_obj->to_char( ch );
      }
      else
      {
         ch->print( "Couldn't give you a map object.\r\n" );
         return;
      }

      ch->print( "Rooms generated.\r\n" );
      do_mapout( ch, "clear" );
      return;
   }
   ch->print( "mapout write:  Create a map in edit buffer.\r\n" );
   ch->print( "mapout stat:   Get information about a written, but not yet created map.\r\n" );
   ch->print( "mapout clear:  Clear a written, but not yet created map. Be careful with this.\r\n" );
   ch->print( "mapout show:   Show a written, but not yet created map.\r\n" );
   ch->print( "mapout create: Turn a written map into rooms in your assigned room vnum range.\r\n" );
}
