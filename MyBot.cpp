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
    
    bool war = false;
    int frame = 0;
    while (true) {
        //log << "frame " << ++frame << endl;
        moved.clear();
        moves.clear();
        killed.clear();
        getFrame(presentMap);

        // 1) Handle expansion using a single piece at a time. We look at each of the border pieces.
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                if (!search.IsBorder(presentMap, loc)) continue;
                search.CheckWar(presentMap, loc, war);

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

                // if (search.IsBorder(presentMap, loc)) continue;

                Site site = presentMap.getSite(loc);
                if (site.owner != myID || moved.count(loc)) continue;


                //if (!war) {
                //    if ((int)site.strength < (int)site.production * 10) continue;
                //} else {
            //        if ((rand() % 100) >= (int)site.strength) continue;
                //}
                //
                if ((rand() % 100) >= (int)site.strength) continue;

                int dist = 0;
                unsigned char spread = search.spread(presentMap, loc, dist);
                if (spread != STILL && dist > 5) {  
                    Location target = presentMap.getLocation(loc, spread);
                    Site targetSite = presentMap.getSite(target);
                    if (targetSite.owner != myID) continue; // only move internally
                    move(loc, spread, false);
                    continue;
                }


                unsigned char dijkstra = search.dijkstra(presentMap, loc, log);
                if (dijkstra != STILL) {
                    Location target = presentMap.getLocation(loc, dijkstra);
                    Site targetSite = presentMap.getSite(target);
                    if (targetSite.owner != myID) continue;

                    for (unsigned char D : CARDINALS) {
                        Location nloc = presentMap.getLocation(target, D);
                        Site nsite = presentMap.getSite(nloc);
                        if (moved.count(nloc) && moved[nloc] == target) {
                            if (nsite.strength + site.strength > 255) continue;
                        }
                    }

                    move(loc, dijkstra, false);
                    continue;
                }

                if (spread != STILL) {
                    Location target = presentMap.getLocation(loc, spread);
                    Site targetSite = presentMap.getSite(target);
                    if (targetSite.owner != myID) continue; // only move internally
                    move(loc, spread, false);
                }
                
            }
        }

        //log << endl;

        sendFrame(moves);
    }

    log.close();

    return 0;
}
