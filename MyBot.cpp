#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>
#include <vector>
#include <algorithm>
#include <climits>
#include "hlt.hpp"
#include "networking.hpp"
using namespace std;

int main() {
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);

    sendInit("yckuoBot");

    std::set<hlt::Move> moves;
    vector<vector<bool>> source, target;
    while(true) {
        moves.clear();

        getFrame(presentMap);
        
        source.resize(presentMap.height, vector<bool>(presentMap.width, false));
        target.resize(presentMap.height, vector<bool>(presentMap.width, false));

        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                auto site = presentMap.getSite({ b, a });
                if (site.owner != myID) continue;
                    
                hlt::Location loc = { b, a };

                vector<int> dangers(5, 0), profits(5, 0), mindangers(5, INT_MAX), strengths(5, 0);
                vector<bool> isMe(5, false);

                for (int D : CARDINALS) {
                    hlt::Location nloc = presentMap.getLocation(loc, D);
                    hlt::Site nsite = presentMap.getSite(nloc);
                    isMe[D] = nsite.owner == myID;
                    strengths[D] = nsite.strength;
                    profits[D] = nsite.production;

                    if (isMe[D]) {
                        dangers[D] = nsite.strength;
                        mindangers[D] = min(mindangers[D], (int)nsite.strength);
                    }
                    for (int D2 : DIRECTIONS) {
                        hlt::Site esite = presentMap.getSite(nloc, D2);
                        if (esite.owner != myID) {
                            dangers[D] += esite.strength;
                            mindangers[D] = min(mindangers[D], (int)esite.strength);
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
                    moves.insert({ {b, a}, (unsigned char)bestD });
                    continue;
                }

                // Next, try a neighbor site not owned by me, where we can kill someone, with the largest profit.
                bestProfit = -1;
                for (int D : CARDINALS) {
                    if (isMe[D]) continue;
                    if (mindangers[D] <= site.strength && profits[D] > bestProfit) {
                        bestProfit = profits[D];
                        bestD = D;
                    }
                }

                if (bestD != STILL) {
                    moves.insert({ {b, a}, (unsigned char)bestD });
                    continue;
                }

                // Next, try a neighbor site owned by me, where we can kill someone. In order to prevent two pieces from
                // swapping locations and ending up killing nobody, we try to detect duplicate moves. If we are to move A
                // to B, and if we see another attempt of someone else moving into A or moving out of B, then we simply reject it.
                // First come first serve, not ideal but will improve this in V3.
                for (int D : CARDINALS) {
                    if (!isMe[D]) continue;
                    int totalStrength = site.strength + strengths[D];
                    if (mindangers[D] <= totalStrength && mindangers[D] > 0) {
                        bestD = D;
                    }
                }

                if (bestD != STILL) {
                    hlt::Location bestLoc = presentMap.getLocation(loc, bestD);
                    // attempt loc to bestLoc
                    if (target[loc.y][loc.x] || source[bestLoc.y][bestLoc.x] || target[bestLoc.y][bestLoc.x]) {
                        bestD = STILL;
                    } else {
                        target[bestLoc.y][bestLoc.x] = true;
                        source[loc.y][loc.x] = true;
                    }
                }

                moves.insert({ {b, a}, (unsigned char)bestD });
            }
        }

        sendFrame(moves);
    }

    return 0;
}
