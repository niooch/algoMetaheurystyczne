#ifndef TYPES_H
#define TYPES_H

#include <string>
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

struct RandomExperimentResult {
    std::vector<long long> lengths;
    std::vector<long long> minGroupsOf10;
    std::vector<long long> minGroupsOf50;
    TourResult bestTour;
    double avgMin100x10 = 0.0;
    double avgMin20x50 = 0.0;
};

struct MstResult {
    std::vector<std::vector<int>> adjacency;
    long long weight = 0;
};

#endif
