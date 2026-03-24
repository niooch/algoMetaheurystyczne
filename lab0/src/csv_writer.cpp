#include "csv_writer.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>

void writeResultsCsv(const std::string& outputPath,
                     const TspInstance& instance,
                     const RandomExperimentResult& randomResult,
                     const MstResult& mstResult,
                     const TourResult& mstTour,
                     unsigned long long seed) {
    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Could not open output CSV: " + outputPath);
    }

    out << std::fixed << std::setprecision(6);
    out << "row_type,instance,algorithm,metric,value,group_index,step,node_id,x,y\n";

    out << "summary," << instance.name << ",random,avg_min_100_groups_of_10,"
        << randomResult.avgMin100x10 << ",,,,,\n";
    out << "summary," << instance.name << ",random,avg_min_20_groups_of_50,"
        << randomResult.avgMin20x50 << ",,,,,\n";
    out << "summary," << instance.name << ",random,overall_min,"
        << randomResult.bestTour.length << ",,,,,\n";
    out << "summary," << instance.name << ",mst,mst_weight,"
        << mstResult.weight << ",,,,,\n";
    out << "summary," << instance.name << ",mst_dfs,tour_length,"
        << mstTour.length << ",,,,,\n";
    out << "summary," << instance.name << ",meta,random_seed,"
        << static_cast<double>(seed) << ",,,,,\n";
    out << "summary," << instance.name << ",meta,node_count,"
        << static_cast<double>(instance.nodes.size()) << ",,,,,\n";

    for (std::size_t i = 0; i < randomResult.minGroupsOf10.size(); ++i) {
        out << "group_min," << instance.name << ",random,min_group_of_10,"
            << randomResult.minGroupsOf10[i] << ',' << i << ",,,,\n";
    }
    for (std::size_t i = 0; i < randomResult.minGroupsOf50.size(); ++i) {
        out << "group_min," << instance.name << ",random,min_group_of_50,"
            << randomResult.minGroupsOf50[i] << ',' << i << ",,,,\n";
    }

    auto writeTour = [&](const std::string& algorithm, const TourResult& tour) {
        for (std::size_t step = 0; step < tour.order.size(); ++step) {
            const Node& node = instance.nodes[tour.order[step]];
            out << "tour," << instance.name << ',' << algorithm
                << ",,," << ','
                << step << ',' << node.id << ',' << node.x << ',' << node.y << '\n';
        }

        if (!tour.order.empty()) {
            const Node& start = instance.nodes[tour.order.front()];
            out << "tour," << instance.name << ',' << algorithm
                << ",,," << ','
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
