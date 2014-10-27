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

    coordinate();
    coordinate(int p_x, int p_y, int p_z);

    void operator=(int pos[3])
    {
        x = pos[0];
        y = pos[1];
        z = pos[2];
    }
};

coordinate operator+(const struct coordinate &coord, dir_types dir);
coordinate operator+(const struct coordinate &coorda, const struct coordinate &coordb);
coordinate operator-(const struct coordinate &coorda, const struct coordinate &coordb);

bool operator<(const struct coordinate &a, const struct coordinate &b);
bool operator<=(const struct coordinate &a, const struct coordinate &b);
bool operator>(const struct coordinate &a, const struct coordinate &b);
bool operator>=(const struct coordinate &a, const struct coordinate &b);
bool operator==(const struct coordinate &a, const struct coordinate &b);


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Frame
 *   Basic information for use by the search.
 *   Support relative coordinate.  (offset)
 */
class search_frame
{
public:
    room_index *target;
    coordinate offset;
    double     value;
    dir_types  last_dir;

    search_frame(void);

    // Allow search callbacks to have "special" weights.
    // Prep for weighted searches...
    double     udf[5];

    search_frame *make_exit(exit_data *exit);
    dir_types     get_dir(void);
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Callback
 *   Generic search capability.
 */
class search_callback
{
public:
    virtual bool search(search_frame *frame) = 0;
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Breadth First Search (brute-force search)
 */
class search_BFS
{
public:
    static void search(room_index *start, search_callback *callback, long max_dist, bool e_closed);
    static void search(room_index *start, search_callback *callback, long max_dist, bool e_closed, bool use_z);
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Line of Sight
 */
class search_LOS
{
public:
    static void search(room_index *start, search_callback *callback, long max_dist, bool e_closed);
    static void search(room_index *start, search_callback *callback, long max_dist, bool e_closed, bool use_z);
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search in a single dir_types
 */
class search_DIR
{
public:
    static void search(room_index *start, search_callback *callback, enum dir_types dir, long max_dist, bool e_closed);
};

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Callbacks
 */
class srch_scan : public search_callback
{
public:
    char_data *actor;
    int        found;

    srch_scan(char_data *p_actor);
    virtual bool search(search_frame *frame);
};

class srch_map : public search_callback
{
public:
    // 5x5 + terminator
    char buf[26];

    srch_map(void);
    virtual bool search(search_frame *frame);
};
