#include "local_search.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

#include "distance.h"

namespace {

struct InvertMove {
    int i = -1;
    int j = -1;
    long long delta = 0;
    bool found = false;
};

bool isValidInvertMove(int i, int j, int nodeCount) {
    return i >= 0 && j >= 0 && i < j && !(i == 0 && j == nodeCount - 1);
}

long long computeInvertDelta(const std::vector<int>& order,
                             int i,
                             int j,
                             const std::vector<std::vector<long long>>& dist) {
    const int n = static_cast<int>(order.size());
    const int prev = order[(i - 1 + n) % n];
    const int first = order[i];
    const int last = order[j];
    const int next = order[(j + 1) % n];

    const long long removed = dist[prev][first] + dist[last][next];
    const long long added = dist[prev][last] + dist[first][next];
    return added - removed;
}

InvertMove findBestFullInvertMove(const std::vector<int>& order,
                                  const std::vector<std::vector<long long>>& dist) {
    const int n = static_cast<int>(order.size());
    InvertMove best;
    long long bestDelta = std::numeric_limits<long long>::max();

    for (int i = 0; i < n - 1; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (!isValidInvertMove(i, j, n)) {
                continue;
            }

            const long long delta = computeInvertDelta(order, i, j, dist);
            if (!best.found || delta < bestDelta) {
                best = {i, j, delta, true};
                bestDelta = delta;
            }
        }
    }

    return best;
}

std::uint64_t encodeMoveKey(int i, int j) {
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(i)) << 32U) |
           static_cast<std::uint32_t>(j);
}

std::size_t invertNeighborhoodSize(int nodeCount) {
    if (nodeCount < 3) {
        return 0;
    }

    const std::size_t allPairs =
        static_cast<std::size_t>(nodeCount) * static_cast<std::size_t>(nodeCount - 1) / 2U;
    return allPairs - 1U;
}

InvertMove findBestSampledInvertMove(const std::vector<int>& order,
                                     const std::vector<std::vector<long long>>& dist,
                                     std::mt19937_64& rng,
                                     std::size_t sampleCount) {
    const int n = static_cast<int>(order.size());
    const std::size_t neighborhoodSize = invertNeighborhoodSize(n);
    if (neighborhoodSize == 0 || sampleCount == 0) {
        return {};
    }

    if (sampleCount >= neighborhoodSize) {
        return findBestFullInvertMove(order, dist);
    }

    std::uniform_int_distribution<int> nodeDist(0, n - 1);
    std::unordered_set<std::uint64_t> usedMoves;
    usedMoves.reserve(sampleCount * 2U);

    InvertMove best;
    long long bestDelta = std::numeric_limits<long long>::max();

    while (usedMoves.size() < sampleCount) {
        int i = nodeDist(rng);
        int j = nodeDist(rng);
        if (i == j) {
            continue;
        }
        if (i > j) {
            std::swap(i, j);
        }
        if (!isValidInvertMove(i, j, n)) {
            continue;
        }

        const std::uint64_t key = encodeMoveKey(i, j);
        if (!usedMoves.insert(key).second) {
            continue;
        }

        const long long delta = computeInvertDelta(order, i, j, dist);
        if (!best.found || delta < bestDelta) {
            best = {i, j, delta, true};
            bestDelta = delta;
        }
    }

    return best;
}

InvertMove findBestMove(const std::vector<int>& order,
                        const std::vector<std::vector<long long>>& dist,
                        std::mt19937_64& rng,
                        const LocalSearchSettings& settings) {
    switch (settings.strategy) {
        case NeighborhoodStrategy::FullInvert:
            return findBestFullInvertMove(order, dist);
        case NeighborhoodStrategy::SampledInvert:
            return findBestSampledInvertMove(order, dist, rng, settings.sampledNeighborCount);
    }

    throw std::runtime_error("Unsupported neighborhood strategy.");
}

} // namespace

LocalSearchResult runLocalSearch(std::vector<int> initialOrder,
                                 const std::vector<std::vector<long long>>& dist,
                                 std::mt19937_64& rng,
                                 const LocalSearchSettings& settings) {
    LocalSearchResult result;
    result.initialLength = computeTourLength(initialOrder, dist);

    std::vector<int> currentOrder = std::move(initialOrder);

    while (settings.maxImprovementSteps <= 0 ||
           result.improvementSteps < settings.maxImprovementSteps) {
        const InvertMove bestMove = findBestMove(currentOrder, dist, rng, settings);
        if (!bestMove.found || bestMove.delta >= 0) {
            break;
        }

        std::reverse(currentOrder.begin() + bestMove.i, currentOrder.begin() + bestMove.j + 1);
        ++result.improvementSteps;
    }

    result.finalTour.order = std::move(currentOrder);
    result.finalTour.length = computeTourLength(result.finalTour.order, dist);
    return result;
}

TourResult buildMstDfsTourFromRoot(const MstResult& mst,
                                   const std::vector<std::vector<long long>>& dist,
                                   int root) {
    const int n = static_cast<int>(mst.adjacency.size());
    if (root < 0 || root >= n) {
        throw std::runtime_error("Invalid DFS root for MST traversal.");
    }

    TourResult result;
    result.order.reserve(n);

    std::vector<bool> visited(n, false);
    std::vector<int> stack;
    stack.push_back(root);

    while (!stack.empty()) {
        const int node = stack.back();
        stack.pop_back();
        if (visited[node]) {
            continue;
        }

        visited[node] = true;
        result.order.push_back(node);

        const auto& neighbors = mst.adjacency[node];
        for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
            if (!visited[*it]) {
                stack.push_back(*it);
            }
        }
    }

    if (static_cast<int>(result.order.size()) != n) {
        throw std::runtime_error("MST DFS traversal did not visit every vertex.");
    }

    result.length = computeTourLength(result.order, dist);
    return result;
}
