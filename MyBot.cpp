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
#include "hlt.hpp"
#include "networking.hpp"
using namespace std;
using namespace hlt;

const double pi = 3.1415926;

unsigned char angle2Direction(float angle) {
    if (angle >= 0.25 * pi && angle <= 0.75 * pi) return NORTH;
    if (angle > 0.75 * pi || angle < -0.75 * pi) return WEST;
    if (angle <= -0.25 * pi && angle >= -0.75 * pi) return SOUTH;
    if (angle > -0.25 * pi && angle < 0.25 * pi) return EAST;
    return STILL;
}

unsigned char opposite(unsigned char D) {
    if (D == NORTH) return SOUTH;
    if (D == SOUTH) return NORTH;
    if (D == WEST) return EAST;
    if (D == EAST) return WEST;
    return STILL;
}

struct LocationP {
    unsigned short x, y, production;
};

static bool compare(LocationP p1, LocationP p2) {
    return p1.production < p2.production;
}

int main() { 
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    ofstream log;
    log.open("log.txt");

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);

    priority_queue<LocationP, vector<LocationP>, function<bool(LocationP, LocationP)>> pq(compare);
    unordered_set<Location, LocationHasher, LocationComparer> pqset;
    for (unsigned short a = 0; a < presentMap.height; a++) {
        for (unsigned short b = 0; b < presentMap.width; b++) {
            Location loc = { b, a };
            Site site = presentMap.getSite(loc);
            if (site.owner == myID) continue;
            pq.push({loc.x, loc.y, site.production});
            pqset.insert(loc);
        }
    }

    sendInit("yckuoBot");

    unordered_map<Location, Location, LocationHasher, LocationComparer> /*sources,*/ targets/*, presources, pretargets*/;

    std::set<hlt::Move> moves;
    while (true) {
//        presources = sources;
//        pretargets = targets;
//        sources.clear();
        targets.clear();

        moves.clear();

        getFrame(presentMap);

        // 1) Identify border and enemy pieces.
        unordered_set<Location, LocationHasher, LocationComparer> enemies, borders;
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                Site site = presentMap.getSite(loc);
                if (site.owner == myID) {
                    int count = 0;
                    for (unsigned char D : CARDINALS) {
                        Site nsite = presentMap.getSite(loc, D);
                        if (nsite.owner != myID) {
                            borders.insert(loc);
                        }
                    }
                } else {
                    int count = 0;
                    for (unsigned char D : CARDINALS) {
                        Site nsite = presentMap.getSite(loc, D);
                        if (nsite.owner == myID) {
                            if (++count >= 2) {
                                enemies.insert(loc);
                            }
                        }
                    }
                    if (!pqset.count(loc)) {
                        pq.push({loc.x, loc.y, site.production});
                        pqset.insert(loc);
                    }
                }
            }
        }

        // 2) Handle expansion using a single piece at a time. We look at each of the border pieces.
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                if (!borders.count(loc)) continue;

                Site site = presentMap.getSite(loc);
                // if (site.owner != myID) continue;

                vector<int> dangers(5, 0), mindangers(5, 0);

                for (int D : DIRECTIONS) {
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
                int bestD = STILL, bestProfit = -1;
                for (int D : CARDINALS) {
                    Site nsite = presentMap.getSite(loc, D);
                    if (nsite.owner == myID || dangers[D] >= site.strength) continue;
                    if (nsite.production > bestProfit) {
                        bestProfit = nsite.production;
                        bestD = D;
                    }
                }
                if (bestD != STILL) {
                    moves.insert({loc, (unsigned char)bestD});
                    Location target = presentMap.getLocation(loc, bestD);
                    enemies.erase(target);
                    // sources[target] = loc;
                    targets[loc] = target;
                    continue;
                }

                // Next, try a neighbor site not owned by me, where we can kill someone, with the largest profit.
                // Individual suicide mission.
                bestProfit = -1;
                for (int D : CARDINALS) {
                    Site nsite = presentMap.getSite(loc, D);
                    if (nsite.owner == myID || mindangers[D] > site.strength) continue;
                    if (nsite.production > bestProfit) {
                        bestProfit = nsite.production;
                        bestD = D;
                    }
                }

                if (bestD != STILL) {
                    moves.insert({loc, (unsigned char)bestD});
                    Location target = presentMap.getLocation(loc, bestD);
                    enemies.erase(target);
                    // sources[target] = loc;
                    targets[loc] = target;
                    continue;
                }
            }
        }

        // 3) Handle expansion using multiple pieces. We look at each of the remaining enemy pieces.
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                if (!enemies.count(loc)) continue;
                Site site = presentMap.getSite(loc);

                int sum = 0, danger = site.strength, mindanger = site.strength;
                for (int D : CARDINALS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(nloc);
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

                for (int D : CARDINALS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(nloc);
                    if (nsite.owner == myID && !targets.count(nloc)) {
                        moves.insert({nloc, opposite(D)});
                        // sources[loc] = nloc;
                        // it's actually multi sources
                        targets[nloc] = loc;
                    }
                }
            }
        }

        // 4) Identify top production locations
        vector<Location> tmp;
        while (!pq.empty() && tmp.size() < 40) {
            LocationP candidate = pq.top();
            Location cloc = { candidate.x, candidate.y };
            Site csite = presentMap.getSite(cloc);
            pqset.erase(cloc);
            pq.pop();
            if (csite.owner == myID) continue;
            tmp.push_back(cloc);
        }

        for (Location cloc : tmp) {
            Site csite = presentMap.getSite(cloc);
            pq.push({cloc.x, cloc.y, csite.production});
        }

        // 5) Look at remaining nodes. Only move internally.
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                Site site = presentMap.getSite(loc);
                if (site.owner != myID || targets.count(loc)) continue;

                bool go = false;
                int r1 = rand() % 100, r2 = (int)site.strength;
                if (r1 < r2) go = true;
                if (!go) continue;


                unsigned char bestD = STILL;
                int bestDist = INT_MAX, bestProfit = -1;

                for (Location cloc : tmp) {
                    Site csite = presentMap.getSite(cloc);

                    float dist = presentMap.getDistance(loc, cloc);
                    if (dist > bestDist) continue;
                    if (dist == bestDist && csite.production <= bestProfit) continue;

                    bestDist = dist;
                    bestProfit = csite.production;
                    float angle = presentMap.getAngle(loc, cloc);
                    bestD = angle2Direction(angle);
                }

                Location target = presentMap.getLocation(loc, bestD);
                Site targetsite = presentMap.getSite(target);

                if (bestD != STILL && targetsite.owner == myID) {
                    moves.insert({loc, bestD});
                    targets[loc] = target;
                    continue;
                }

                // Move internal strong pieces towards the boundary
                bestD = STILL;
                bestDist = INT_MAX;
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

                // if local production is high, less chance of flowing out; if local production is low, higher
                // probability to flow outwards
                /*bool go = false;

                int r1 = rand() % 100, r2 = (int)site.strength;
                if (r1 < r2) go = true;*/

                // int sum = (int)site.strength + targetsite.strength, diff = (int)site.strength - targetsite.strength;
                // if (site.strength + site.production >= 255 && targetsite.strength < 255) {
                //    go = true;
                // } else if (site.strength < 10 /*|| sum > 400*/ || diff < 20) {
                //    go = false;
                // } else {
                    // int r = min(0, site.production - 3);
                    // if (rand() % (1 << r) == 0) go = true;
                //    go = true;
                //}

                // if (!go) continue;

                moves.insert({ loc, (unsigned char)bestD });
                // sources[target] = loc;
                targets[loc] = target;
            }
        }


        log << endl;

        sendFrame(moves);
    }

    log.close();

    return 0;
}
