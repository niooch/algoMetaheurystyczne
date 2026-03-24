#include "random_experiment.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <stdexcept>

#include "distance.h"

std::pair<double, std::vector<long long>> averageOfGroupMins(const std::vector<long long>& lengths,
                                                             std::size_t groupSize) {
    if (lengths.empty() || groupSize == 0 || lengths.size() % groupSize != 0) {
        throw std::runtime_error("Invalid grouping for average-of-minimums calculation.");
    }

    std::vector<long long> mins;
    mins.reserve(lengths.size() / groupSize);

    for (std::size_t i = 0; i < lengths.size(); i += groupSize) {
        const auto begin = lengths.begin() + static_cast<std::ptrdiff_t>(i);
        const auto end = begin + static_cast<std::ptrdiff_t>(groupSize);
        mins.push_back(*std::min_element(begin, end));
    }

    const long long sum = std::accumulate(mins.begin(), mins.end(), 0LL);
    const double avg = static_cast<double>(sum) / static_cast<double>(mins.size());
    return {avg, mins};
}

RandomExperimentResult runRandomExperiment(const std::vector<std::vector<long long>>& dist,
                                           std::mt19937_64& rng,
                                           int samples) {
    const int n = static_cast<int>(dist.size());
    std::vector<int> base(n);
    std::iota(base.begin(), base.end(), 0);

    RandomExperimentResult result;
    result.lengths.reserve(samples);
    result.bestTour.length = std::numeric_limits<long long>::max();

    for (int i = 0; i < samples; ++i) {
        std::vector<int> perm = base;
        std::shuffle(perm.begin(), perm.end(), rng);
        const long long len = computeTourLength(perm, dist);
        result.lengths.push_back(len);
        if (len < result.bestTour.length) {
            result.bestTour.order = perm;
            result.bestTour.length = len;
        }
    }

    const auto groups10 = averageOfGroupMins(result.lengths, 10);
    result.avgMin100x10 = groups10.first;
    result.minGroupsOf10 = groups10.second;

    const auto groups50 = averageOfGroupMins(result.lengths, 50);
    result.avgMin20x50 = groups50.first;
    result.minGroupsOf50 = groups50.second;

    return result;
}
