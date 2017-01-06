#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <climits>
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

        double ca = 0, cb = 0, sumw = 0;
        for (unsigned short a = 0; a < presentMap.height; a++) {
            for (unsigned short b = 0; b < presentMap.width; b++) {
                auto site = presentMap.getSite({ b, a});
                if (site.owner != myID) continue;
                ca += a * site.strength;
                cb += b * site.strength;
                sumw += site.strength;
            }
        }
        ca /= sumw;
        cb /= sumw;
        Location center = { (unsigned short)cb, (unsigned short)ca };
        log << "center " << center.x << ", " << center.y << endl;
        
        for(unsigned short a = 0; a < presentMap.height; a++) {
            for(unsigned short b = 0; b < presentMap.width; b++) {
                auto site = presentMap.getSite({ b, a });
                if (site.owner != myID) continue;
                    
                hlt::Location loc = { b, a };

                vector<int> dangers(5, 0), powers(5, 0), profits(5, 0), mindangers(5, INT_MAX), strengths(5, 0);
                vector<bool> isMe(5, false);

                for (int D : DIRECTIONS) {
                    hlt::Location nloc = presentMap.getLocation(loc, D);
                    hlt::Site nsite = presentMap.getSite(nloc);
                    isMe[D] = nsite.owner == myID;
                    strengths[D] = nsite.strength;
                    profits[D] = nsite.production;

                    if (!isMe[D]) {
                        dangers[D] = nsite.strength;
                        mindangers[D] = min(mindangers[D], (int)nsite.strength);
                    }
                    for (int D2 : DIRECTIONS) {
                        hlt::Site esite = presentMap.getSite(nloc, D2);
                        if (esite.owner != myID) {
                            dangers[D] += esite.strength;
                            mindangers[D] = min(mindangers[D], (int)esite.strength);
                        } else {
                            powers[D] += esite.strength;
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

                
                if (dangers[STILL] > 0) {
                    // TODO: if production is 0, might as well move towards danger to kill something
                    // moves.insert({ {b, a}, STILL });
                    continue;
                }

                float angle = presentMap.getAngle(center, loc);
                unsigned char D = STILL;

                vector<int> options;
                if (angle >= 0 && angle < 0.5 * pi) {
                    options = {NORTH, EAST};
                } else if (angle >= 0.5 * pi) {
                    options = {NORTH, WEST};
                } else if (angle <= 0 && angle > -0.5 * pi) {
                    options = {SOUTH, EAST};
                } else if (angle <= -0.5) {
                    options = {SOUTH, WEST};
                } else {
                    continue;
                }

                D = options[rand()%2];

                if (!isMe[D]) continue;

                // TODO: powers not needed anymore?
                Location outloc = presentMap.getLocation(loc, D);
                Site outsite = presentMap.getSite(outloc);
                if (strengths[D] >= site.strength || site.strength - strengths[D] <= 60) continue;

                if (D == STILL) continue;
                moves.insert({ {b, a}, D });


                // moves.insert({ {b, a}, (unsigned char)bestD });
            }
        }
        log << endl;

        sendFrame(moves);
    }

    log.close();

    return 0;
}
