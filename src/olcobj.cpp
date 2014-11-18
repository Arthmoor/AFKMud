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
 *                          Oasis OLC Module                                *
 ****************************************************************************/

/****************************************************************************
 * ResortMUD 4.0 Beta by Ntanel, Garinan, Badastaz, Josh, Digifuzz, Senir,  *
 * Kratas, Scion, Shogar and Tagith.  Special thanks to Thoric, Nivek,      *
 * Altrag, Arlorn, Justice, Samson, Dace, HyperEye and Yakkov.              *
 ****************************************************************************
 * Copyright (C) 1996 - 2001 Haslage Net Electronics: MudWorld              *
 * of Lorain, Ohio - ALL RIGHTS RESERVED                                    *
 * The text and pictures of this publication, or any part thereof, may not  *
 * be reproduced or transmitted in any form or by any means, electronic or  *
 * mechanical, includes photocopying, recording, storage in a information   *
 * retrieval system, or otherwise, without the prior written or e-mail      *
 * consent from the publisher.                                              *
 ****************************************************************************
 * GREETING must mention ResortMUD programmers and the help file named      *
 * CREDITS must remain completely intact as listed in the SMAUG license.    *
 ****************************************************************************/

/**************************************************************************\
 *                                                                        *
 *     OasisOLC II for Smaug 1.40 written by Evan Cortens(Tagith)         *
 *                                                                        *
 *   Based on OasisOLC for CircleMUD3.0bpl9 written by Harvey Gilpin      *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 *                    Object editing module (oedit.c) v1.0                *
 *                                                                        *
\**************************************************************************/

#include "mud.h"
#include "area.h"
#include "descriptor.h"
#include "liquids.h"
#include "mud_prog.h"
#include "objindex.h"
#include "roomindex.h"
#include "treasure.h"

/* External functions */
extern int top_affect;
extern const char *liquid_types[];

void medit_disp_aff_flags( descriptor_data * );
void medit_disp_ris( descriptor_data * );
void olc_log( descriptor_data *, const char *, ... ) __attribute__ ( ( format( printf, 2, 3 ) ) );
int get_traptype( const string & );
bool can_omodify( char_data *, obj_data * );

/* Internal functions */
void oedit_disp_menu( descriptor_data * );

void ostat_plus( char_data * ch, obj_data * obj, bool olc )
{
   liquid_data *liq = nullptr;
   skill_type *sktmp;
   int dam;
   char buf[MSL], lbuf[10];
   int x;

   /******
    * A more informative ostat, so You actually know what those obj->value[x] mean
    * without looking in the code for it. Combines parts of look, examine, the
    * identification spell, and things that were never seen.. Probably overkill
    * on most things, but I'm lazy and hate digging through code to see what
    * value[x] means... -Druid
    ******/
   ch->print( "&wAdditional Object information:\r\n" );

   switch ( obj->item_type )
   {
      default:
         ch->print( "Additional details not available. This item type is not yet supported.\r\n" );
         ch->print( "Please report this to your immortal supervisor so this can be resolved.\r\n" );
         break;

      case ITEM_LIGHT:
         ch->printf( "%sValue[2] - Hours left: ", olc ? "&gG&w) " : "&w" );
         if( obj->value[2] >= 0 )
            ch->printf( "&c%d\r\n", obj->value[2] );
         else
            ch->print( "&cInfinite\r\n" );
         break;

      case ITEM_POTION:
      case ITEM_PILL:
      case ITEM_SCROLL:
         ch->printf( "%sValue[0] - Spell Level: %d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );

         for( x = 1; x <= 3; ++x )
         {
            snprintf( lbuf, 10, "&g%c&w) ", 'E' + x );
            if( obj->value[x] >= 0 && ( sktmp = get_skilltype( obj->value[x] ) ) != nullptr )
               ch->printf( "%sValue[%d] - Spell (%d): &c%s\r\n", olc ? lbuf : "", x, obj->value[x], sktmp->name );
            else
               ch->printf( "%sValue[%d] - Spell: None\r\n", olc ? lbuf : "&w", x );
         }

         if( obj->item_type == ITEM_PILL )
            ch->printf( "%sValue[4] - Food Value: &c%d\r\n", olc ? "&gI&w) " : "&w", obj->value[4] );
         break;

      case ITEM_SALVE:
      case ITEM_WAND:
      case ITEM_STAFF:
         ch->printf( "%sValue[0] - Spell Level: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[1] - Max Charges: &c%d\r\n", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch->printf( "%sValue[2] - Charges Remaining: &c%d\r\n", olc ? "&gG&w) " : "&w", obj->value[2] );
         if( obj->item_type != ITEM_SALVE )
         {
            if( obj->value[3] >= 0 && ( sktmp = get_skilltype( obj->value[3] ) ) != nullptr )
               ch->printf( "%sValue[3] - Spell (%d): &c%s\r\n", olc ? "&gH&w) " : "&w", obj->value[3], sktmp->name );
            else
               ch->printf( "%sValue[3] - Spell: &cNone\r\n", olc ? "&gH&w) " : "&w" );
            break;
         }
         ch->printf( "%sValue[3] - Delay (beats): &c%d\r\n", olc ? "&gH&w) " : "&w", obj->value[3] );
         for( x = 4; x <= 5; ++x )
         {
            snprintf( lbuf, 10, "&g%c&w) ", 'E' + x );
            if( obj->value[x] >= 0 && ( sktmp = get_skilltype( obj->value[x] ) ) != nullptr )
               ch->printf( "%sValue[%d] - Spell (%d): &c%s\r\n", olc ? lbuf : "&w", x, obj->value[x], sktmp->name );
            else
               ch->printf( "%sValue[%d] - Spell: None\r\n", olc ? lbuf : "&w", x );
         }
         break;

      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         ch->printf( "%sValue[0] - Base Condition: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[1] - Min. Damage: &c%d\r\n", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch->printf( "%sValue[2] - Max Damage: &c%d\r\n", olc ? "&gG&w) " : "&w", obj->value[2] );
         ch->printf( "&RAverage Hit: %d\r\n", ( obj->value[1] + obj->value[2] ) / 2 );
         if( obj->item_type != ITEM_MISSILE_WEAPON )
            ch->printf( "%sValue[3] - Damage Type (%d): &c%s\r\n", olc ? "&gH&w) " : "&w", obj->value[3], attack_table[obj->value[3]] );
         ch->printf( "%sValue[4] - Skill Required (%d): &c%s\r\n", olc ? "&gI&w) " : "&w", obj->value[4], weapon_skills[obj->value[4]] );
         if( obj->item_type == ITEM_MISSILE_WEAPON )
         {
            ch->printf( "%sValue[5] - Projectile Fired (%d): &c%s\r\n", olc ? "&gJ&w) " : "&w", obj->value[5], projectiles[obj->value[5]] );
            ch->print( "Projectile fired must match on the projectiles this weapon fires.\r\n" );
         }
         ch->printf( "%sValue[6] - Current Condition: &c%d\r\n", olc ? "&gK&w) " : "&w", obj->value[6] );
         ch->printf( "Condition: %s\r\n", condtxt( obj->value[6], obj->value[0] ).c_str(  ) );
         ch->printf( "%sValue[7] - Available sockets: &c%d\r\n", olc ? "&gL&w) " : "&w", obj->value[7] );
         ch->printf( "Socket 1: %s\r\n", obj->socket[0] );
         ch->printf( "Socket 2: %s\r\n", obj->socket[1] );
         ch->printf( "Socket 3: %s\r\n", obj->socket[2] );
         ch->print( "&WThe following 3 settings only apply to automatically generated weapons.\r\n" );
         ch->printf( "%sValue[8] - Weapon Type (%d): &c%s\r\n", olc ? "&gM&w) " : "&w", obj->value[8], weapon_type[obj->value[8]].name );
         ch->printf( "%sValue[9] - Weapon Material (%d): &c%s\r\n", olc ? "&gN&w) " : "&w", obj->value[9], materials[obj->value[9]].name );
         ch->printf( "%sValue[10] - Weapon Quality (%d): &c%s\r\n", olc ? "&gO&w) " : "&w", obj->value[10], weapon_quality[obj->value[10]] );
         break;

      case ITEM_ARMOR:
         ch->printf( "%sValue[0] - Current AC: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[1] - Base AC: &c%d\r\n", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch->printf( "Condition: %s\r\n", condtxt( obj->value[0], obj->value[1] ).c_str(  ) );
         ch->printf( "%sValue[2] - Available sockets( applies only to body armor ): &c%d\r\n", olc ? "&gG&w) " : "&w", obj->value[2] );
         ch->printf( "Socket 1: %s\r\n", obj->socket[0] );
         ch->printf( "Socket 2: %s\r\n", obj->socket[1] );
         ch->printf( "Socket 3: %s\r\n", obj->socket[2] );
         ch->print( "&WThe following 2 settings only apply to automatically generated armors.\r\n" );
         ch->printf( "%sValue[3] - Armor Type (%d): &c%s\r\n", olc ? "&gH&w) " : "&w", obj->value[3], armor_type[obj->value[3]].name );
         ch->printf( "%sValue[4] - Armor Material (%d): &c%s\r\n", olc ? "&gI&w) " : "&w", obj->value[4], materials[obj->value[4]].name );
         break;

      case ITEM_COOK:
      case ITEM_FOOD:
         ch->printf( "%sValue[0] - Food Value: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[1] - Condition (%d): &c", olc ? "&gF&w) " : "&w", obj->value[1] );
         if( obj->timer > 0 && obj->value[1] > 0 )
            dam = ( obj->timer * 10 ) / obj->value[1];
         else
            dam = 10;
         if( dam >= 10 )
            mudstrlcpy( buf, "It is fresh.", MSL );
         else if( dam == 9 )
            mudstrlcpy( buf, "It is nearly fresh.", MSL );
         else if( dam == 8 )
            mudstrlcpy( buf, "It is perfectly fine.", MSL );
         else if( dam == 7 )
            mudstrlcpy( buf, "It looks good.", MSL );
         else if( dam == 6 )
            mudstrlcpy( buf, "It looks ok.", MSL );
         else if( dam == 5 )
            mudstrlcpy( buf, "It is a little stale.", MSL );
         else if( dam == 4 )
            mudstrlcpy( buf, "It is a bit stale.", MSL );
         else if( dam == 3 )
            mudstrlcpy( buf, "It smells slightly off.", MSL );
         else if( dam == 2 )
            mudstrlcpy( buf, "It smells quite rank.", MSL );
         else if( dam == 1 )
            mudstrlcpy( buf, "It smells revolting!", MSL );
         else if( dam <= 0 )
            mudstrlcpy( buf, "It is crawling with maggots!", MSL );
         mudstrlcat( buf, "\r\n", MSL );
         ch->print( buf );
         if( obj->item_type == ITEM_COOK )
         {
            ch->printf( "%sValue[2] - Condition (%d): &c", olc ? "&gG&w) " : "&w", obj->value[2] );
            dam = obj->value[2];
            if( dam >= 3 )
               mudstrlcpy( buf, "It is burned to a crisp.", MSL );
            else if( dam == 2 )
               mudstrlcpy( buf, "It is a little over cooked.", MSL );
            else if( dam == 1 )
               mudstrlcpy( buf, "It is perfectly roasted.", MSL );
            else
               mudstrlcpy( buf, "It is raw.", MSL );
            mudstrlcat( buf, "\r\n", MSL );
            ch->print( buf );
         }
         if( obj->value[3] != 0 )
         {
            ch->printf( "%sValue[3] - Poisoned (%d): &cYes\r\n", olc ? "&gH&w) " : "&w", obj->value[3] );
            x = 2 * obj->value[0] * ( obj->value[3] > 0 ? obj->value[3] : 1 );
            ch->printf( "Duration: %d\r\n", x );
         }
         if( obj->timer > 0 && obj->value[1] > 0 )
            dam = ( obj->timer * 10 ) / obj->value[1];
         else
            dam = 10;
         if( obj->value[3] <= 0 && ( ( dam < 4 && number_range( 0, dam + 1 ) == 0 ) || ( obj->item_type == ITEM_COOK && obj->value[2] == 0 ) ) )
         {
            ch->print( "Poison: Yes\r\n" );
            x = 2 * obj->value[0] * ( obj->value[3] > 0 ? obj->value[3] : 1 );
            ch->printf( "Duration: %d\r\n", x );
         }
         if( obj->value[4] )
            ch->printf( "%sValue[4] - Timer: &c%d\r\n", olc ? "&gI&w) " : "&w", obj->value[4] );
         break;

      case ITEM_DRINK_CON:
         if( !( liq = get_liq_vnum( obj->value[2] ) ) )
            bug( "%s: bad liquid number %d.", __func__, obj->value[2] );

         ch->printf( "%sValue[0] - Capacity: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[1] - Quantity Left (%d): &c", olc ? "&gF&w) " : "&w", obj->value[1] );
         if( obj->value[1] > obj->value[0] )
            ch->print( "More than Full\r\n" );
         else if( obj->value[1] == obj->value[0] )
            ch->print( "Full\r\n" );
         else if( obj->value[1] >= ( 3 * obj->value[0] / 4 ) )
            ch->print( "Almost Full\r\n" );
         else if( obj->value[1] > ( obj->value[0] / 2 ) )
            ch->print( "More than half full\r\n" );
         else if( obj->value[1] == ( obj->value[0] / 2 ) )
            ch->print( "Half full\r\n" );
         else if( obj->value[1] >= ( obj->value[0] / 4 ) )
            ch->print( "Less than half full\r\n" );
         else if( obj->value[1] >= 1 )
            ch->print( "Almost Empty\r\n" );
         else
            ch->print( "Empty\r\n" );
         ch->printf( "%sValue[2] - Liquid (%d): &c%s\r\n", olc ? "&gG&w) " : "&w", obj->value[2], liq->name.c_str(  ) );
         ch->printf( "Liquid type : %s\r\n", liquid_types[liq->type] );
         ch->printf( "Liquid color: %s\r\n", liq->color.c_str(  ) );
         if( liq->mod[COND_DRUNK] != 0 )
            ch->printf( "Affects Drunkeness by: %d\r\n", liq->mod[COND_DRUNK] );
         if( liq->mod[COND_FULL] != 0 )
            ch->printf( "Affects Hunger by: %d\r\n", liq->mod[COND_FULL] );
         if( liq->mod[COND_THIRST] != 0 )
            ch->printf( "Affects Thirst by: %d\r\n", liq->mod[COND_THIRST] );
         ch->printf( "Poisoned: &c%s\r\n", liq->type == LIQTYPE_POISON ? "Yes" : "No" );
         break;

      case ITEM_HERB:
         ch->printf( "%sValue[1] - Charges: &c%d\r\n", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch->printf( "%sValue[2] - Herb #: &cY%d\r\n", olc ? "&gG&w) " : "&w", obj->value[2] );
         break;

      case ITEM_CONTAINER:
         ch->printf( "%sValue[0] - Capacity (%d): &c", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%s\r\n",
                     obj->value[0] < 76 ? "Small capacity" :
                     obj->value[0] < 150 ? "Small to medium capacity" :
                     obj->value[0] < 300 ? "Medium capacity" : obj->value[0] < 550 ? "Medium to large capacity" : obj->value[0] < 751 ? "Large capacity" : "Giant capacity" );
         ch->printf( "%sValue[1] - Flags (%d): &c", olc ? "&gF&w) " : "&w", obj->value[1] );
         if( obj->value[1] <= 0 )
            ch->print( " None\r\n" );
         else
            ch->printf( "%s\r\n", flag_string( obj->value[1], container_flags ) );
         ch->printf( "%sValue[2] - Key (%d): &c", olc ? "&gG&w) " : "&w", obj->value[2] );
         if( obj->value[2] <= 0 )
            ch->print( "None\r\n" );
         else
         {
            obj_index *key = get_obj_index( obj->value[2] );

            if( key )
               ch->printf( "%s\r\n", key->short_descr );
            else
               ch->print( "ERROR: Key does not exist!\r\n" );
         }
         ch->printf( "%sValue[3] - Condition: &c%d\r\n", olc ? "&gH&w) " : "&w", obj->value[3] );
         if( obj->timer )
            ch->printf( "Object Timer, Time Left: %d\r\n", obj->timer );
         break;

      case ITEM_PROJECTILE:
         ch->printf( "%sValue[0] - Condition: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[1] - Min. Damage: &c%d\r\n", olc ? "&gF&w) " : "&w", obj->value[1] );
         ch->printf( "%sValue[2] - Max Damage: &c%d\r\n", olc ? "&gG&w) " : "&w", obj->value[2] );
         ch->printf( "%sValue[3] - Damage Type (%d): &c%s\r\n", olc ? "&gH&w) " : "&w", obj->value[3], attack_table[obj->value[3]] );
         ch->printf( "%sValue[4] - Projectile Type (%d): &c%s\r\n", olc ? "&gI&w) " : "&w", obj->value[4], projectiles[obj->value[4]] );
         ch->print( "Projectile type must match on the missileweapon which fires it.\r\n" );
         ch->printf( "%sValue[5] - Current Condition: &c%d\r\n", olc ? "&gJ&w) " : "&w", obj->value[5] );
         ch->printf( "Condition: %s", condtxt( obj->value[5], obj->value[0] ).c_str(  ) );
         break;

      case ITEM_MONEY:
         ch->printf( "%sValue[0] - # of Coins: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         break;

      case ITEM_FURNITURE:
         ch->printf( "%sValue[2] - Furniture Flags: &c%s\r\n", olc ? "&gG&w) " : "&w", flag_string( obj->value[2], furniture_flags ) );
         break;

      case ITEM_TRAP:
         ch->printf( "%sValue[0] - Charges Remaining: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[1] - Type (%d): &c%s\r\n", olc ? "&gF&w) " : "&w", obj->value[1], trap_types[obj->value[1]] );
         switch ( obj->value[1] )
         {
            default:
               mudstrlcpy( buf, "Hit by a trap", MSL );
               ch->printf( "Does Damage from (%d) to (%d)\r\n", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_POISON_GAS:
               mudstrlcpy( buf, "Surrounded by a green cloud of gas", MSL );
               ch->print( "Casts spell: Poison\r\n" );
               break;
            case TRAP_TYPE_POISON_DART:
               mudstrlcpy( buf, "Hit by a dart", MSL );
               ch->print( "Casts spell: Poison\r\n" );
               ch->printf( "Does Damage from (%d) to (%d)\r\n", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_POISON_NEEDLE:
               mudstrlcpy( buf, "Pricked by a needle", MSL );
               ch->print( "Casts spell: Poison\r\n" );
               ch->printf( "Does Damage from (%d) to (%d)\r\n", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_POISON_DAGGER:
               mudstrlcpy( buf, "Stabbed by a dagger", MSL );
               ch->print( "Casts spell: Poison\r\n" );
               ch->printf( "Does Damage from (%d) to (%d)\r\n", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_POISON_ARROW:
               mudstrlcpy( buf, "Struck with an arrow", MSL );
               ch->print( "Casts spell: Poison\r\n" );
               ch->printf( "Does Damage from (%d) to (%d)\r\n", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_BLINDNESS_GAS:
               mudstrlcpy( buf, "Surrounded by a red cloud of gas", MSL );
               ch->print( "Casts spell: Blind\r\n" );
               break;
            case TRAP_TYPE_SLEEPING_GAS:
               mudstrlcpy( buf, "Surrounded by a yellow cloud of gas", MSL );
               ch->print( "Casts spell: Sleep\r\n" );
               break;
            case TRAP_TYPE_FLAME:
               mudstrlcpy( buf, "Struck by a burst of flame", MSL );
               ch->print( "Casts spell: Flamestrike\r\n" );
               break;
            case TRAP_TYPE_EXPLOSION:
               mudstrlcpy( buf, "Hit by an explosion", MSL );
               ch->print( "Casts spell: Fireball\r\n" );
               break;
            case TRAP_TYPE_ACID_SPRAY:
               mudstrlcpy( buf, "Covered by a spray of acid", MSL );
               ch->print( "Casts spell: Acid Blast\r\n" );
               break;
            case TRAP_TYPE_ELECTRIC_SHOCK:
               mudstrlcpy( buf, "Suddenly shocked", MSL );
               ch->print( "Casts spell: Lightning Bolt\r\n" );
               break;
            case TRAP_TYPE_BLADE:
               mudstrlcpy( buf, "Sliced by a razor sharp blade", MSL );
               ch->printf( "Does Damage from (%d) to (%d)\r\n", obj->value[4], obj->value[5] );
               break;
            case TRAP_TYPE_SEX_CHANGE:
               mudstrlcpy( buf, "Surrounded by a mysterious aura", MSL );
               ch->print( "Casts spell: Change Sex\r\n" );
               break;
         }
         ch->printf( "Text Displayed: %s\r\n", buf );
         ch->printf( "%sValue[3] - Trap Flags (%d): &c", olc ? "&gH&w) " : "&w", obj->value[3] );
         ch->printf( "%s\r\n", flag_string( obj->value[3], trap_flags ) );
         ch->printf( "%sValue[4] - Min. Damage: &c%d\r\n", olc ? "&gI&w) " : "&w", obj->value[4] );
         ch->printf( "%sValue[5] - Max Damage: &c%d\r\n", olc ? "&gJ&w) " : "&w", obj->value[5] );
         break;

      case ITEM_KEY:
         ch->printf( "%sValue[0] - Lock #: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[4] - Durability: &c%d\r\n", olc ? "&gI&w) " : "&w", obj->value[4] );
         ch->printf( "%sValue[5] - Container Lock Number: &c%d\r\n", olc ? "&gJ&w) " : "&w", obj->value[5] );
         break;

      case ITEM_SWITCH:
      case ITEM_LEVER:
      case ITEM_PULLCHAIN:
      case ITEM_BUTTON:
         ch->printf( "%sValue[0] - Flags (%d): &c%s\r\n", olc ? "&gE&w) " : "&w", obj->value[0], flag_string( obj->value[0], trig_flags ) );
         ch->print( "Trigger Position: " );
         if( obj->item_type != ITEM_BUTTON )
         {
            if( IS_SET( obj->value[0], TRIG_UP ) )
               ch->print( "Up\r\n" );
            else
               ch->print( "Down\r\n" );
         }
         else
         {
            if( IS_SET( obj->value[0], TRIG_UP ) )
               ch->print( "In\r\n" );
            else
               ch->print( "Out\r\n" );
         }
         ch->printf( "Automatically Reset Trigger? : %s\r\n", IS_SET( obj->value[0], TRIG_AUTORETURN ) ? "Yes" : "No" );
         if( HAS_PROG( obj->pIndexData, PULL_PROG ) || HAS_PROG( obj->pIndexData, PUSH_PROG ) )
            ch->print( "Object Has: " );
         if( HAS_PROG( obj->pIndexData, PULL_PROG ) && HAS_PROG( obj->pIndexData, PUSH_PROG ) )
            ch->print( "Push and Pull Programs\r\n" );
         else if( HAS_PROG( obj->pIndexData, PULL_PROG ) )
            ch->print( "Pull Program\r\n" );
         else if( HAS_PROG( obj->pIndexData, PUSH_PROG ) )
            ch->print( "Push Program\r\n" );
         if( IS_SET( obj->value[0], TRIG_OLOAD ) || IS_SET( obj->value[0], TRIG_MLOAD ) )
         {
            if( IS_SET( obj->value[0], TRIG_OLOAD ) )
            {
               ch->printf( "Triggers: Object Load &c%d&w into room &c%d&w at level &c%d&w.\r\n",
                  obj->value[1], obj->value[2], obj->value[3] );
            }

            if( IS_SET( obj->value[0], TRIG_MLOAD ) )
            {
               ch->printf( "Triggers: Mob Load &c%d&w into room &c%d&w.\r\n",
                  obj->value[1], obj->value[2] );
            }
         }
         if( IS_SET( obj->value[0], TRIG_CONTAINER ) )
         {
            ch->printf( "Triggers: Container &c%d&w in room &c%d&w.\r\n", obj->value[2], obj->value[1] );
            if( obj->value[3] > 0 )
            {
               ch->printf( "Manipulates the following container flags: &c%s&w\r\n",
                  flag_string( obj->value[3], container_flags ) );
            }
         }
         if( IS_SET( obj->value[0], TRIG_DEATH ) )
         {
            ch->print( "Triggers: Death - The player touching it is instantly killed. Corpse is retrievable.\r\n" );
         }
         if( IS_SET( obj->value[0], TRIG_CAST ) )
         {
            ch->printf( "Triggers: Casts '&c%s&w' at the player touching it.\r\n",
               !IS_VALID_SN( obj->value[1] ) ? "&RINVALID SN" : skill_table[obj->value[1]]->name );
            if( obj->value[2] > 0 )
               ch->printf( "Casts at spell level &c%d&w.\r\n", obj->value[2] );
            else
               ch->print( "Casts at the player's level.\r\n" );
         }
         if( IS_SET( obj->value[0], TRIG_TELEPORT ) || IS_SET( obj->value[0], TRIG_TELEPORTALL ) || IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) )
         {
            ch->printf( "Triggers: Teleport %s\r\n",
                        IS_SET( obj->value[0], TRIG_TELEPORT ) ? "Character <actor>" :
                        IS_SET( obj->value[0], TRIG_TELEPORTALL ) ? "All Characters in room" :
                        IS_SET( obj->value[0], TRIG_TELEPORTPLUS ) ? "All Characters and Objects in room" : "");
            ch->printf( "%sValue[1] - Teleport to Room: &c%d\r\n", olc ? "&gF&w) " : "&w", obj->value[1] );
            ch->printf( "Show Room Description on Teleport? %s\r\n", IS_SET( obj->value[0], TRIG_SHOWROOMDESC ) ? "Yes" : "No" );
         }
         if( IS_SET( obj->value[0], TRIG_RAND4 ) || IS_SET( obj->value[0], TRIG_RAND6 ) )
            ch->printf( "Triggers: Randomize %s Exits in room %d.\r\n", IS_SET( obj->value[0], TRIG_RAND4 ) ? "4" : "6", obj->value[1] );
         if( IS_SET( obj->value[0], TRIG_DOOR ) )
         {
            ch->print( "Triggers: Door\r\n" );
            if( IS_SET( obj->value[0], TRIG_PASSAGE ) )
               ch->print( "Triggers: Create Passage\r\n" );
            if( IS_SET( obj->value[0], TRIG_UNLOCK ) )
               ch->print( "Triggers: Unlock Door\r\n" );
            if( IS_SET( obj->value[0], TRIG_LOCK ) )
               ch->print( "Triggers: Lock Door\r\n" );
            if( IS_SET( obj->value[0], TRIG_OPEN ) )
               ch->print( "Triggers: Open Door\r\n" );
            if( IS_SET( obj->value[0], TRIG_CLOSE ) )
               ch->print( "Triggers: Close Door\r\n" );
            ch->printf( "%sValue[1] - In Room: &c%d\r\n", olc ? "&gF&w) " : "&w", obj->value[1] );
            ch->printf( "To the: %s\r\n",
                        IS_SET( obj->value[0], TRIG_D_NORTH ) ? "North" :
                        IS_SET( obj->value[0], TRIG_D_SOUTH ) ? "South" :
                        IS_SET( obj->value[0], TRIG_D_EAST ) ? "East" :
                        IS_SET( obj->value[0], TRIG_D_WEST ) ? "West" :
                        IS_SET( obj->value[0], TRIG_D_UP ) ? "UP" : IS_SET( obj->value[0], TRIG_D_DOWN ) ? "Down" : "Unknown" );
            if( IS_SET( obj->value[0], TRIG_PASSAGE ) )
               ch->printf( "%sValue[2] - To Room: &c%d\r\n", olc ? "&gG&w) " : "&w", obj->value[2] );
         }
         break;

      case ITEM_BLOOD:
         ch->printf( "%sValue[1] - Amount Remaining: &c%d\r\n", olc ? "&gF&w) " : "&w", obj->value[1] );
         if( obj->timer )
            ch->printf( "Object Timer, Time Left: %d\r\n", obj->timer );
         break;

      case ITEM_CAMPGEAR:
         ch->printf( "%sValue[0] - Type of Gear (%d): &c", olc ? "&gE&w) " : "&w", obj->value[0] );
         switch ( obj->value[0] )
         {
            default:
               ch->print( "Unknown\r\n" );
               break;

            case 1:
               ch->print( "Bedroll\r\n" );
               break;

            case 2:
               ch->print( "Misc. Gear\r\n" );
               break;

            case 3:
               ch->print( "Firewood\r\n" );
               break;
         }
         break;

      case ITEM_ORE:
         ch->printf( "%sValue[0] - Type of Ore (%d): &c%s\r\n", olc ? "&gE&w) " : "&w", obj->value[0], ores[obj->value[0]] );
         ch->printf( "%sValue[1] - Purity: &c%d", olc ? "&gF&w) " : "&w", obj->value[1] );
         break;

      case ITEM_PIECE:
         ch->printf( "%sValue[0] - Obj Vnum for Other Half: &c%d\r\n", olc ? "&gE&w) " : "&w", obj->value[0] );
         ch->printf( "%sValue[1] - Obj Vnum for Combined Object: &c%d", olc ? "&gF&w) " : "&w", obj->value[1] );
         break;
   }
}

olc_data::olc_data(  )
{
   init_memory( &mob, &changed, sizeof( changed ) );
}

void cleanup_olc( descriptor_data * d )
{
   if( d->olc )
   {
      if( d->character )
      {
         d->character->pcdata->dest_buf = nullptr;
         act( AT_ACTION, "$n stops using OLC.", d->character, nullptr, nullptr, TO_CANSEE );
      }
      d->connected = CON_PLAYING;
      deleteptr( d->olc );
   }
}

/*
 * Starts it all off
 */
CMDF( do_ooedit )
{
   obj_data *obj;
   descriptor_data *d;

   if( ch->isnpc(  ) )
   {
      ch->print( "I don't think so...\r\n" );
      return;
   }

   if( argument.empty(  ) )
   {
      ch->print( "OEdit what?\r\n" );
      return;
   }

   if( !( obj = ch->get_obj_world( argument ) ) )
   {
      ch->print( "Nothing like that in hell, earth, or heaven.\r\n" );
      return;
   }

   /*
    * Make sure the object isnt already being edited 
    */
   list < descriptor_data * >::iterator ds;
   for( ds = dlist.begin(  ); ds != dlist.end(  ); ++ds )
   {
      d = *ds;

      if( d->connected == CON_OEDIT )
         if( d->olc && OLC_VNUM( d ) == obj->pIndexData->vnum )
         {
            ch->printf( "That object is currently being edited by %s.\r\n", d->character->name );
            return;
         }
   }
   if( !can_omodify( ch, obj ) )
      return;

   d = ch->desc;
   d->olc = new olc_data;
   OLC_VNUM( d ) = obj->pIndexData->vnum;
   OLC_CHANGE( d ) = false;
   OLC_VAL( d ) = 0;
   d->character->pcdata->dest_buf = obj;
   d->connected = CON_OEDIT;
   oedit_disp_menu( d );

   act( AT_ACTION, "$n starts using OLC.", ch, nullptr, nullptr, TO_CANSEE );
}

CMDF( do_ocopy )
{
   area_data *pArea;
   string arg1;
   int ovnum, cvnum;
   obj_index *orig;

   if( argument.empty(  ) )
   {
      ch->print( "Usage: ocopy <original> <new>\r\n" );
      return;
   }

   argument = one_argument( argument, arg1 );
   if( arg1.empty(  ) || argument.empty(  ) )
   {
      ch->print( "Usage: ocopy <original> <new>\r\n" );
      return;
   }

   if( !is_number( arg1 ) || !is_number( argument ) )
   {
      ch->print( "Values must be numeric.\r\n" );
      return;
   }

   ovnum = atoi( arg1.c_str(  ) );
   cvnum = atoi( argument.c_str(  ) );

   if( ch->get_trust(  ) < LEVEL_GREATER )
   {
      if( !ch->pcdata || !( pArea = ch->pcdata->area ) )
      {
         ch->print( "You must have an assigned area to copy objects.\r\n" );
         return;
      }
      if( cvnum < pArea->low_vnum || cvnum > pArea->hi_vnum )
      {
         ch->print( "That number is not in your allocated range.\r\n" );
         return;
      }
   }
   else
   {
      if( !( pArea = ch->pcdata->area ) )
         pArea = ch->in_room->area;
   }

   if( get_obj_index( cvnum ) )
   {
      ch->print( "Target vnum already exists.\r\n" );
      return;
   }

   if( !( orig = get_obj_index( ovnum ) ) )
   {
      ch->print( "Source vnum does not exist.\r\n" );
      return;
   }
   make_object( cvnum, ovnum, orig->name, pArea );
   ch->print( "Object copied.\r\n" );
}

/**************************************************************************
 Menu functions 
 **************************************************************************/

/* For container flags */
void oedit_disp_container_flags_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < MAX_CONT_FLAG; ++i )
      d->character->printf( "&g%d&w) %s\r\n", i + 1, container_flags[i] );
   d->character->printf( "Container flags: &c%s&w\r\n", flag_string( obj->value[1], container_flags ) );

   d->character->print( "Enter flag, 0 to quit : " );
}

/* For furniture flags */
void oedit_disp_furniture_flags_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int i = 0; i < MAX_FURNFLAG; ++i )
      d->character->printf( "&g%d&w) %s\r\n", i + 1, furniture_flags[i] );
   d->character->printf( "Furniture flags: &c%s&w\r\n", flag_string( obj->value[2], furniture_flags ) );

   d->character->print( "Enter flag, 0 to quit : " );
}

/*
 * Display lever flags menu
 */
void oedit_disp_lever_flags_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < MAX_TRIGFLAG; ++counter )
      d->character->printf( "&g%2d&w) %s\r\n", counter + 1, trig_flags[counter] );
   d->character->printf( "Lever flags: &c%s&w\r\nEnter flag, 0 to quit: ", flag_string( obj->value[0], trig_flags ) );
}

/*
 * Fancy layering stuff, trying to lessen confusion :)
 */
void oedit_disp_layer_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;

   OLC_MODE( d ) = OEDIT_LAYERS;
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   d->character->print( "Choose which layer, or combination of layers fits best: \r\n\r\n" );
   d->character->printf( "[&c%s&w] &g1&w) Nothing Layers\r\n", ( obj->pIndexData->layers == 0 ) ? "X" : " " );
   d->character->printf( "[&c%s&w] &g2&w) Silk Shirt\r\n", IS_SET( obj->pIndexData->layers, 1 ) ? "X" : " " );
   d->character->printf( "[&c%s&w] &g3&w) Leather Vest\r\n", IS_SET( obj->pIndexData->layers, 2 ) ? "X" : " " );
   d->character->printf( "[&c%s&w] &g4&w) Light Chainmail\r\n", IS_SET( obj->pIndexData->layers, 4 ) ? "X" : " " );
   d->character->printf( "[&c%s&w] &g5&w) Leather Jacket\r\n", IS_SET( obj->pIndexData->layers, 8 ) ? "X" : " " );
   d->character->printf( "[&c%s&w] &g6&w) Light Cloak\r\n", IS_SET( obj->pIndexData->layers, 16 ) ? "X" : " " );
   d->character->printf( "[&c%s&w] &g7&w) Loose Cloak\r\n", IS_SET( obj->pIndexData->layers, 32 ) ? "X" : " " );
   d->character->printf( "[&c%s&w] &g8&w) Cape\r\n", IS_SET( obj->pIndexData->layers, 64 ) ? "X" : " " );
   d->character->printf( "[&c%s&w] &g9&w) Magical Effects\r\n", IS_SET( obj->pIndexData->layers, 128 ) ? "X" : " " );
   d->character->print( "\r\nLayer or 0 to exit: " );
}

/* For extra descriptions */
void oedit_disp_extradesc_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   list < extra_descr_data * >::iterator ed;
   extra_descr_data *edesc;
   int count = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   if( !obj->pIndexData->extradesc.empty(  ) )
   {
      for( ed = obj->pIndexData->extradesc.begin(  ); ed != obj->pIndexData->extradesc.end(  ); ++ed )
      {
         edesc = *ed;
         d->character->printf( "&g%2d&w) Keyword: &O%s\r\n", ++count, edesc->keyword.c_str(  ) );
      }
   }

   if( !obj->extradesc.empty(  ) )
   {
      for( ed = obj->extradesc.begin(  ); ed != obj->extradesc.end(  ); ++ed )
      {
         edesc = *ed;
         d->character->printf( "&g%2d&w) Keyword: &O%s\r\n", ++count, edesc->keyword.c_str(  ) );
      }
   }

   if( !obj->pIndexData->extradesc.empty(  ) || !obj->extradesc.empty(  ) )
      d->character->print( "\r\n" );

   d->character->print( "&gA&w) Add a new description\r\n" );
   d->character->print( "&gR&w) Remove a description\r\n" );
   d->character->print( "&gQ&w) Quit\r\n" );
   d->character->print( "\r\nEnter choice: " );

   OLC_MODE( d ) = OEDIT_EXTRADESC_MENU;
}

void oedit_disp_extra_choice( descriptor_data * d )
{
   extra_descr_data *ed = ( extra_descr_data * ) d->character->pcdata->spare_ptr;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   d->character->printf( "&g1&w) Keyword: &O%s\r\n", ed->keyword.c_str(  ) );
   d->character->printf( "&g2&w) Description: \r\n&O%s&w\r\n", ( !ed->desc.empty(  ) )? ed->desc.c_str(  ) : "(none)" );
   d->character->print( "&gQ&w) Quit\r\n" );
   d->character->print( "\r\nChange which option? " );

   OLC_MODE( d ) = OEDIT_EXTRADESC_CHOICE;
}

/* Ask for *which* apply to edit and prompt for some other options */
void oedit_disp_prompt_apply_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   list < affect_data * >::iterator paf;
   int counter = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( paf = obj->pIndexData->affects.begin(  ); paf != obj->pIndexData->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      d->character->printf( " &g%2d&w) ", ++counter );
      d->character->showaffect( af );
   }

   for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      d->character->printf( " &g%2d&w) ", ++counter );
      d->character->showaffect( af );
   }
   d->character->print( " \r\n &gA&w) Add an affect\r\n" );
   d->character->print( " &gR&w) Remove an affect\r\n" );
   d->character->print( " &gQ&w) Quit\r\n" );

   d->character->print( "\r\nEnter option or affect#: " );
   OLC_MODE( d ) = OEDIT_AFFECT_MENU;
}

/*. Ask for liquid type .*/
void oedit_liquid_type( descriptor_data * d )
{
   int counter, i, col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );

   counter = 0;
   for( i = 0; i < MAX_CONDS; ++i )
   {
      if( liquid_table[i] )
      {
         d->character->printf( " &w%2d&g ) &c%-20.20s ", counter, liquid_table[i]->name.c_str(  ) );

         if( ++col % 3 == 0 )
            d->character->print( "\r\n" );
         ++counter;
      }
   }

   d->character->print( "\r\n&wEnter drink type: " );
   OLC_MODE( d ) = OEDIT_VALUE_2;
}

/*
 * Display the menu of apply types
 */
void oedit_disp_affect_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < MAX_APPLY_TYPE; ++counter )
   {
      /*
       * Don't want people choosing these ones 
       */
      if( counter == 0 || counter == APPLY_EXT_AFFECT )
         continue;

      d->character->printf( "&g%2d&w) %-20.20s ", counter, a_types[counter] );
      if( ++col % 3 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter apply type (0 to quit): " );
   OLC_MODE( d ) = OEDIT_AFFECT_LOCATION;
}

/*
 * Display menu of projectile types
 */
void oedit_disp_proj_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < PROJ_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, projectiles[counter] );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter projectile type: " );
}

/*
 * Display menu of weapongen types
 */
void oedit_disp_wgen_type_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < TWTP_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, weapon_type[counter].name );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter weapon type: " );
}

/*
 * Display menu of armorgen types
 */
void oedit_disp_agen_type_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < TATP_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, armor_type[counter].name );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter armor type: " );
}

/*
 * Display menu of weapon/armorgen materials
 */
void oedit_disp_gen_material_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < TMAT_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, materials[counter].name );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter material type: " );
}

/*
 * Display menu of weapongen qualities
 */
void oedit_disp_wgen_qual_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < TQUAL_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, weapon_quality[counter] );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter quality: " );
}

/*
 * Display menu of damage types
 */
void oedit_disp_damage_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < DAM_MAX_TYPE; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, attack_table[counter] );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter damage type: " );
}

/*
 * Display menu of trap types
 */
void oedit_disp_traptype_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter <= TRAP_TYPE_SEX_CHANGE; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, trap_types[counter] );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter trap type: " );
}

/*
 * Display trap flags menu
 */
void oedit_disp_trapflags( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter <= TRAPFLAG_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter + 1, capitalize( trap_flags[counter] ) );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->printf( "\r\nTrap flags: &c%s&w\r\nEnter trap flag, 0 to quit:  ", flag_string( obj->value[3], trap_flags ) );
}

/*
 * Display menu of weapon types
 */
void oedit_disp_weapon_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < WEP_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, weapon_skills[counter] );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter weapon type: " );
}

/*
 * Display menu of campgear types
 */
void oedit_disp_gear_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < GEAR_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, campgear[counter] );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter gear type: " );
}

/*
 * Display menu of ore types
 */
void oedit_disp_ore_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < ORE_MAX; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, ores[counter] );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter ore type: " );
}

/* spell type */
void oedit_disp_spells_menu( descriptor_data * d )
{
   d->character->print( "Enter the name of the spell: " );
}

/* object value 0 */
void oedit_disp_val0_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_0;

   switch ( obj->item_type )
   {
      case ITEM_SALVE:
      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_WAND:
      case ITEM_STAFF:
      case ITEM_POTION:
         d->character->print( "Spell level : " );
         break;
      case ITEM_MISSILE_WEAPON:
      case ITEM_WEAPON:
      case ITEM_PROJECTILE:
         d->character->print( "Condition : " );
         break;
      case ITEM_ARMOR:
         d->character->print( "Current AC : " );
         break;
      case ITEM_QUIVER:
      case ITEM_KEYRING:
      case ITEM_PIPE:
      case ITEM_CONTAINER:
      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
         d->character->print( "Capacity : " );
         break;
      case ITEM_FOOD:
      case ITEM_COOK:
         d->character->print( "Hours to fill stomach : " );
         break;
      case ITEM_MONEY:
         d->character->print( "Amount of gold coins : " );
         break;
      case ITEM_LEVER:
      case ITEM_SWITCH:
         oedit_disp_lever_flags_menu( d );
         break;
      case ITEM_TRAP:
         d->character->print( "Charges: " );
         break;
      case ITEM_KEY:
         d->character->print( "Room lock vnum: " );
         break;
      case ITEM_ORE:
         oedit_disp_ore_menu( d );
         break;
      case ITEM_CAMPGEAR:
         oedit_disp_gear_menu( d );
         break;
      case ITEM_PIECE:
         d->character->print( "Vnum for second half of object: " );
         break;
      default:
         oedit_disp_menu( d );
   }
}

/* object value 1 */
void oedit_disp_val1_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_1;

   switch ( obj->item_type )
   {
      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         oedit_disp_spells_menu( d );
         break;

      case ITEM_SALVE:
      case ITEM_HERB:
         d->character->print( "Charges: " );
         break;

      case ITEM_PIPE:
         d->character->print( "Number of draws: " );
         break;

      case ITEM_WAND:
      case ITEM_STAFF:
         d->character->print( "Max number of charges : " );
         break;

      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
      case ITEM_PROJECTILE:
         d->character->print( "Number of damage dice : " );
         break;

      case ITEM_FOOD:
      case ITEM_COOK:
         d->character->print( "Condition: " );
         break;

      case ITEM_CONTAINER:
         oedit_disp_container_flags_menu( d );
         break;

      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
         d->character->print( "Quantity : " );
         break;

      case ITEM_ARMOR:
         d->character->print( "Original AC: " );
         break;

      case ITEM_LEVER:
      case ITEM_SWITCH:
         if( IS_SET( obj->value[0], TRIG_CAST ) )
            oedit_disp_spells_menu( d );
         else
            d->character->print( "Vnum: " );
         break;

      case ITEM_ORE:
         d->character->print( "Purity: " );
         break;

      case ITEM_PIECE:
         d->character->print( "Vnum for complete assembled object: " );
         break;

      case ITEM_TRAP:
         oedit_disp_traptype_menu( d );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 2 */
void oedit_disp_val2_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_2;

   switch ( obj->item_type )
   {
      case ITEM_LIGHT:
         d->character->print( "Number of hours (0 = burnt, -1 is infinite) : " );
         break;
      case ITEM_PILL:
      case ITEM_SCROLL:
      case ITEM_POTION:
         oedit_disp_spells_menu( d );
         break;
      case ITEM_WAND:
      case ITEM_STAFF:
         d->character->print( "Number of charges remaining : " );
         break;
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
      case ITEM_PROJECTILE:
         d->character->print( "Size of damage dice : " );
         break;
      case ITEM_CONTAINER:
         d->character->print( "Vnum of key to open container (-1 for no key) : " );
         break;
      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
         oedit_liquid_type( d );
         break;
      case ITEM_ARMOR:
         d->character->print( "Available sockets: " );
         break;
      case ITEM_FURNITURE:
         oedit_disp_furniture_flags_menu( d );
         break;
      default:
         oedit_disp_menu( d );
   }
}

/* object value 3 */
void oedit_disp_val3_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_3;

   switch ( obj->item_type )
   {
      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_WAND:
      case ITEM_STAFF:
         oedit_disp_spells_menu( d );
         break;

      case ITEM_WEAPON:
      case ITEM_PROJECTILE:
         oedit_disp_damage_menu( d );
         break;

      case ITEM_ARMOR:
         oedit_disp_agen_type_menu( d );
         break;

      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
      case ITEM_FOOD:
         d->character->print( "Poisoned (0 = not poisoned) : " );
         break;

      case ITEM_TRAP:
         oedit_disp_trapflags( d );
         OLC_MODE( d ) = OEDIT_TRAPFLAGS;
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 4 */
void oedit_disp_val4_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_4;

   switch ( obj->item_type )
   {
      case ITEM_SALVE:
         oedit_disp_spells_menu( d );
         break;

      case ITEM_FOOD:
      case ITEM_COOK:
         d->character->print( "Food value: " );
         break;

      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         oedit_disp_weapon_menu( d );
         break;

      case ITEM_PROJECTILE:
         oedit_disp_proj_menu( d );
         break;

      case ITEM_ARMOR:
         oedit_disp_gen_material_menu( d );
         break;

      case ITEM_KEY:
         d->character->print( "Durability: " );
         break;

      case ITEM_TRAP:
         d->character->print( "Minimum Damage: " );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 5 */
void oedit_disp_val5_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_5;

   switch ( obj->item_type )
   {
      case ITEM_SALVE:
         oedit_disp_spells_menu( d );
         break;

      case ITEM_MISSILE_WEAPON:
         oedit_disp_proj_menu( d );
         break;

      case ITEM_KEY:
         d->character->print( "Container lock vnum: " );
         break;

      case ITEM_PROJECTILE:
         d->character->print( "Current condition: " );
         break;

      case ITEM_TRAP:
         d->character->print( "Maximum Damage: " );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 6 */
void oedit_disp_val6_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_6;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         d->character->print( "Current condition: " );
         break;
      default:
         oedit_disp_menu( d );
   }
}

/* object value 7 */
void oedit_disp_val7_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_7;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
      case ITEM_MISSILE_WEAPON:
         d->character->print( "Available sockets: " );
         break;
      default:
         oedit_disp_menu( d );
   }
}

/* object value 8 */
void oedit_disp_val8_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_8;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
         oedit_disp_wgen_type_menu( d );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 9 */
void oedit_disp_val9_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_9;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
         oedit_disp_gen_material_menu( d );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object value 10 */
void oedit_disp_val10_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   OLC_MODE( d ) = OEDIT_VALUE_10;

   switch ( obj->item_type )
   {
      case ITEM_WEAPON:
         oedit_disp_wgen_qual_menu( d );
         break;

      default:
         oedit_disp_menu( d );
   }
}

/* object type */
void oedit_disp_type_menu( descriptor_data * d )
{
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < MAX_ITEM_TYPE; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter, o_types[counter] );
      if( ++col % 3 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->print( "\r\nEnter object type: " );
}

/* object extra flags */
void oedit_disp_extra_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < MAX_ITEM_FLAG; ++counter )
   {
      d->character->printf( "&g%2d&w) %-20.20s ", counter + 1, capitalize( o_flags[counter] ) );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->printf( "\r\nObject flags: &c%s&w\r\nEnter object extra flag (0 to quit): ", bitset_string( obj->extra_flags, o_flags ) );
}

/*
 * Display wear flags menu
 */
void oedit_disp_wear_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   int col = 0;

   d->write_to_buffer( "50\x1B[;H\x1B[2J" );
   for( int counter = 0; counter < MAX_WEAR_FLAG; ++counter )
   {
      if( counter == ITEM_DUAL_WIELD )
         continue;

      d->character->printf( "&g%2d&w) %-20.20s ", counter + 1, capitalize( w_flags[counter] ) );
      if( ++col % 2 == 0 )
         d->character->print( "\r\n" );
   }
   d->character->printf( "\r\nWear flags: &c%s&w\r\nEnter wear flag, 0 to quit:  ", bitset_string( obj->wear_flags, w_flags ) );
}

/* display main menu */
void oedit_disp_menu( descriptor_data * d )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;

   /*
    * Ominous looking ANSI code of some sort - perhaps there's some way to use this elsewhere ? 
    */
   d->write_to_buffer( "50\x1B[;H\x1B[2J" );

   /*
    * . Build first half of menu .
    */
   d->character->printf( "&w-- Item number : [&c%d&w]\r\n"
                         "&g1&w) Name     : &O%s\r\n"
                         "&g2&w) S-Desc   : &O%s\r\n"
                         "&g3&w) L-Desc   :-\r\n&O%s\r\n"
                         "&g4&w) A-Desc   :-\r\n&O%s\r\n"
                         "&g5&w) Type        : &c%s\r\n"
                         "&g6&w) Extra flags : &c%s\r\n",
                         obj->pIndexData->vnum, obj->name, obj->short_descr, obj->objdesc,
                         obj->action_desc ? obj->action_desc : "<not set>\r\n", capitalize( obj->item_type_name(  ) ).c_str(  ), bitset_string( obj->extra_flags, o_flags ) );

   /*
    * Build second half of the menu 
    */
   d->character->printf( "&g7&w) Wear flags  : &c%s\r\n" "&g8&w) Weight      : &c%d\r\n" "&g9&w) Cost        : &c%d\r\n" "&gA&w) Ego         : &c%d\r\n" "&gB&w) Timer       : &c%d\r\n" "&gC&w) Level       : &c%d\r\n"   /* -- Object level . */
                         "&gD&w) Layers      : &c%d\r\n",
                         bitset_string( obj->wear_flags, w_flags ), obj->weight, obj->cost, obj->ego, obj->timer, obj->level, obj->pIndexData->layers );

   ostat_plus( d->character, obj, true );

   d->character->print( "&gP&w) Affect menu\r\n" "&gR&w) Extra descriptions menu\r\n" "&gQ&w) Quit\r\n" "Enter choice : " );

   OLC_MODE( d ) = OEDIT_MAIN_MENU;
}

/***************************************************************************
 Object affect editing/removing functions
 ***************************************************************************/
void edit_object_affect( descriptor_data * d, int number )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   int count = 0;
   list < affect_data * >::iterator paf;

   for( paf = obj->pIndexData->affects.begin(  ); paf != obj->pIndexData->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      if( count == number )
      {
         d->character->pcdata->spare_ptr = af;
         OLC_VAL( d ) = true;
         oedit_disp_affect_menu( d );
         return;
      }
      ++count;
   }

   for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); ++paf )
   {
      affect_data *af = *paf;

      if( count == number )
      {
         d->character->pcdata->spare_ptr = af;
         OLC_VAL( d ) = true;
         oedit_disp_affect_menu( d );
         return;
      }
      ++count;
   }
   d->character->print( "Affect not found.\r\n" );
}

void remove_affect_from_obj( obj_data * obj, int number )
{
   int count = 0;
   list < affect_data * >::iterator paf;

   if( !obj->pIndexData->affects.empty(  ) )
   {
      for( paf = obj->pIndexData->affects.begin(  ); paf != obj->pIndexData->affects.end(  ); )
      {
         affect_data *aff = *paf;
         ++paf;

         if( count == number )
         {
            obj->pIndexData->affects.remove( aff );
            deleteptr( aff );
            --top_affect;
            return;
         }
         ++count;
      }
   }

   if( !obj->affects.empty(  ) )
   {
      for( paf = obj->affects.begin(  ); paf != obj->affects.end(  ); )
      {
         affect_data *aff = *paf;
         ++paf;

         if( count == number )
         {
            obj->affects.remove( aff );
            deleteptr( aff );
            --top_affect;
            return;
         }
         ++count;
      }
   }
}

extra_descr_data *oedit_find_extradesc( obj_data * obj, int number )
{
   int count = 0;
   list < extra_descr_data * >::iterator ed;
   extra_descr_data *edesc;

   for( ed = obj->pIndexData->extradesc.begin(  ); ed != obj->pIndexData->extradesc.end(  ); ++ed )
   {
      edesc = *ed;

      if( ++count == number )
         return edesc;
   }

   for( ed = obj->extradesc.begin(  ); ed != obj->extradesc.end(  ); ++ed )
   {
      edesc = *ed;

      if( ++count == number )
         return edesc;
   }
   return nullptr;
}

/*
 * Bogus command for resetting stuff
 */
CMDF( do_oedit_reset )
{
   obj_data *obj = ( obj_data * ) ch->pcdata->dest_buf;
   extra_descr_data *ed = ( extra_descr_data * ) ch->pcdata->spare_ptr;

   switch ( ch->substate )
   {
      default:
         return;

      case SUB_OBJ_EXTRA:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error, report to Samson.\r\n" );
            bug( "%s: sub_obj_extra: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         ed->desc = ch->copy_buffer(  );
         ch->stop_editing(  );
         ch->pcdata->dest_buf = obj;
         ch->pcdata->spare_ptr = ed;
         ch->substate = SUB_NONE;
         ch->desc->connected = CON_OEDIT;
         OLC_MODE( ch->desc ) = OEDIT_EXTRADESC_CHOICE;
         oedit_disp_extra_choice( ch->desc );
         return;

      case SUB_OBJ_LONG:
         if( !ch->pcdata->dest_buf )
         {
            ch->print( "Fatal error, report to Samson.\r\n" );
            bug( "%s: sub_obj_long: nullptr ch->pcdata->dest_buf", __func__ );
            ch->substate = SUB_NONE;
            return;
         }
         STRFREE( obj->objdesc );
         obj->objdesc = ch->copy_buffer( true );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->objdesc );
            obj->pIndexData->objdesc = QUICKLINK( obj->objdesc );
         }
         ch->stop_editing(  );
         ch->pcdata->dest_buf = obj;
         ch->desc->connected = CON_OEDIT;
         ch->substate = SUB_NONE;
         OLC_MODE( ch->desc ) = OEDIT_MAIN_MENU;
         oedit_disp_menu( ch->desc );
         return;
   }
}

/*
 * This function interprets the arguments that the character passed
 * to it based on which OLC mode you are in at the time
 */
void oedit_parse( descriptor_data * d, string & arg )
{
   obj_data *obj = ( obj_data * ) d->character->pcdata->dest_buf;
   affect_data *paf = ( affect_data * ) d->character->pcdata->spare_ptr;
   affect_data *npaf;
   extra_descr_data *ed = ( extra_descr_data * ) d->character->pcdata->spare_ptr;
   string arg1;
   int number = 0, max_val, min_val, value;
   /*
    * bool found; 
    */

   switch ( OLC_MODE( d ) )
   {
      case OEDIT_MAIN_MENU:
         /*
          * switch to whichever mode the user selected, display prompt or menu 
          */
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               cleanup_olc( d );
               return;
            case '1':
               d->character->print( "Enter namelist : " );
               OLC_MODE( d ) = OEDIT_EDIT_NAMELIST;
               break;
            case '2':
               d->character->print( "Enter short desc : " );
               OLC_MODE( d ) = OEDIT_SHORTDESC;
               break;
            case '3':
               d->character->print( "Enter long desc :-\r\n| " );
               OLC_MODE( d ) = OEDIT_LONGDESC;
               break;
            case '4':
               d->character->print( "Enter action desc :-\r\n" );
               OLC_MODE( d ) = OEDIT_ACTDESC;
               break;
            case '5':
               oedit_disp_type_menu( d );
               OLC_MODE( d ) = OEDIT_TYPE;
               break;
            case '6':
               oedit_disp_extra_menu( d );
               OLC_MODE( d ) = OEDIT_EXTRAS;
               break;
            case '7':
               oedit_disp_wear_menu( d );
               OLC_MODE( d ) = OEDIT_WEAR;
               break;
            case '8':
               d->character->print( "Enter weight : " );
               OLC_MODE( d ) = OEDIT_WEIGHT;
               break;
            case '9':
               d->character->print( "Enter cost : " );
               OLC_MODE( d ) = OEDIT_COST;
               break;
            case 'A':
               d->character->print( "Enter cost per day : " );
               OLC_MODE( d ) = OEDIT_COSTPERDAY;
               break;
            case 'B':
               d->character->print( "Enter timer : " );
               OLC_MODE( d ) = OEDIT_TIMER;
               break;
            case 'C':
               d->character->print( "Enter level : " );
               OLC_MODE( d ) = OEDIT_LEVEL;
               break;
            case 'D':
               if( obj->wear_flags.test( ITEM_WEAR_BODY )
                   || obj->wear_flags.test( ITEM_WEAR_ABOUT )
                   || obj->wear_flags.test( ITEM_WEAR_ARMS )
                   || obj->wear_flags.test( ITEM_WEAR_FEET )
                   || obj->wear_flags.test( ITEM_WEAR_HANDS ) || obj->wear_flags.test( ITEM_WEAR_LEGS ) || obj->wear_flags.test( ITEM_WEAR_WAIST ) )
               {
                  oedit_disp_layer_menu( d );
                  OLC_MODE( d ) = OEDIT_LAYERS;
               }
               else
                  d->character->print( "The wear location of this object is not layerable.\r\n" );
               break;
            case 'E':
               oedit_disp_val0_menu( d );
               break;
            case 'F':
               oedit_disp_val1_menu( d );
               break;
            case 'G':
               oedit_disp_val2_menu( d );
               break;
            case 'H':
               oedit_disp_val3_menu( d );
               break;
            case 'I':
               oedit_disp_val4_menu( d );
               break;
            case 'J':
               oedit_disp_val5_menu( d );
               break;
            case 'K':
               oedit_disp_val6_menu( d );
               break;
            case 'L':
               oedit_disp_val7_menu( d );
               break;
            case 'M':
               oedit_disp_val8_menu( d );
               break;
            case 'N':
               oedit_disp_val9_menu( d );
               break;
            case 'O':
               oedit_disp_val10_menu( d );
               break;
            case 'P':
               oedit_disp_prompt_apply_menu( d );
               break;
            case 'R':
               oedit_disp_extradesc_menu( d );
               break;
            default:
               oedit_disp_menu( d );
               break;
         }
         return;  /* end of OEDIT_MAIN_MENU */

      case OEDIT_EDIT_NAMELIST:
         STRFREE( obj->name );
         obj->name = STRALLOC( arg.c_str(  ) );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->name );
            obj->pIndexData->name = QUICKLINK( obj->name );
         }
         olc_log( d, "Changed name to %s", obj->name );
         break;

      case OEDIT_SHORTDESC:
         STRFREE( obj->short_descr );
         obj->short_descr = STRALLOC( arg.c_str(  ) );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->short_descr );
            obj->pIndexData->short_descr = QUICKLINK( obj->short_descr );
         }
         olc_log( d, "Changed short to %s", obj->short_descr );
         break;

      case OEDIT_LONGDESC:
         STRFREE( obj->objdesc );
         obj->objdesc = STRALLOC( arg.c_str(  ) );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->objdesc );
            obj->pIndexData->objdesc = QUICKLINK( obj->objdesc );
         }
         olc_log( d, "Changed longdesc to %s", obj->objdesc );
         break;

      case OEDIT_ACTDESC:
         STRFREE( obj->action_desc );
         obj->action_desc = STRALLOC( arg.c_str(  ) );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         {
            STRFREE( obj->pIndexData->action_desc );
            obj->pIndexData->action_desc = QUICKLINK( obj->action_desc );
         }
         olc_log( d, "Changed actiondesc to %s", obj->action_desc );
         break;

      case OEDIT_TYPE:
         if( is_number( arg ) )
            number = atoi( arg.c_str(  ) );
         else
            number = get_otype( arg );

         if( ( number < 1 ) || ( number >= MAX_ITEM_TYPE ) )
         {
            d->character->print( "Invalid choice, try again : " );
            return;
         }
         else
         {
            obj->item_type = number;
            if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
               obj->pIndexData->item_type = obj->item_type;
         }
         olc_log( d, "Changed object type to %s", o_types[number] );
         break;

      case OEDIT_EXTRAS:
         while( !arg.empty(  ) )
         {
            arg = one_argument( arg, arg1 );
            if( is_number( arg1 ) )
            {
               number = atoi( arg1.c_str(  ) );

               if( number == 0 )
               {
                  oedit_disp_menu( d );
                  return;
               }
               number -= 1;   /* Offset for 0 */
               if( number < 0 || number >= MAX_ITEM_FLAG )
               {
                  oedit_disp_extra_menu( d );
                  return;
               }
            }
            else
            {
               number = get_oflag( arg1 );
               if( number < 0 || number >= MAX_ITEM_FLAG )
               {
                  oedit_disp_extra_menu( d );
                  return;
               }
            }

            if( number == ITEM_PROTOTYPE && d->character->get_trust(  ) < LEVEL_GREATER && !hasname( d->character->pcdata->bestowments, "protoflag" ) )
               d->character->print( "You cannot change the prototype flag.\r\n" );
            else
            {
               obj->extra_flags.flip( number );
               olc_log( d, "%s the flag %s", obj->extra_flags.test( number ) ? "Added" : "Removed", o_flags[number] );
            }

            /*
             * If you used a number, you can only do one flag at a time 
             */
            if( is_number( arg ) )
               break;
         }
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->extra_flags = obj->extra_flags;
         oedit_disp_extra_menu( d );
         return;

      case OEDIT_WEAR:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;
            else if( number < 0 || number >= MAX_WEAR_FLAG )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            else
            {
               number -= 1;   /* Offset to accomodate 0 */
               obj->wear_flags.flip( number );
               olc_log( d, "%s the wearloc %s", obj->wear_flags.test( number ) ? "Added" : "Removed", w_flags[number] );
            }
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_wflag( arg1 );
               if( number != -1 )
               {
                  obj->wear_flags.flip( number );
                  olc_log( d, "%s the wearloc %s", obj->wear_flags.test( number ) ? "Added" : "Removed", w_flags[number] );
               }
            }
         }
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->wear_flags = obj->wear_flags;
         oedit_disp_wear_menu( d );
         return;

      case OEDIT_WEIGHT:
         number = atoi( arg.c_str(  ) );
         obj->weight = number;
         olc_log( d, "Changed weight to %d", obj->weight );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->weight = obj->weight;
         break;

      case OEDIT_COST:
         number = atoi( arg.c_str(  ) );
         obj->cost = number;
         olc_log( d, "Changed cost to %d", obj->cost );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->cost = obj->cost;
         break;

      case OEDIT_COSTPERDAY:
         number = atoi( arg.c_str(  ) );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
         {
            obj->pIndexData->ego = number;
            obj->ego = number;
            olc_log( d, "Changed ego to %d", obj->pIndexData->ego );
            if( number == -2 )
               obj->ego = obj->pIndexData->set_ego(  );
            if( obj->ego == -2 )
               olc_log( d, "%s", "&YWARNING: This object exceeds allowable ego specs.\r\n" );
         }
         else
         {
            obj->ego = number;
            if( number == -2 )
               obj->ego = obj->pIndexData->set_ego(  );
            olc_log( d, "Changed rent to %d", obj->ego );
            if( obj->ego == -2 )
               olc_log( d, "%s", "&YWARNING: This object exceeds allowable ego specs.\r\n" );
         }
         break;

      case OEDIT_TIMER:
         number = atoi( arg.c_str(  ) );
         obj->timer = number;
         olc_log( d, "Changed timer to %d", obj->timer );
         break;

      case OEDIT_LEVEL:
         number = atoi( arg.c_str(  ) );
         obj->level = URANGE( 0, number, MAX_LEVEL );
         olc_log( d, "Changed object level to %d", obj->level );
         break;

      case OEDIT_LAYERS:  // FIXME: Jesus Christ, for the love of God, etc. This is *NOT* right!
         /*
          * Like they say, easy on the user, hard on the programmer :) 
          * Or did I just make that up....
          */
         number = atoi( arg.c_str(  ) );
         switch ( number )
         {
            case 0:
               oedit_disp_menu( d );
               return;
            case 1:
               obj->pIndexData->layers = 0;
               break;
            case 2:
               TOGGLE_BIT( obj->pIndexData->layers, 1 );
               break;
            case 3:
               TOGGLE_BIT( obj->pIndexData->layers, 2 );
               break;
            case 4:
               TOGGLE_BIT( obj->pIndexData->layers, 4 );
               break;
            case 5:
               TOGGLE_BIT( obj->pIndexData->layers, 8 );
               break;
            case 6:
               TOGGLE_BIT( obj->pIndexData->layers, 16 );
               break;
            case 7:
               TOGGLE_BIT( obj->pIndexData->layers, 32 );
               break;
            case 8:
               TOGGLE_BIT( obj->pIndexData->layers, 64 );
               break;
            case 9:
               TOGGLE_BIT( obj->pIndexData->layers, 128 );
               break;
            default:
               d->character->print( "Invalid selection, try again: " );
               return;
         }
         olc_log( d, "Changed layers to %d", obj->pIndexData->layers );
         oedit_disp_layer_menu( d );
         return;

      case OEDIT_TRAPFLAGS:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
               break;
            else if( number < 0 || number > TRAPFLAG_MAX + 1 )
            {
               d->character->print( "Invalid flag, try again: " );
               return;
            }
            else
            {
               number -= 1;   /* Offset to accomodate 0 */
               TOGGLE_BIT( obj->value[3], 1 << number );
               olc_log( d, "%s the trapflag %s", IS_SET( obj->value[3], 1 << number ) ? "Added" : "Removed", trap_flags[number] );
            }
         }
         else
         {
            while( !arg.empty(  ) )
            {
               arg = one_argument( arg, arg1 );
               number = get_trapflag( arg1 );
               if( number != -1 )
               {
                  TOGGLE_BIT( obj->value[3], 1 << number );
                  olc_log( d, "%s the trapflag %s", IS_SET( obj->value[3], 1 << number ) ? "Added" : "Removed", trap_flags[number] );
               }
            }
         }
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[3] = obj->value[3];
         oedit_disp_trapflags( d );
         return;

      case OEDIT_VALUE_0:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_LEVER:
            case ITEM_SWITCH:
               if( number < 0 || number > 29 )
                  oedit_disp_lever_flags_menu( d );
               else
               {
                  if( number != 0 )
                  {
                     TOGGLE_BIT( obj->value[0], 1 << ( number - 1 ) );
                     if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
                        TOGGLE_BIT( obj->pIndexData->value[0], 1 << ( number - 1 ) );
                  }
               }
               break;

            default:
               obj->value[0] = number;
               if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
                  obj->pIndexData->value[0] = number;
         }
         olc_log( d, "Changed v0 to %d", obj->value[0] );
         break;

      case OEDIT_VALUE_1:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_PILL:
            case ITEM_SCROLL:
            case ITEM_POTION:
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               obj->value[1] = number;
               if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
                  obj->pIndexData->value[1] = number;
               break;

            case ITEM_LEVER:
            case ITEM_SWITCH:
               if( IS_SET( obj->value[0], TRIG_CAST ) )
                  number = skill_lookup( arg );
               obj->value[1] = number;
               if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
                  obj->pIndexData->value[1] = number;
               break;

            case ITEM_CONTAINER:
               number = atoi( arg.c_str(  ) );
               if( number < 0 || number > 31 )
                  oedit_disp_container_flags_menu( d );
               else
               {
                  /*
                   * if 0, quit 
                   */
                  if( number != 0 )
                  {
                     number = 1 << ( number - 1 );
                     TOGGLE_BIT( obj->value[1], number );
                     if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
                        TOGGLE_BIT( obj->pIndexData->value[1], number );
                  }
               }
               break;

            default:
               obj->value[1] = number;
               if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
                  obj->pIndexData->value[1] = number;
               break;
         }
         olc_log( d, "Changed v1 to %d", obj->value[1] );
         break;

      case OEDIT_VALUE_2:
         number = atoi( arg.c_str(  ) );
         /*
          * Some error checking done here 
          */
         switch ( obj->item_type )
         {
            case ITEM_SCROLL:
            case ITEM_POTION:
            case ITEM_PILL:
               min_val = -1;
               max_val = num_skills - 1;
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               break;

            case ITEM_WEAPON:
               min_val = 1;
               max_val = 100;
               break;

            case ITEM_ARMOR:
               min_val = 0;
               max_val = 3;
               break;

            case ITEM_DRINK_CON:
            case ITEM_FOUNTAIN:
               min_val = 0;
               max_val = top_liquid - 1;
               break;

            case ITEM_FURNITURE:
               min_val = 0;
               max_val = 2147483647;
               number = atoi( arg.c_str(  ) );
               if( number < 0 || number > 31 )
                  oedit_disp_furniture_flags_menu( d );
               else
               {
                  /*
                   * if 0, quit 
                   */
                  if( number != 0 )
                  {
                     number = 1 << ( number - 1 );
                     TOGGLE_BIT( obj->value[2], number );
                     if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
                        TOGGLE_BIT( obj->pIndexData->value[2], number );
                  }
               }
               break;

            default:
               /*
                * Would require modifying if you have bvnum 
                */
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[2] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v2 to %d", obj->value[2] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[2] = obj->value[2];
         break;

      case OEDIT_VALUE_3:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_PILL:
            case ITEM_SCROLL:
            case ITEM_POTION:
            case ITEM_WAND:
            case ITEM_STAFF:
               min_val = -1;
               max_val = num_skills - 1;
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               break;

            case ITEM_WEAPON:
               min_val = 0;
               max_val = DAM_MAX_TYPE - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val3_menu( d );
                  return;
               }
               break;

            case ITEM_ARMOR:
               min_val = 0;
               max_val = TATP_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val3_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[3] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v3 to %d", obj->value[3] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[3] = obj->value[3];
         if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
            obj->armorgen(  );
         break;

      case OEDIT_VALUE_4:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_SALVE:
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               min_val = -1;
               max_val = num_skills - 1;
               break;

            case ITEM_FOOD:
               min_val = 0;
               max_val = 32000;
               break;

            case ITEM_WEAPON:
            case ITEM_MISSILE_WEAPON:
               min_val = 0;
               max_val = WEP_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val4_menu( d );
                  return;
               }
               break;

            case ITEM_ARMOR:
               min_val = 0;
               max_val = TMAT_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val4_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[4] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v4 to %d", obj->value[4] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[4] = obj->value[4];
         if( obj->item_type == ITEM_ARMOR && obj->value[3] > 0 && obj->value[4] > 0 )
            obj->armorgen(  );
         break;

      case OEDIT_VALUE_5:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_SALVE:
               if( !is_number( arg ) )
                  number = skill_lookup( arg );
               min_val = -1;
               max_val = num_skills - 1;
               break;

            case ITEM_MISSILE_WEAPON:
               min_val = 0;
               max_val = PROJ_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val5_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         if( obj->item_type == ITEM_CORPSE_PC )
         {
            olc_log( d, "Error - can't change skeleton value on corpses." );
            break;
         }
         obj->value[5] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v5 to %d", obj->value[5] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[5] = obj->value[5];
         break;

      case OEDIT_VALUE_6:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
            case ITEM_MISSILE_WEAPON:
               min_val = 0;
               max_val = sysdata->initcond;
               if( number < min_val || number > max_val )
                  return;
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[6] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v6 to %d", obj->value[6] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[6] = obj->value[6];
         break;

      case OEDIT_VALUE_7:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
               min_val = 0;
               max_val = 3;
               if( number < min_val || number > max_val )
                  return;
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[7] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v7 to %d", obj->value[7] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[7] = obj->value[7];
         break;

      case OEDIT_VALUE_8:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
               min_val = 0;
               max_val = TWTP_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val8_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[8] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v8 to %d", obj->value[8] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[8] = obj->value[8];
         if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
            obj->weapongen(  );
         break;

      case OEDIT_VALUE_9:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
               min_val = 0;
               max_val = TMAT_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val9_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[9] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v9 to %d", obj->value[9] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[9] = obj->value[9];
         if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
            obj->weapongen(  );
         break;

      case OEDIT_VALUE_10:
         number = atoi( arg.c_str(  ) );
         switch ( obj->item_type )
         {
            case ITEM_WEAPON:
               min_val = 0;
               max_val = TQUAL_MAX - 1;
               if( number < min_val || number > max_val )
               {
                  oedit_disp_val9_menu( d );
                  return;
               }
               break;

            default:
               min_val = -32000;
               max_val = 32000;
               break;
         }
         obj->value[10] = URANGE( min_val, number, max_val );
         olc_log( d, "Changed v10 to %d", obj->value[10] );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->value[10] = obj->value[10];
         if( obj->item_type == ITEM_WEAPON && obj->value[8] > 0 && obj->value[9] > 0 && obj->value[10] > 0 )
            obj->weapongen(  );
         break;

      case OEDIT_AFFECT_MENU:
         number = atoi( arg.c_str(  ) );

         switch ( arg[0] )
         {
            default:   /* if its a number, then its prolly for editing an affect */
               if( is_number( arg ) )
                  edit_object_affect( d, number );
               else
                  oedit_disp_prompt_apply_menu( d );
               return;

            case 'r':
            case 'R':
               /*
                * Chop off the 'R', if theres a number following use it, otherwise prompt for input 
                */
               arg = one_argument( arg, arg1 );
               if( !arg.empty(  ) )
               {
                  number = atoi( arg.c_str(  ) );
                  remove_affect_from_obj( obj, number );
                  oedit_disp_prompt_apply_menu( d );
               }
               else
               {
                  d->character->print( "Remove which affect? " );
                  OLC_MODE( d ) = OEDIT_AFFECT_REMOVE;
               }
               return;

            case 'a':
            case 'A':
               paf = new affect_data;
               d->character->pcdata->spare_ptr = paf;
               oedit_disp_affect_menu( d );
               return;

            case 'q':
            case 'Q':
               d->character->pcdata->spare_ptr = nullptr;
               break;
         }
         break;   /* If we reach here, we're done */

      case OEDIT_AFFECT_LOCATION:
         if( is_number( arg ) )
         {
            number = atoi( arg.c_str(  ) );
            if( number == 0 )
            {
               /*
                * Junk the affect 
                */
               d->character->pcdata->spare_ptr = nullptr;
               deleteptr( paf );
               break;
            }
         }
         else
            number = get_atype( arg );

         if( number < 0 || number >= MAX_APPLY_TYPE || number == APPLY_EXT_AFFECT )
         {
            d->character->print( "Invalid location, try again: " );
            return;
         }
         paf->location = number;
         OLC_MODE( d ) = OEDIT_AFFECT_MODIFIER;
         /*
          * Insert all special affect handling here ie: non numerical stuff 
          */
         /*
          * And add the apropriate case statement below 
          */
         if( number == APPLY_AFFECT )
         {
            d->character->tempnum = 0;
            medit_disp_aff_flags( d );
         }
         else if( number == APPLY_RESISTANT || number == APPLY_IMMUNE || number == APPLY_SUSCEPTIBLE )
         {
            d->character->tempnum = 0;
            medit_disp_ris( d );
         }
         else if( number == APPLY_WEAPONSPELL || number == APPLY_WEARSPELL || number == APPLY_REMOVESPELL )
            oedit_disp_spells_menu( d );
         else
            d->character->print( "\r\nModifier: " );
         return;

      case OEDIT_AFFECT_MODIFIER:
         switch ( paf->location )
         {
            case APPLY_AFFECT:
            case APPLY_RESISTANT:
            case APPLY_IMMUNE:
            case APPLY_SUSCEPTIBLE:
               if( is_number( arg ) )
               {
                  number = atoi( arg.c_str(  ) );
                  if( number == 0 )
                  {
                     value = d->character->tempnum;
                     break;
                  }
                  TOGGLE_BIT( d->character->tempnum, 1 << ( number - 1 ) );
               }
               else
               {
                  while( !arg.empty(  ) )
                  {
                     arg = one_argument( arg, arg1 );
                     if( paf->location == APPLY_AFFECT )
                        number = get_aflag( arg1 );
                     else
                        number = get_risflag( arg1 );
                     if( number < 0 )
                        d->character->printf( "Invalid flag: %s\r\n", arg1.c_str(  ) );
                     else
                        TOGGLE_BIT( d->character->tempnum, 1 << number );
                  }
               }
               if( paf->location == APPLY_AFFECT )
                  medit_disp_aff_flags( d );
               else
                  medit_disp_ris( d );
               return;

            case APPLY_WEAPONSPELL:
            case APPLY_WEARSPELL:
            case APPLY_REMOVESPELL:
               if( is_number( arg ) )
               {
                  number = atoi( arg.c_str(  ) );
                  if( IS_VALID_SN( number ) )
                     value = number;
                  else
                  {
                     d->character->print( "Invalid sn, try again: " );
                     return;
                  }
               }
               else
               {
                  value = find_spell( nullptr, arg, false );
                  if( value < 0 )
                  {
                     d->character->printf( "Invalid spell %s, try again: ", arg.c_str(  ) );
                     return;
                  }
               }
               break;

            default:
               value = atoi( arg.c_str(  ) );
               break;
         }
         /*
          * Link it in 
          */
         if( !value || OLC_VAL( d ) == true )
         {
            paf->modifier = value;
            olc_log( d, "Modified affect to: %s by %d", a_types[paf->location], value );
            OLC_VAL( d ) = false;
            oedit_disp_prompt_apply_menu( d );
            return;
         }
         npaf = new affect_data;
         npaf->type = -1;
         npaf->duration = -1;
         npaf->location = URANGE( 0, paf->location, MAX_APPLY_TYPE );
         npaf->modifier = value;
         npaf->bit = 0;

         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->affects.push_back( npaf );
         else
            obj->affects.push_back( npaf );
         ++top_affect;
         olc_log( d, "Added new affect: %s by %d", a_types[npaf->location], npaf->modifier );

         deleteptr( paf );
         d->character->pcdata->spare_ptr = nullptr;
         oedit_disp_prompt_apply_menu( d );
         return;

      case OEDIT_AFFECT_RIS:
         /*
          * Unnecessary atm 
          */
         number = atoi( arg.c_str(  ) );
         if( number < 0 || number > 31 )
         {
            d->character->print( "Unknown flag, try again: " );
            return;
         }
         return;

      case OEDIT_AFFECT_REMOVE:
         number = atoi( arg.c_str(  ) );
         remove_affect_from_obj( obj, number );
         olc_log( d, "Removed affect #%d", number );
         oedit_disp_prompt_apply_menu( d );
         return;

      case OEDIT_EXTRADESC_KEY:
         /*
          * if ( SetOExtra( obj, arg ) || SetOExtraProto( obj->pIndexData, arg ) )
          * {
          * d->character->print( "A extradesc with that keyword already exists.\r\n" );
          * oedit_disp_extradesc_menu(d);
          * return;
          * } 
          */
         olc_log( d, "Changed exdesc %s to %s", ed->keyword.c_str(  ), arg.c_str(  ) );
         ed->keyword = arg;
         oedit_disp_extra_choice( d );
         return;

      case OEDIT_EXTRADESC_DESCRIPTION:
         /*
          * Should never reach this 
          */
         bug( "%s: Reached OEDIT_EXTRADESC_DESCRIPTION", __func__ );
         break;

      case OEDIT_EXTRADESC_CHOICE:
         number = atoi( arg.c_str(  ) );
         switch ( number )
         {
            default:
            case 0:
               OLC_MODE( d ) = OEDIT_EXTRADESC_MENU;
               oedit_disp_extradesc_menu( d );
               return;

            case 1:
               OLC_MODE( d ) = OEDIT_EXTRADESC_KEY;
               d->character->print( "Enter keywords, speperated by spaces: " );
               return;

            case 2:
               OLC_MODE( d ) = OEDIT_EXTRADESC_DESCRIPTION;
               d->character->substate = SUB_OBJ_EXTRA;
               d->character->last_cmd = do_oedit_reset;

               d->character->print( "Enter new extra description - :\r\n" );
               d->character->editor_desc_printf( "Extra description for object vnum %d", obj->pIndexData->vnum );
               d->character->start_editing( ed->desc );
               return;
         }
         break;

      case OEDIT_EXTRADESC_DELETE:
         if( !( ed = oedit_find_extradesc( obj, atoi( arg.c_str(  ) ) ) ) )
         {
            d->character->print( "Extra description not found, try again: " );
            return;
         }
         olc_log( d, "Deleted exdesc %s", ed->keyword.c_str(  ) );
         if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
            obj->pIndexData->extradesc.remove( ed );
         else
            obj->extradesc.remove( ed );
         deleteptr( ed );
         --top_ed;
         oedit_disp_extradesc_menu( d );
         return;

      case OEDIT_EXTRADESC_MENU:
         switch ( UPPER( arg[0] ) )
         {
            case 'Q':
               break;

            case 'A':
               ed = new extra_descr_data;
               if( obj->extra_flags.test( ITEM_PROTOTYPE ) )
                  obj->pIndexData->extradesc.push_back( ed );
               else
                  obj->extradesc.push_back( ed );
               ++top_ed;
               d->character->pcdata->spare_ptr = ed;
               olc_log( d, "Added new exdesc" );
               oedit_disp_extra_choice( d );
               return;

            case 'R':
               OLC_MODE( d ) = OEDIT_EXTRADESC_DELETE;
               d->character->print( "Delete which extra description? " );
               return;

            default:
               if( is_number( arg ) )
               {
                  if( !( ed = oedit_find_extradesc( obj, atoi( arg.c_str(  ) ) ) ) )
                  {
                     d->character->print( "Not found, try again: " );
                     return;
                  }
                  d->character->pcdata->spare_ptr = ed;
                  oedit_disp_extra_choice( d );
               }
               else
                  oedit_disp_extradesc_menu( d );
               return;
         }
         break;

      default:
         bug( "%s: Reached default case!", __func__ );
         break;
   }

   /*
    * . If we get here, we have changed something .
    */
   OLC_CHANGE( d ) = true; /*. Has changed flag . */
   oedit_disp_menu( d );
}
