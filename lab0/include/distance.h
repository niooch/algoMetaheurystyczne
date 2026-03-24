#ifndef DISTANCE_H
#define DISTANCE_H

#include <vector>

#include "types.h"

long long euc2dDistance(const Node& a, const Node& b);
std::vector<std::vector<long long>> buildDistanceMatrix(const TspInstance& instance);
long long computeTourLength(const std::vector<int>& order,
                            const std::vector<std::vector<long long>>& dist);

#endif
