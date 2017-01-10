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

const double pi = 3.1415926;
GameMap presentMap;
unsigned char myID;

unsigned char angle2Direction(float angle) {
    if (angle >= 0.25 * pi && angle <= 0.75 * pi) return NORTH;
    if (angle > 0.75 * pi || angle < -0.75 * pi) return WEST;
    if (angle <= -0.25 * pi && angle >= -0.75 * pi) return SOUTH;
    if (angle > -0.25 * pi && angle < 0.25 * pi) return EAST;
    return STILL;
}

int main() { 
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    ofstream log;
    log.open("log.txt");

    getInit(myID, presentMap);

    sendInit("yckuoBot");

    unordered_map<Location, Location, LocationHasher, LocationComparer> targets;

    Search search;

    std::set<hlt::Move> moves;
    int frame = 0;
    while (true) {
        log << "frame " << ++frame << endl;
        targets.clear();

        moves.clear();

        getFrame(presentMap);

        unordered_set<Location, LocationHasher, LocationComparer> killed;

        // 1) Handle expansion using a single piece at a time. We look at each of the border pieces.
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                Site site = presentMap.getSite(loc);
                if (site.owner != myID) continue;

                bool isBorder = false;
                for (unsigned char D : CARDINALS) {
                    Site nsite = presentMap.getSite(loc, D);
                    if (presentMap.getSite(loc, D).owner != myID) isBorder = true;
                }
                if (!isBorder) continue;

                vector<int> dangers(5, 0), mindangers(5, 0);

                for (unsigned char D : DIRECTIONS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(nloc);

                    if (nsite.owner != myID) {
                        dangers[D] = (int)nsite.strength + nsite.production;
                        mindangers[D] = (int)nsite.strength + nsite.production;
                    }
                    for (int D2 : CARDINALS) {
                        Site esite = presentMap.getSite(nloc, D2);
                        if (esite.owner != myID && esite.owner != 0) {
                            dangers[D] += (int)esite.strength + esite.production;
                            mindangers[D] = min(mindangers[D], (int)esite.strength + esite.production);
                        }
                    }
                }

                // first, find the neighbor site not owned by me, with the largest profit (could be 0), and without enemies that can kill me.
                unsigned char bestD = STILL;
                int bestProfit = -1;
                for (unsigned char D : CARDINALS) {
                    Site nsite = presentMap.getSite(loc, D);
                    if (nsite.owner == myID || dangers[D] >= site.strength) continue;

                    if (nsite.production > bestProfit) {
                        bestProfit = nsite.production;
                        bestD = D;
                    }
                }
                if (bestD != STILL) {
                    moves.insert({loc, bestD});
                    Location target = presentMap.getLocation(loc, bestD);
                    killed.insert(target);
                    targets[loc] = target;
                    continue;
                }

                // TODO: first combine pieces for expansion with guaranteed survival?

                // Next, try a neighbor site not owned by me, where we can kill someone, with the largest profit.
                // Individual suicide mission.
                bestProfit = -1;
                for (unsigned char D : CARDINALS) {
                    Site nsite = presentMap.getSite(loc, D);
                    if (nsite.owner == myID || mindangers[D] > site.strength) continue;
                    if (nsite.production > bestProfit) {
                        bestProfit = nsite.production;
                        bestD = D;
                    }
                }

                if (bestD != STILL) {
                    moves.insert({loc, bestD});
                    Location target = presentMap.getLocation(loc, bestD);
                    killed.insert(target); // BUG: not necessarily killed
                    targets[loc] = target;
                    continue;
                }
            }
        }

        // 3) Handle expansion using multiple pieces. We look at each of the remaining enemy pieces.
        // Multi - suicide mission
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };

                Site site = presentMap.getSite(loc);

                if (site.owner == myID || killed.count(loc)) continue;

                int sum = 0, danger = site.strength, mindanger = site.strength;
                for (unsigned char D : CARDINALS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(loc, D);
                    if (nsite.owner == myID) {
                        if (!targets.count(nloc)) sum += nsite.strength;
                    } else {
                        if (nsite.owner != 0) {
                            danger += nsite.strength;
                            mindanger = min(mindanger, (int)nsite.strength);
                        }
                    }
                }

                // TODO: further optimize
                if (sum < mindanger) continue;
                // multi suicide mission.

                for (unsigned char D : CARDINALS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(nloc);
                    if (nsite.owner == myID && !targets.count(nloc)) {
                        moves.insert({nloc, opposite(D)});
                        targets[nloc] = loc;
                    }
                }
            }
        }

        // 4) Look at remaining nodes. Only move internally.
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };

                bool isBorder = false;
                for (unsigned char D : CARDINALS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(loc, D);
                    if (nsite.owner != myID && !killed.count(nloc)) {
                        isBorder = true;
                    }
                }
                //if (isBorder) continue;

                Site site = presentMap.getSite(loc);
                if (site.owner != myID || targets.count(loc)) continue;

//              if (site.strength < site.production * 5) continue;


                bool go = false;
                int r1 = rand() % 100, r2 = (int)site.strength;
                if (r1 < r2) go = true;
                if (!go) continue;

                unsigned char bestD = search.dijkstra(presentMap, loc, myID);
                
                Location target = presentMap.getLocation(loc, bestD);
                Site targetsite = presentMap.getSite(target);

                if (bestD != STILL && targetsite.owner == myID) {
                    moves.insert({loc, bestD});
                    targets[loc] = target;
                    continue;
                }


                // Move internal strong pieces towards the boundary
/*                bestD = STILL;
                int bestDist = INT_MAX;
                for (int D : CARDINALS) {
                    Location nloc = loc;
                    Site nsite = presentMap.getSite(nloc);
                    int dist = 0;
                    while (nsite.owner == myID) {
                        nloc = presentMap.getLocation(nloc, D);
                        nsite = presentMap.getSite(nloc);
                        if (++dist > presentMap.width) break;
                    }

                    if (dist < bestDist || (dist == bestDist && (D == NORTH || D == WEST))) {
                        bestDist = dist;
                        bestD = D;
                    }
                }

                // Diagonal
                vector<vector<int>> diags = {{NORTH, WEST}, {WEST, SOUTH}, {SOUTH, EAST}, {EAST, NORTH}};
                for (vector<int> diag : diags) {
                    int dist = 0;
                    Location nloc = loc;
                    Site nsite = presentMap.getSite(nloc);
                    while (nsite.owner == myID) {
                        nloc = presentMap.getLocation(nloc, diag[0]);
                        nloc = presentMap.getLocation(nloc, diag[1]);
                        nsite = presentMap.getSite(nloc);
                        dist += 2;
                        if (dist > presentMap.width) break;
                    }

                    if (dist < bestDist) {
                        bestDist = dist;
                        bestD = diag[rand() % 2];
                    }
                }

                if (bestD == STILL) continue;

                target = presentMap.getLocation(loc, bestD);
                targetsite = presentMap.getSite(target);
                if (targetsite.owner != myID) continue; // only move internally

                moves.insert({ loc, (unsigned char)bestD });
                targets[loc] = target;*/
            }
        }

        log << endl;

        sendFrame(moves);
    }

    log.close();

    return 0;
}
