#include "results_writer.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using CsvRow = std::array<std::string, 14>;

std::string toString(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
}

template <typename T>
std::string toString(T value) {
    return std::to_string(value);
}

void writeRow(std::ofstream& out, const CsvRow& row) {
    for (std::size_t i = 0; i < row.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << row[i];
    }
    out << '\n';
}

void writeSummaryMetric(std::ofstream& out,
                        const std::string& instanceName,
                        const std::string& algorithm,
                        const std::string& metric,
                        const std::string& value) {
    writeRow(out,
             {"summary", instanceName, algorithm, metric, value, "", "", "", "", "", "", "", "", ""});
}

void writeExperimentRows(std::ofstream& out,
                         const TspInstance& instance,
                         const AggregateExperimentResult& experiment) {
    writeSummaryMetric(out,
                       instance.name,
                       experiment.algorithm,
                       "avg_final_length",
                       toString(experiment.averageFinalLength));
    writeSummaryMetric(out,
                       instance.name,
                       experiment.algorithm,
                       "avg_improvement_steps",
                       toString(experiment.averageImprovementSteps));
    writeSummaryMetric(out,
                       instance.name,
                       experiment.algorithm,
                       "best_length",
                       toString(experiment.bestTour.length));
    writeSummaryMetric(out,
                       instance.name,
                       experiment.algorithm,
                       "run_count",
                       toString(experiment.runs.size()));

    for (const RunMetric& run : experiment.runs) {
        const std::string startNodeId =
            (run.startNodeIndex >= 0)
                ? toString(instance.nodes[static_cast<std::size_t>(run.startNodeIndex)].id)
                : "";

        writeRow(out,
                 {"run",
                  instance.name,
                  experiment.algorithm,
                  "",
                  "",
                  toString(run.runIndex),
                  toString(run.initialLength),
                  toString(run.finalLength),
                  toString(run.improvementSteps),
                  startNodeId,
                  "",
                  "",
                  "",
                  ""});
    }

    for (std::size_t step = 0; step < experiment.bestTour.order.size(); ++step) {
        const Node& node = instance.nodes[static_cast<std::size_t>(experiment.bestTour.order[step])];
        writeRow(out,
                 {"tour",
                  instance.name,
                  experiment.algorithm,
                  "best_tour",
                  toString(experiment.bestTour.length),
                  "",
                  "",
                  "",
                  "",
                  "",
                  toString(step),
                  toString(node.id),
                  toString(node.x),
                  toString(node.y)});
    }

    if (!experiment.bestTour.order.empty()) {
        const Node& start =
            instance.nodes[static_cast<std::size_t>(experiment.bestTour.order.front())];
        writeRow(out,
                 {"tour",
                  instance.name,
                  experiment.algorithm,
                  "best_tour",
                  toString(experiment.bestTour.length),
                  "",
                  "",
                  "",
                  "",
                  "",
                  toString(experiment.bestTour.order.size()),
                  toString(start.id),
                  toString(start.x),
                  toString(start.y)});
    }
}

} // namespace

void writeLab1ResultsCsv(const std::string& outputPath,
                         const TspInstance& instance,
                         const Lab1Results& results) {
    std::ofstream out(outputPath);
    if (!out) {
        throw std::runtime_error("Could not open output CSV: " + outputPath);
    }

    out << "row_type,instance,algorithm,metric,value,run_index,initial_length,final_length,"
           "improvement_steps,start_node_id,step,node_id,x,y\n";

    writeSummaryMetric(out, instance.name, "meta", "random_seed", toString(results.seed));
    writeSummaryMetric(out, instance.name, "meta", "thread_count", toString(results.threadCount));
    writeSummaryMetric(out, instance.name, "meta", "node_count", toString(instance.nodes.size()));
    writeSummaryMetric(
        out, instance.name, "meta", "random_start_cap", toString(results.randomStartCap));
    writeSummaryMetric(
        out, instance.name, "meta", "random_start_count", toString(results.randomStartCount));
    writeSummaryMetric(
        out, instance.name, "meta", "sampled_neighbor_cap", toString(results.sampledNeighborCap));
    writeSummaryMetric(out,
                       instance.name,
                       "meta",
                       "sampled_neighbor_count",
                       toString(results.sampledNeighborCount));
    writeSummaryMetric(out, instance.name, "meta", "mst_start_cap", toString(results.mstStartCap));
    writeSummaryMetric(
        out, instance.name, "meta", "mst_start_count", toString(results.mstStartCount));
    writeSummaryMetric(
        out, instance.name, "meta", "improvement_step_cap", toString(results.improvementStepCap));
    writeSummaryMetric(
        out, instance.name, "mst", "weight", toString(results.mst.weight));

    writeExperimentRows(out, instance, results.fullLocalSearch);
    writeExperimentRows(out, instance, results.sampledLocalSearch);
    writeExperimentRows(out, instance, results.mstSeededLocalSearch);
}

std::string defaultLab1OutputPath(const std::string& inputPath) {
    const auto dot = inputPath.find_last_of('.');
    if (dot == std::string::npos) {
        return inputPath + "_lab1_results.csv";
    }
    return inputPath.substr(0, dot) + "_lab1_results.csv";
}
