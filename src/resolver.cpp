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
 *                      External DNS Resolver Module                        *
 ****************************************************************************/

/***************************************************************************
 *                          SMC version 0.9.7b3                            *
 *          Additions to Rom2.3 (C) 1995, 1996 by Tom Adriaenssen          *
 *                                                                         *
 * Share and enjoy! But please give the original authors some credit.      *
 *                                                                         *
 * Ideas, tips, or comments can be send to:                                *
 *          tadriaen@zorro.ruca.ua.ac.be                                   *
 *          shadow@www.dma.be                                              *
 ***************************************************************************/

#include <netdb.h>
#include <iostream>
#include <string>

std::string resolve_address(std::string_view address) {
   if (address == "::1" || address == "127.0.0.1") return "localhost";

   addrinfo hints{}, *res = nullptr;
   hints.ai_family = AF_UNSPEC;
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_flags = AI_NUMERICHOST;

   // Attempt to parse the address
   if (getaddrinfo(address.data(), nullptr, &hints, &res) != 0) {
      return std::string(address); // Fail: return original string
   }

   char host[NI_MAXHOST];
   // Attempt reverse lookup
   int rc = getnameinfo(res->ai_addr, res->ai_addrlen,
                        host, sizeof(host), nullptr, 0, NI_NAMEREQD);

   freeaddrinfo(res);

   if (rc == 0) return std::string(host); // Success: return hostname
   return std::string(address);          // Fail: return original IP
}
int main(int argc, char* argv[]) {
   if (argc != 2) {
      std::cerr << "bad.resolver.call" << std::endl;
      return 1;
   }

   std::string result = resolve_address(argv[1]);

   std::cout << result << std::endl;

   return 0;
}
