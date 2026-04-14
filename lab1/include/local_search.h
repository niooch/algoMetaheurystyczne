#ifndef LAB1_LOCAL_SEARCH_H
#define LAB1_LOCAL_SEARCH_H

#include <cstddef>
#include <random>
#include <vector>

#include "types.h"

enum class NeighborhoodStrategy {
    FullInvert,
    SampledInvert
};

struct LocalSearchSettings {
    NeighborhoodStrategy strategy = NeighborhoodStrategy::FullInvert;
    std::size_t sampledNeighborCount = 0;
    int maxImprovementSteps = 0;
};

struct LocalSearchResult {
    long long initialLength = 0;
    TourResult finalTour;
    int improvementSteps = 0;
};

LocalSearchResult runLocalSearch(std::vector<int> initialOrder,
                                 const std::vector<std::vector<long long>>& dist,
                                 std::mt19937_64& rng,
                                 const LocalSearchSettings& settings);

TourResult buildMstDfsTourFromRoot(const MstResult& mst,
                                   const std::vector<std::vector<long long>>& dist,
                                   int root);

#endif
