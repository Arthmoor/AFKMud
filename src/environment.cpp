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
 *                Overland Map Environmental Affects Code                   *
 *  Modified from code used in Altanos 9 (Shatai and Eizneckam) by Samson   *
 ****************************************************************************/

#include "mud.h"
#include "descriptor.h"
#include "objindex.h"
#include "overland.h"
#include "roomindex.h"

struct environment_data
{
   environment_data(  );
   /*
    * No destructor needed 
    */

   short direction;
   short mx;
   short my;
   short cmap;
   short type;
   short damage_per_shake;
   short radius;
   short time_left;
   short intensity;
};

list < environment_data * >envlist;

enum env_types
{
   ENV_QUAKE, ENV_FIRE, ENV_RAIN, ENV_THUNDER, ENV_TORNADO, ENV_MAX
};

const char *env_name[] = {
   "quake", "fire", "rainstorm", "thunderstorm", "tornado"
};

const char *env_distances[] = {
   "hundreds of miles away in the distance",
   "far off in the skyline",
   "many miles away at great distance",
   "far off many miles away",
   "tens of miles away in the distance",
   "far off in the distance",
   "several miles away",
   "off in the distance",
   "not far from here",
   "in the near vicinity",
   "in the immediate area"
};

environment_data::environment_data(  )
{
   init_memory( &direction, &intensity, sizeof( intensity ) );
}

void free_env( environment_data * env )
{
   envlist.remove( env );
   deleteptr( env );
}

void free_envs( void )
{
   list < environment_data * >::iterator en;

   for( en = envlist.begin(  ); en != envlist.end(  ); )
   {
      environment_data *env = *en;
      ++en;

      free_env( env );
   }
}

int get_env_type( const string & type )
{
   for( int x = 0; x < ENV_MAX; ++x )
      if( !str_cmp( type, env_name[x] ) )
         return x;
   return -1;
}

void environment_pulse_update( void )
{
   list < environment_data * >::iterator en;

   for( en = envlist.begin(  ); en != envlist.end(  ); )
   {
      environment_data *env = *en;
      ++en;

      switch ( env->type )
      {
         default:
            break;

         case ENV_FIRE:
         case ENV_RAIN:
         case ENV_THUNDER:
         case ENV_TORNADO:
            switch ( env->direction )
            {
               case DIR_NORTH:
                  --env->my;
                  break;
               case DIR_EAST:
                  ++env->mx;
                  break;
               case DIR_SOUTH:
                  ++env->my;
                  break;
               case DIR_WEST:
                  --env->mx;
                  break;
               case DIR_SOUTHWEST:
                  ++env->my;
                  --env->mx;
                  break;
               case DIR_NORTHWEST:
                  --env->my;
                  --env->mx;
                  break;
               case DIR_NORTHEAST:
                  --env->my;
                  ++env->mx;
                  break;
               case DIR_SOUTHEAST:
                  ++env->my;
                  ++env->mx;
                  break;
               default:
                  break;
            }
            break;
         case ENV_QUAKE:
            env->mx += 1;
            env->my += 1;
            break;
      }
      --env->time_left;
      if( env->time_left < 1 )
         free_env( env );
   }
}

void environment_actual_update( void )
{
   list < descriptor_data * >::iterator ds;

   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      list < environment_data * >::iterator ev;
      descriptor_data *d = *ds;
      int atx, aty;

      if( d->connected != CON_PLAYING )
         continue;

      char_data *ch = d->original ? d->original : d->character;

      if( !ch )
         continue;

      if( !ch->has_pcflag( PCFLAG_ONMAP ) )
         continue;

      atx = ch->mx;
      aty = ch->my;

      for( ev = envlist.begin(  ); ev != envlist.end(  ); ++ev )
      {
         environment_data *en = *ev;

         if( distance( atx, aty, en->mx, en->my ) > en->radius )
            continue;

         switch ( en->type )
         {
            default:
               break;

            case ENV_FIRE:
               if( number_range( 0, 3 ) == 2 )
               {
                  list < obj_data * >::iterator iobj;
                  obj_data *obj;
                  bool firefound = false;

                  for( iobj = ch->in_room->objects.begin(  ); iobj != ch->in_room->objects.end(  ); ++iobj )
                  {
                     obj_data *fire = *iobj;

                     if( fire->pIndexData->vnum != OBJ_VNUM_OVFIRE )
                        continue;

                     if( is_same_obj_map( ch, fire ) )
                     {
                        firefound = true;
                        break;
                     }
                  }

                  if( !firefound )
                  {
                     ch->print( "&RScorching flames erupt all around!\r\n" );
                     if( !( obj = get_obj_index( OBJ_VNUM_OVFIRE )->create_object( 1 ) ) )
                        log_printf( "create_object: %s:%s, line %d.", __FILE__, __FUNCTION__, __LINE__ );
                     else
                     {
                        obj->timer = en->time_left;
                        obj->to_room( ch->in_room, ch );
                     }
                  }
               }
               break;
            case ENV_QUAKE:
               ch->print( "&OThe rumble of an earthquake roars loudly!\r\n" );
               break;
            case ENV_RAIN:
               ch->print( "&cA gentle rain falls from the sky.\r\n" );
               break;
            case ENV_THUNDER:
               switch ( number_range( 0, 5 ) )
               {
                  case 0:
                     ch->print( "&YCracks of lightning hit the ground!\r\n" );
                     break;
                  case 1:
                     ch->print( "&YThe sky lights up brightly!\r\n" );
                     break;
                  case 2:
                     ch->print( "&YYou are almost blinded by lightning!\r\n" );
                     break;
                  case 3:
                     ch->print( "&YLightning crashes down in front of you!\r\n" );
                     break;
                  default:
                     ch->print( "&YLightning flashes in the sky!\r\n" );
                     break;
               }
               break;
            case ENV_TORNADO:
               ch->print( "&cThe roar of the tornado hurts your ears!\r\n" );
               ch->mx = ( atx + number_range( 5, 10 ) + en->radius / 2 );
               ch->my = ( aty + number_range( 5, 15 ) + en->radius / 2 );
               ch->print( "&RThe force of the tornado TOSSES you around!\r\n" );
               interpret( ch, "look" );
               break;
         }
      }
   }
}

void generate_random_environment( int type )
{
   environment_data *q;
   int schance = 0;

   schance = number_range( 1, 100 );

   q = new environment_data;
   envlist.push_back( q );
   q->type = type;
   q->mx = number_range( 0, MAX_X );
   q->my = number_range( 0, MAX_Y );
   q->cmap = number_range( MAP_ONE, MAP_MAX - 1 );

   switch ( type )
   {
      default:
      case ENV_FIRE:
         q->radius = number_range( 1, 10 );
         q->damage_per_shake = number_range( 5, 25 );
         break;

      case ENV_QUAKE:
         q->time_left = number_range( 10, 50 );
         q->radius = number_range( 50, 75 );
         q->damage_per_shake = number_range( 5, 25 );
         q->intensity = number_range( 1, 8 );
         break;

      case ENV_RAIN:
      case ENV_THUNDER:
         q->radius = number_range( 1, 25 );
         q->damage_per_shake = number_range( 5, 25 );
         break;

      case ENV_TORNADO:
         q->time_left = number_range( 10, 50 );
         q->radius = number_range( 1, 4 );
         q->damage_per_shake = number_range( 5, 25 );
         break;
   }
   if( type == ENV_QUAKE )
   {
      q->direction = 10;
      return;
   }

   if( schance < 15 )
      q->direction = DIR_NORTH;
   else if( schance < 30 )
      q->direction = DIR_SOUTHEAST;
   else if( schance < 50 )
      q->direction = DIR_EAST;
   else if( schance < 75 )
      q->direction = DIR_SOUTHWEST;
   else
      q->direction = 15;
}

void environment_update( void )
{
   if( number_range( 1, 100 ) > 95 )
   {
      int schance, type = ENV_RAIN;

      schance = number_range( 1, 100 );
      if( schance < 100 )
         type = ENV_QUAKE;
      if( schance < 80 )
         type = ENV_TORNADO;
      if( schance < 60 )
         type = ENV_FIRE;
      if( schance < 40 )
         type = ENV_THUNDER;
      if( schance < 20 )
         type = ENV_RAIN;

      generate_random_environment( type );
   }
   environment_actual_update(  );
   environment_pulse_update(  );
}

CMDF( do_makeenv )
{
   string etype, arg2, arg;
   int chosenradius;
   int door, atype, intensity = 0;
   int atx = -1, aty = -1, atmap = -1;
   environment_data *t;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs cannot create environmental affects.\r\n" );
      return;
   }

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
   {
      ch->print( "This can only be done on the overland.\r\n" );
      return;
   }

   argument = one_argument( argument, etype );
   argument = one_argument( argument, arg );
   argument = one_argument( argument, arg2 );

   if( etype.empty(  ) )
   {
      ch->print( "What type of affect are you trying to create?\r\n" );
      ch->print( "Possible affects are:\r\n\r\n" );
      ch->print( "quake fire rainstorm thunderstorm tornado\r\n" );
      return;
   }

   atype = get_env_type( etype );
   if( atype < 0 || atype >= ENV_MAX )
   {
      do_makeenv( ch, "" );
      return;
   }

   atx = ch->mx;
   aty = ch->my;
   atmap = ch->cmap;

   if( arg.empty(  ) )
   {
      ch->print( "Usage: makeenv <type> <radius> <direction> <intensity>\r\n" );
      ch->print( "Intensity is used only for earthquakes and is ignored otherwise.\r\n" );
      return;
   }

   if( !is_number( arg ) )
   {
      ch->print( "Radius value must be numerical.\r\n" );
      return;
   }

   if( !is_number( argument ) && atype == ENV_QUAKE )
   {
      ch->print( "Intensity value must be numerical.\r\n" );
      return;
   }

   intensity = atoi( argument.c_str(  ) );
   if( intensity < 1 && atype == ENV_QUAKE )
   {
      ch->print( "Intensity must be greater than zero.\r\n" );
      return;
   }

   chosenradius = atoi( arg.c_str(  ) );

   door = get_dirnum( arg2 );
   if( door < 0 || door > MAX_DIR )
      door = DIR_SOMEWHERE;

   if( door != DIR_SOMEWHERE && atype != ENV_QUAKE )
      ch->printf( "You create a %s to the %s!\r\n", etype.c_str(  ), dir_name[door] );
   else
   {
      if( atype == ENV_QUAKE )
         ch->print( "You create an earthquake!\r\n" );
      else
         ch->printf( "You create a non-moving %s!\r\n", etype.c_str(  ) );
   }
   t = new environment_data;
   t->mx = atx;
   t->my = aty;
   t->cmap = atmap;
   t->type = atype;
   t->direction = door;
   t->radius = chosenradius;
   t->damage_per_shake = ( ch->level / 4 );
   t->time_left = number_range( 10, 50 );
   t->intensity = intensity;
   envlist.push_back( t );
}

CMDF( do_env )
{
   int count = 0;
   list < environment_data * >::iterator env;

   for( env = envlist.begin(  ); env != envlist.end(  ); ++env )
   {
      environment_data *en = *env;

      ++count;
      if( en->type == ENV_QUAKE )
      {
         ch->printf( "&GA %d by %d, intensity %d, earthquake at coordinates %dX %dY on %s.\r\n", en->radius, en->radius, en->intensity, en->mx, en->my, map_names[en->cmap] );
      }
      else
      {
         ch->printf( "&GA %d by %d %s bound %s at coordinates %d,%d on %s.\r\n",
                     en->radius, en->radius, ( en->direction < 10 ) ? dir_name[en->direction] : "nowhere", env_name[en->type], en->mx, en->my, map_names[en->cmap] );
      }
   }

   if( count == 0 )
   {
      ch->print( "No environmental effects found.\r\n" );
      return;
   }
   ch->printf( "&G%d total environmental effects found.\r\n", count );
}

bool survey_environment( char_data * ch )
{
   list < environment_data * >::iterator env;
   double dist, angle, eta;
   int dir = -1, iMes;
   bool found = false;

   if( !ch->has_pcflag( PCFLAG_ONMAP ) )
      return false;

   ch->print( "&WAn imp appears with an ear-splitting BANG!\r\n" );

   for( env = envlist.begin(  ); env != envlist.end(  ); ++env )
   {
      environment_data *en = *env;

      if( ch->cmap != en->cmap )
         continue;

      dist = ( int )distance( ch->mx, ch->my, en->mx, en->my );

      if( dist > 300 )
         continue;

      found = true;

      angle = calc_angle( ch->mx, ch->my, en->mx, en->my, &dist );

      if( angle == -1 )
         dir = -1;
      else if( angle >= 360 )
         dir = DIR_NORTH;
      else if( angle >= 315 )
         dir = DIR_NORTHWEST;
      else if( angle >= 270 )
         dir = DIR_WEST;
      else if( angle >= 225 )
         dir = DIR_SOUTHWEST;
      else if( angle >= 180 )
         dir = DIR_SOUTH;
      else if( angle >= 135 )
         dir = DIR_SOUTHEAST;
      else if( angle >= 90 )
         dir = DIR_EAST;
      else if( angle >= 45 )
         dir = DIR_NORTHEAST;
      else if( angle >= 0 )
         dir = DIR_NORTH;

      if( dist > 200 )
         iMes = 0;
      else if( dist > 150 )
         iMes = 1;
      else if( dist > 100 )
         iMes = 2;
      else if( dist > 75 )
         iMes = 3;
      else if( dist > 50 )
         iMes = 4;
      else if( dist > 25 )
         iMes = 5;
      else if( dist > 15 )
         iMes = 6;
      else if( dist > 10 )
         iMes = 7;
      else if( dist > 5 )
         iMes = 8;
      else if( dist > 1 )
         iMes = 9;
      else
         iMes = 10;

      eta = dist * 10;
      if( dir == -1 )
         ch->printf( "&GAn %s bound &R%s&G within the immediate vicinity.\r\n", dir_name[en->direction], env_name[en->type] );
      else
         ch->printf( "&GA &R%s&G %s in the general %sern direction.\r\n", env_name[en->type], env_distances[iMes], dir_name[dir] );
      if( eta / 60 > 0 )
         ch->printf( "&GThis %s is about %d minutes away from you.\r\n", env_name[en->type], ( int )( eta / 60 ) );
      else
         ch->printf( "&GThis %s should be in the vicinity now.\r\n", env_name[en->type] );
   }

   if( !found )
   {
      ch->print( "&GNo environmental affects to report!\r\n" );
      ch->print( "&WThe imp vanishes with an ear-splitting BANG!\r\n" );
      return false;
   }

   ch->print( "&WThe imp vanishes with an ear-splitting BANG!\r\n" );
   return true;
}
