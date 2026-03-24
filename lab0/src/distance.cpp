#include "distance.h"

#include <cmath>

long long euc2dDistance(const Node& a, const Node& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return static_cast<long long>(std::llround(std::sqrt(dx * dx + dy * dy)));
}

std::vector<std::vector<long long>> buildDistanceMatrix(const TspInstance& instance) {
    const std::size_t n = instance.nodes.size();
    std::vector<std::vector<long long>> dist(n, std::vector<long long>(n, 0));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            const long long d = euc2dDistance(instance.nodes[i], instance.nodes[j]);
            dist[i][j] = d;
            dist[j][i] = d;
        }
    }
    return dist;
}

long long computeTourLength(const std::vector<int>& order,
                            const std::vector<std::vector<long long>>& dist) {
    if (order.empty()) {
        return 0;
    }

    long long total = 0;
    for (std::size_t i = 0; i + 1 < order.size(); ++i) {
        total += dist[order[i]][order[i + 1]];
    }
    total += dist[order.back()][order.front()];
    return total;
}
