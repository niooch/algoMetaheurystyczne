#ifndef LAB1_EXPERIMENT_TYPES_H
#define LAB1_EXPERIMENT_TYPES_H

#include <cstddef>
#include <string>
#include <vector>

#include "types.h"

struct RunMetric {
    std::size_t runIndex = 0;
    long long initialLength = 0;
    long long finalLength = 0;
    int improvementSteps = 0;
    int startNodeIndex = -1;
};

struct AggregateExperimentResult {
    std::string algorithm;
    std::vector<RunMetric> runs;
    TourResult bestTour;
    double averageFinalLength = 0.0;
    double averageImprovementSteps = 0.0;
};

struct Lab1Results {
    AggregateExperimentResult fullLocalSearch;
    AggregateExperimentResult sampledLocalSearch;
    AggregateExperimentResult mstSeededLocalSearch;
    MstResult mst;
    unsigned long long seed = 0;
    unsigned int threadCount = 1;
    int randomStartCap = 0;
    int randomStartCount = 0;
    int sampledNeighborCap = 0;
    int sampledNeighborCount = 0;
    int mstStartCap = 0;
    int mstStartCount = 0;
    int improvementStepCap = 0;
};

#endif
