#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <algorithm>
#include <climits>
#include "hlt.hpp"
#include "networking.hpp"
using namespace std;

static unsigned long hash(hlt::Location loc) {
    return loc.x * 10000 + loc.y;
}

static bool compare(hlt::Location loc1, hlt::Location loc2) {
    if (loc1.x != loc2.x) return loc1.x < loc2.x;
    return loc1.y < loc2.y;
}

class Hasher {
public:
    size_t operator() (hlt::Location const& loc) const {
        return loc.x * 10000 + loc.y;
    }
};

class Comparer {
public:
    bool operator() (hlt::Location const& loc1, hlt::Location const& loc2) const {
        if (loc1.x != loc2.x) return loc1.x < loc2.x;
        return loc1.y < loc2.y;
    }
};

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

        unordered_map<hlt::Location, unsigned char, Hasher, Comparer> attempts;
        
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

                    if (!isMe[D]) {
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

                // Next, try a neighbor site owned by me, where we can kill someone. Minimum efforts to prevent swapping
                // positions.
                for (int D : CARDINALS) {
                    if (!isMe[D]) continue;
                    int totalStrength = site.strength + strengths[D];
                    if (mindangers[D] <= totalStrength && mindangers[D] > 0) {
                        bestD = D;
                    }
                }

                if (bestD != STILL) {
                    attempts[loc] = bestD;
                }
                

                moves.insert({ {b, a}, (unsigned char)bestD });
            }
        }

        for (auto it : attempts) {
            hlt::Location target = presentMap.getLocation(it.first, it.second);
            if (!attempts.count(target)) {
                //moves.insert({ {it.first, it.second} });
            }
        }

        sendFrame(moves);
    }

    return 0;
}
