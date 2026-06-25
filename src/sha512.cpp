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
 *                        SHA-512 Encryption Module                         *
 ****************************************************************************/

/*
 * Updated to C++, zedwood.com 2012
 * Based on Olivier Gay's version
 * See Modified BSD License below:
 *
 * FIPS 180-2 SHA-224/256/384/512 implementation
 * Issue date:  04/30/2005
 * http://www.ouah.org/ogay/sha2/
 *
 * Copyright (C) 2005, 2007 Olivier Gay <olivier.gay@a3.epfl.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <array>
#include <charconv>
#include <cstring>
#include <iomanip>
#include <fstream>
#include <random>
#include "sha512.h"

const unsigned long long SHA512::sha512_k[80] = //ULL = uint64
            {0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
             0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
             0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
             0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
             0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
             0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
             0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
             0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
             0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
             0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
             0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
             0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
             0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
             0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
             0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
             0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
             0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
             0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
             0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
             0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
             0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
             0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
             0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
             0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
             0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
             0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
             0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
             0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
             0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
             0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
             0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
             0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
             0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
             0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
             0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
             0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
             0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
             0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
             0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
             0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};

void SHA512::transform(const unsigned char *message, unsigned int block_nb)
{
    uint64 w[80];
    uint64 wv[8];
    uint64 t1, t2;
    const unsigned char *sub_block;
    int i, j;
    for (i = 0; i < (int) block_nb; i++) {
        sub_block = message + (i << 7);
        for (j = 0; j < 16; j++) {
            SHA2_PACK64(&sub_block[j << 3], &w[j]);
        }
        for (j = 16; j < 80; j++) {
            w[j] =  SHA512_F4(w[j -  2]) + w[j -  7] + SHA512_F3(w[j - 15]) + w[j - 16];
        }
        for (j = 0; j < 8; j++) {
            wv[j] = m_h[j];
        }
        for (j = 0; j < 80; j++) {
            t1 = wv[7] + SHA512_F2(wv[4]) + SHA2_CH(wv[4], wv[5], wv[6])
                + sha512_k[j] + w[j];
            t2 = SHA512_F1(wv[0]) + SHA2_MAJ(wv[0], wv[1], wv[2]);
            wv[7] = wv[6];
            wv[6] = wv[5];
            wv[5] = wv[4];
            wv[4] = wv[3] + t1;
            wv[3] = wv[2];
            wv[2] = wv[1];
            wv[1] = wv[0];
            wv[0] = t1 + t2;
        }
        for (j = 0; j < 8; j++) {
            m_h[j] += wv[j];
        }

    }
}

void SHA512::init()
{
    m_h[0] = 0x6a09e667f3bcc908ULL;
    m_h[1] = 0xbb67ae8584caa73bULL;
    m_h[2] = 0x3c6ef372fe94f82bULL;
    m_h[3] = 0xa54ff53a5f1d36f1ULL;
    m_h[4] = 0x510e527fade682d1ULL;
    m_h[5] = 0x9b05688c2b3e6c1fULL;
    m_h[6] = 0x1f83d9abfb41bd6bULL;
    m_h[7] = 0x5be0cd19137e2179ULL;
    m_len = 0;
    m_tot_len = 0;
}

void SHA512::update(const unsigned char *message, unsigned int len)
{
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;
    tmp_len = SHA384_512_BLOCK_SIZE - m_len;
    rem_len = len < tmp_len ? len : tmp_len;
    memcpy(&m_block[m_len], message, rem_len);
    if (m_len + len < SHA384_512_BLOCK_SIZE) {
        m_len += len;
        return;
    }
    new_len = len - rem_len;
    block_nb = new_len / SHA384_512_BLOCK_SIZE;
    shifted_message = message + rem_len;
    transform(m_block, 1);
    transform(shifted_message, block_nb);
    rem_len = new_len % SHA384_512_BLOCK_SIZE;
    memcpy(m_block, &shifted_message[block_nb << 7], rem_len);
    m_len = rem_len;
    m_tot_len += (block_nb + 1) << 7;
}

void SHA512::final(unsigned char *digest)
{
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;
    int i;
    block_nb = 1 + ((SHA384_512_BLOCK_SIZE - 17)
                     < (m_len % SHA384_512_BLOCK_SIZE));
    len_b = (m_tot_len + m_len) << 3;
    pm_len = block_nb << 7;
    memset(m_block + m_len, 0, pm_len - m_len);
    m_block[m_len] = 0x80;
    SHA2_UNPACK32(len_b, m_block + pm_len - 4);
    transform(m_block, block_nb);
    for (i = 0 ; i < 8; i++) {
        SHA2_UNPACK64(m_h[i], &digest[i << 3]);
    }
}

std::string sha512(std::string input)
{
    unsigned char digest[SHA512::DIGEST_SIZE];
    memset(digest,0,SHA512::DIGEST_SIZE);
    SHA512 ctx = SHA512();
    ctx.init();
    ctx.update((unsigned char*)input.c_str(), input.length());
    ctx.final(digest);

    char buf[2*SHA512::DIGEST_SIZE+1];
    buf[2*SHA512::DIGEST_SIZE] = 0;
    for (unsigned int i = 0; i < SHA512::DIGEST_SIZE; i++)
        sprintf(buf+i*2, "%02x", digest[i]);
    return std::string(buf);
}

// This section added by AFKMud on 6/14/2026 by Samson. Took Zedwood's advice and added "work factor" and salting.

constexpr int compute_rounds = 210000; // Modern baseline security.
extern std::mt19937 global_rng;

void binary_hmac_sha512( const uint8_t* key, size_t key_len, const uint8_t* msg, size_t msg_len, uint8_t* out_digest )
{
   uint8_t k_ipad[128] = {0};
   uint8_t k_opad[128] = {0};
   uint8_t hashed_key[64];

   if( key_len > 128 )
   {
      SHA512 ctx;
      ctx.init();
      ctx.update(key, key_len);
      ctx.final(hashed_key);
      key = hashed_key;
      key_len = 64;
   }

   std::memcpy( k_ipad, key, key_len );
   std::memcpy( k_opad, key, key_len );

   for( size_t i = 0; i < 128; ++i )
   {
      k_ipad[i] ^= 0x36;
      k_opad[i] ^= 0x5C;
   }

   SHA512 inner_ctx;
   inner_ctx.init();
   inner_ctx.update(k_ipad, 128);
   if( msg_len > 0 )
   {
      inner_ctx.update( msg, msg_len );
   }
   uint8_t inner_digest[64];
   inner_ctx.final(inner_digest);

   SHA512 outer_ctx;
   outer_ctx.init();
   outer_ctx.update( k_opad, 128 );
   outer_ctx.update( inner_digest, 64 );
   outer_ctx.final( out_digest );
}

std::string password_hash( std::string_view plaintext )
{
   constexpr std::string_view chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
   std::uniform_int_distribution<> dis( 0, chars.size() - 1 );

   // Generate the random salt, using the MUD's random number generator.
   char salt[16];
   for( int i = 0; i < 16; ++i )
   {
      salt[i] = chars[dis(global_rng)];
   }

   uint8_t salt_block[20];
   std::memcpy(salt_block, salt, 16);
   salt_block[16] = 0; salt_block[17] = 0; salt_block[18] = 0; salt_block[19] = 1;

   const uint8_t* pass_bytes = reinterpret_cast<const uint8_t*>( plaintext.data() );
   size_t pass_len = plaintext.length();

   uint8_t u[64];
   uint8_t t[64];

   // HMAC binary compilation.
   binary_hmac_sha512( pass_bytes, pass_len, salt_block, 20, u );
   std::memcpy( t, u, 64 );

   // Apply the work factor rounds.
   for( int i = 1; i < compute_rounds; ++i )
   {
      binary_hmac_sha512( pass_bytes, pass_len, u, 64, u );
      for( size_t j = 0; j < 64; ++j ) { t[j] ^= u[j]; }
   }

   // Format: $sha512$rounds$salt$hash
   // Size: 8 + rounds_str + 1 + 16 + 1 + 128 = ~155+ bytes
   std::string rounds_str = std::to_string( compute_rounds );
   std::string result;
   result.reserve( 160 + rounds_str.size() );

   result += "$sha512$";
   result += rounds_str;
   result += "$";
   result.append( salt, 16 );
   result += "$";

   const char hex_digits[] = "0123456789abcdef";
   for( int i = 0; i < 64; ++i )
   {
      result += hex_digits[(t[i] >> 4) & 0x0F];
      result += hex_digits[t[i] & 0x0F];
   }

   return result;
}

int get_rounds_from_hash( std::string_view hashed_string )
{
   if( !hashed_string.starts_with( "$sha512$" ) )
      return -1;

   size_t second_dollar = hashed_string.find( '$', 8 );
   if( second_dollar == std::string_view::npos )
      return -1;

   int check_rounds = 0;
   std::string_view rounds_sv = hashed_string.substr( 8, second_dollar - 8 );

   // Use std::from_chars for efficient, non-throwing conversion
   auto [ptr, ec] = std::from_chars( rounds_sv.data(), rounds_sv.data() + rounds_sv.size(), check_rounds );

   if( ec != std::errc() )
      return -1;

   return check_rounds;
}

// This may not be the most efficient thing, but it mimics the behavior of PHP's password_needs_rehash functionality.
std::string check_hash_update( std::string_view plaintext, std::string_view hashed_string )
{
   // Covers case of old pfile with SHA-256.
   if( !hashed_string.starts_with( "$sha512$" ) )
      return( password_hash( plaintext ) );

   // Covers case where the rounds check may have changed over time.
   if( get_rounds_from_hash( hashed_string ) != compute_rounds )
      return( password_hash( plaintext ) );

   // Return their original hashed string back since it doesn't need changing.
   return( std::string{hashed_string} );
}

bool password_verify( std::string_view plaintext, std::string_view hashed_string )
{
   if( !hashed_string.starts_with( "$sha512$" ) )
      return false;

   size_t second_dollar = hashed_string.find( '$', 8 );
   size_t third_dollar = hashed_string.find( '$', second_dollar + 1 );
   if( second_dollar == std::string_view::npos || third_dollar == std::string_view::npos )
      return false;

   int hashed_rounds = 0;
   auto rounds_sv = hashed_string.substr( 8, second_dollar - 8 );
   auto [ptr, ec] = std::from_chars( rounds_sv.data(), rounds_sv.data() + rounds_sv.size(), hashed_rounds );
   if( ec != std::errc() )
      return false;

   std::string_view salt = hashed_string.substr( second_dollar + 1, third_dollar - second_dollar - 1 );
   std::string_view expected_hex = hashed_string.substr( third_dollar + 1 );

   if( expected_hex.size() != 128 )
      return false;

   const uint8_t* pass_bytes = reinterpret_cast<const uint8_t*>( plaintext.data() );
   size_t pass_len = plaintext.length();

   uint8_t salt_block[132];
   size_t salt_len = salt.size();
   std::memcpy( salt_block, salt.data(), salt_len );
   salt_block[salt_len++] = 0; salt_block[salt_len++] = 0;
   salt_block[salt_len++] = 0; salt_block[salt_len++] = 1;

   uint8_t u[64];
   uint8_t t[64];

   binary_hmac_sha512( pass_bytes, pass_len, salt_block, salt_len, u );
   std::memcpy( t, u, 64 );

   for( int i = 1; i < hashed_rounds; ++i )
   {
      binary_hmac_sha512( pass_bytes, pass_len, u, 64, u );
      for( size_t j = 0; j < 64; ++j )
      {
         t[j] ^= u[j];
      }
   }

   auto hex_to_byte = [](char c) -> int {
      if( c >= '0' && c <= '9' )
         return c - '0';
      if( c >= 'a' && c <= 'f' )
         return c - 'a' + 10;
      return -1;
   };

   volatile int diff = 0;
   for( size_t i = 0; i < 64; ++i )
   {
      int hi = hex_to_byte( expected_hex[i * 2] );
      int lo = hex_to_byte( expected_hex[i * 2 + 1] );
      if( hi == -1 || lo == -1 )
         return false;

      uint8_t expected_byte = static_cast<uint8_t>( ( hi << 4 ) | lo );
      diff |= ( t[i] ^ expected_byte );
   }
   return diff == 0;
}
