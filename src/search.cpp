#include <cstdio>

#include "mud.h"
#include "roomindex.h"
#include "search.h"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Coordinate
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
coordinate::coordinate()
{
    x = 0;
    y = 0;
    z = 0;
}

coordinate::coordinate(int p_x, int p_y, int p_z)
{
    x = p_x;
    y = p_y;
    z = p_z;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Coordinate Operators: direction
 */
coordinate operator+(const struct coordinate &coord, dir_types dir)
{
    coordinate ret = coord;

    switch(dir)
    {
    case DIR_NORTH:
        ret.y++;
        break;
    case DIR_EAST:
        ret.x++;
        break;
    case DIR_SOUTH:
        ret.y--;
        break;
    case DIR_WEST:
        ret.x--;
        break;
    case DIR_NORTHEAST:
        ret.x++;
        ret.y++;
        break;
    case DIR_SOUTHEAST:
        ret.x++;
        ret.y--;
        break;
    case DIR_SOUTHWEST:
        ret.x--;
        ret.y--;
        break;
    case DIR_NORTHWEST:
        ret.x--;
        ret.y++;
        break;
    case DIR_UP:
        ret.z++;
        break;
    case DIR_DOWN:
        ret.z--;
        break;

    default:
        break;
    }

    return ret;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Coordinate Operators: coordinate
 */
coordinate operator+(const struct coordinate &coorda, const struct coordinate &coordb)
{
    coordinate ret;
    
    ret.x = coorda.x+coordb.x;
    ret.y = coorda.y+coordb.y;
    ret.z = coorda.z+coordb.z;
    return ret;
}

coordinate operator-(const struct coordinate &coorda, const struct coordinate &coordb)
{
    coordinate ret;
    
    ret.x = coorda.x-coordb.x;
    ret.y = coorda.y-coordb.y;
    ret.z = coorda.z-coordb.z;
    return ret;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Coordinate Operators: comparison
 */
bool operator<(const struct coordinate &a, const struct coordinate &b)
{
    long aval, bval;

    // Handle Z here
    if (b.z < a.z)
        return true;
    if (b.z > a.z)
        return false;

    // Convert each xy into a long
    aval = a.x + (a.y << 16);
    bval = b.x + (b.y << 16);
    return (bval < aval);
}

bool operator<=(const struct coordinate &a, const struct coordinate &b)
{
    long aval, bval;

    // Handle Z here
    if (b.z < a.z)
        return true;
    if (b.z > a.z)
        return false;

    // Convert each xy into a long
    aval = a.x + (a.y << 16);
    bval = b.x + (b.y << 16);
    return (bval <= aval);
}

bool operator>(const struct coordinate &a, const struct coordinate &b)
{
    long aval, bval;

    // Handle Z here
    if (b.z < a.z)
        return false;
    if (b.z > a.z)
        return true;

    // Convert each xy into a long
    aval = a.x + (a.y << 16);
    bval = b.x + (b.y << 16);
    return (bval > aval);
}

bool operator>=(const struct coordinate &a, const struct coordinate &b)
{
    long aval, bval;

    // Handle Z here
    if (b.z < a.z)
        return false;
    if (b.z > a.z)
        return true;

    // Convert each xy into a long
    aval = a.x + (a.y << 16);
    bval = b.x + (b.y << 16);
    return (bval >= aval);
}

bool operator==(const struct coordinate &a, const struct coordinate &b)
{
    return (a.z == b.z && a.x == b.x && a.y == b.y);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Frame
 *
 * Used to represent a single node in the search tree.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
search_frame::search_frame(void)
{
    udf[0]   = 0;
    udf[1]   = 0;
    udf[2]   = 0;
    udf[3]   = 0;
    udf[4]   = 0;

    value    = 0;
    last_dir = DIR_SOMEWHERE;
    offset.x = 0;
    offset.y = 0;
    offset.z = 0;
}

search_frame *search_frame::make_exit(exit_data *exit)
{
    search_frame *ret = NULL;
    room_index *to = NULL;

    // Validate Exit
    if (exit->vdir == DIR_SOMEWHERE || !(to = exit->to_room))
        return NULL;

    // generate result
    ret           = new search_frame();
    ret->offset   = offset+(dir_types)exit->vdir;
    ret->last_dir = (dir_types)exit->vdir;
    ret->target   = to;
    ret->value    = value+1;

    return ret;
}

dir_types search_frame::get_dir(void)
{
    long x,y,z;

    x = offset.x;
    y = offset.y;
    z = offset.z;

    if (x == 0 && y == 0 && z == 0)
        return DIR_SOMEWHERE;

    if (z != 0)
    {
        if (z > 0)
            return DIR_UP;
        return DIR_DOWN;
    }

    if (x == 0)
    {
        if (y > 0)
            return DIR_NORTH;
        return DIR_SOUTH;
    }

    if (y == 0)
    {
        if (x > 0)
            return DIR_EAST;
        return DIR_WEST;
    }

    if (x > 0)
    {
        if (y > 0)
            return DIR_NORTHEAST;
        return DIR_SOUTHEAST;
    }

    if (y > 0)
        return DIR_NORTHWEST;
    return DIR_SOUTHWEST;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Algorithms
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void search_BFS::search(room_index *start, search_callback *callback, long max_dist, bool e_closed)
{
    search(start, callback, max_dist, e_closed, false);
}

void search_BFS::search(room_index *start, search_callback *callback, long max_dist, bool e_closed, bool use_z)
{
    std::list<search_frame*> open;   // Frame Queue
    std::set<long>           closed; // Visited Rooms

    search_frame *frame, *tmp_frame; // Frames
    exit_data *pexit;                // Exit
    list<exit_data*>::iterator ex;

    long cnt;
    bool found = false;

    // Initialize
    cnt             = 1;
    frame           = new search_frame();
    frame->value    = 0;
    frame->target   = start;
    frame->last_dir = DIR_SOMEWHERE;
    open.push_back(frame);
    closed.insert(frame->target->vnum);

    // Search Loop
    while(!found && cnt)
    {
        --cnt;
        frame = open.front();
        open.pop_front();

        // Search Exits
        for (ex = frame->target->exits.begin(); ex != frame->target->exits.end(); ++ex)
        {
            pexit = *ex;

            if (!pexit)
                continue;
            // No "other" dir_types...
            if (pexit->vdir == DIR_SOMEWHERE)
                continue;

            // No "Z" search
            if (!use_z && (pexit->vdir == DIR_UP || pexit->vdir == DIR_DOWN))
                continue;

            // Don't pass through e_closed.
            if (!e_closed && IS_EXIT_FLAG(pexit, EX_CLOSED))
                continue;

            tmp_frame = frame->make_exit(pexit);

            // The exit was unsuitable for searching.
            if (!tmp_frame)
                continue;

            // Too Far
            if (tmp_frame->value >= max_dist)
            {
                delete tmp_frame;
                continue;
            }

            // Already Visited
            if (closed.find(tmp_frame->target->vnum) != closed.end())
            {
                delete tmp_frame;
                continue;
            }

            // Handle Callback
            if (callback->search(tmp_frame))
            {
                delete tmp_frame;
                found = true;
                break;
            }

            cnt++;
            // Update Open and Closed
            open.push_back(tmp_frame);
            closed.insert(tmp_frame->target->vnum);
        }

        // finally, cleanup this frame
        delete frame;
    }

    // cleanup any remaining frames.
    while (open.size() > 0)
    {
        frame = open.front();
        open.pop_front();
        delete frame;
    }
}

void search_LOS::search(room_index *start, search_callback *callback, long max_dist, bool e_closed)
{
    search(start, callback, max_dist, e_closed, false);
}

void search_LOS::search(room_index *start, search_callback *callback, long max_dist, bool e_closed, bool use_z)
{
    list<exit_data*>::iterator ex;
    room_index *from, *to;
    exit_data *pexit, *tmp;
    dir_types  dir;
    search_frame *frame;
    bool found = false;

    // Flyweight Frame
    frame = new search_frame();

    // Exit Loop (search for appropriate dir_typess)
    for (ex = start->exits.begin(); ex != start->exits.end(); ++ex)
    {
        pexit = *ex;

        dir = (dir_types)pexit->vdir;

        // No "other" dir_types...
        if (pexit->vdir == DIR_SOMEWHERE)
            continue;

        if (!use_z && (dir == DIR_UP || dir == DIR_DOWN))
            continue;

        // Don't pass through e_closed.
        if (!e_closed && IS_EXIT_FLAG(pexit, EX_CLOSED))
            continue;

        // Reset Frame
        frame->offset.x = 0;
        frame->offset.y = 0;
        frame->offset.z = 0;
        frame->value    = 0.00;
        frame->last_dir = dir;

        // Init
        from = start;

        // Handle the distance
        while(++frame->value <= max_dist)
        {
            // No exit?
            if (!(tmp = from->get_exit(dir)))
                break;

            // Exit closed?
            if (!e_closed && IS_EXIT_FLAG(tmp, EX_CLOSED))
                break;

            // No target?
            if (!(to = tmp->to_room))
                break;

            // Update Frame
            frame->target = to;
            frame->offset = frame->offset + dir;

            // Found
            if (callback->search(frame))
            {
                found = true;
                break;
            }
            from = to;
        }
        if (found)
            break;
    }

    // Cleanup
    delete frame;
}

void search_DIR::search(room_index *start, search_callback *callback, dir_types dir, long max_dist, bool e_closed)
{
    room_index *from, *to;
    exit_data *pexit;
    search_frame *frame;
    bool found = false;

    // Flyweight Frame
    frame = new search_frame();

    // No exit
    if (!(pexit = start->get_exit(dir)))
        return;

    // Don't pass through e_closed.
    if (!e_closed && IS_EXIT_FLAG(pexit, EX_CLOSED))
        return;

    // Reset Frame
    frame->offset.x = 0;
    frame->offset.y = 0;
    frame->offset.z = 0;
    frame->value    = 0.00;
    frame->last_dir = dir;

    // Init
    from = start;

    // Handle the distance
    while(++frame->value <= max_dist)
    {
        // No exit?
        if (!(pexit = from->get_exit(dir)))
            break;

        // Exit closed?
        if (!e_closed && IS_EXIT_FLAG(pexit, EX_CLOSED))
            break;

        // No target?
        if (!(to = pexit->to_room))
            break;

        // Update Frame
        frame->target = to;
        frame->offset = frame->offset + dir;

        // Found
        if (callback->search(frame))
        {
            found = true;
            break;
        }
        from = to;
    }

    // Cleanup
    delete frame;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Search Callbacks
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Scan
 */
CMDF( do_scan2 )
{
    srch_scan cb(ch);
    char arg[MIL];
    dir_types dir;

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "BFS"))
    {
        ch->print("Using BFS Search:\r\n");
        search_BFS::search(ch->in_room, &cb, 4, false);
    }
    else if (!str_cmp(arg, "DIR"))
    {
        ch->print("Using DIR Search:\r\n");

        dir = (dir_types)get_dir(argument);
        search_DIR::search(ch->in_room, &cb, dir, 4, false);
    }
    else
    {
        ch->print("Using LOS Search:\r\n");
        search_LOS::search(ch->in_room, &cb, 4, false);
    }

    if (!cb.found)
        ch->print("Nobody.\r\n");
}

srch_scan::srch_scan(char_data *p_actor)
{
    actor = p_actor;
    found = 0;
}

bool srch_scan::search(search_frame *frame)
{
    char_data *target;
    list<char_data*>::iterator ich;

    if( frame->target->people.empty() )
        return false;

    actor->printf("  %d %s\r\n", (int)frame->value, dir_name[frame->get_dir()]);
    actor->print("--------------------\r\n");

    for (ich = frame->target->people.begin(); ich != frame->target->people.end(); ++ich)
    {
        target = *ich;

        if (target == actor)
            continue;
        actor->printf("%s\r\n", target->short_descr);
        ++found;
    }
    return false;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Map
 */
CMDF( do_map )
{
    srch_map cb;
    char arg[MIL];
    dir_types dir;

    argument = one_argument(argument, arg);

    if (!str_cmp(arg, "BFS"))
    {
        ch->print("Using BFS Search:\r\n");
        search_BFS::search(ch->in_room, &cb, 4, false);
    }
    else if (!str_cmp(arg, "DIR"))
    {
        ch->print("Using DIR Search:\r\n");

        dir = (dir_types)get_dir(argument);
        search_DIR::search(ch->in_room, &cb, dir, 4, false);
    }
    else
    {
        ch->print("Using LOS Search:\r\n");
        search_LOS::search(ch->in_room, &cb, 4, false);
    }

    // Output map
    ch->printf( "-------\r\n|%-5.5s|\r\n|%-5.5s|\r\n|%-5.5s|\r\n|%-5.5s|\r\n|%-5.5s|\r\n-------\r\n",
        cb.buf, cb.buf+5, cb.buf+10, cb.buf+15, cb.buf+20);
}

srch_map::srch_map(void)
{
    for (int i=0; i<25; i++)
        buf[i] = ' ';
    buf[12] = '#';
}

bool srch_map::search(search_frame *frame)
{
    int x,y,i;

    // No "Z" movement allowed
    if (frame->offset.z)
        return false;

    // Localize
    x = frame->offset.x;
    y = frame->offset.y;

    // X/Y constraints
    if (x > 2 || x < -2)
        return false;
    if (y > 2 || y < -2)
        return false;

    // Normalize (0 to 4 instead of -2 to 2)
    x += 2;
    y += 2;

    // Generate array position.
    // Flip the y (so north is up, south is down)
    i  = ((4-y)*5)+(x%5);

    // Already found
    if (buf[i] != ' ')
        return false;

    // Update map
    buf[i] = '.';
    return false;
}
