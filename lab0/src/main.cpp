#include <iostream>
#include <random>
#include <string>

#include "csv_writer.h"
#include "distance.h"
#include "mst.h"
#include "parser.h"
#include "random_experiment.h"

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
        const MstResult mstResult = primMst(dist);
        const TourResult mstTour = mstDfsTour(dist);

        writeResultsCsv(outputPath, instance, randomResult, mstResult, mstTour, seed);

        std::cout << "Instance: " << instance.name << "\n";
        std::cout << "Nodes: " << instance.nodes.size() << "\n";
        std::cout << "Random seed: " << seed << "\n";
        std::cout << "Average min (100 groups of 10): " << randomResult.avgMin100x10 << "\n";
        std::cout << "Average min (20 groups of 50):  " << randomResult.avgMin20x50 << "\n";
        std::cout << "Overall random minimum:         " << randomResult.bestTour.length << "\n";
        std::cout << "MST weight:                     " << mstResult.weight << "\n";
        std::cout << "MST + DFS tour length:          " << mstTour.length << "\n";
        std::cout << "CSV written to: " << outputPath << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
