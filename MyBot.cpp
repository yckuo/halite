#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <climits>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <functional>
#include <stack>
#include "hlt.hpp"
#include "networking.hpp"
#include "algorithm.hpp"
using namespace std;
using namespace hlt;
using namespace algorithm;

GameMap presentMap;
unsigned char myID;
unordered_map<Location, Location, LocationHasher, LocationComparer> moved;
unordered_set<Location, LocationHasher, LocationComparer> killed;
set<Move> moves;

void move(const Location& loc, unsigned char D, bool kill) {
    moves.insert({loc, D});
    Location target = presentMap.getLocation(loc, D);
    moved[loc] = target;
    if (kill) killed.insert(target);
}

int main() { 
    srand(time(NULL));

    cout.sync_with_stdio(0);

    ofstream log;
    log.open("log.txt");

    getInit(myID, presentMap);

    sendInit("yckuoBot");

    Search search(myID);
    
    int frame = 0;
    while (true) {
        log << "frame " << ++frame << endl;
        moved.clear();
        moves.clear();
        killed.clear();
        getFrame(presentMap);

        // 1) Handle expansion using a single piece at a time. We look at each of the border pieces.
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                if (!search.IsBorder(presentMap, loc)) continue;

                Site site = presentMap.getSite(loc);

                // Find all survival neighbors sorted by production
                vector<unsigned char> survivals = search.neighbors(presentMap, loc, true);

                if (!survivals.empty()) {
                    move(loc, survivals[0], true);
                    continue;
                }

                // TODO: first combine pieces for expansion with guaranteed survival?

                // Next, try a neighbor site not owned by me, where we can kill someone, with the largest profit.
                // Individual suicide mission.
                vector<unsigned char> killables = search.neighbors(presentMap, loc, false);
                
                if (!killables.empty()) {
                    move(loc, killables[0], true); // BUG: not necessarily killed
                }
            }
        }

        // 2) Handle expansion using multiple pieces. We look at each of the remaining enemy pieces.
        // Multi - suicide mission
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                Site site = presentMap.getSite(loc);
                if (site.owner == myID || killed.count(loc)) continue;

                unordered_set<Location, LocationHasher, LocationComparer> requireds = search.requireds(presentMap, loc, moved);
                for (unsigned char D : CARDINALS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    if (requireds.count(nloc)) {
                        move(nloc, opposite(D), true);
                    }
                }
            }
        }
        
        
        // 3) Look at remaining nodes. Only move internally.
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };

                /*bool isBorder = false;
                for (unsigned char D : CARDINALS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(loc, D);
                    if (nsite.owner != myID && !killed.count(nloc)) {
                        isBorder = true;
                    }
                }
                if (isBorder) continue;*/
                if (search.IsBorder(presentMap, loc)) continue;

                Site site = presentMap.getSite(loc);
                if (site.owner != myID || moved.count(loc)) continue;

//              if (site.strength < site.production * 5) continue;


                bool go = false;
                int r1 = rand() % 100, r2 = (int)site.strength;
                if (r1 < r2) go = true;
                if (!go) continue;

                // unsigned char bestD = search.dijkstra(presentMap, loc);
   /*             
                Location target = presentMap.getLocation(loc, bestD);
                Site movedite = presentMap.getSite(target);

                if (bestD != STILL && movedite.owner == myID) {
                    moves.insert({loc, bestD});
                    moved[loc] = target;
                    continue;
                }
*/

                unsigned char spread = search.spread(presentMap, loc);
                if (spread == STILL) continue;

                Location target = presentMap.getLocation(loc, spread);
                Site targetSite = presentMap.getSite(target);
                if (targetSite.owner != myID) continue; // only move internally
                move(loc, spread, false);
                
            }
        }

        log << endl;

        sendFrame(moves);
    }

    log.close();

    return 0;
}
