#ifndef ALGORITHM_H
#define ALGORITHM_H

#include <functional>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include "hlt.hpp"
using namespace std;
using namespace hlt;

namespace algorithm {
    class Search {
    public:
        Search(unsigned char myID) : myID(myID) {
        }

        struct LocationP {
            LocationP(unsigned short x, unsigned short y, int cost) : x(x), y(y), cost(cost) {
            }
            unsigned short x, y;
            int cost;
        };

        static bool compareLocationP(const LocationP& p1, const LocationP& p2) {
            return p1.cost < p2.cost;
        }

        static bool compare(LocationP p1, LocationP p2) {
            return p1.cost > p2.cost;
        }

        int getTotalDamage(GameMap& presentMap, const Location& loc) const {
            Site site = presentMap.getSite(loc);
            int ret = site.owner == myID ? 0 : ((int)site.strength + site.production);
            for (unsigned char D : CARDINALS) {
                Site nsite = presentMap.getSite(loc, D);
                int damage = (nsite.owner == myID || nsite.owner == 0) ? 0 : ((int)nsite.strength + nsite.production);
                ret += damage;
            }
            return ret;
        }

        int getMinDamage(GameMap& presentMap, const Location& loc) const {
            Site site = presentMap.getSite(loc);
            int ret = site.owner == myID ? INT_MAX : ((int)site.strength + site.production);
            for (unsigned char D : CARDINALS) {
                Site nsite = presentMap.getSite(loc, D);
                if (nsite.owner != myID && nsite.owner != 0) {
                    int damage = (int)nsite.strength + nsite.production;
                    ret = min(ret, damage);
                }
            }
            return ret;
        }

        struct DirectionP {
            DirectionP(unsigned char D, unsigned short production) : D(D), production(production) {
            }
            unsigned char D;
            unsigned short production;
        };

        static bool compareDirectionP(const DirectionP& d1, const DirectionP& d2) {
            if (d1.production != d2.production) return d1.production > d2.production;
            return d1.D == NORTH || d1.D == WEST; // tie breaker
        }


        bool IsBorder(GameMap& presentMap, const Location& loc, unsigned char myID) {
            Site site = presentMap.getSite(loc);
            if (site.owner != myID) return false;

            bool ret = false;
            for (unsigned char D : CARDINALS) {
                Site nsite = presentMap.getSite(loc, D);
                if (nsite.owner != myID) ret = true;
            }
            return ret;
        }

        vector<unsigned char> neighbors(GameMap& presentMap, const Location& loc, bool survivalOrKillable) const { 
            Site site = presentMap.getSite(loc);
            vector<unsigned char> ret;
            vector<DirectionP> tmp;

            for (unsigned char D : CARDINALS) {
                Location nloc = presentMap.getLocation(loc, D);
                Site nsite = presentMap.getSite(nloc);
                if (nsite.owner == myID) continue;

                int totaldamage = getTotalDamage(presentMap, nloc);
                int mindamage = getMinDamage(presentMap, nloc);

                if (survivalOrKillable && (totaldamage >= site.strength) ||
                    !survivalOrKillable && (mindamage >= site.strength)) continue;
                
                tmp.push_back(DirectionP(D, nsite.production));
            }

            sort(tmp.begin(), tmp.end(), compareDirectionP);
            for (auto t : tmp) ret.push_back(t.D);
            // for (auto t : tmp) ret.insert(ret.begin(), t.D);
            return ret;
        }

        unordered_set<Location, LocationHasher, LocationComparer> requireds(GameMap& presentMap, const Location& loc, unordered_map<Location, Location, LocationHasher, LocationComparer>& moved) {
            int mindamage = getMinDamage(presentMap, loc);
            int sum = 0;
            unordered_set<Location, LocationHasher, LocationComparer> ret;
            vector<LocationP> tmp;
            for (unsigned char D : CARDINALS) {
                Location nloc = presentMap.getLocation(loc, D);
                Site nsite = presentMap.getSite(nloc);
                if (nsite.owner != myID || moved.count(nloc)) continue;
                tmp.push_back(LocationP(nloc.x, nloc.y, nsite.strength));
                sum += nsite.strength;
            }

            if (sum <= mindamage) return ret;
            sort(tmp.begin(), tmp.end(), compareLocationP);

            sum = 0;
            for (int i=0; i<tmp.size(); i++) {
                sum += tmp[i].cost;
                ret.insert({tmp[i].x, tmp[i].y});
                if (sum > mindamage) break;
            }

            return ret;
        }

        unsigned char dijkstra(GameMap& presentMap, const Location& start) const {
            
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
                    int cost = curp.cost + 30 + (nextsite.owner == myID ? 0 : nextsite.strength);

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

    private:
        unsigned char myID;
    };
}

#endif
