#include <iostream>
#include <random>
#include <string>

#include "distance.h"
#include "experiments.h"
#include "parser.h"
#include "results_writer.h"

int main(int argc, char* argv[]) {
    try {
        if (argc < 2 || argc > 4) {
            std::cerr << "Usage: " << argv[0] << " <input.tsp> [output.csv] [seed]\n";
            return 1;
        }

        const std::string inputPath = argv[1];
        const std::string outputPath =
            (argc >= 3) ? argv[2] : defaultLab1OutputPath(inputPath);

        unsigned long long seed = 0;
        if (argc >= 4) {
            seed = std::stoull(argv[3]);
        } else {
            std::random_device rd;
            seed = (static_cast<unsigned long long>(rd()) << 32U) ^ rd();
        }

        const TspInstance instance = readTsplibFile(inputPath);
        const auto dist = buildDistanceMatrix(instance);

        const Lab1Results results = runLab1Experiments(dist, seed);

        writeLab1ResultsCsv(outputPath, instance, results);

        std::cout << "Instance: " << instance.name << "\n";
        std::cout << "Nodes: " << instance.nodes.size() << "\n";
        std::cout << "Random seed: " << seed << "\n";
        std::cout << "Worker threads:                  " << results.threadCount << "\n";
        std::cout << "Random-start cap:                " << results.randomStartCap << "\n";
        std::cout << "Random starts used:              " << results.randomStartCount << "\n";
        std::cout << "Sampled-neighbor cap:            " << results.sampledNeighborCap << "\n";
        std::cout << "Sampled neighbors used:          " << results.sampledNeighborCount
                  << "\n";
        std::cout << "MST-start cap:                   " << results.mstStartCap << "\n";
        std::cout << "MST starts used:                 " << results.mstStartCount << "\n";
        std::cout << "Improvement-step cap:            " << results.improvementStepCap
                  << "\n";
        std::cout << "Exercise 1 average final length:  "
                  << results.fullLocalSearch.averageFinalLength << "\n";
        std::cout << "Exercise 1 average improvement steps: "
                  << results.fullLocalSearch.averageImprovementSteps << "\n";
        std::cout << "Exercise 1 best length:           "
                  << results.fullLocalSearch.bestTour.length << "\n";
        std::cout << "Exercise 2 average final length:  "
                  << results.sampledLocalSearch.averageFinalLength << "\n";
        std::cout << "Exercise 2 average improvement steps: "
                  << results.sampledLocalSearch.averageImprovementSteps << "\n";
        std::cout << "Exercise 2 best length:           "
                  << results.sampledLocalSearch.bestTour.length << "\n";
        std::cout << "MST weight:                       " << results.mst.weight << "\n";
        std::cout << "Exercise 3 average final length:  "
                  << results.mstSeededLocalSearch.averageFinalLength << "\n";
        std::cout << "Exercise 3 average improvement steps: "
                  << results.mstSeededLocalSearch.averageImprovementSteps << "\n";
        std::cout << "Exercise 3 best length:           "
                  << results.mstSeededLocalSearch.bestTour.length << "\n";
        std::cout << "CSV written to: " << outputPath << "\n";
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
