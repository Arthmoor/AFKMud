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
 * Original SMAUG 1.8b written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, Edmond, Conran, and Nivek.                         *
 *                                                                          *
 * Original MERC 2.1 code by Hatchet, Furey, and Kahn.                      *
 *                                                                          *
 * Original DikuMUD code by: Hans Staerfeldt, Katja Nyboe, Tom Madsen,      *
 * Michael Seifert, and Sebastian Hammer.                                   *
 ****************************************************************************
 *                             Map reading module                           *
 ****************************************************************************/

#include "mud.h"
#include "boards.h"
#include "objindex.h"
#include "roomindex.h"

struct map_data   /* contains per-room data */
{
   int vnum;   /* which map this room belongs to */
   int x;   /* horizontal coordinate */
   int y;   /* vertical coordinate */
   char entry; /* code that shows up on map */
};

struct map_index
{
   map_index *next;
   int vnum;   /* vnum of the map */
   int map_of_vnums[49][81];  /* room vnums aranged as a map */
};

map_index *get_map_index( int vnum );
void init_maps( void );

/* Useful Externals */
extern int top_exit;

/* Local function prototypes */
map_index *make_new_map_index( int );
void map_to_rooms( char_data *, map_index * );
void map_stats( char_data *, int *, int *, int * );
int num_rooms_avail( char_data * );

int number_to_room_num( int );
int exit_lookup( int, int );
void draw_map( char_data *, room_index *, int, int );
char *you_are_here( int, int, const string & );

/* Local Variables & Structs */
char text_rmap[MSL];
map_index *first_map;   /* should be global */

struct map_stuff
{
   int vnum;
   int proto_vnum;
   int exits;
   int index;
   char code;
};

/* Lets make it check the map and make sure it uses [ ] instead of [] */
char *check_map( char *str )
{
   static char newstr[MSL];
   int i, j;

   for( i = j = 0; str[i] != '\0'; ++i )
   {
      if( str[i - 1] == '[' && str[i] == ']' )
      {
         newstr[j++] = ' ';
         newstr[j++] = str[i];
      }
      else
         newstr[j++] = str[i];
   }
   newstr[j] = '\0';
   return newstr;
}

/* Be careful not to give
 * this an existing map_index
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
   mindex->next = first_map;
   first_map = mindex;
   return mindex;
}

char count_lines( char *txt )
{
   int i;
   char *c, buf[MSL];

   if( !txt )
      return '0';
   i = 1;
   for( c = txt; *c != '\0'; ++c )
      if( *c == '\n' )
         ++i;
   if( i > 9 )
      return '+';
   snprintf( buf, MSL, "%d", i );
   return ( buf[0] );
}

map_index *get_map_index( int vnum )
{
   map_index *map;

   for( map = first_map; map; map = map->next )
   {
      if( map->vnum == vnum )
         return map;
   }
   return nullptr;
}

void map_stats( char_data * ch, int *rooms, int *rows, int *cols )
{
   int row, col, n;
   int leftmost, rightmost;
   char *l, c;

   if( !ch->pcdata->pnote )
   {
      bug( "%s: ch->pcdata->pnote == nullptr!", __func__ );
      return;
   }
   n = 0;
   row = col = 0;
   leftmost = rightmost = 0;
   l = ch->pcdata->pnote->text;
   do
   {
      c = l[0];
      switch ( c )
      {
         default:
         case '\n':
            break;

         case '\r':
            col = 0;
            ++row;
            break;

         case ' ':
            ++col;
            break;
      }

      if( c == '[' )
      {
         /*
          * Increase col 
          */
         ++col;
         /*
          * This is a room since in [ ] 
          */
         ++n;
         /*
          * This will later handle a letter etc 
          */
         ++col;
         /*
          * Make sure it has a closeing ] 
          */
         if( c == ']' )
            ++col;
         if( col < leftmost )
            leftmost = col;
         if( col > rightmost )
            rightmost = col;
      }
      if( ( c == ' ' || c == '-' || c == '|' || c == '=' || c == '\\' || c == '/' || c == '^' || c == ':' ) )
         ++col;
      ++l;
   }
   while( c != '\0' );
   *cols = ( rightmost - leftmost );
   *rows = row;   /* [sic.] */
   *rooms = n;
}

CMDF( do_mapout )
{
   string arg;
   obj_data *map_obj;   /* an obj made with map as an ed */
   obj_index *map_obj_index;  /*    obj_index for previous     */
   extra_descr_data *ed;   /*    the ed for it to go in     */
   int rooms, rows, cols, avail_rooms;

   if( !ch )
   {
      bug( "%s: null ch", __func__ );
      return;
   }
   if( ch->isnpc(  ) )
   {
      ch->print( "Not in mobs.\r\n" );
      return;
   }
   if( !ch->desc )
   {
      bug( "%s: no descriptor", __func__ );
      return;
   }
   switch ( ch->substate )
   {
      default:
         break;
      case SUB_WRITING_NOTE:
         if( ch->pcdata->dest_buf != ch->pcdata->pnote )
            bug( "%s: sub_writing_map: ch->pcdata->dest_buf != ch->pcdata->pnote", __func__ );
         STRFREE( ch->pcdata->pnote->text );
         ch->pcdata->pnote->text = ch->copy_buffer( true );
         ch->stop_editing(  );
         return;
   }

   ch->set_color( AT_NOTE );
   argument = one_argument( argument, arg );
   smash_tilde( argument );

   if( !str_cmp( arg, "stat" ) )
   {
      if( !ch->pcdata->pnote )
      {
         ch->print( "You have no map in progress.\r\n" );
         return;
      }
      map_stats( ch, &rooms, &rows, &cols );
      ch->printf( "Map represents %d rooms, %d rows, and %d columns\r\n", rooms, rows, cols );
      avail_rooms = num_rooms_avail( ch );
      ch->printf( "You currently have %d unused rooms.\r\n", avail_rooms );
      act( AT_ACTION, "$n glances at an etherial map.", ch, nullptr, nullptr, TO_ROOM );
      return;
   }

   if( !str_cmp( arg, "write" ) )
   {
      ch->note_attach(  );
      ch->substate = SUB_WRITING_NOTE;
      ch->pcdata->dest_buf = ch->pcdata->pnote;
      ch->start_editing( ch->pcdata->pnote->text );
      return;
   }

   if( !str_cmp( arg, "clear" ) )
   {
      if( !ch->pcdata->pnote )
      {
         ch->print( "You have no map in progress\r\n" );
         return;
      }
      STRFREE( ch->pcdata->pnote->text );
      STRFREE( ch->pcdata->pnote->subject );
      STRFREE( ch->pcdata->pnote->to_list );
      STRFREE( ch->pcdata->pnote->sender );
      deleteptr( ch->pcdata->pnote );
      ch->pcdata->pnote = nullptr;
      ch->print( "Map cleared.\r\n" );
      return;
   }

   if( !str_cmp( arg, "show" ) )
   {
      if( !ch->pcdata->pnote )
      {
         ch->print( "You have no map in progress.\r\n" );
         return;
      }
      ch->print( ch->pcdata->pnote->text );
      do_mapout( ch, "stat" );
      return;
   }

   if( !str_cmp( arg, "create" ) )
   {
      if( !ch->pcdata->pnote )
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
            log_printf( "create_object: %s:%s, line %d.", __FILE__, __func__, __LINE__ );
         ed = set_extra_descr( map_obj, "runes map scrawls" );
         ed->desc = ch->pcdata->pnote->text;
         map_obj->to_char( ch );
      }
      else
      {
         ch->print( "Couldn't give you a map object.\r\n" );
         return;
      }

      do_mapout( ch, "clear" );
      ch->print( "Ok.\r\n" );
      return;
   }
   ch->print( "mapout write: create a map in edit buffer.\r\n" );
   ch->print( "mapout stat: get information about a written, but not yet created map.\r\n" );
   ch->print( "mapout clear: clear a written, but not yet created map.\r\n" );
   ch->print( "mapout show: show a written, but not yet created map.\r\n" );
   ch->print( "mapout create: turn a written map into rooms in your assigned room vnum range.\r\n" );
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
            bug( "%s: make_room failed", __func__ );
            return -1;
         }
         /*
          * Clones current room  (quietly) 
          */
         location->area = ch->pcdata->area;
         location->name = STRALLOC( "Newly Created Room" );
         location->roomdesc = STRALLOC( "Newly Created Room\r\n" );
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

int num_rooms_avail( char_data * ch )
{
   int i, n;

   n = 0;
   for( i = ch->pcdata->low_vnum; i <= ch->pcdata->hi_vnum; ++i )
      if( !get_room_index( i ) )
         ++n;
   return n;
}

/* This function takes the character string in ch->pcdata->pnote and
 *  creates rooms laid out in the appropriate configuration.
 */
void map_to_rooms( char_data * ch, map_index * m_index )
{
   struct map_stuff rmap[49][78];   /* size of edit buffer */
   char *newmap;
   int row, col, i, n, x, y, tvnum, proto_vnum = 0;
   int newx, newy;
   char *l, c;
   room_index *newrm;
   map_index *map_index = nullptr, *tmp;
   exit_data *xit;   /* these are for exits */
   bool getroomnext = false;

   if( !ch->pcdata->pnote )
   {
      bug( "%s: ch->pcdata->pnote == nullptr!", __func__ );
      return;
   }

   /*
    * Make sure format is right 
    */
   newmap = check_map( ch->pcdata->pnote->text );
   STRFREE( ch->pcdata->pnote->text );
   ch->pcdata->pnote->text = STRALLOC( newmap );

   n = 0;
   row = col = 0;

   /*
    * Check to make sure map_index exists.  
    * If not, then make a new one.
    */
   if( !m_index )
   {  /* Make a new vnum */
      for( i = ch->pcdata->low_vnum; i <= ch->pcdata->hi_vnum; ++i )
      {
         if( ( tmp = get_map_index( i ) ) == nullptr )
         {
            map_index = make_new_map_index( i );
            break;
         }
      }
   }
   else
   {
      map_index = m_index;
   }

   /*
    *  
    */
   if( !map_index )
   {
      ch->print( "Couldn't find or make a map_index for you!\r\n" );
      bug( "%s: Couldn't find or make a map_index", __func__ );
      /*
       * do something. return failed or somesuch 
       */
      return;
   }

   for( x = 0; x < 49; ++x )
   {
      for( y = 0; y < 78; ++y )
      {
         rmap[x][y].vnum = 0;
         rmap[x][y].proto_vnum = 0;
         rmap[x][y].exits = 0;
         rmap[x][y].index = 0;
      }
   }

   l = ch->pcdata->pnote->text;
   do
   {
      c = l[0];

      switch ( c )
      {
         default:
         case '\n':
            break;

         case '\r':
            col = 0;
            ++row;
            break;
      }

      if( c != ' ' && c != '-' && c != '|' && c != '=' && c != '\\' && c != '/' && c != '^' && c != ':' && c != '[' && c != ']' && c != '^' && !getroomnext )
      {
         ++l;
         continue;
      }

      if( getroomnext )
      {
         ++n;
         /*
          * Actual room info 
          */
         rmap[row][col].vnum = add_new_room_to_map( ch, c );
         map_index->map_of_vnums[row][col] = rmap[row][col].vnum;
         rmap[row][col].proto_vnum = proto_vnum;
         getroomnext = false;
      }
      else
      {
         map_index->map_of_vnums[row][col] = 0;
         rmap[row][col].vnum = 0;
         rmap[row][col].exits = 0;
      }
      rmap[row][col].code = c;
      /*
       * Handle rooms 
       */
      if( c == '[' )
         getroomnext = true;
      ++col;
      ++l;
   }
   while( c != '\0' );

   for( y = 0; y < ( row + 1 ); ++y )
   {  /* rows */
      for( x = 0; x < 78; ++x )
      {  /* cols (78, i think) */

         if( rmap[y][x].vnum == 0 )
            continue;

         newrm = get_room_index( rmap[y][x].vnum );
         /*
          * Continue if no newrm 
          */
         if( !newrm )
            continue;

         /*
          * Check up 
          */
         if( y > 1 )
         {
            newx = x;
            newy = y;
            --newy;
            while( newy >= 0 && ( rmap[newy][x].code == '^' ) )
               --newy;

            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[newy][x].vnum )
               break;
            if( ( tvnum = rmap[newy][x].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_UP );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               xit->flags.reset(  );
            }
         }

         /*
          * Check down 
          */
         if( y < 48 )
         {
            newx = x;
            newy = y;
            ++newy;
            while( newy <= 48 && ( rmap[newy][x].code == '^' ) )
               ++newy;
            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[newy][x].vnum )
               break;
            if( ( tvnum = rmap[newy][x].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_DOWN );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               xit->flags.reset(  );
            }
         }

         /*
          * Check north 
          */
         if( y > 1 )
         {
            newx = x;
            newy = y;
            --newy;
            while( newy >= 0 && ( rmap[newy][x].code == '|' || rmap[newy][x].code == ':' || rmap[newy][x].code == '=' ) )
               --newy;
            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[newy][x].vnum )
               break;
            if( ( tvnum = rmap[newy][x].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_NORTH );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               if( rmap[newy + 1][x].code == ':' || rmap[newy + 1][x].code == '=' )
               {
                  SET_EXIT_FLAG( xit, EX_ISDOOR );
                  SET_EXIT_FLAG( xit, EX_CLOSED );
               }
               else
                  xit->flags.reset(  );
            }
         }

         /*
          * Check south 
          */
         if( y < 48 )
         {
            newx = x;
            newy = y;
            ++newy;
            while( newy <= 48 && ( rmap[newy][x].code == '|' || rmap[newy][x].code == ':' || rmap[newy][x].code == '=' ) )
               ++newy;
            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[newy][x].vnum )
               break;
            if( ( tvnum = rmap[newy][x].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_SOUTH );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               if( rmap[newy - 1][x].code == ':' || rmap[newy - 1][x].code == '=' )
               {
                  SET_EXIT_FLAG( xit, EX_ISDOOR );
                  SET_EXIT_FLAG( xit, EX_CLOSED );
               }
               else
                  xit->flags.reset(  );
            }
         }

         /*
          * Check east 
          */
         if( x < 79 )
         {
            newx = x;
            newy = y;
            ++newx;
            while( newx <= 79 && ( rmap[y][newx].code == '-' || rmap[y][newx].code == ':' || rmap[y][newx].code == '='
                                   || rmap[y][newx].code == '[' || rmap[y][newx].code == ']' ) )
               ++newx;
            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[y][newx].vnum )
               break;
            if( ( tvnum = rmap[y][newx].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_EAST );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               if( rmap[y][newx - 2].code == ':' || rmap[y][newx - 2].code == '=' )
               {
                  SET_EXIT_FLAG( xit, EX_ISDOOR );
                  SET_EXIT_FLAG( xit, EX_CLOSED );
               }
               else
                  xit->flags.reset(  );
            }
         }

         /*
          * Check west 
          */
         if( x > 1 )
         {
            newx = x;
            newy = y;
            --newx;
            while( newx >= 0 && ( rmap[y][newx].code == '-' || rmap[y][newx].code == ':' || rmap[y][newx].code == '='
                                  || rmap[y][newx].code == '[' || rmap[y][newx].code == ']' ) )
               --newx;

            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[y][newx].vnum )
               break;
            if( ( tvnum = rmap[y][newx].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_WEST );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               if( rmap[y][newx + 2].code == ':' || rmap[y][newx + 2].code == '=' )
               {
                  SET_EXIT_FLAG( xit, EX_ISDOOR );
                  SET_EXIT_FLAG( xit, EX_CLOSED );
               }
               else
                  xit->flags.reset(  );
            }
         }

         /*
          * Check southeast 
          */
         if( y < 48 && x < 79 )
         {
            newx = x;
            newy = y;
            newx += 2;
            ++newy;
            while( newx <= 79 && newy <= 48 && ( rmap[newy][newx].code == '\\' || rmap[newy][newx].code == ':' || rmap[newy][newx].code == '=' ) )
            {
               ++newx;
               ++newy;
            }
            if( rmap[newy][newx].code == '[' )
               ++newx;
            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[newy][newx].vnum )
               break;
            if( ( tvnum = rmap[newy][newx].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_SOUTHEAST );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               xit->flags.reset(  );
            }
         }

         /*
          * Check northeast 
          */
         if( y > 1 && x < 79 )
         {
            newx = x;
            newy = y;
            newx += 2;
            --newy;
            while( newx >= 0 && newy <= 48 && ( rmap[newy][newx].code == '/' || rmap[newy][newx].code == ':' || rmap[newy][newx].code == '=' ) )
            {
               ++newx;
               --newy;
            }
            if( rmap[newy][newx].code == '[' )
               ++newx;

            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[newy][newx].vnum )
               break;
            if( ( tvnum = rmap[newy][newx].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_NORTHEAST );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               xit->flags.reset(  );
            }
         }

         /*
          * Check northwest 
          */
         if( y > 1 && x > 1 )
         {
            newx = x;
            newy = y;
            newx -= 2;
            --newy;
            while( newx >= 0 && newy >= 0 && ( rmap[newy][newx].code == '\\' || rmap[newy][newx].code == ':' || rmap[newy][newx].code == '=' ) )
            {
               --newx;
               --newy;
            }
            if( rmap[newy][newx].code == ']' )
               --newx;

            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[newy][newx].vnum )
               break;
            if( ( tvnum = rmap[newy][newx].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_NORTHWEST );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               xit->flags.reset(  );
            }
         }

         /*
          * Check southwest 
          */
         if( y < 48 && x > 1 )
         {
            newx = x;
            newy = y;
            newx -= 2;
            ++newy;
            while( newx >= 0 && newy <= 48 && ( rmap[newy][newx].code == '/' || rmap[newy][newx].code == ':' || rmap[newy][newx].code == '=' ) )
            {
               --newx;
               ++newy;
            }
            if( rmap[newy][newx].code == ']' )
               --newx;

            /*
             * dont link it to itself 
             */
            if( rmap[y][x].vnum == rmap[newy][newx].vnum )
               break;
            if( ( tvnum = rmap[newy][newx].vnum ) != 0 )
            {
               xit = get_room_index( tvnum )->make_exit( newrm, DIR_SOUTHWEST );
               xit->keyword = nullptr;
               xit->exitdesc = nullptr;
               xit->key = -1;
               xit->flags.reset(  );
            }
         }
      }
   }
}
