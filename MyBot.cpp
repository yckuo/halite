#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <climits>
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
    log.open("log.txt");

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);

    sendInit("yckuoBot");

    std::set<hlt::Move> moves;
    double pi = 3.1415926;
    while(true) {
        moves.clear();

        getFrame(presentMap);

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
                        dangers[D] = nsite.strength;
                        mindangers[D] = min(mindangers[D], (int)nsite.strength);
                    }
                    for (int D2 : DIRECTIONS) {
                        Site esite = presentMap.getSite(nloc, D2);
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
                    if (!isMe[D] && mindangers[D] <= site.strength && profits[D] > bestProfit) {
                        bestProfit = profits[D];
                        bestD = D;
                    }
                }

                if (bestD != STILL) {
                    moves.insert({ {b, a}, (unsigned char)bestD });
                    continue;
                }

                // If there is enemy near by, and we get some production where we are, why not just stay and wait for growth.
                if (dangers[STILL] > 0 && profits[STILL] > 0) {
                    continue;
                }

                if (site.strength == 0) {
                    continue;
                }

                bestD = STILL;
                int bestDist = INT_MAX;
                bestProfit = -1;
                for (int D : CARDINALS) {
                    Location nloc = loc;
                    Site nsite = presentMap.getSite(nloc);
                    int dist = 0;
                    while (nsite.owner == myID && dist < presentMap.width) {
                        nloc = presentMap.getLocation(nloc, D);
                        nsite = presentMap.getSite(nloc);
                        dist++;
                    }

                    if (nsite.owner == myID) continue;
                    if (dist < bestDist || (dist == bestDist && (nsite.production > bestProfit || (nsite.production == bestProfit && (D == NORTH || D == WEST))))) {
                        bestDist = dist;
                        bestProfit = nsite.production;
                        bestD = D;
                    }
                }

                Location outloc = presentMap.getLocation(loc, (unsigned char)bestD);
                Site outsite = presentMap.getSite(outloc);
                if (outsite.owner != myID) {
                    moves.insert({ {b, a}, (unsigned char)bestD });
                    continue;
                }

                if (site.strength > strengths[bestD] && site.strength - strengths[bestD] > 50 || site.strength == 255 && strengths[bestD] != 255) {
                    moves.insert({ {b, a}, (unsigned char)bestD });
                }
            }
        }
        log << endl;

        sendFrame(moves);
    }

    log.close();

    return 0;
}
