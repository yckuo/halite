#ifndef HLT_H
#define HLT_H

#include <list>
#include <vector>
#include <random>

#define STILL 0
#define NORTH 1
#define EAST 2
#define SOUTH 3
#define WEST 4

const int DIRECTIONS[] = {STILL, NORTH, EAST, SOUTH, WEST};
const int CARDINALS[] = {NORTH, EAST, SOUTH, WEST};

namespace hlt{
    struct Location{
        unsigned short x, y;
        bool operator == (const Location& loc) const {
            return x == loc.x && y == loc.y;
        }
        bool operator != (const Location& loc) const {
            return x != loc.x || y != loc.y;
        }
    };

    struct LocationHasher {
        bool operator() (const Location& loc) const {
            return loc.x * 10000 + loc.y;
        }
    };

    struct LocationComparer {
        bool operator() (const Location& loc1, const Location& loc2) const {
            return loc1 == loc2;
        }
    };

    static bool operator<(const Location& l1, const Location& l2) {
        return ((l1.x + l1.y)*((unsigned int)l1.x + l1.y + 1) / 2) + l1.y < ((l2.x + l2.y)*((unsigned int)l2.x + l2.y + 1) / 2) + l2.y;
    }

    struct Site{
        unsigned char owner;
        unsigned char strength;
        unsigned char production;
    };

    unsigned char opposite(unsigned char D) {
        if (D == NORTH) return SOUTH;
        if (D == SOUTH) return NORTH;
        if (D == WEST) return EAST;
        if (D == EAST) return WEST;
        return STILL;
    }

    const double pi = 3.1415926;

    unsigned char angle2Direction(float angle) {
        if (angle >= 0.25 * pi && angle <= 0.75 * pi) return NORTH;
        if (angle > 0.75 * pi || angle < -0.75 * pi) return WEST;
        if (angle <= -0.25 * pi && angle >= -0.75 * pi) return SOUTH;
        if (angle > -0.25 * pi && angle < 0.25 * pi) return EAST;
        return STILL;
    }

    class GameMap{
    public:
        std::vector< std::vector<Site> > contents;
        unsigned short width, height; //Number of rows & columns, NOT maximum index.

        GameMap() {
            width = 0;
            height = 0;
            contents = std::vector< std::vector<Site> >(height, std::vector<Site>(width, { 0, 0, 0 }));
        }
        GameMap(const GameMap &otherMap) {
            width = otherMap.width;
            height = otherMap.height;
            contents = otherMap.contents;
        }
        GameMap(int w, int h) {
            width = w;
            height = h;
            contents = std::vector< std::vector<Site> >(height, std::vector<Site>(width, { 0, 0, 0 }));
        }

        bool inBounds(Location l) {
            return l.x < width && l.y < height;
        }
        float getDistance(Location l1, Location l2) {
            short dx = abs(l1.x - l2.x), dy = abs(l1.y - l2.y);
            if(dx > width / 2) dx = width - dx;
            if(dy > height / 2) dy = height - dy;
            return dx + dy;
        }
        float getAngle(Location l1, Location l2) {
            short dx = l2.x - l1.x, dy = l2.y - l1.y;
            if(dx > width - dx) dx -= width;
            else if(-dx > width + dx) dx += width;
            if(dy > height - dy) dy -= height;
            else if(-dy > height + dy) dy += height;
            return atan2(dy, dx);
        }

        Location getLocation(Location l, unsigned char direction) {
            if(direction != STILL) {
                if(direction == NORTH) {
                    if(l.y == 0) l.y = height - 1;
                    else l.y--;
                }
                else if(direction == EAST) {
                    if(l.x == width - 1) l.x = 0;
                    else l.x++;
                }
                else if(direction == SOUTH) {
                    if(l.y == height - 1) l.y = 0;
                    else l.y++;
                }
                else if(direction == WEST) {
                    if(l.x == 0) l.x = width - 1;
                    else l.x--;
                }
            }
            return l;
        }
        Site& getSite(Location l, unsigned char direction = STILL) {
            l = getLocation(l, direction);
            return contents[l.y][l.x];
        }
    };

    struct Move{
        Location loc; unsigned char dir;
    };
    static bool operator<(const Move& m1, const Move& m2) {
        unsigned int l1Prod = ((m1.loc.x + m1.loc.y)*((unsigned int)m1.loc.x + m1.loc.y + 1) / 2) + m1.loc.y, l2Prod = ((m2.loc.x + m2.loc.y)*((unsigned int)m2.loc.x + m2.loc.y + 1) / 2) + m2.loc.y;
        return ((l1Prod + m1.dir)*(l1Prod + m1.dir + 1) / 2) + m1.dir < ((l2Prod + m2.dir)*(l2Prod + m2.dir + 1) / 2) + m2.dir;
    }
}

#endif
