/****************************************************************************
 *             ___________.__               .__                             *
 *             \_   _____/|  | ___.__. _____|__|__ __  _____                *
 *              |    __)_ |  |<   |  |/  ___/  |  |  \/     \               *
 *              |        \|  |_\___  |\___ \|  |  |  /  Y Y  \              *
 *             /_______  /|____/ ____/____  >__|____/|__|_|  /              *
 *                     \/      \/         \/     Engine    \/               *
 *                       A SMAUG Derived Game Engine.                       *
 * ------------------------------------------------------------------------ *
 * Elysium Engine Copyright 1999-2010 by Steven Loar                        *
 * Elysium Engine Development Team: Kayle (Steven Loar), Katiara, Scoyn,    *
 *                                  and Mikon.                              *
 * ------------------------------------------------------------------------ *
 *                   ^     +----- |  / ^     ^ |     | +-\                  *
 *                  / \    |      | /  |\   /| |     | |  \                 *
 *                 /   \   +---   |<   | \ / | |     | |  |                 *
 *                /-----\  |      | \  |  v  | |     | |  /                 *
 *               /       \ |      |  \ |     | +-----+ +-/                  *
 * ------------------------------------------------------------------------ *
 * AFKMud Copyright 1997-2025 by Roger Libiez (Samson),                     *
 * Levi Beckerson (Whir), Michael Ward (Tarl), Erik Wolfe (Dwip),           *
 * Cameron Carroll (Cam), Cyberfox, Karangi, Rathian, Raine,                *
 * Xorith, and Adjani.                                                      *
 * All Rights Reserved.                                                     *
 *                                                                          *
 * External contributions from Remcon, Quixadhal, Zarius, Kayle,            *
 * and many others.                                                         *
 *                                                                          *
 * Original SMAUG 1.8b written by Thoric (Derek Snider) with Altrag,        *
 * Blodkai, Haus, Narn, Scryn, Swordbearer, Tricops, Gorog, Rennard,        *
 * Grishnakh, Fireblade, Edmond, Conran, and Nivek.                         *
 * ------------------------------------------------------------------------ *
 * Merc 2.1 Diku Mud improvments copyright (C) 1992, 1993 by Michael        *
 * Chastain, Michael Quan, and Mitchell Tse.                                *
 *                                                                          *
 * Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,          *
 * Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.     *
 * ------------------------------------------------------------------------ *
 *                         Weather System Header                            *
 ****************************************************************************
 *           Base Weather Model Copyright (c) 2007 Chris Jacobson           *
 ****************************************************************************/

#ifndef __WEATHER_H__
#define __WEATHER_H__

/*
 * This might not have all the bells and whistles I'd intended, and it might be 
 * missing some of the things people wanted. But I did the best I could, and it's
 * sure as hell better than the crap that used to be there. -Kayle
 */

//Change these values to expand or contract your weather map according to your world size.
const int WEATHER_SIZE_X      = 3; //number of cells wide
const int WEATHER_SIZE_Y      = 3; //number of cells tall

//Hemisphere defines.
const int HEMISPHERE_NORTH    = 0;
const int HEMISPHERE_SOUTH    = 1;
const int HEMISPHERE_MAX      = 2;

//Climate defines - Add more if you want, but make sure you add appropriate data to the
// system itself in EnforceClimateConditions()
const int CLIMATE_RAINFOREST  = 0;
const int CLIMATE_SAVANNA     = 1;
const int CLIMATE_DESERT      = 2;
const int CLIMATE_STEPPE      = 3;
const int CLIMATE_CHAPPARAL   = 4;
const int CLIMATE_GRASSLANDS  = 5;
const int CLIMATE_DECIDUOUS   = 6;
const int CLIMATE_TAIGA       = 7;
const int CLIMATE_TUNDRA      = 8;
const int CLIMATE_ALPINE      = 9;
const int CLIMATE_ARCTIC      = 10;
const int MAX_CLIMATE         = 11;

class WeatherCell
{
 private:
   WeatherCell( const WeatherCell & p );
     WeatherCell & operator=( const WeatherCell & );

 public:
     WeatherCell(  );
    ~WeatherCell(  );

   int climate;        // Climate flag for the cell
   int hemisphere;     // Hemisphere flag for the cell
   int temperature;    // Fahrenheit because I'm American, by god
   int pressure;       // 0..100 for now, later change to barometric pressures
   int cloudcover;     // 0..100, amount of clouds in the sky
   int humidity;       // 0+
   int precipitation;  // 0..100
   int energy;         // 0..100 Storm Energy, chance of storm.
   /*
   *  Instead of a wind direction we use an X/Y speed
   *  It makes the math below much simpler this way.
   *  Its not hard to determine a basic cardinal direction from this
   *  If you want to, a good rule of thumb is that if one directional
   *  speed is more than double that of the other, ignore it; that is
   *  if you have speed X = 15 and speed Y = 3, the wind is obviously
   *  to the east.  If X = 15 and Y = 10, then its a south-east wind. 
   */
   int windSpeedX;    //  < 0 = west, > 0 = east
   int windSpeedY;    //  < 0 = north, > 0 = south
};

// File that stores Weather Information 
#define WEATHER_FILE "weathermap.dat"

//So it can be utilized from other parts of the code
extern WeatherCell weatherMap[WEATHER_SIZE_X][WEATHER_SIZE_Y];
extern WeatherCell weatherDelta[WEATHER_SIZE_X][WEATHER_SIZE_Y];

//Defines
void InitializeWeatherMap( void );
void UpdateWeather( void );
void RandomizeCells( void );
void save_weathermap( void );
bool load_weathermap( void );

int get_hemisphere( char *type );
int get_climate( char *type );

WeatherCell *getWeatherCell( area_data *pArea );

void IncreaseTemp( WeatherCell *cell, int change );
void DecreaseTemp( WeatherCell *cell, int change );
void IncreasePrecip( WeatherCell *cell, int change );
void DecreasePrecip( WeatherCell *cell, int change );
void IncreasePressure( WeatherCell *cell, int change );
void DecreasePressure( WeatherCell *cell, int change );
void IncreaseEnergy( WeatherCell *cell, int change );
void DecreaseEnergy( WeatherCell *cell, int change );
void IncreaseCloudCover( WeatherCell *cell, int change );
void DecreaseCloudCover( WeatherCell *cell, int change );
void IncreaseHumidity( WeatherCell *cell, int change );
void DecreaseHumidity( WeatherCell *cell, int change );
void IncreaseWindX( WeatherCell *cell, int change );
void DecreaseWindX( WeatherCell *cell, int change );
void IncreaseWindY( WeatherCell *cell, int change );
void DecreaseWindY( WeatherCell *cell, int change );

int getCloudCover( WeatherCell *cell );
int getTemp( WeatherCell *cell );
int getEnergy( WeatherCell *cell );
int getPressure( WeatherCell *cell );
int getHumidity( WeatherCell *cell );
int getPrecip( WeatherCell *cell );
int getWindX( WeatherCell *cell );
int getWindY( WeatherCell *cell );

bool isExtremelyCloudy( int cloudCover );
bool isModeratelyCloudy( int cloudCover );
bool isPartlyCloudy( int cloudCover );
bool isCloudy( int cloudCover );
bool isSwelteringHeat( int temp );
bool isVeryHot( int temp );
bool isHot( int temp );
bool isWarm( int temp );
bool isTemperate( int temp );
bool isCool( int temp );
bool isChilly( int temp );
bool isCold( int temp );
bool isFrosty( int temp );
bool isFreezing( int temp );
bool isReallyCold( int temp );
bool isVeryCold( int temp );
bool isExtremelyCold( int temp );
bool isStormy( int energy );
bool isHighPressure( int pressure );
bool isLowPressure( int pressure );
bool isExtremelyHumid( int humidity );
bool isModeratelyHumid( int humidity );
bool isMinorlyHumid( int humidity );
bool isHumid( int humidity );
bool isTorrentialDownpour( int precip );
bool isRainingCatsAndDogs( int precip );
bool isPouring( int precip );
bool isRaingingHeavily( int precip );
bool isDownpour( int precip );
bool isRainingSteadily( int precip );
bool isRaining( int precip );
bool isRainingLightly( int precip );
bool isDrizzling( int precip );
bool isMisting( int precip );
bool isCalmWindE( int windx );
bool isBreezyWindE( int windx );
bool isBlusteryWindE( int windx );
bool isWindyWindE( int windx );
bool isGustyWindE( int windx );
bool isGaleForceWindE( int windx );
bool isCalmWindW( int windx );
bool isBreezyWindW( int windx );
bool isBlusteryWindW( int windx );
bool isWindyWindW( int windx );
bool isGustyWindW( int windx );
bool isGaleForceWindW( int windx );
bool isCalmWindN( int windy );
bool isBreezyWindN( int windy );
bool isBlusteryWindN( int windy );
bool isWindyWindN( int windy );
bool isGustyWindN( int windy );
bool isGaleForceWindN( int windy );
bool isCalmWindS( int windy );
bool isBreezyWindS( int windy );
bool isBlusteryWindS( int windy );
bool isWindyWindS( int windy );
bool isGustyWindS( int windy );
bool isGaleForceWindS( int windy );
#endif
