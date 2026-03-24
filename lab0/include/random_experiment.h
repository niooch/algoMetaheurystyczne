#ifndef RANDOM_EXPERIMENT_H
#define RANDOM_EXPERIMENT_H

#include <random>
#include <utility>
#include <vector>

#include "types.h"

std::pair<double, std::vector<long long>> averageOfGroupMins(const std::vector<long long>& lengths,
                                                             std::size_t groupSize);
RandomExperimentResult runRandomExperiment(const std::vector<std::vector<long long>>& dist,
                                           std::mt19937_64& rng,
                                           int samples = 1000);

#endif
