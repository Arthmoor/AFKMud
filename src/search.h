#include <set>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Coordinate Subsystem
 *
 * Support to "index" a coordinate based layout.
 */

struct coordinate
{
   int x;
   int y;
   int z;

     coordinate(  );
     coordinate( int, int, int );

   void operator=( int pos[3] )
   {
      x = pos[0];
      y = pos[1];
      z = pos[2];
   }
};

coordinate operator+( const struct coordinate &, dir_types );
coordinate operator+( const struct coordinate &, const struct coordinate & );
coordinate operator-( const struct coordinate &, const struct coordinate & );

bool operator<( const struct coordinate &, const struct coordinate & );
bool operator<=( const struct coordinate &, const struct coordinate & );
bool operator>( const struct coordinate &, const struct coordinate & );
bool operator>=( const struct coordinate &, const struct coordinate & );
bool operator==( const struct coordinate &, const struct coordinate & );


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Frame
 *   Basic information for use by the search.
 *   Support relative coordinate.  (offset)
 */
class search_frame
{
 public:
   room_index * target;
   coordinate offset;
   double value;
   dir_types last_dir;

     search_frame(  );

   // Allow search callbacks to have "special" weights.
   // Prep for weighted searches...
   double udf[5];

   search_frame *make_exit( exit_data * );
   dir_types get_dir(  );
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Callback
 *   Generic search capability.
 */
class search_callback
{
 public:
   virtual ~ search_callback(  )
   {
   }

   virtual bool search( search_frame * ) = 0;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Breadth First Search (brute-force search)
 */
class search_BFS
{
 public:
   static void search( room_index *, search_callback *, long, bool );
   static void search( room_index *, search_callback *, long, bool, bool );
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Line of Sight
 */
class search_LOS
{
 public:
   static void search( room_index *, search_callback *, long, bool );
   static void search( room_index *, search_callback *, long, bool, bool );
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search in a single dir_types
 */
class search_DIR
{
 public:
   static void search( room_index *, search_callback *, enum dir_types, long, bool );
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Callbacks
 */
class srch_scan:public search_callback
{
 public:
   char_data * actor;
   int found;

     srch_scan( char_data * );
   virtual bool search( search_frame * );
};

class srch_map:public search_callback
{
 public:
   // 5x5 + terminator
   char buf[26];

     srch_map(  );
   virtual bool search( search_frame * );
};
