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

using US=unsigned short;
using UC=unsigned char;
typedef pair<US, US> PII;

int enemystrength(hlt::GameMap& presentMap, UC myID, US a, US b) {
    vector<PII> dirs = {{0,0},{-1,0},{0,1},{1,0},{0,-1}};
    int ret = 0;
    for (PII dir : dirs) {
        US nb = b + dir.second, na = a + dir.first;
        auto psite = presentMap.getSite({ nb, na });
        if (psite.owner == myID) continue;
        ret += psite.strength;
    }
    return ret;
}


int main() {
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);

    sendInit("yckuoBot");

    std::set<hlt::Move> moves;
    while(true) {
        moves.clear();

        getFrame(presentMap);
        
        for(US a = 0; a < presentMap.height; a++) {
            for(US b = 0; b < presentMap.width; b++) {
                auto site = presentMap.getSite({ b, a });
                if (site.owner == myID) {
                    // moves.insert({ { b, a }, (unsigned char)(rand() % 5) });
                    
                    hlt::Location loc = { b, a };

                    vector<int> dangers(5, 0), profits(5, 0), mindangers(5, INT_MAX), strengths(5, 0);
                    vector<bool> isMe(5, false);

                    for (int D : CARDINALS) {
                        hlt::Location nloc = presentMap.getLocation(loc, D);
                        hlt::Site nsite = presentMap.getSite(nloc);
                        isMe[D] = nsite.owner == myID;
                        strengths[D] = nsite.strength;
                        // if (nsite.owner == myID) continue;

                        profits[D] = nsite.production;
                        for (int D2 : DIRECTIONS) {
                            hlt::Site esite = presentMap.getSite(nloc, D2);
                            if (esite.owner != myID) {
                                dangers[D] += esite.strength;
                                mindangers[D] = min(mindangers[D], (int)esite.strength);
                            }
                        }
                    }

                    int bestD = STILL, bestProfit = -1;
                    for (int D : CARDINALS) {
                        if (!isMe[D] && dangers[D] == 0 && profits[D] > bestProfit) {
                            bestProfit = profits[D];
                            bestD = D;
                        }
                    }

                    if (bestD != STILL) {
                        moves.insert({ {b, a}, (unsigned char)bestD });
                        continue;
                    }

                    for (int D : CARDINALS) {
                        if (!isMe[D] && dangers[D] < site.strength) {
                            bestD = D;
                            break;
                        }
                    }

                    if (bestD != STILL) {
                        moves.insert({ {b, a}, (unsigned char)bestD });
                        continue;
                    }

                    for (int D : CARDINALS) {
                        int totalStrength = site.strength;
                        if (isMe[D]) totalStrength += strengths[D];
                        if (mindangers[D] <= totalStrength) {
                            bestD = D;
                            break;
                        }
                    }

                    moves.insert({ {b, a}, (unsigned char)bestD });
                }
            }
        }

        sendFrame(moves);
    }

    return 0;
}
