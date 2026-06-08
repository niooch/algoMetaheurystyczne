#include <bits/stdc++.h>
using namespace std;

struct Point {
    int id;
    double x, y;
};

struct Individual {
    vector<int> route;
    double cost = numeric_limits<double>::max();
};

struct Params {
    int populationSize = 200;
    int generations = 2000;
    int noImproveLimit = 500;

    double crossoverRate = 0.9;
    double mutationRate = 0.15;

    int eliteCount = 2;
    int tournamentSize = 5;

    string crossoverType = "ox"; // ox albo pmx
    int runs = 1;

    unsigned seed = chrono::steady_clock::now().time_since_epoch().count();

    string historyFile = "history.csv";
    string bestRouteFile = "best_route.csv";
    string pointsFile = "points.csv";
    string runsFile = "runs.csv";
};

struct RunResult {
    vector<int> bestRoute;
    double bestCost = numeric_limits<double>::max();
    vector<double> history;
    int generationsDone = 0;
    unsigned seed = 0;
};

double distEuclidean(const Point& a, const Point& b) {
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    return sqrt(dx * dx + dy * dy);
}

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

        if (!coordSection) continue;
        if (line.find("EOF") != string::npos) break;
        if (line.empty()) continue;

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

vector<vector<double>> buildDistanceMatrix(const vector<Point>& points) {
    int n = points.size();
    vector<vector<double>> dist(n, vector<double>(n, 0.0));

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            dist[i][j] = distEuclidean(points[i], points[j]);
        }
    }

    return dist;
}

double routeLength(const vector<int>& route, const vector<vector<double>>& dist) {
    int n = route.size();
    double sum = 0.0;

    for (int i = 0; i < n; i++) {
        int a = route[i];
        int b = route[(i + 1) % n];
        sum += dist[a][b];
    }

    return sum;
}

vector<int> randomRoute(int n, mt19937& rng) {
    vector<int> route(n);
    iota(route.begin(), route.end(), 0);
    shuffle(route.begin(), route.end(), rng);
    return route;
}

Individual makeIndividual(const vector<int>& route, const vector<vector<double>>& dist) {
    Individual ind;
    ind.route = route;
    ind.cost = routeLength(route, dist);
    return ind;
}

bool betterIndividual(const Individual& a, const Individual& b) {
    return a.cost < b.cost;
}

Individual tournamentSelection(
    const vector<Individual>& population,
    int tournamentSize,
    mt19937& rng
) {
    uniform_int_distribution<int> distPop(0, (int)population.size() - 1);

    Individual best = population[distPop(rng)];

    for (int i = 1; i < tournamentSize; i++) {
        const Individual& candidate = population[distPop(rng)];

        if (candidate.cost < best.cost) {
            best = candidate;
        }
    }

    return best;
}

/*
    OX - Order Crossover.
    Działa dobrze dla permutacji.
*/
vector<int> crossoverOX(
    const vector<int>& p1,
    const vector<int>& p2,
    mt19937& rng
) {
    int n = p1.size();
    vector<int> child(n, -1);

    uniform_int_distribution<int> posDist(0, n - 1);

    int a = posDist(rng);
    int b = posDist(rng);

    if (a > b) swap(a, b);

    vector<int> used(n, 0);

    for (int i = a; i <= b; i++) {
        child[i] = p1[i];
        used[p1[i]] = 1;
    }

    int childPos = (b + 1) % n;

    for (int k = 0; k < n; k++) {
        int gene = p2[(b + 1 + k) % n];

        if (!used[gene]) {
            child[childPos] = gene;
            used[gene] = 1;
            childPos = (childPos + 1) % n;
        }
    }

    return child;
}

/*
    PMX - Partially Mapped Crossover.
*/
vector<int> crossoverPMX(
    const vector<int>& p1,
    const vector<int>& p2,
    mt19937& rng
) {
    int n = p1.size();
    vector<int> child(n, -1);

    uniform_int_distribution<int> posDist(0, n - 1);

    int a = posDist(rng);
    int b = posDist(rng);

    if (a > b) swap(a, b);

    vector<int> positionInP2(n);

    for (int i = 0; i < n; i++) {
        positionInP2[p2[i]] = i;
    }

    for (int i = a; i <= b; i++) {
        child[i] = p1[i];
    }

    for (int i = a; i <= b; i++) {
        int gene = p2[i];

        bool alreadyInChild = false;

        for (int j = a; j <= b; j++) {
            if (child[j] == gene) {
                alreadyInChild = true;
                break;
            }
        }

        if (!alreadyInChild) {
            int pos = i;

            while (true) {
                int mappedGene = p1[pos];
                pos = positionInP2[mappedGene];

                if (child[pos] == -1) {
                    child[pos] = gene;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < n; i++) {
        if (child[i] == -1) {
            child[i] = p2[i];
        }
    }

    return child;
}

vector<int> crossover(
    const vector<int>& p1,
    const vector<int>& p2,
    const string& type,
    mt19937& rng
) {
    if (type == "pmx") {
        return crossoverPMX(p1, p2, rng);
    }

    return crossoverOX(p1, p2, rng);
}

/*
    Mutacja: odwrócenie losowego fragmentu trasy, czyli ruch 2-opt.
    Jest lepsza dla TSP niż zwykła zamiana dwóch wierzchołków.
*/
void mutateReverse(vector<int>& route, mt19937& rng) {
    int n = route.size();
    uniform_int_distribution<int> posDist(0, n - 1);

    int i = posDist(rng);
    int j = posDist(rng);

    while (i == j) {
        j = posDist(rng);
    }

    if (i > j) swap(i, j);

    reverse(route.begin() + i, route.begin() + j + 1);
}

vector<Individual> generateInitialPopulation(
    int populationSize,
    int n,
    const vector<vector<double>>& dist,
    mt19937& rng
) {
    vector<Individual> population;
    population.reserve(populationSize);

    for (int i = 0; i < populationSize; i++) {
        vector<int> route = randomRoute(n, rng);
        population.push_back(makeIndividual(route, dist));
    }

    sort(population.begin(), population.end(), betterIndividual);

    return population;
}

RunResult geneticAlgorithmSingleRun(
    const vector<Point>& points,
    const vector<vector<double>>& dist,
    const Params& params,
    unsigned seed
) {
    int n = points.size();

    mt19937 rng(seed);
    uniform_real_distribution<double> realDist(0.0, 1.0);

    vector<Individual> population = generateInitialPopulation(
        params.populationSize,
        n,
        dist,
        rng
    );

    Individual globalBest = population[0];

    vector<double> history;
    int generationsWithoutImprovement = 0;
    int generationsDone = 0;

    for (int generation = 1; generation <= params.generations; generation++) {
        generationsDone = generation;

        sort(population.begin(), population.end(), betterIndividual);

        if (population[0].cost < globalBest.cost) {
            globalBest = population[0];
            generationsWithoutImprovement = 0;
        } else {
            generationsWithoutImprovement++;
        }

        history.push_back(globalBest.cost);

        if (generationsWithoutImprovement >= params.noImproveLimit) {
            break;
        }

        vector<Individual> newPopulation;
        newPopulation.reserve(params.populationSize);

        /*
            Elityzm: kilka najlepszych osobników przechodzi bez zmian.
        */
        for (int i = 0; i < params.eliteCount && i < (int)population.size(); i++) {
            newPopulation.push_back(population[i]);
        }

        while ((int)newPopulation.size() < params.populationSize) {
            Individual parent1 = tournamentSelection(
                population,
                params.tournamentSize,
                rng
            );

            Individual parent2 = tournamentSelection(
                population,
                params.tournamentSize,
                rng
            );

            /*
                Staramy się unikać krzyżowania identycznych rodziców.
            */
            int safety = 0;
            while (parent1.route == parent2.route && safety < 10) {
                parent2 = tournamentSelection(population, params.tournamentSize, rng);
                safety++;
            }

            vector<int> childRoute;

            if (realDist(rng) < params.crossoverRate) {
                childRoute = crossover(
                    parent1.route,
                    parent2.route,
                    params.crossoverType,
                    rng
                );
            } else {
                childRoute = parent1.route;
            }

            if (realDist(rng) < params.mutationRate) {
                mutateReverse(childRoute, rng);
            }

            newPopulation.push_back(makeIndividual(childRoute, dist));
        }

        population = move(newPopulation);
    }

    RunResult result;
    result.bestRoute = globalBest.route;
    result.bestCost = globalBest.cost;
    result.history = history;
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

void saveHistoryCSV(const vector<double>& history, const string& filename) {
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
    const Params& params
) {
    int n = points.size();

    vector<vector<double>> dist = buildDistanceMatrix(points);

    vector<RunResult> results;
    results.reserve(params.runs);

    cout << fixed << setprecision(4);

    cout << "========================================\n";
    cout << "Algorytm genetyczny dla TSP\n";
    cout << "========================================\n";
    cout << "Liczba wierzcholkow: " << n << "\n";
    cout << "Populacja: " << params.populationSize << "\n";
    cout << "Pokolenia: " << params.generations << "\n";
    cout << "Limit bez poprawy: " << params.noImproveLimit << "\n";
    cout << "Crossover rate: " << params.crossoverRate << "\n";
    cout << "Mutation rate: " << params.mutationRate << "\n";
    cout << "Elita: " << params.eliteCount << "\n";
    cout << "Turniej: " << params.tournamentSize << "\n";
    cout << "Krzyzowanie: " << params.crossoverType << "\n";
    cout << "Liczba prob: " << params.runs << "\n";
    cout << "Seed bazowy: " << params.seed << "\n";
    cout << "========================================\n\n";

    for (int run = 0; run < params.runs; run++) {
        unsigned runSeed = params.seed + run * 1000003u;

        RunResult result = geneticAlgorithmSingleRun(
            points,
            dist,
            params,
            runSeed
        );

        results.push_back(result);

        cout << "Proba " << setw(4) << run + 1 << "/"
             << params.runs
             << " | seed = " << setw(10) << runSeed
             << " | najlepszy koszt = " << setw(12) << result.bestCost
             << " | pokolenia = " << result.generationsDone
             << "\n";
    }

    int bestRunIndex = 0;
    double sum = 0.0;

    for (int i = 0; i < (int)results.size(); i++) {
        sum += results[i].bestCost;

        if (results[i].bestCost < results[bestRunIndex].bestCost) {
            bestRunIndex = i;
        }
    }

    double average = sum / results.size();

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
    cout << "Pokolenia najlepszej proby: " << bestResult.generationsDone << "\n\n";

    cout << "Najlepsza trasa:\n";

    for (int idx : bestResult.bestRoute) {
        cout << points[idx].id << " ";
    }

    cout << points[bestResult.bestRoute[0]].id << "\n\n";

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
    cout << "  --population N          rozmiar populacji, np. 200\n";
    cout << "  --generations N         maksymalna liczba pokolen, np. 2000\n";
    cout << "  --no-improve N          stop po N pokoleniach bez poprawy, np. 500\n";
    cout << "  --crossover-rate X      prawdopodobienstwo krzyzowania, np. 0.9\n";
    cout << "  --mutation-rate X       prawdopodobienstwo mutacji, np. 0.15\n";
    cout << "  --elite N               liczba elit, np. 2\n";
    cout << "  --tournament N          rozmiar turnieju, np. 5\n";
    cout << "  --crossover TYPE        ox albo pmx\n";
    cout << "  --runs N                liczba prob, np. 100\n";
    cout << "  --seed N                seed bazowy\n";
    cout << "  --history FILE          plik historii\n";
    cout << "  --route FILE            plik najlepszej trasy\n";
    cout << "  --points FILE           plik punktow\n";
    cout << "  --runs-file FILE        plik wynikow prob\n\n";

    cout << "Przyklad:\n";
    cout << "  " << programName
         << " eg7146.tsp --population 250 --generations 3000 "
         << "--no-improve 700 --crossover ox --runs 100 --seed 123\n";
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

        if (arg == "--population") {
            requireValue(arg);
            p.populationSize = stoi(argv[++i]);
        } else if (arg == "--generations") {
            requireValue(arg);
            p.generations = stoi(argv[++i]);
        } else if (arg == "--no-improve") {
            requireValue(arg);
            p.noImproveLimit = stoi(argv[++i]);
        } else if (arg == "--crossover-rate") {
            requireValue(arg);
            p.crossoverRate = stod(argv[++i]);
        } else if (arg == "--mutation-rate") {
            requireValue(arg);
            p.mutationRate = stod(argv[++i]);
        } else if (arg == "--elite") {
            requireValue(arg);
            p.eliteCount = stoi(argv[++i]);
        } else if (arg == "--tournament") {
            requireValue(arg);
            p.tournamentSize = stoi(argv[++i]);
        } else if (arg == "--crossover") {
            requireValue(arg);
            p.crossoverType = argv[++i];

            if (p.crossoverType != "ox" && p.crossoverType != "pmx") {
                throw runtime_error("Nieznany crossover. Uzyj: ox albo pmx.");
            }
        } else if (arg == "--runs") {
            requireValue(arg);
            p.runs = stoi(argv[++i]);
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

    if (p.noImproveLimit <= 0) {
        throw runtime_error("Limit bez poprawy musi byc dodatni.");
    }

    if (p.crossoverRate < 0 || p.crossoverRate > 1) {
        throw runtime_error("Crossover rate musi byc z zakresu [0, 1].");
    }

    if (p.mutationRate < 0 || p.mutationRate > 1) {
        throw runtime_error("Mutation rate musi byc z zakresu [0, 1].");
    }

    if (p.eliteCount < 0 || p.eliteCount >= p.populationSize) {
        throw runtime_error("Elite musi byc z zakresu [0, populationSize).");
    }

    if (p.tournamentSize <= 0) {
        throw runtime_error("Rozmiar turnieju musi byc dodatni.");
    }

    if (p.runs <= 0) {
        throw runtime_error("Liczba prob musi byc dodatnia.");
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
