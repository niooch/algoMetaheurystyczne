#include "experiments.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "local_search.h"
#include "mst.h"

namespace {

constexpr int kMaxRandomStarts = 50;
constexpr int kMaxSampledNeighbors = 20;
constexpr int kMaxMstStarts = 10;
constexpr int kMaxImprovementSteps = 5;

struct CompletedRun {
    RunMetric metric;
    TourResult finalTour;
};

class ProgressTracker {
public:
    explicit ProgressTracker(int totalRunCount)
        : totalRunCount_(std::max(totalRunCount, 0)), startTime_(Clock::now()) {
        if (totalRunCount_ > 0) {
            monitorThread_ = std::thread([this]() { monitorLoop(); });
        }
    }

    ~ProgressTracker() {
        finish();
    }

    void setPhase(const std::string& phaseLabel, int phaseRunCount) {
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            phaseLabel_ = phaseLabel;
        }

        phaseRunCount_.store(std::max(phaseRunCount, 0), std::memory_order_relaxed);
        phaseCompleted_.store(0, std::memory_order_relaxed);
        printStatus();
    }

    void markRunCompleted() {
        overallCompleted_.fetch_add(1, std::memory_order_relaxed);
        phaseCompleted_.fetch_add(1, std::memory_order_relaxed);
    }

    void finish() {
        if (finished_.exchange(true, std::memory_order_relaxed)) {
            return;
        }

        if (monitorThread_.joinable()) {
            monitorThread_.join();
        }

        if (totalRunCount_ > 0) {
            printStatus();

            std::lock_guard<std::mutex> lock(outputMutex_);
            if (linePrinted_) {
                std::cerr << '\n';
            }
        }
    }

private:
    using Clock = std::chrono::steady_clock;

    static std::string formatDuration(std::chrono::seconds duration) {
        const long long totalSeconds = duration.count();
        const long long hours = totalSeconds / 3600;
        const long long minutes = (totalSeconds % 3600) / 60;
        const long long seconds = totalSeconds % 60;

        std::ostringstream out;
        out << std::setfill('0');
        if (hours > 0) {
            out << hours << ':' << std::setw(2) << minutes << ':' << std::setw(2) << seconds;
        } else {
            out << std::setw(2) << minutes << ':' << std::setw(2) << seconds;
        }
        return out.str();
    }

    void monitorLoop() {
        while (!finished_.load(std::memory_order_relaxed)) {
            printStatus();

            if (overallCompleted_.load(std::memory_order_relaxed) >= totalRunCount_) {
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    void printStatus() {
        if (totalRunCount_ <= 0) {
            return;
        }

        const int overallCompleted = overallCompleted_.load(std::memory_order_relaxed);
        const int phaseCompleted = phaseCompleted_.load(std::memory_order_relaxed);
        const int phaseRunCount = phaseRunCount_.load(std::memory_order_relaxed);

        std::string phaseLabel;
        {
            std::lock_guard<std::mutex> lock(stateMutex_);
            phaseLabel = phaseLabel_;
        }

        const auto elapsed =
            std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - startTime_);

        std::string eta = "--:--";
        if (overallCompleted >= totalRunCount_) {
            eta = "00:00";
        } else if (overallCompleted > 0) {
            const double secondsPerRun =
                static_cast<double>(elapsed.count()) / static_cast<double>(overallCompleted);
            const long long remainingSeconds = static_cast<long long>(std::llround(
                secondsPerRun * static_cast<double>(totalRunCount_ - overallCompleted)));
            eta = formatDuration(std::chrono::seconds(remainingSeconds));
        }

        const double percent =
            100.0 * static_cast<double>(overallCompleted) / static_cast<double>(totalRunCount_);

        std::ostringstream line;
        line << '\r' << phaseLabel << " | phase " << phaseCompleted << '/' << phaseRunCount
             << " | total " << overallCompleted << '/' << totalRunCount_ << " | "
             << std::fixed << std::setprecision(1) << percent << "% | elapsed "
             << formatDuration(elapsed) << " | ETA " << eta;

        std::lock_guard<std::mutex> lock(outputMutex_);
        std::string text = line.str();
        if (lastLineLength_ > text.size()) {
            text.append(lastLineLength_ - text.size(), ' ');
        }

        std::cerr << text << std::flush;
        lastLineLength_ = text.size();
        linePrinted_ = true;
    }

    const int totalRunCount_ = 0;
    const Clock::time_point startTime_;
    std::atomic<int> overallCompleted_{0};
    std::atomic<int> phaseCompleted_{0};
    std::atomic<int> phaseRunCount_{0};
    std::atomic<bool> finished_{false};
    std::thread monitorThread_;
    std::mutex stateMutex_;
    std::string phaseLabel_ = "Preparing runs";
    std::mutex outputMutex_;
    std::size_t lastLineLength_ = 0;
    bool linePrinted_ = false;
};

std::vector<int> makeRandomPermutation(int nodeCount, std::mt19937_64& rng) {
    std::vector<int> order(static_cast<std::size_t>(nodeCount));
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng);
    return order;
}

AggregateExperimentResult makeAggregateResult(const std::string& algorithm, int runCount) {
    AggregateExperimentResult result;
    result.algorithm = algorithm;
    result.runs.reserve(static_cast<std::size_t>(runCount));
    result.bestTour.length = std::numeric_limits<long long>::max();
    return result;
}

void finalizeAggregate(AggregateExperimentResult& result,
                       long double finalLengthSum,
                       long double improvementStepsSum) {
    if (result.runs.empty()) {
        result.bestTour.length = 0;
        return;
    }

    const long double runCount = static_cast<long double>(result.runs.size());
    result.averageFinalLength = static_cast<double>(finalLengthSum / runCount);
    result.averageImprovementSteps = static_cast<double>(improvementStepsSum / runCount);
}

void updateBestTour(AggregateExperimentResult& result, const TourResult& candidate) {
    if (candidate.length < result.bestTour.length) {
        result.bestTour = candidate;
    }
}

std::uint64_t splitMix64(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

std::uint64_t makeRunSeed(std::uint64_t baseSeed, std::uint64_t experimentSalt, int runIndex) {
    return splitMix64(baseSeed ^ experimentSalt ^ splitMix64(static_cast<std::uint64_t>(runIndex)));
}

unsigned int resolveThreadCount(int runCount) {
    const unsigned int hardwareThreads = std::thread::hardware_concurrency();
    const unsigned int availableThreads = (hardwareThreads == 0U) ? 1U : hardwareThreads;
    const unsigned int boundedByWork =
        std::min<unsigned int>(availableThreads, static_cast<unsigned int>(std::max(runCount, 1)));
    return std::max(1U, boundedByWork);
}

template <typename RunFactory>
AggregateExperimentResult runParallelExperiment(const std::string& algorithm,
                                                int runCount,
                                                unsigned long long baseSeed,
                                                std::uint64_t experimentSalt,
                                                unsigned int threadCount,
                                                ProgressTracker& progress,
                                                RunFactory&& runFactory) {
    AggregateExperimentResult result = makeAggregateResult(algorithm, runCount);
    if (runCount <= 0) {
        result.bestTour.length = 0;
        return result;
    }

    std::vector<CompletedRun> completedRuns(static_cast<std::size_t>(runCount));
    std::atomic<int> nextRun{0};
    std::atomic<bool> failed{false};
    std::exception_ptr firstException;
    std::mutex exceptionMutex;

    const auto worker = [&]() {
        while (true) {
            if (failed.load(std::memory_order_relaxed)) {
                break;
            }

            const int runIndex = nextRun.fetch_add(1);
            if (runIndex >= runCount) {
                break;
            }

            try {
                std::mt19937_64 rng(makeRunSeed(baseSeed, experimentSalt, runIndex));
                completedRuns[static_cast<std::size_t>(runIndex)] = runFactory(runIndex, rng);
                progress.markRunCompleted();
            } catch (...) {
                failed.store(true, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(exceptionMutex);
                if (!firstException) {
                    firstException = std::current_exception();
                }
                break;
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(threadCount > 0 ? threadCount - 1U : 0U);
    for (unsigned int i = 1; i < threadCount; ++i) {
        workers.emplace_back(worker);
    }
    worker();

    for (std::thread& thread : workers) {
        thread.join();
    }

    if (firstException) {
        std::rethrow_exception(firstException);
    }

    long double finalLengthSum = 0.0L;
    long double improvementStepsSum = 0.0L;
    for (CompletedRun& completedRun : completedRuns) {
        result.runs.push_back(completedRun.metric);
        finalLengthSum += static_cast<long double>(completedRun.metric.finalLength);
        improvementStepsSum += static_cast<long double>(completedRun.metric.improvementSteps);
        updateBestTour(result, completedRun.finalTour);
    }

    finalizeAggregate(result, finalLengthSum, improvementStepsSum);
    return result;
}

AggregateExperimentResult runRandomStartExperiment(const std::vector<std::vector<long long>>& dist,
                                                   const std::string& algorithm,
                                                   const LocalSearchSettings& settings,
                                                   int startCount,
                                                   unsigned long long seed,
                                                   unsigned int threadCount,
                                                   std::uint64_t experimentSalt,
                                                   ProgressTracker& progress) {
    const int nodeCount = static_cast<int>(dist.size());
    return runParallelExperiment(
        algorithm,
        startCount,
        seed,
        experimentSalt,
        threadCount,
        progress,
        [&](int runIndex, std::mt19937_64& rng) {
            const LocalSearchResult localResult =
                runLocalSearch(makeRandomPermutation(nodeCount, rng), dist, rng, settings);

            return CompletedRun{
                RunMetric{
                    static_cast<std::size_t>(runIndex),
                    localResult.initialLength,
                    localResult.finalTour.length,
                    localResult.improvementSteps,
                    -1,
                },
                std::move(localResult.finalTour),
            };
        });
}

AggregateExperimentResult runMstSeededExperiment(const std::vector<std::vector<long long>>& dist,
                                                 const MstResult& mst,
                                                 int startCount,
                                                 unsigned long long seed,
                                                 unsigned int threadCount,
                                                 ProgressTracker& progress) {
    const int nodeCount = static_cast<int>(dist.size());
    return runParallelExperiment(
        "mst_seeded_ls",
        startCount,
        seed,
        0x6a09e667f3bcc909ULL,
        threadCount,
        progress,
        [&](int runIndex, std::mt19937_64& rng) {
            std::uniform_int_distribution<int> rootDist(0, nodeCount - 1);
            const int root = rootDist(rng);
            TourResult initialTour = buildMstDfsTourFromRoot(mst, dist, root);
            const LocalSearchResult localResult =
                runLocalSearch(std::move(initialTour.order),
                               dist,
                               rng,
                               LocalSearchSettings{
                                   NeighborhoodStrategy::FullInvert, 0, kMaxImprovementSteps});

            return CompletedRun{
                RunMetric{
                    static_cast<std::size_t>(runIndex),
                    localResult.initialLength,
                    localResult.finalTour.length,
                    localResult.improvementSteps,
                    root,
                },
                std::move(localResult.finalTour),
            };
        });
}

int ceilSqrt(int value) {
    return static_cast<int>(std::ceil(std::sqrt(static_cast<double>(value))));
}

} // namespace

Lab1Results runLab1Experiments(const std::vector<std::vector<long long>>& dist,
                               unsigned long long seed) {
    const int nodeCount = static_cast<int>(dist.size());
    Lab1Results results;
    results.seed = seed;
    results.randomStartCap = kMaxRandomStarts;
    results.randomStartCount = std::min(nodeCount, results.randomStartCap);
    results.sampledNeighborCap = kMaxSampledNeighbors;
    results.sampledNeighborCount = std::min(nodeCount, results.sampledNeighborCap);
    results.mstStartCap = kMaxMstStarts;
    results.mstStartCount = std::min(ceilSqrt(nodeCount), results.mstStartCap);
    results.improvementStepCap = kMaxImprovementSteps;
    results.threadCount =
        resolveThreadCount(std::max(results.randomStartCount, results.mstStartCount));
    results.mst = primMst(dist);

    const int totalRunCount =
        results.randomStartCount + results.randomStartCount + results.mstStartCount;
    ProgressTracker progress(totalRunCount);

    progress.setPhase("Exercise 1/3 full invert", results.randomStartCount);
    results.fullLocalSearch =
        runRandomStartExperiment(dist,
                                 "ls_full_invert",
                                 LocalSearchSettings{
                                     NeighborhoodStrategy::FullInvert, 0, results.improvementStepCap},
                                 results.randomStartCount,
                                 seed,
                                 results.threadCount,
                                 0x243f6a8885a308d3ULL,
                                 progress);

    progress.setPhase("Exercise 2/3 sampled invert", results.randomStartCount);
    results.sampledLocalSearch =
        runRandomStartExperiment(dist,
                                 "ls_sampled_invert",
                                 LocalSearchSettings{
                                     NeighborhoodStrategy::SampledInvert,
                                     static_cast<std::size_t>(results.sampledNeighborCount),
                                     results.improvementStepCap},
                                 results.randomStartCount,
                                 seed,
                                 results.threadCount,
                                 0x13198a2e03707344ULL,
                                 progress);

    progress.setPhase("Exercise 3/3 MST seeded", results.mstStartCount);
    results.mstSeededLocalSearch = runMstSeededExperiment(
        dist, results.mst, results.mstStartCount, seed, results.threadCount, progress);

    progress.finish();

    return results;
}
