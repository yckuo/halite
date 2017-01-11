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
#include "hlt.hpp"
#include "networking.hpp"
using namespace std;
using namespace hlt;

int main() { 
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    ofstream log;
    log.open("logPrevBot.txt");

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);

    sendInit("prevBot");

    unordered_map<Location, Location, LocationHasher, LocationComparer> sources, targets, presources, pretargets;

    std::set<hlt::Move> moves;
    while (true) {
        presources = sources;
        pretargets = targets;
        sources.clear();
        targets.clear();

        moves.clear();

        getFrame(presentMap);

        // Identify border and enemy pieces.
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
                }
            }
        }

        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Site site = presentMap.getSite({ b, a });
                if (site.owner != myID) continue;
    
                Location loc = { b, a };

                vector<int> dangers(5, 0), mindangers(5, INT_MAX);

                for (int D : DIRECTIONS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(nloc);

                    if (nsite.owner != myID) {
                        dangers[D] = nsite.strength + nsite.production;
                        mindangers[D] = min(mindangers[D], (int)nsite.strength + nsite.production);
                    }
                    for (int D2 : CARDINALS) {
                        Site esite = presentMap.getSite(nloc, D2);
                        if (esite.owner != myID && esite.owner != 0) {
                            dangers[D] += esite.strength + esite.production;
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
                    sources[target] = loc;
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
                    sources[target] = loc;
                    targets[loc] = target;
                    continue;
                }

                // Now we look at internal pieces.
                if (borders.count(loc)) continue;

                // Move internal strong pieces towards the boundary
                bestD = STILL;
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

                Location target = presentMap.getLocation(loc, (unsigned char)bestD);
                Site targetsite = presentMap.getSite(target);

                // if local production is high, less chance of flowing out; if local production is low, higher
                // probability to flow outwards
                bool go = false;

                int r1 = rand() % 100, r2 = (int)site.strength;
                if (r1 < r2) go = true;

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

                if (!go) continue;

                moves.insert({ loc, (unsigned char)bestD });
                sources[target] = loc;
                targets[loc] = target;
            }
        }

        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                Site site = presentMap.getSite(loc);
                if (!enemies.count(loc)) continue;
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

        log << endl;

        sendFrame(moves);
    }

    log.close();

    return 0;
}
