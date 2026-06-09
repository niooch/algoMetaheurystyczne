#include <bits/stdc++.h>
using namespace std;

struct Point {
    int id;
    double x, y;
};

struct Individual {
    vector<int> route;
    long long cost = LLONG_MAX;
};

struct Params {
    int populationSize = 100;
    int generations = 300;
    int islands = 2;
    int eliteCount = 2;

    double crossoverRate = 0.95;
    double mutationRate = 0.20;

    int tournamentSize = 3;
    int migrationInterval = 10;
    int migrationCount = 1;
    int noImproveLimit = 80;

    bool memeticEnabled = true;
    int memeticTopK = 2;
    int memeticAttempts = 20;

    bool useNearestNeighborInit = true;

    string crossoverType = "ox"; // ox albo pmx

    int runs = 1;
    int threads = 1;
    bool autoConfig = true;

    unsigned seed = chrono::steady_clock::now().time_since_epoch().count();

    string historyFile = "history.csv";
    string bestRouteFile = "best_route.csv";
    string pointsFile = "points.csv";
    string runsFile = "runs.csv";
};

struct RunResult {
    vector<int> bestRoute;
    long long bestCost = LLONG_MAX;
    vector<long long> history;
    int generationsDone = 0;
    unsigned seed = 0;
};

struct DistanceMatrix {
    int n = 0;
    vector<int> data;

    DistanceMatrix() = default;

    explicit DistanceMatrix(int n_) {
        n = n_;
        data.assign(n * n, 0);
    }

    inline int get(int i, int j) const {
        return data[i * n + j];
    }

    inline int& at(int i, int j) {
        return data[i * n + j];
    }
};

vector<Point> readTSP(const string& filename) {
    ifstream file(filename);

    if (!file) {
        throw runtime_error("Nie mozna otworzyc pliku: " + filename);
    }

    vector<Point> points;
    string line;
    bool coordSection = false;

    while (getline(file, line)) {
        if (line.find("NODE_COORD_SECTION") != string::npos) {
            coordSection = true;
            continue;
        }

        if (!coordSection) {
            continue;
        }

        if (line.find("EOF") != string::npos) {
            break;
        }

        if (line.empty()) {
            continue;
        }

        stringstream ss(line);
        int id;
        double x, y;

        if (ss >> id >> x >> y) {
            points.push_back({id, x, y});
        }
    }

    if (points.size() < 3) {
        throw runtime_error("Za malo poprawnych punktow w pliku.");
    }

    return points;
}

int roundedEuclideanDistance(const Point& a, const Point& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return (int)llround(sqrt(dx * dx + dy * dy));
}

DistanceMatrix buildDistanceMatrix(const vector<Point>& points) {
    int n = points.size();
    DistanceMatrix dist(n);

    for (int i = 0; i < n; i++) {
        dist.at(i, i) = 0;

        for (int j = i + 1; j < n; j++) {
            int d = roundedEuclideanDistance(points[i], points[j]);
            dist.at(i, j) = d;
            dist.at(j, i) = d;
        }
    }

    return dist;
}

long long routeLength(const vector<int>& route, const DistanceMatrix& dist) {
    int n = route.size();
    long long sum = 0;

    for (int i = 0; i < n; i++) {
        int a = route[i];
        int b = route[(i + 1) % n];
        sum += dist.get(a, b);
    }

    return sum;
}

long long invertDeltaCost(
    const vector<int>& route,
    const DistanceMatrix& dist,
    int i,
    int j,
    long long currentCost
) {
    int n = route.size();

    if (i == 0 && j == n - 1) {
        return currentCost;
    }

    int iPrev = (i - 1 + n) % n;
    int jNext = (j + 1) % n;

    long long oldEdges =
        dist.get(route[iPrev], route[i]) +
        dist.get(route[j], route[jNext]);

    long long newEdges =
        dist.get(route[iPrev], route[j]) +
        dist.get(route[i], route[jNext]);

    return currentCost - oldEdges + newEdges;
}

vector<int> randomRoute(int n, mt19937& rng) {
    vector<int> route(n);
    iota(route.begin(), route.end(), 0);
    shuffle(route.begin(), route.end(), rng);
    return route;
}

vector<int> nearestNeighborTour(const DistanceMatrix& dist, int start) {
    int n = dist.n;

    vector<int> route;
    route.reserve(n);

    vector<char> used(n, 0);

    int current = start;
    route.push_back(current);
    used[current] = 1;

    for (int step = 1; step < n; step++) {
        int best = -1;
        int bestDistance = INT_MAX;

        for (int v = 0; v < n; v++) {
            if (!used[v]) {
                int d = dist.get(current, v);

                if (d < bestDistance) {
                    bestDistance = d;
                    best = v;
                }
            }
        }

        current = best;
        route.push_back(current);
        used[current] = 1;
    }

    return route;
}

Individual makeIndividual(vector<int>&& route, const DistanceMatrix& dist) {
    Individual ind;
    ind.cost = routeLength(route, dist);
    ind.route = move(route);
    return ind;
}

bool betterIndividual(const Individual& a, const Individual& b) {
    return a.cost < b.cost;
}

int tournamentSelectIndex(
    const vector<Individual>& population,
    int k,
    mt19937& rng
) {
    uniform_int_distribution<int> popDist(0, (int)population.size() - 1);

    int bestIndex = popDist(rng);

    for (int i = 1; i < k; i++) {
        int candidateIndex = popDist(rng);

        if (population[candidateIndex].cost < population[bestIndex].cost) {
            bestIndex = candidateIndex;
        }
    }

    return bestIndex;
}

vector<int> oxCrossover(
    const vector<int>& parent1,
    const vector<int>& parent2,
    mt19937& rng
) {
    int n = parent1.size();

    uniform_int_distribution<int> posDist(0, n - 1);

    int a = posDist(rng);
    int b = posDist(rng);

    if (a > b) {
        swap(a, b);
    }

    vector<int> child(n, -1);
    vector<char> used(n, 0);

    for (int i = a; i <= b; i++) {
        child[i] = parent1[i];
        used[parent1[i]] = 1;
    }

    int writePos = (b + 1) % n;
    int readPos = (b + 1) % n;
    int remaining = n - (b - a + 1);

    while (remaining > 0) {
        int gene = parent2[readPos];

        if (!used[gene]) {
            child[writePos] = gene;
            used[gene] = 1;
            writePos = (writePos + 1) % n;
            remaining--;
        }

        readPos = (readPos + 1) % n;
    }

    return child;
}

vector<int> pmxCrossover(
    const vector<int>& parent1,
    const vector<int>& parent2,
    mt19937& rng
) {
    int n = parent1.size();

    uniform_int_distribution<int> posDist(0, n - 1);

    int a = posDist(rng);
    int b = posDist(rng);

    if (a > b) {
        swap(a, b);
    }

    vector<int> child(n, -1);
    vector<int> positionInParent2(n);

    for (int i = 0; i < n; i++) {
        positionInParent2[parent2[i]] = i;
    }

    vector<char> inSegment(n, 0);

    for (int i = a; i <= b; i++) {
        child[i] = parent1[i];
        inSegment[parent1[i]] = 1;
    }

    for (int i = a; i <= b; i++) {
        int value = parent2[i];

        if (inSegment[value]) {
            continue;
        }

        int pos = i;

        while (a <= pos && pos <= b) {
            int mapped = parent1[pos];
            pos = positionInParent2[mapped];
        }

        child[pos] = value;
    }

    for (int i = 0; i < n; i++) {
        if (child[i] == -1) {
            child[i] = parent2[i];
        }
    }

    return child;
}

vector<int> crossover(
    const vector<int>& parent1,
    const vector<int>& parent2,
    const string& type,
    mt19937& rng
) {
    if (type == "pmx") {
        return pmxCrossover(parent1, parent2, rng);
    }

    return oxCrossover(parent1, parent2, rng);
}

void mutateInvert(vector<int>& route, mt19937& rng) {
    int n = route.size();

    uniform_int_distribution<int> posDist(0, n - 1);

    int i = posDist(rng);
    int j = posDist(rng);

    while (i == j) {
        j = posDist(rng);
    }

    if (i > j) {
        swap(i, j);
    }

    reverse(route.begin() + i, route.begin() + j + 1);
}

pair<vector<int>, long long> localMemeticSearch(
    const vector<int>& route,
    long long cost,
    const DistanceMatrix& dist,
    mt19937& rng,
    int attempts
) {
    int n = route.size();

    vector<int> current = route;
    long long currentCost = cost;

    uniform_int_distribution<int> posDist(0, n - 1);

    for (int attempt = 0; attempt < attempts; attempt++) {
        int i = posDist(rng);
        int j = posDist(rng);

        while (i == j) {
            j = posDist(rng);
        }

        if (i > j) {
            swap(i, j);
        }

        if (i == 0 && j == n - 1) {
            continue;
        }

        long long newCost = invertDeltaCost(current, dist, i, j, currentCost);

        if (newCost < currentCost) {
            reverse(current.begin() + i, current.begin() + j + 1);
            currentCost = newCost;
        }
    }

    return {move(current), currentCost};
}

vector<Individual> createInitialPopulation(
    const DistanceMatrix& dist,
    const Params& params,
    mt19937& rng
) {
    int n = dist.n;

    vector<Individual> population;
    population.reserve(params.populationSize);

    int nnCount = 0;

    if (params.useNearestNeighborInit) {
        if (n > 5000) {
            nnCount = 1;
        } else if (n > 2500) {
            nnCount = 2;
        } else {
            nnCount = max(1, params.populationSize / 5);
        }
    }

    uniform_int_distribution<int> startDist(0, n - 1);

    for (int i = 0; i < nnCount; i++) {
        int start = startDist(rng);
        vector<int> route = nearestNeighborTour(dist, start);
        population.push_back(makeIndividual(move(route), dist));
    }

    for (int i = nnCount; i < params.populationSize; i++) {
        population.push_back(makeIndividual(randomRoute(n, rng), dist));
    }

    sort(population.begin(), population.end(), betterIndividual);

    return population;
}

vector<Individual> evolveOneIsland(
    vector<Individual>& population,
    const DistanceMatrix& dist,
    const Params& params,
    mt19937& rng
) {
    sort(population.begin(), population.end(), betterIndividual);

    vector<Individual> newPopulation;
    newPopulation.reserve(population.size());

    for (int i = 0; i < params.eliteCount && i < (int)population.size(); i++) {
        newPopulation.push_back(population[i]);
    }

    uniform_real_distribution<double> realDist(0.0, 1.0);

    while ((int)newPopulation.size() < (int)population.size()) {
        int p1Index = tournamentSelectIndex(
            population,
            params.tournamentSize,
            rng
        );

        int p2Index = tournamentSelectIndex(
            population,
            params.tournamentSize,
            rng
        );

        int safety = 0;

        while (p1Index == p2Index && safety < 10) {
            p2Index = tournamentSelectIndex(
                population,
                params.tournamentSize,
                rng
            );
            safety++;
        }

        const vector<int>& p1 = population[p1Index].route;
        const vector<int>& p2 = population[p2Index].route;

        vector<int> child1;
        vector<int> child2;

        if (realDist(rng) < params.crossoverRate) {
            child1 = crossover(p1, p2, params.crossoverType, rng);
            child2 = crossover(p2, p1, params.crossoverType, rng);
        } else {
            child1 = p1;
            child2 = p2;
        }

        if (realDist(rng) < params.mutationRate) {
            mutateInvert(child1, rng);
        }

        if (realDist(rng) < params.mutationRate) {
            mutateInvert(child2, rng);
        }

        Individual ind1 = makeIndividual(move(child1), dist);
        Individual ind2 = makeIndividual(move(child2), dist);

        if (params.memeticEnabled) {
            auto improved1 = localMemeticSearch(
                ind1.route,
                ind1.cost,
                dist,
                rng,
                params.memeticAttempts
            );

            ind1.route = move(improved1.first);
            ind1.cost = improved1.second;

            auto improved2 = localMemeticSearch(
                ind2.route,
                ind2.cost,
                dist,
                rng,
                params.memeticAttempts
            );

            ind2.route = move(improved2.first);
            ind2.cost = improved2.second;
        }

        newPopulation.push_back(move(ind1));

        if ((int)newPopulation.size() < (int)population.size()) {
            newPopulation.push_back(move(ind2));
        }
    }

    sort(newPopulation.begin(), newPopulation.end(), betterIndividual);

    if (params.memeticEnabled && params.memeticTopK > 0) {
        int limit = min(params.memeticTopK, (int)newPopulation.size());

        for (int i = 0; i < limit; i++) {
            auto improved = localMemeticSearch(
                newPopulation[i].route,
                newPopulation[i].cost,
                dist,
                rng,
                params.memeticAttempts
            );

            newPopulation[i].route = move(improved.first);
            newPopulation[i].cost = improved.second;
        }

        sort(newPopulation.begin(), newPopulation.end(), betterIndividual);
    }

    return newPopulation;
}

void migrate(vector<vector<Individual>>& islands, const Params& params) {
    int islandCount = islands.size();

    if (islandCount <= 1 || params.migrationCount <= 0) {
        return;
    }

    vector<vector<Individual>> migrants(islandCount);

    for (int i = 0; i < islandCount; i++) {
        sort(islands[i].begin(), islands[i].end(), betterIndividual);

        int count = min(params.migrationCount, (int)islands[i].size());

        for (int j = 0; j < count; j++) {
            migrants[i].push_back(islands[i][j]);
        }
    }

    for (int i = 0; i < islandCount; i++) {
        int targetIndex = (i + 1) % islandCount;
        vector<Individual>& target = islands[targetIndex];

        sort(target.begin(), target.end(), [](const Individual& a, const Individual& b) {
            return a.cost > b.cost;
        });

        int count = min((int)migrants[i].size(), (int)target.size());

        for (int j = 0; j < count; j++) {
            target[j] = migrants[i][j];
        }

        sort(target.begin(), target.end(), betterIndividual);
    }
}

Params applyAutoConfig(int n, Params params) {
    if (!params.autoConfig) {
        return params;
    }

    int runs = params.runs;
    int threads = params.threads;
    unsigned seed = params.seed;

    string crossoverType = params.crossoverType;

    string historyFile = params.historyFile;
    string bestRouteFile = params.bestRouteFile;
    string pointsFile = params.pointsFile;
    string runsFile = params.runsFile;

    Params cfg;

    if (n <= 300) {
        cfg.populationSize = 84;
        cfg.generations = 120;
        cfg.islands = 3;
        cfg.eliteCount = 2;
        cfg.crossoverRate = 0.95;
        cfg.mutationRate = 0.20;
        cfg.tournamentSize = 3;
        cfg.migrationInterval = 10;
        cfg.migrationCount = 1;
        cfg.noImproveLimit = 32;
        cfg.memeticEnabled = true;
        cfg.memeticTopK = 3;
        cfg.memeticAttempts = 18;
        cfg.useNearestNeighborInit = true;
    } else if (n <= 1000) {
        cfg.populationSize = 64;
        cfg.generations = 90;
        cfg.islands = 3;
        cfg.eliteCount = 2;
        cfg.crossoverRate = 0.95;
        cfg.mutationRate = 0.21;
        cfg.tournamentSize = 3;
        cfg.migrationInterval = 9;
        cfg.migrationCount = 1;
        cfg.noImproveLimit = 26;
        cfg.memeticEnabled = true;
        cfg.memeticTopK = 2;
        cfg.memeticAttempts = 12;
        cfg.useNearestNeighborInit = true;
    } else if (n <= 2500) {
        cfg.populationSize = 48;
        cfg.generations = 60;
        cfg.islands = 2;
        cfg.eliteCount = 2;
        cfg.crossoverRate = 0.95;
        cfg.mutationRate = 0.22;
        cfg.tournamentSize = 3;
        cfg.migrationInterval = 8;
        cfg.migrationCount = 1;
        cfg.noImproveLimit = 18;
        cfg.memeticEnabled = true;
        cfg.memeticTopK = 1;
        cfg.memeticAttempts = 8;
        cfg.useNearestNeighborInit = true;
    } else if (n <= 5000) {
        cfg.populationSize = 28;
        cfg.generations = 32;
        cfg.islands = 2;
        cfg.eliteCount = 1;
        cfg.crossoverRate = 0.95;
        cfg.mutationRate = 0.24;
        cfg.tournamentSize = 3;
        cfg.migrationInterval = 6;
        cfg.migrationCount = 1;
        cfg.noImproveLimit = 10;
        cfg.memeticEnabled = false;
        cfg.memeticTopK = 0;
        cfg.memeticAttempts = 0;
        cfg.useNearestNeighborInit = true;
    } else {
        cfg.populationSize = 18;
        cfg.generations = 20;
        cfg.islands = 2;
        cfg.eliteCount = 1;
        cfg.crossoverRate = 0.95;
        cfg.mutationRate = 0.25;
        cfg.tournamentSize = 3;
        cfg.migrationInterval = 5;
        cfg.migrationCount = 1;
        cfg.noImproveLimit = 8;
        cfg.memeticEnabled = false;
        cfg.memeticTopK = 0;
        cfg.memeticAttempts = 0;
        cfg.useNearestNeighborInit = true;
    }

    cfg.runs = runs;
    cfg.threads = threads;
    cfg.seed = seed;
    cfg.crossoverType = crossoverType;

    cfg.autoConfig = true;

    cfg.historyFile = historyFile;
    cfg.bestRouteFile = bestRouteFile;
    cfg.pointsFile = pointsFile;
    cfg.runsFile = runsFile;

    return cfg;
}

RunResult geneticAlgorithmSingleRun(
    const DistanceMatrix& dist,
    Params params,
    unsigned seed
) {
    int n = dist.n;

    mt19937 rng(seed);

    vector<vector<Individual>> islands;
    islands.reserve(params.islands);

    for (int i = 0; i < params.islands; i++) {
        islands.push_back(createInitialPopulation(dist, params, rng));
    }

    Individual globalBest = islands[0][0];

    for (const auto& island : islands) {
        if (island[0].cost < globalBest.cost) {
            globalBest = island[0];
        }
    }

    vector<long long> history;
    history.reserve(params.generations);

    int noImprove = 0;
    int generationsDone = 0;

    for (int generation = 1; generation <= params.generations; generation++) {
        generationsDone = generation;

        for (int i = 0; i < params.islands; i++) {
            islands[i] = evolveOneIsland(islands[i], dist, params, rng);
        }

        if (
            params.migrationInterval > 0 &&
            generation % params.migrationInterval == 0
        ) {
            migrate(islands, params);
        }

        long long currentBestCost = LLONG_MAX;
        int bestIslandIndex = 0;

        for (int i = 0; i < params.islands; i++) {
            sort(islands[i].begin(), islands[i].end(), betterIndividual);

            if (islands[i][0].cost < currentBestCost) {
                currentBestCost = islands[i][0].cost;
                bestIslandIndex = i;
            }
        }

        if (currentBestCost < globalBest.cost) {
            globalBest = islands[bestIslandIndex][0];
            noImprove = 0;
        } else {
            noImprove++;
        }

        history.push_back(globalBest.cost);

        if (noImprove >= params.noImproveLimit) {
            break;
        }
    }

    RunResult result;
    result.bestRoute = move(globalBest.route);
    result.bestCost = globalBest.cost;
    result.history = move(history);
    result.generationsDone = generationsDone;
    result.seed = seed;

    return result;
}

void savePointsCSV(const vector<Point>& points, const string& filename) {
    ofstream file(filename);

    if (!file) {
        throw runtime_error("Nie mozna utworzyc pliku: " + filename);
    }

    file << "id,x,y\n";

    for (const auto& p : points) {
        file << p.id << "," << p.x << "," << p.y << "\n";
    }
}

void saveBestRouteCSV(
    const vector<Point>& points,
    const vector<int>& best,
    const string& filename
) {
    ofstream file(filename);

    if (!file) {
        throw runtime_error("Nie mozna utworzyc pliku: " + filename);
    }

    file << "id,x,y,order\n";

    for (int i = 0; i < (int)best.size(); i++) {
        int idx = best[i];

        file << points[idx].id << ","
             << points[idx].x << ","
             << points[idx].y << ","
             << i << "\n";
    }

    int first = best[0];

    file << points[first].id << ","
         << points[first].x << ","
         << points[first].y << ","
         << best.size() << "\n";
}

void saveHistoryCSV(const vector<long long>& history, const string& filename) {
    ofstream file(filename);

    if (!file) {
        throw runtime_error("Nie mozna utworzyc pliku: " + filename);
    }

    file << "generation,bestCost\n";

    for (int i = 0; i < (int)history.size(); i++) {
        file << i + 1 << "," << history[i] << "\n";
    }
}

void saveRunsCSV(const vector<RunResult>& results, const string& filename) {
    ofstream file(filename);

    if (!file) {
        throw runtime_error("Nie mozna utworzyc pliku: " + filename);
    }

    file << "run,seed,bestCost,generationsDone\n";

    for (int i = 0; i < (int)results.size(); i++) {
        file << i + 1 << ","
             << results[i].seed << ","
             << results[i].bestCost << ","
             << results[i].generationsDone << "\n";
    }
}

void runGeneticAlgorithm(
    const vector<Point>& points,
    Params params
) {
    int n = points.size();

    params = applyAutoConfig(n, params);

    cout << fixed << setprecision(4);

    cout << "========================================\n";
    cout << "Algorytm genetyczny dla TSP - improved\n";
    cout << "========================================\n";
    cout << "Liczba wierzcholkow: " << n << "\n";
    cout << "Auto config: " << (params.autoConfig ? "tak" : "nie") << "\n";
    cout << "Populacja na wyspe: " << params.populationSize << "\n";
    cout << "Liczba wysp: " << params.islands << "\n";
    cout << "Pokolenia: " << params.generations << "\n";
    cout << "Limit bez poprawy: " << params.noImproveLimit << "\n";
    cout << "Elita: " << params.eliteCount << "\n";
    cout << "Crossover rate: " << params.crossoverRate << "\n";
    cout << "Mutation rate: " << params.mutationRate << "\n";
    cout << "Turniej: " << params.tournamentSize << "\n";
    cout << "Migracja co: " << params.migrationInterval << "\n";
    cout << "Liczba migrantow: " << params.migrationCount << "\n";
    cout << "Krzyzowanie: " << params.crossoverType << "\n";
    cout << "Nearest neighbor init: "
         << (params.useNearestNeighborInit ? "tak" : "nie") << "\n";
    cout << "Memetyka: " << (params.memeticEnabled ? "tak" : "nie") << "\n";
    cout << "Memetic top K: " << params.memeticTopK << "\n";
    cout << "Memetic attempts: " << params.memeticAttempts << "\n";
    cout << "Liczba prob: " << params.runs << "\n";
    cout << "Watki: " << params.threads << "\n";
    cout << "Seed bazowy: " << params.seed << "\n";
    cout << "========================================\n\n";

    cout << "Budowanie macierzy odleglosci...\n";
    DistanceMatrix dist = buildDistanceMatrix(points);
    cout << "Macierz gotowa.\n\n";

    vector<RunResult> results(params.runs);

    atomic<int> nextRun(0);
    mutex coutMutex;

    int threadCount = max(1, params.threads);
    vector<thread> workers;

    auto workerFunction = [&]() {
        while (true) {
            int runIndex = nextRun.fetch_add(1);

            if (runIndex >= params.runs) {
                break;
            }

            unsigned runSeed = params.seed + runIndex * 1000003u;

            RunResult result = geneticAlgorithmSingleRun(
                dist,
                params,
                runSeed
            );

            results[runIndex] = move(result);

            lock_guard<mutex> lock(coutMutex);

            cout << "Proba " << setw(4) << runIndex + 1 << "/"
                 << params.runs
                 << " | seed = " << setw(10) << runSeed
                 << " | najlepszy koszt = " << setw(12)
                 << results[runIndex].bestCost
                 << " | pokolenia = "
                 << results[runIndex].generationsDone
                 << "\n";
        }
    };

    for (int t = 0; t < threadCount; t++) {
        workers.emplace_back(workerFunction);
    }

    for (auto& worker : workers) {
        worker.join();
    }

    int bestRunIndex = 0;
    long long sum = 0;

    for (int i = 0; i < params.runs; i++) {
        sum += results[i].bestCost;

        if (results[i].bestCost < results[bestRunIndex].bestCost) {
            bestRunIndex = i;
        }
    }

    double average = (double)sum / params.runs;

    const RunResult& bestResult = results[bestRunIndex];

    saveRunsCSV(results, params.runsFile);
    saveHistoryCSV(bestResult.history, params.historyFile);
    saveBestRouteCSV(points, bestResult.bestRoute, params.bestRouteFile);
    savePointsCSV(points, params.pointsFile);

    cout << "\n========================================\n";
    cout << "Wyniki koncowe\n";
    cout << "========================================\n";
    cout << "Najlepszy koszt: " << bestResult.bestCost << "\n";
    cout << "Sredni koszt: " << average << "\n";
    cout << "Najlepsza proba: " << bestRunIndex + 1 << "\n";
    cout << "Seed najlepszej proby: " << bestResult.seed << "\n";
    cout << "Pokolenia najlepszej proby: "
         << bestResult.generationsDone << "\n\n";

    cout << "Zapisano pliki:\n";
    cout << "  " << params.runsFile << "\n";
    cout << "  " << params.historyFile << "\n";
    cout << "  " << params.bestRouteFile << "\n";
    cout << "  " << params.pointsFile << "\n";
}

void printUsage(const string& programName) {
    cout << "Uzycie:\n";
    cout << "  " << programName << " plik.tsp [parametry]\n\n";

    cout << "Parametry opcjonalne:\n";
    cout << "  --auto-config 0/1       automatyczny dobor parametrow, domyslnie 1\n";
    cout << "  --population N          rozmiar populacji na wyspe\n";
    cout << "  --generations N         maksymalna liczba pokolen\n";
    cout << "  --islands N             liczba wysp\n";
    cout << "  --elite N               liczba elit\n";
    cout << "  --crossover-rate X      prawdopodobienstwo krzyzowania\n";
    cout << "  --mutation-rate X       prawdopodobienstwo mutacji\n";
    cout << "  --tournament N          rozmiar turnieju\n";
    cout << "  --migration-interval N  migracja co N pokolen\n";
    cout << "  --migration-count N     liczba migrantow\n";
    cout << "  --no-improve N          limit pokolen bez poprawy\n";
    cout << "  --crossover TYPE        ox albo pmx\n";
    cout << "  --nn-init 0/1           inicjalizacja nearest-neighbor\n";
    cout << "  --memetic 0/1           wlacz lokalny 2-opt\n";
    cout << "  --memetic-top N         ile najlepszych dodatkowo poprawiac\n";
    cout << "  --memetic-attempts N    liczba prob 2-opt w lokalnym poprawianiu\n";
    cout << "  --runs N                liczba prob\n";
    cout << "  --threads N             liczba watkow\n";
    cout << "  --seed N                seed bazowy\n";
    cout << "  --history FILE          plik historii\n";
    cout << "  --route FILE            plik najlepszej trasy\n";
    cout << "  --points FILE           plik punktow\n";
    cout << "  --runs-file FILE        plik wynikow prob\n\n";

    cout << "Przyklad:\n";
    cout << "  " << programName
         << " data/eg7146.tsp --auto-config 1 --crossover ox "
         << "--runs 100 --threads 8 --seed 123\n";
}

Params parseParams(int argc, char** argv) {
    Params p;

    for (int i = 2; i < argc; i++) {
        string arg = argv[i];

        auto requireValue = [&](const string& name) {
            if (i + 1 >= argc) {
                throw runtime_error("Brakuje wartosci dla parametru " + name);
            }
        };

        if (arg == "--auto-config") {
            requireValue(arg);
            p.autoConfig = stoi(argv[++i]) != 0;
        } else if (arg == "--population") {
            requireValue(arg);
            p.populationSize = stoi(argv[++i]);
        } else if (arg == "--generations") {
            requireValue(arg);
            p.generations = stoi(argv[++i]);
        } else if (arg == "--islands") {
            requireValue(arg);
            p.islands = stoi(argv[++i]);
        } else if (arg == "--elite") {
            requireValue(arg);
            p.eliteCount = stoi(argv[++i]);
        } else if (arg == "--crossover-rate") {
            requireValue(arg);
            p.crossoverRate = stod(argv[++i]);
        } else if (arg == "--mutation-rate") {
            requireValue(arg);
            p.mutationRate = stod(argv[++i]);
        } else if (arg == "--tournament") {
            requireValue(arg);
            p.tournamentSize = stoi(argv[++i]);
        } else if (arg == "--migration-interval") {
            requireValue(arg);
            p.migrationInterval = stoi(argv[++i]);
        } else if (arg == "--migration-count") {
            requireValue(arg);
            p.migrationCount = stoi(argv[++i]);
        } else if (arg == "--no-improve") {
            requireValue(arg);
            p.noImproveLimit = stoi(argv[++i]);
        } else if (arg == "--crossover") {
            requireValue(arg);
            p.crossoverType = argv[++i];

            if (p.crossoverType != "ox" && p.crossoverType != "pmx") {
                throw runtime_error("Nieznany crossover. Uzyj: ox albo pmx.");
            }
        } else if (arg == "--nn-init") {
            requireValue(arg);
            p.useNearestNeighborInit = stoi(argv[++i]) != 0;
        } else if (arg == "--memetic") {
            requireValue(arg);
            p.memeticEnabled = stoi(argv[++i]) != 0;
        } else if (arg == "--memetic-top") {
            requireValue(arg);
            p.memeticTopK = stoi(argv[++i]);
        } else if (arg == "--memetic-attempts") {
            requireValue(arg);
            p.memeticAttempts = stoi(argv[++i]);
        } else if (arg == "--runs") {
            requireValue(arg);
            p.runs = stoi(argv[++i]);
        } else if (arg == "--threads") {
            requireValue(arg);
            p.threads = stoi(argv[++i]);
        } else if (arg == "--seed") {
            requireValue(arg);
            p.seed = static_cast<unsigned>(stoul(argv[++i]));
        } else if (arg == "--history") {
            requireValue(arg);
            p.historyFile = argv[++i];
        } else if (arg == "--route") {
            requireValue(arg);
            p.bestRouteFile = argv[++i];
        } else if (arg == "--points") {
            requireValue(arg);
            p.pointsFile = argv[++i];
        } else if (arg == "--runs-file") {
            requireValue(arg);
            p.runsFile = argv[++i];
        } else {
            throw runtime_error("Nieznany parametr: " + arg);
        }
    }

    if (p.populationSize < 2) {
        throw runtime_error("Populacja musi miec co najmniej 2 osobniki.");
    }

    if (p.generations <= 0) {
        throw runtime_error("Liczba pokolen musi byc dodatnia.");
    }

    if (p.islands <= 0) {
        throw runtime_error("Liczba wysp musi byc dodatnia.");
    }

    if (p.eliteCount < 0 || p.eliteCount >= p.populationSize) {
        throw runtime_error("Elite musi byc z zakresu [0, populationSize).");
    }

    if (p.crossoverRate < 0 || p.crossoverRate > 1) {
        throw runtime_error("Crossover rate musi byc z zakresu [0, 1].");
    }

    if (p.mutationRate < 0 || p.mutationRate > 1) {
        throw runtime_error("Mutation rate musi byc z zakresu [0, 1].");
    }

    if (p.tournamentSize <= 0) {
        throw runtime_error("Rozmiar turnieju musi byc dodatni.");
    }

    if (p.migrationInterval < 0) {
        throw runtime_error("Migration interval nie moze byc ujemny.");
    }

    if (p.migrationCount < 0) {
        throw runtime_error("Migration count nie moze byc ujemny.");
    }

    if (p.noImproveLimit <= 0) {
        throw runtime_error("Limit bez poprawy musi byc dodatni.");
    }

    if (p.memeticTopK < 0 || p.memeticAttempts < 0) {
        throw runtime_error("Parametry memetyki nie moga byc ujemne.");
    }

    if (p.runs <= 0) {
        throw runtime_error("Liczba prob musi byc dodatnia.");
    }

    if (p.threads <= 0) {
        throw runtime_error("Liczba watkow musi byc dodatnia.");
    }

    return p;
}

int main(int argc, char** argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2) {
        printUsage(argv[0]);
        return 0;
    }

    string filename = argv[1];

    try {
        Params params = parseParams(argc, argv);
        vector<Point> points = readTSP(filename);

        runGeneticAlgorithm(points, params);

    } catch (const exception& e) {
        cerr << "\nBlad: " << e.what() << "\n\n";
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
