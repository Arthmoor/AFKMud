/****************************************************************************
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 ****************************************************************************
 * AFKMud Copyright 1997-2012 by Roger Libiez (Samson),                     *
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
 *                          Dynamic Channel System                          *
 ****************************************************************************/

#include <fstream>
#include "mud.h"
#include "channels.h"
#include "commands.h"
#include "descriptor.h"
#include "objindex.h"
#include "roomindex.h"

char *translate( int, const string &, const string & );
char *mini_c_time( time_t, int );
#if !defined(__CYGWIN__)
#ifdef MULTIPORT
void mud_message( char_data *, mud_channel *, const string & );
#endif
#endif

list < mud_channel * >chanlist;

const char *chan_types[] = {
   "Global", "Zone", "Guild", "Council", "PK", "Log"
};

const char *chan_flags[] = {
   "keephistory", "interport"
};

mud_channel::mud_channel(  )
{
   init_memory( &history, &type, sizeof( type ) );
}

mud_channel::~mud_channel(  )
{
   int loopa;

   for( loopa = 0; loopa < MAX_CHANHISTORY; ++loopa )
   {
      DISPOSE( this->history[loopa][0] );
      DISPOSE( this->history[loopa][1] );
   }
   chanlist.remove( this );
}

int get_chantypes( const string & name )
{
   for( size_t x = 0; x < sizeof( chan_types ) / sizeof( chan_types[0] ); ++x )
      if( !str_cmp( name, chan_types[x] ) )
         return x;
   return -1;
}

int get_chanflag( const string & flag )
{
   for( size_t x = 0; x < ( sizeof( chan_flags ) / sizeof( chan_flags[0] ) ); ++x )
      if( !str_cmp( flag, chan_flags[x] ) )
         return x;
   return -1;
}

/* Load the channel file */
void load_mudchannels( void )
{
   mud_channel *channel;
   int filever = 0;
   ifstream stream;

   log_string( "Loading channels..." );

   chanlist.clear(  );

   stream.open( CHANNEL_FILE );
   if( !stream.is_open(  ) )
   {
      log_string( "No channel file found." );
      return;
   }

   do
   {
      string key, value;
      char buf[MIL];

      stream >> key;
      stream.getline( buf, MIL );
      value = buf;

      strip_lspace( key );
      strip_tilde( value );
      strip_lspace( value );

      if( key.empty(  ) )
         continue;

      if( key == "#VERSION" )
         filever = atoi( value.c_str(  ) );
      else if( key == "#CHANNEL" )
         channel = new mud_channel;
      else if( key == "ChanName" )
         channel->name = value;
      else if( key == "ChanColorname" )
         channel->colorname = value;
      else if( key == "ChanLevel" )
         channel->level = atoi( value.c_str(  ) );
      else if( key == "ChanType" )
         channel->type = atoi( value.c_str(  ) );
      else if( key == "ChanFlags" )
      {
         if( filever < 1 )
            channel->flags = atoi( value.c_str(  ) );
         else
            flag_string_set( value, channel->flags, chan_flags );
      }
      else if( key == "End" )
         chanlist.push_back( channel );
      else
         log_printf( "%s: Bad line in channel file: %s %s", __FUNCTION__, key.c_str(  ), value.c_str(  ) );
   }
   while( !stream.eof(  ) );
   stream.close(  );
}

#define CHANNEL_VERSION 1
void save_mudchannels( void )
{
   ofstream stream;

   stream.open( CHANNEL_FILE );
   if( !stream.is_open(  ) )
   {
      bug( "%s: fopen", __FUNCTION__ );
      perror( CHANNEL_FILE );
   }
   else
   {
      stream << "#VERSION " << CHANNEL_VERSION << endl;
      list < mud_channel * >::iterator chan;
      for( chan = chanlist.begin(  ); chan != chanlist.end(  ); ++chan )
      {
         mud_channel *channel = *chan;

         if( !channel->name.empty(  ) )
         {
            stream << "#CHANNEL" << endl;
            stream << "ChanName      " << channel->name << endl;
            stream << "ChanColorname " << channel->colorname << endl;
            stream << "ChanLevel     " << channel->level << endl;
            stream << "ChanType      " << channel->type << endl;
            if( channel->flags.any(  ) )
               stream << "ChanFlags     " << bitset_string( channel->flags, chan_flags ) << endl;
            stream << "End" << endl << endl;
         }
      }
      stream.close(  );
   }
}

mud_channel *find_channel( const string & name )
{
   list < mud_channel * >::iterator chan;

   for( chan = chanlist.begin(  ); chan != chanlist.end(  ); ++chan )
   {
      mud_channel *channel = *chan;
      if( !str_prefix( name, channel->name ) )
         return channel;
   }
   return NULL;
}

CMDF( do_makechannel )
{
   mud_channel *channel;

   if( argument.empty(  ) )
   {
      ch->print( "&GSyntax: makechannel <name>\r\n" );
      return;
   }

   if( ( channel = find_channel( argument ) ) )
   {
      ch->print( "&RA channel with that name already exists.\r\n" );
      return;
   }

   channel = new mud_channel;
   channel->name = argument;
   channel->colorname = "chat";
   channel->level = LEVEL_IMMORTAL;
   channel->type = CHAN_GLOBAL;
   channel->flags.reset(  );
   chanlist.push_back( channel );
   ch->printf( "&YNew channel &G%s &Ycreated.\r\n", argument.c_str(  ) );
   save_mudchannels(  );
}

CMDF( do_setchannel )
{
   mud_channel *channel;
   string arg, arg2;

   if( argument.empty(  ) )
   {
      ch->print( "&GSyntax: setchannel <channel> <field> <value>\r\n\r\n" );
      ch->print( "&YField may be one of the following:\r\n" );
      ch->print( "name level type flags color\r\n" );
      return;
   }

   argument = one_argument( argument, arg );

   if( !( channel = find_channel( arg ) ) )
   {
      ch->print( "&RNo channel by that name exists.\r\n" );
      return;
   }

   argument = one_argument( argument, arg2 );

   if( arg.empty(  ) || arg2.empty(  ) )
   {
      do_setchannel( ch, "" );
      return;
   }

   if( !str_cmp( arg2, "name" ) )
   {
      ch->printf( "&YChannel &G%s &Yrenamed to &G%s\r\n", channel->name.c_str(  ), argument.c_str(  ) );
      channel->name = argument;
      save_mudchannels(  );
      return;
   }

   if( !str_cmp( arg2, "color" ) )
   {
      ch->printf( "&YChannel &G%s &Ycolor changed to &G%s\r\n", channel->name.c_str(  ), argument.c_str(  ) );
      channel->colorname = argument;
      save_mudchannels(  );
      return;
   }

   if( !str_cmp( arg2, "level" ) )
   {
      int level;

      if( !is_number( argument ) )
      {
         ch->print( "&RLevel must be numerical.\r\n" );
         return;
      }

      level = atoi( argument.c_str(  ) );

      if( level < 1 || level > MAX_LEVEL )
      {
         ch->printf( "&RInvalid level. Acceptable range is 1 to %d.\r\n", MAX_LEVEL );
         return;
      }

      channel->level = level;
      ch->printf( "&YChannel &G%s &Ylevel changed to &G%d\r\n", channel->name.c_str(  ), level );
      save_mudchannels(  );
      return;
   }

   if( !str_cmp( arg2, "type" ) )
   {
      int type = get_chantypes( argument );

      if( type == -1 )
      {
         ch->print( "&RInvalid channel type.\r\n" );
         return;
      }

      channel->type = type;
      ch->printf( "&YChannel &G%s &Ytype changed to &G%s\r\n", channel->name.c_str(  ), argument.c_str(  ) );
      save_mudchannels(  );
      return;
   }

   if( !str_cmp( arg2, "flags" ) )
   {
      string arg3;
      int value;

      if( argument.empty(  ) )
      {
         do_setchannel( ch, "" );
         return;
      }

      while( !argument.empty(  ) )
      {
         argument = one_argument( argument, arg3 );
         value = get_chanflag( arg3 );
         if( value < 0 || value >= CHAN_MAXFLAG )
            ch->printf( "Unknown flag: %s\r\n", arg3.c_str(  ) );
         else
            channel->flags.flip( value );
      }
      ch->print( "Channel flags set.\r\n" );
      save_mudchannels(  );
      return;
   }
   do_setchannel( ch, "" );
}

void free_mudchannels( void )
{
   list < mud_channel * >::iterator chan;

   for( chan = chanlist.begin(  ); chan != chanlist.end(  ); )
   {
      mud_channel *channel = *chan;
      ++chan;

      deleteptr( channel );
   }
}

CMDF( do_destroychannel )
{
   mud_channel *channel;

   if( argument.empty(  ) )
   {
      ch->print( "&GSyntax: destroychannel <name>\r\n" );
      return;
   }

   if( !( channel = find_channel( argument ) ) )
   {
      ch->print( "&RNo channel with that name exists.\r\n" );
      return;
   }
   deleteptr( channel );
   ch->printf( "&YChannel &G%s &Ydestroyed.\r\n", argument.c_str(  ) );
   save_mudchannels(  );
}

CMDF( do_showchannels )
{
   list < mud_channel * >::iterator chan;

   ch->print( "&WName               &YLevel &cColor      &BType       &GFlags\r\n" );
   ch->print( "&W----------------------------------------------------------\r\n" );
   for( chan = chanlist.begin(  ); chan != chanlist.end(  ); ++chan )
   {
      mud_channel *channel = *chan;

      ch->printf( "&W%-18s &Y%-4d  &c%-10s &B%-10s &G%s\r\n", capitalize( channel->name ).c_str(  ),
                  channel->level, channel->colorname.c_str(  ), chan_types[channel->type], bitset_string( channel->flags, chan_flags ) );
   }
}

CMDF( do_listen )
{
   mud_channel *channel;
   list < mud_channel * >::iterator chan;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs cannot change channels.\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "&GSyntax: listen <channel>\r\n" );
      ch->print( "&GSyntax: listen all\r\n" );
      ch->print( "&GSyntax: listen none\r\n" );
      ch->print( "&GFor a list of channels, type &Wchannels\r\n\r\n" );
      ch->print( "&YYou are listening to the following local mud channels:\r\n\r\n" );
      if( !ch->pcdata->chan_listen.empty(  ) )
         ch->printf( "&W%s\r\n", ch->pcdata->chan_listen.c_str(  ) );
      else
         ch->print( "&WNone\r\n" );
      return;
   }

   if( !str_cmp( argument, "all" ) )
   {
      for( chan = chanlist.begin(  ); chan != chanlist.end(  ); ++chan )
      {
         channel = *chan;

         if( ch->level >= channel->level && !hasname( ch->pcdata->chan_listen, channel->name ) )
            addname( ch->pcdata->chan_listen, channel->name );
      }
      ch->print( "&YYou are now listening to all available channels.\r\n" );
      return;
   }

   if( !str_cmp( argument, "none" ) )
   {
      ch->pcdata->chan_listen.clear(  );
      ch->print( "&YYou no longer listen to any available channels.\r\n" );
      return;
   }

   if( hasname( ch->pcdata->chan_listen, argument ) )
   {
      removename( ch->pcdata->chan_listen, argument );
      ch->printf( "&YYou no longer listen to &W%s\r\n", argument.c_str(  ) );
   }
   else
   {
      if( !( channel = find_channel( argument ) ) )
      {
         ch->print( "No such channel.\r\n" );
         return;
      }
      if( channel->level > ch->level )
      {
         ch->print( "That channel is above your level.\r\n" );
         return;
      }
      addname( ch->pcdata->chan_listen, channel->name );
      ch->printf( "&YYou now listen to &W%s\r\n", channel->name.c_str(  ) );
   }
}

/* Revised channel display by Zarius */
CMDF( do_channels )
{
   list < mud_channel * >::iterator chan;

   if( ch->isnpc(  ) )
   {
      ch->print( "NPCs cannot list channels.\r\n" );
      return;
   }

   ch->print( "&YThe following channels are available:\r\n" );
   ch->print( "To toggle a channel, use the &Wlisten &Ycommand.\r\n\r\n" );

   ch->print( "&WChannel        On/Off&D\r\n" );
   ch->print( "&B-----------------------&D\r\n" );
   for( chan = chanlist.begin(  ); chan != chanlist.end(  ); ++chan )
   {
      mud_channel *channel = *chan;

      if( ch->level >= channel->level )
      {
         ch->printf( "&w%-17s%s&D\r\n", capitalize( channel->name ).c_str(  ), ( hasname( ch->pcdata->chan_listen, channel->name ) ) ? "&GOn" : "&ROff" );
      }
   }
   ch->print( "\r\n" );
}

void show_channel_history( char_data * ch, mud_channel * channel )
{
   const char *name;

   ch->printf( "&cThe last %d %s messages:\r\n", MAX_CHANHISTORY, channel->name.c_str(  ) );
   for( int x = 0; x < MAX_CHANHISTORY; ++x )
   {
      if( channel->history[x][0] )
      {
         switch ( channel->hlevel[x] )
         {
            case 0:
               name = channel->history[x][0];
               break;
            case 1:
               if( ch->has_aflag( AFF_DETECT_INVIS ) || ch->has_pcflag( PCFLAG_HOLYLIGHT ) )
                  name = channel->history[x][0];
               else
                  name = "Someone";
               break;
            case 2:
               if( ch->level >= channel->hinvis[x] )
                  name = channel->history[x][0];
               else
                  name = "Someone";
               break;
            default:
               name = "Someone";
         }
         ch->printf( channel->history[x][1], mini_c_time( channel->htime[x], ch->pcdata->timezone ), name );
      }
      else
         break;
   }
}

/* Channel history. Records the last MAX_CHANHISTORY messages to channels which keep historys */
void update_channel_history( char_data * ch, mud_channel * channel, const string & argument, bool emote )
{
   for( int x = 0; x < MAX_CHANHISTORY; ++x )
   {
      int type = 0;
      if( ch && ch->has_aflag( AFF_INVISIBLE ) )
         type = 1;
      if( ch && ch->has_pcflag( PCFLAG_WIZINVIS ) )
         type = 2;

      if( !channel->history[x][0] )
      {
         if( !ch )
            channel->history[x][0] = str_dup( "System" );
         else if( ch->isnpc(  ) )
            channel->history[x][0] = str_dup( ch->short_descr );
         else
            channel->history[x][0] = str_dup( ch->name );

         const string newarg = add_percent( argument );
         strdup_printf( &channel->history[x][1], "   &R[%%s] &G%%s%s %s\r\n", emote ? "" : ":", newarg.c_str(  ) );
         channel->htime[x] = current_time;
         channel->hlevel[x] = type;
         if( type == 2 )
            channel->hinvis[x] = ch->pcdata->wizinvis;
         else
            channel->hinvis[x] = 0;
         break;
      }

      if( x == MAX_CHANHISTORY - 1 )
      {
         for( int y = 1; y < MAX_CHANHISTORY; ++y )
         {
            int z = y - 1;

            if( channel->history[z][0] != NULL )
            {
               DISPOSE( channel->history[z][0] );
               DISPOSE( channel->history[z][1] );
               channel->history[z][0] = str_dup( channel->history[y][0] );
               channel->history[z][1] = str_dup( channel->history[y][1] );
               channel->hlevel[z] = channel->hlevel[y];
               channel->hinvis[z] = channel->hinvis[y];
               channel->htime[z] = channel->htime[y];
            }
         }
         DISPOSE( channel->history[x][0] );
         DISPOSE( channel->history[x][1] );
         if( !ch )
            channel->history[x][0] = str_dup( "System" );
         else if( ch->isnpc(  ) )
            channel->history[x][0] = str_dup( ch->short_descr );
         else
            channel->history[x][0] = str_dup( ch->name );

         const string newarg = add_percent( argument );
         strdup_printf( &channel->history[x][1], "   &R[%%s] &G%%s%s %s\r\n", emote ? "" : ":", newarg.c_str(  ) );
         channel->hlevel[x] = type;
         channel->htime[x] = current_time;
         if( type == 2 )
            channel->hinvis[x] = ch->pcdata->wizinvis;
         else
            channel->hinvis[x] = 0;
      }
   }
}

/* Duplicate of to_channel from act_comm.c modified for dynamic channels */
void send_tochannel( char_data * ch, mud_channel * channel, string & argument )
{
   int speaking = -1;

   for( int lang = 0; lang < LANG_UNKNOWN; ++lang )
      if( ch->speaking == lang )
      {
         speaking = lang;
         break;
      }

   if( ch->isnpc(  ) && channel->type == CHAN_GUILD )
   {
      ch->print( "Mobs can't be in clans/guilds.\r\n" );
      return;
   }

   if( !ch->IS_PKILL(  ) && channel->type == CHAN_PK )
   {
      if( !ch->is_immortal(  ) )
      {
         ch->print( "Peacefuls have no need to use wartalk.\r\n" );
         return;
      }
   }

   if( ch->in_room->flags.test( ROOM_SILENCE ) )
   {
      ch->print( "The room absorbs your words!\r\n" );
      return;
   }

   if( ch->has_aflag( AFF_SILENCE ) )
   {
      ch->print( "You are unable to utter a sound!\r\n" );
      return;
   }

   if( ch->isnpc(  ) && ch->has_aflag( AFF_CHARM ) )
   {
      if( ch->master )
         ch->master->print( "I don't think so...\r\n" );
      return;
   }

   if( argument.empty(  ) || !str_cmp( argument, "hpurge" ) )
   {
      if( !channel->flags.test( CHAN_KEEPHISTORY ) )
      {
         ch->printf( "%s what?\r\n", capitalize( channel->name ).c_str(  ) );
         return;
      }

      if( !str_cmp( argument, "hpurge" ) )
      {
         for( int x = 0; x < MAX_CHANHISTORY; ++x )
         {
            DISPOSE( channel->history[x][0] );
            DISPOSE( channel->history[x][1] );
         }
         ch->printf( "The %s channel history has been purged.\r\n", channel->name.c_str(  ) );
         return;
      }
      show_channel_history( ch, channel );
      return;
   }

   if( ch->has_pcflag( PCFLAG_SILENCE ) )
   {
      ch->printf( "You can't %s.\r\n", channel->name.c_str(  ) );
      return;
   }

   /*
    * Inverts the speech of anyone carrying the burgundy amulet 
    */
   if( str_cmp( ch->name, "Krusty" ) )
   {
      list < obj_data * >::iterator iobj;

      for( iobj = ch->carrying.begin(  ); iobj != ch->carrying.end(  ); ++iobj )
      {
         obj_data *obj = ( *iobj ); /* Burgundy Amulet */
         if( obj->pIndexData->vnum == 1405 ) /* The amulet itself */
         {
            argument = invert_string( argument );
            break;
         }
      }
   }

   string arg, word;
   char_data *victim = NULL;
   social_type *social = NULL;
   string socbuf_char, socbuf_vict, socbuf_other;

   arg = argument;
   arg = one_argument( arg, word );

   if( word[0] == '@' && ( social = find_social( word.substr( 1, word.length(  ) ) ) ) != NULL )
   {
      if( !arg.empty(  ) )
      {
         string name;

         one_argument( arg, name );

         if( ( victim = ch->get_char_world( name ) ) )
            arg = one_argument( arg, name );

         if( !victim )
         {
            socbuf_char = social->char_no_arg;
            socbuf_vict = social->others_no_arg;
            socbuf_other = social->others_no_arg;
            if( socbuf_char.empty(  ) && socbuf_other.empty(  ) )
               social = NULL;
         }
         else if( victim == ch )
         {
            socbuf_char = social->char_auto;
            socbuf_vict = social->others_auto;
            socbuf_other = social->others_auto;
            if( socbuf_char.empty(  ) && socbuf_other.empty(  ) )
               social = NULL;
         }
         else
         {
            socbuf_char = social->char_found;
            socbuf_vict = social->vict_found;
            socbuf_other = social->others_found;
            if( socbuf_char.empty(  ) && socbuf_other.empty(  ) && socbuf_vict.empty(  ) )
               social = NULL;
         }
      }
      else
      {
         socbuf_char = social->char_no_arg;
         socbuf_vict = social->others_no_arg;
         socbuf_other = social->others_no_arg;
         if( socbuf_char.empty(  ) && socbuf_other.empty(  ) )
            social = NULL;
      }
   }

   bool emote = false;
   if( word[0] == ',' )
   {
      emote = true;
      argument = argument.substr( 1, argument.length(  ) );
   }

   if( social )
   {
      act_printf( AT_PLAIN, ch, argument.c_str(  ), victim, TO_CHAR, "&W[&[%s]%s&W] &[%s]%s",
                  channel->colorname.c_str(  ), capitalize( channel->name ).c_str(  ), channel->colorname.c_str(  ), socbuf_char.c_str(  ) );
   }
   else if( emote )
   {
      ch->printf( "&W[&[%s]%s&W] &[%s]%s %s\r\n",
                  channel->colorname.c_str(  ), capitalize( channel->name ).c_str(  ), channel->colorname.c_str(  ), ch->name, argument.c_str(  ) );
   }
   else
      ch->printf( "&[%s]You %s '%s'\r\n", channel->colorname.c_str(  ), channel->name.c_str(  ), argument.c_str(  ) );

   if( ch->in_room->flags.test( ROOM_LOGSPEECH ) )
      append_to_file( LOG_FILE, "%s: %s (%s)", ch->isnpc(  )? ch->short_descr : ch->name, argument.c_str(  ), channel->name.c_str(  ) );

   /*
    * Channel history. Records the last MAX_CHANHISTORY messages to channels which keep historys 
    */
   if( channel->flags.test( CHAN_KEEPHISTORY ) )
      update_channel_history( ch, channel, argument, emote );

   list < char_data * >::iterator ich;
   for( ich = pclist.begin(  ); ich != pclist.end(  ); ++ich )
   {
      char_data *vch = *ich;

      /*
       * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
       */
      bool mapped = false;
      int origmap = -1, origx = -1, origy = -1;

      if( vch == ch || !vch->desc )
         continue;

      if( vch->desc->connected == CON_PLAYING && hasname( vch->pcdata->chan_listen, channel->name ) )
      {
         string sbuf = argument;
         char lbuf[MIL + 4];  /* invis level string + buf */

         if( vch->level < channel->level )
            continue;

         if( vch->in_room->flags.test( ROOM_SILENCE ) )
            continue;

         if( channel->type == CHAN_ZONE && vch->in_room->area != ch->in_room->area )
            continue;

         if( channel->type == CHAN_PK && !vch->IS_PKILL(  ) && !vch->is_immortal(  ) )
            continue;

         if( channel->type == CHAN_GUILD )
         {
            if( vch->isnpc(  ) )
               continue;
            if( vch->pcdata->clan != ch->pcdata->clan )
               continue;
         }

         int position = vch->position;
         vch->position = POS_STANDING;

         if( ch->has_pcflag( PCFLAG_WIZINVIS ) && vch->can_see( ch, false ) && vch->is_immortal(  ) )
            snprintf( lbuf, MIL + 4, "&[%s](%d) ", channel->colorname.c_str(  ), ( !ch->isnpc(  ) )? ch->pcdata->wizinvis : ch->mobinvis );
         else
            lbuf[0] = '\0';

         if( speaking != -1 && ( !ch->isnpc(  ) || ch->speaking ) )
         {
            int speakswell = UMIN( knows_language( vch, ch->speaking, ch ), knows_language( ch, ch->speaking, vch ) );

            if( speakswell < 85 )
               sbuf = translate( speakswell, argument, lang_names[speaking] );
         }

         /*
          * Check to see if target is ignoring the sender 
          */
         if( is_ignoring( vch, ch ) )
         {
            /*
             * If the sender is an imm then they cannot be ignored 
             */
            if( !ch->is_immortal(  ) || vch->level > ch->level )
            {
               /*
                * Off to oblivion! 
                */
               continue;
            }
         }

         MOBtrigger = false;

         /*
          * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
          */
         if( ch->has_pcflag( PCFLAG_ONMAP ) )
         {
            mapped = true;
            origx = ch->mx;
            origy = ch->my;
            origmap = ch->cmap;
         }
         if( ch->isnpc(  ) && ch->has_actflag( ACT_ONMAP ) )
         {
            mapped = true;
            origx = ch->mx;
            origy = ch->my;
            origmap = ch->cmap;
         }
         fix_maps( vch, ch );

         char buf[MSL];
         if( !social && !emote )
         {
            snprintf( buf, MSL, "&[%s]$n %ss '$t&[%s]'", channel->colorname.c_str(  ), channel->name.c_str(  ), channel->colorname.c_str(  ) );
            mudstrlcat( lbuf, buf, MIL + 4 );
            act( AT_PLAIN, lbuf, ch, sbuf.c_str(  ), vch, TO_VICT );
         }

         if( emote )
         {
            snprintf( buf, MSL, "&W[&[%s]%s&W] &[%s]$n $t", channel->colorname.c_str(  ), capitalize( channel->name ).c_str(  ), channel->colorname.c_str(  ) );
            mudstrlcat( lbuf, buf, MIL + 4 );
            act( AT_PLAIN, lbuf, ch, sbuf.c_str(  ), vch, TO_VICT );
         }

         if( social )
         {
            if( vch == victim )
            {
               act_printf( AT_PLAIN, ch, NULL, vch, TO_VICT, "&W[&[%s]%s&W] &[%s]%s",
                           channel->colorname.c_str(  ), capitalize( channel->name ).c_str(  ), channel->colorname.c_str(  ), socbuf_vict.c_str(  ) );
            }
            else
            {
               act_printf( AT_PLAIN, ch, vch, victim, TO_THIRD, "&W[&[%s]%s&W] &[%s]%s", channel->colorname.c_str(  ),
                           capitalize( channel->name ).c_str(  ), channel->colorname.c_str(  ), socbuf_other.c_str(  ) );
            }
         }

         vch->position = position;
         /*
          * Hackish solution to stop that damned "someone chat" bug - Matarael 17.3.2002 
          */
         if( mapped )
         {
            ch->cmap = origmap;
            ch->mx = origx;
            ch->my = origy;
            if( ch->isnpc(  ) )
               ch->set_actflag( ACT_ONMAP );
            else
               ch->set_pcflag( PCFLAG_ONMAP );
         }
         else
         {
            if( ch->isnpc(  ) )
               ch->unset_actflag( ACT_ONMAP );
            else
               ch->unset_pcflag( PCFLAG_ONMAP );
            ch->cmap = -1;
            ch->mx = -1;
            ch->my = -1;
         }
      }
   }
}

void to_channel( const string & argument, const string & xchannel, int level )
{
   mud_channel *channel;

   if( dlist.empty(  ) || argument.empty(  ) )
      return;

   if( !( channel = find_channel( xchannel ) ) )
      return;

   if( channel->type != CHAN_LOG )
      return;

   if( channel->flags.test( CHAN_KEEPHISTORY ) )
      update_channel_history( NULL, channel, argument, false );

   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      descriptor_data *d = *ds;
      char_data *vch = d->original ? d->original : d->character;

      if( !vch )
         continue;

      if( d->original )
         continue;

      /*
       * This could be coming in higher than the normal level, so check first 
       */
      if( vch->level < level )
         continue;

      if( d->connected == CON_PLAYING && vch->level >= channel->level && hasname( vch->pcdata->chan_listen, channel->name ) )
      {
         char buf[MSL];

         snprintf( buf, MSL, "%s: %s\r\n", capitalize( channel->name ).c_str(  ), argument.c_str(  ) );
         vch->set_color( AT_LOG );
         vch->print( buf );
      }
   }
}

bool local_channel_hook( char_data * ch, const string & command, string & argument )
{
   mud_channel *channel;

   if( !ch )
      return false;

   if( !( channel = find_channel( command ) ) )
      return false;

   if( ch->level < channel->level )
      return false;

   /*
    * Logs are meant to be seen, not talked on, but if they keep history, they are viewable.
    */
   if( channel->type == CHAN_LOG )
   {
      if( channel->flags.test( CHAN_KEEPHISTORY ) )
         show_channel_history( ch, channel );
      return true;
   }

   if( !ch->isnpc(  ) && !hasname( ch->pcdata->chan_listen, channel->name ) )
   {
      ch->printf( "&RYou are not listening to the &G%s &Rchannel.\r\n", channel->name.c_str(  ) );
      return true;
   }

#if !defined(__CYGWIN__)
#ifdef MULTIPORT
   if( channel->flags.test( CHAN_INTERPORT ) )
      mud_message( ch, channel, argument );
#endif
#endif
   send_tochannel( ch, channel, argument );
   return true;
}
