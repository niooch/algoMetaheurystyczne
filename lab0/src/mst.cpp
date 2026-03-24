#include "mst.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>

#include "distance.h"

MstResult primMst(const std::vector<std::vector<long long>>& dist) {
    const int n = static_cast<int>(dist.size());
    std::vector<long long> minKey(n, std::numeric_limits<long long>::max());
    std::vector<int> parent(n, -1);
    std::vector<bool> used(n, false);

    minKey[0] = 0;

    for (int iter = 0; iter < n; ++iter) {
        int u = -1;
        for (int v = 0; v < n; ++v) {
            if (!used[v] && (u == -1 || minKey[v] < minKey[u])) {
                u = v;
            }
        }

        if (u == -1) {
            throw std::runtime_error("Graph appears disconnected.");
        }

        used[u] = true;
        for (int v = 0; v < n; ++v) {
            if (!used[v] && dist[u][v] < minKey[v]) {
                minKey[v] = dist[u][v];
                parent[v] = u;
            }
        }
    }

    MstResult result;
    result.adjacency.assign(n, {});
    result.weight = 0;

    for (int v = 1; v < n; ++v) {
        const int p = parent[v];
        result.adjacency[p].push_back(v);
        result.adjacency[v].push_back(p);
        result.weight += dist[p][v];
    }

    for (auto& neighbors : result.adjacency) {
        std::sort(neighbors.begin(), neighbors.end());
    }

    return result;
}

TourResult mstDfsTour(const std::vector<std::vector<long long>>& dist) {
    const MstResult mst = primMst(dist);

    TourResult result;
    std::vector<bool> visited(mst.adjacency.size(), false);

    std::function<void(int)> dfs = [&](int u) {
        visited[u] = true;
        result.order.push_back(u);
        for (int v : mst.adjacency[u]) {
            if (!visited[v]) {
                dfs(v);
            }
        }
    };

    dfs(0);
    result.length = computeTourLength(result.order, dist);
    return result;
}
