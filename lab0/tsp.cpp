#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

struct Node {
    int id = 0;
    double x = 0.0;
    double y = 0.0;
};

struct TspInstance {
    std::string name;
    std::vector<Node> nodes;
};

struct TourResult {
    std::vector<int> order; // indices into instance.nodes, without repeated start
    long long length = 0;
};

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

long long euc2dDistance(const Node& a, const Node& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return static_cast<long long>(std::llround(std::sqrt(dx * dx + dy * dy)));
}

TspInstance readTsplibFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open input file: " + path);
    }

    TspInstance instance;
    std::string line;
    bool inNodeSection = false;

    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        if (!inNodeSection) {
            if (line.rfind("NAME", 0) == 0) {
                const auto pos = line.find(':');
                if (pos != std::string::npos) {
                    instance.name = trim(line.substr(pos + 1));
                }
            }
            if (line == "NODE_COORD_SECTION") {
                inNodeSection = true;
            }
            continue;
        }

        if (line == "EOF") {
            break;
        }

        std::istringstream iss(line);
        Node node;
        if (iss >> node.id >> node.x >> node.y) {
            instance.nodes.push_back(node);
        }
    }

    if (instance.nodes.empty()) {
        throw std::runtime_error("No nodes found in NODE_COORD_SECTION.");
    }

    if (instance.name.empty()) {
        instance.name = "unnamed";
    }

    return instance;
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

std::vector<std::vector<int>> primMstAdjacency(const std::vector<std::vector<long long>>& dist) {
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

    std::vector<std::vector<int>> adj(n);
    for (int v = 1; v < n; ++v) {
        const int p = parent[v];
        adj[p].push_back(v);
        adj[v].push_back(p);
    }

    for (auto& nbrs : adj) {
        std::sort(nbrs.begin(), nbrs.end());
    }

    return adj;
}

void dfsPreorder(int u,
                 int parent,
                 const std::vector<std::vector<int>>& adj,
                 std::vector<int>& order) {
    order.push_back(u);
    for (int v : adj[u]) {
        if (v != parent) {
            dfsPreorder(v, u, adj, order);
        }
    }
}

TourResult mstDfsTour(const std::vector<std::vector<long long>>& dist) {
    const auto adj = primMstAdjacency(dist);
    TourResult result;
    dfsPreorder(0, -1, adj, result.order);
    result.length = computeTourLength(result.order, dist);
    return result;
}

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

struct RandomExperimentResult {
    std::vector<long long> lengths;
    TourResult bestTour;
    double avgMin100x10 = 0.0;
    double avgMin50x20 = 0.0;
};

RandomExperimentResult runRandomExperiment(const std::vector<std::vector<long long>>& dist,
                                           std::mt19937_64& rng,
                                           int samples = 1000) {
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
            result.bestTour.order = std::move(perm);
            result.bestTour.length = len;
        }
    }

    result.avgMin100x10 = averageOfGroupMins(result.lengths, 10).first;
    result.avgMin50x20 = averageOfGroupMins(result.lengths, 20).first;
    return result;
}

void writeResultsCsv(const std::string& outputPath,
                     const TspInstance& instance,
                     const RandomExperimentResult& randomResult,
                     const TourResult& mstTour,
                     unsigned long long seed) {
    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Could not open output CSV: " + outputPath);
    }

    out << std::fixed << std::setprecision(6);
    out << "row_type,instance,algorithm,metric,value,step,node_id,x,y\n";

    out << "summary," << instance.name << ",random,avg_min_100_groups_of_10,"
        << randomResult.avgMin100x10 << ",,,,\n";
    out << "summary," << instance.name << ",random,avg_min_50_groups_of_20,"
        << randomResult.avgMin50x20 << ",,,,\n";
    out << "summary," << instance.name << ",random,overall_min,"
        << randomResult.bestTour.length << ",,,,\n";
    out << "summary," << instance.name << ",mst_dfs,tour_length,"
        << mstTour.length << ",,,,\n";
    out << "summary," << instance.name << ",meta,random_seed,"
        << static_cast<double>(seed) << ",,,,\n";
    out << "summary," << instance.name << ",meta,node_count,"
        << static_cast<double>(instance.nodes.size()) << ",,,,\n";

    auto writeTour = [&](const std::string& algorithm, const TourResult& tour) {
        for (std::size_t step = 0; step < tour.order.size(); ++step) {
            const Node& node = instance.nodes[tour.order[step]];
            out << "tour," << instance.name << ',' << algorithm
                << ",," << ','
                << step << ',' << node.id << ',' << node.x << ',' << node.y << '\n';
        }
        if (!tour.order.empty()) {
            const Node& start = instance.nodes[tour.order.front()];
            out << "tour," << instance.name << ',' << algorithm
                << ",," << ','
                << tour.order.size() << ',' << start.id << ',' << start.x << ',' << start.y << '\n';
        }
    };

    writeTour("random_best", randomResult.bestTour);
    writeTour("mst_dfs", mstTour);
}

std::string defaultOutputPath(const std::string& inputPath) {
    const auto dot = inputPath.find_last_of('.');
    if (dot == std::string::npos) {
        return inputPath + "_results.csv";
    }
    return inputPath.substr(0, dot) + "_results.csv";
}

int main(int argc, char* argv[]) {
    try {
        if (argc < 2 || argc > 4) {
            std::cerr << "Usage: " << argv[0]
                      << " <input.tsp> [output.csv] [seed]\n";
            return 1;
        }

        const std::string inputPath = argv[1];
        const std::string outputPath = (argc >= 3) ? argv[2] : defaultOutputPath(inputPath);

        unsigned long long seed = 0;
        if (argc >= 4) {
            seed = std::stoull(argv[3]);
        } else {
            std::random_device rd;
            seed = (static_cast<unsigned long long>(rd()) << 32) ^ rd();
        }

        const TspInstance instance = readTsplibFile(inputPath);
        const auto dist = buildDistanceMatrix(instance);

        std::mt19937_64 rng(seed);
        const RandomExperimentResult randomResult = runRandomExperiment(dist, rng, 1000);
        const TourResult mstTour = mstDfsTour(dist);

        writeResultsCsv(outputPath, instance, randomResult, mstTour, seed);

        std::cout << "Instance: " << instance.name << "\n";
        std::cout << "Nodes: " << instance.nodes.size() << "\n";
        std::cout << "Random seed: " << seed << "\n";
        std::cout << "Average min (100 groups of 10): " << randomResult.avgMin100x10 << "\n";
        std::cout << "Average min (50 groups of 20):  " << randomResult.avgMin50x20 << "\n";
        std::cout << "Overall random minimum:         " << randomResult.bestTour.length << "\n";
        std::cout << "MST + DFS tour length:          " << mstTour.length << "\n";
        std::cout << "CSV written to: " << outputPath << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
