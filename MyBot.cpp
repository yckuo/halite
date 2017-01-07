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

const double pi = 3.1415926;

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

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);

    sendInit("yckuoBot");

    unordered_map<Location, Location, LocationHasher, LocationComparer> sources, targets, presources, pretargets;

    std::set<hlt::Move> moves;
    while(true) {
        presources = sources;
        pretargets = targets;
        sources.clear();
        targets.clear();

        moves.clear();

        getFrame(presentMap);

        unordered_set<Location, LocationHasher, LocationComparer> borders, enemies;
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                Location loc = { b, a };
                Site site = presentMap.getSite(loc);
                if (site.owner == myID) continue;
                int count = 0;
                for (unsigned char D : CARDINALS) {
                    Site nsite = presentMap.getSite(loc, D);
                    if (nsite.owner == myID) {
                        if (++count >= 2) {
                            borders.insert(loc);
                            enemies.insert(loc);
                            break;
                        }
                    }
                }
            }
        }

        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                auto site = presentMap.getSite({ b, a });
                if (site.owner != myID) continue;
    
                Location loc = { b, a };

                vector<int> dangers(5, 0), profits(5, 0), mindangers(5, INT_MAX), strengths(5, 0);
                vector<bool> isMe(5, false);

                for (int D : DIRECTIONS) {
                    Location nloc = presentMap.getLocation(loc, D);
                    Site nsite = presentMap.getSite(nloc);
                    isMe[D] = nsite.owner == myID;
                    strengths[D] = nsite.strength;
                    profits[D] = nsite.production; // TODO: consider potential future profit (surrounding enemy tiles)

                    if (!isMe[D]) {
                        dangers[D] = nsite.strength + nsite.production;
                        mindangers[D] = min(mindangers[D], (int)nsite.strength + nsite.production);
                    }
                    for (int D2 : CARDINALS) {
                        Site esite = presentMap.getSite(nloc, D2);
                        if (esite.owner != myID) {
                            dangers[D] += esite.strength + esite.production;
                            mindangers[D] = min(mindangers[D], (int)esite.strength + esite.production);
                        }
                    }
                }

                // first, find the neighbor site not owned by me, with the largest profit (could be 0), and without enemies that can kill me.
                int bestD = STILL, bestProfit = -1;
                for (int D : CARDINALS) {
                    if (!isMe[D] && dangers[D] < site.strength && profits[D] > bestProfit) {
                        bestProfit = profits[D];
                        bestD = D;
                    }
                }

                if (bestD != STILL) {
                    moves.insert({ loc, (unsigned char)bestD });
                    enemies.erase(presentMap.getLocation(loc, bestD));
                    continue;
                }

                // Next, try a neighbor site not owned by me, where we can kill someone, with the largest profit.
                bestProfit = -1;
                for (int D : CARDINALS) {
                    if (!isMe[D] && mindangers[D] <= site.strength && profits[D] > bestProfit) {
                        bestProfit = profits[D];
                        bestD = D;
                    }
                }

                if (bestD != STILL) {
                    moves.insert({ {b, a}, (unsigned char)bestD });
                    enemies.erase(presentMap.getLocation(loc, bestD));
                    continue;
                }

                // If there is enemy near by, and we get some production where we are, why not just stay and wait for growth.
                if (dangers[STILL] > 0 /*&& profits[STILL] > 0*/) {
                    continue;
                }

                if (site.strength == 0) {
                    continue;
                }

                float bestDist = 10000;
                for (Location border : borders) {
                    float dist = presentMap.getDistance(loc, border);
                    bestDist = min(bestDist, dist);
                }

                vector<int> dcounts(5, 0);
                for (Location border : borders) {
                    float dist = presentMap.getDistance(loc, border);
                    if (bestDist == dist) {
                        float angle = presentMap.getAngle(loc, border);
                        unsigned char D = angle2Direction(angle);
                        dcounts[D]++;
                    }
                }


                Location source = presources[loc];
                bestD = STILL;
                for (int D : CARDINALS) {
                    // add momentum
                    if (presentMap.getLocation(source, D) == loc) {
                        dcounts[D] ++;
                    }
                    if (dcounts[D] > dcounts[bestD]) bestD = D;
                }

                if (bestD == STILL) continue;

                Location outloc = presentMap.getLocation(loc, (unsigned char)bestD);

                Site outsite = presentMap.getSite(outloc);
                /*if (outsite.owner != myID) {
                    moves.insert({ {b, a}, (unsigned char)bestD });
                    continue;
                }*/
                if (site.strength <= strengths[bestD]) continue;

                int diff = site.strength - strengths[bestD], sum = (int)site.strength + strengths[bestD];
                if (diff > 30 || (site.strength == 255  && strengths[bestD] != 255)) {
                    moves.insert({ loc, (unsigned char)bestD });
                    Location target = presentMap.getLocation(loc, bestD);
                    sources[target] = loc;
                    targets[loc] = target;
                }
            }
        }
        log << endl;

        sendFrame(moves);
    }

    log.close();

    return 0;
}
