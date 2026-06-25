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
 *                  Noplex's Immhost Verification Module                    *
 ****************************************************************************/

/*******************************************************
		Crimson Blade Codebase
	Copyright 2000-2002 Noplex (John Bellone)
		admin@crimsonblade.org
		Coders: Noplex, Krowe
		 Based on Smaug 1.4a
*******************************************************/

/*
======================
Advanced Immortal Host
======================
By Noplex with help from Senir and Samson
*/

#pragma once

constexpr int MAX_DOMAIN = 10;

class immortal_host_log
{
 private:
   immortal_host_log( const immortal_host_log & i );
     immortal_host_log & operator=( const immortal_host_log & );

 public:
     immortal_host_log(  );
    ~immortal_host_log(  );

   std::string host; // Host address of the login attempt.
   std::string date; // Time the attempt took place.
};

class immortal_host
{
 private:
   immortal_host( const immortal_host & i );
     immortal_host & operator=( const immortal_host & );

 public:
     immortal_host(  );
    ~immortal_host(  );

   std::list<immortal_host_log *> loglist; // List of logs for this host.
   std::string domain[MAX_DOMAIN];         // List of valid domains for this host.
   std::string name;                       // Name of the person this host is for.
};
