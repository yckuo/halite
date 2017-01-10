#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <functional>
#include <vector>
#include <queue>
#include <unordered_map>
#include "hlt.hpp"
using namespace std;
using namespace hlt;

namespace algorithm {
    class Search {
    public:
        struct LocationP {
            LocationP(unsigned short x, unsigned short y, int cost) : x(x), y(y), cost(cost) {
            }
            unsigned short x, y;
            int cost;
        };

        static bool compare(LocationP p1, LocationP p2) {
            return p1.cost > p2.cost;
        }

        unsigned char dijkstra(GameMap& presentMap, const Location& start, unsigned char myID) const {
            
            // Dijkstra's algorithm
            priority_queue<LocationP, vector<LocationP>, function<bool(LocationP, LocationP)>> pq(compare);
            unordered_map<Location, int, LocationHasher, LocationComparer> costs;
            unordered_map<Location, unsigned char, LocationHasher, LocationComparer> crumbs;
            pq.push({start.x, start.y, 0});
            costs[start] = 0;

            Location bestend = start;
            int bestscore = -1;

            int round = 0, maxround = (presentMap.width/3)*(presentMap.width/3);
            while (!pq.empty()) {
                LocationP curp = pq.top();
                pq.pop();
                Location cur = { curp.x, curp.y };
                Site cursite = presentMap.getSite(cur);

                if (++round > maxround || curp.cost > 1000) break;

                int score = cursite.owner == myID ? 0 : (cursite.production - cursite.strength/10);

                if (score > bestscore) {
                    bestscore = score;
                    bestend = cur;
                }

                for (unsigned char D : CARDINALS) {
                    Location next = presentMap.getLocation(cur, D);
                    Site nextsite = presentMap.getSite(next);
                    int cost = curp.cost + 10 + (nextsite.owner == myID ? 0 : nextsite.strength);

                    if (!costs.count(next) || costs[next] > cost) {
                        costs[next] = cost;
                        LocationP lp(next.x, next.y, cost);
                        pq.push(lp);
                        crumbs[next] = opposite(D);
                    }
                }
            }

            unsigned char ret = STILL;
            Site bestsite = presentMap.getSite(bestend);
            if (bestsite.owner != myID) {
                Location cur = bestend;
                while (cur != start) {
                    ret = crumbs[cur];
                    cur = presentMap.getLocation(cur, ret);
                }
            }
            
            return ret;
        }
    };
}

#endif
