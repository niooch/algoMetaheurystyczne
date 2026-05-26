#include <bits/stdc++.h>
using namespace std;

struct Point {
    int id;
    double x, y;
};

struct Params {
    int maxIterations = 10000;
    int noImproveLimit = 2000;
    int tabuTenure = 50;

    /*
        Jeśli candidateSample = 0, algorytm przegląda deterministycznie całe sąsiedztwo 2-opt.
        Jeśli candidateSample > 0, algorytm losuje tylko tyle kandydatów w każdej iteracji.
    */
    int candidateSample = 0;

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
    int iterationsDone = 0;
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

double routeLength(
    const vector<int>& route,
    const vector<vector<double>>& dist
) {
    int n = route.size();
    double sum = 0.0;

    for (int i = 0; i < n; i++) {
        int a = route[i];
        int b = route[(i + 1) % n];
        sum += dist[a][b];
    }

    return sum;
}

vector<int> randomInitialRoute(int n, mt19937& rng) {
    vector<int> route(n);
    iota(route.begin(), route.end(), 0);
    shuffle(route.begin(), route.end(), rng);
    return route;
}

/*
    Delta dla ruchu 2-opt.

    Odwracamy fragment trasy od i do j.

    Przed:
        a - b ... c - d

    Po:
        a - c ... b - d

    Zmieniamy tylko dwie krawędzie, więc koszt liczymy w O(1).
*/
double calculate2OptDelta(
    const vector<int>& route,
    const vector<vector<double>>& dist,
    int i,
    int j
) {
    int n = route.size();

    int a = route[(i - 1 + n) % n];
    int b = route[i];
    int c = route[j];
    int d = route[(j + 1) % n];

    double oldEdges = dist[a][b] + dist[c][d];
    double newEdges = dist[a][c] + dist[b][d];

    return newEdges - oldEdges;
}

void savePointsCSV(
    const vector<Point>& points,
    const string& filename
) {
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

void saveHistoryCSV(
    const vector<double>& history,
    const string& filename
) {
    ofstream file(filename);

    if (!file) {
        throw runtime_error("Nie mozna utworzyc pliku: " + filename);
    }

    file << "iteration,bestCost\n";

    for (int i = 0; i < (int)history.size(); i++) {
        file << i + 1 << "," << history[i] << "\n";
    }
}

RunResult tabuSearchSingleRun(
    const vector<Point>& points,
    const vector<vector<double>>& dist,
    const Params& params,
    unsigned seed
) {
    int n = points.size();

    mt19937 rng(seed);
    uniform_int_distribution<int> posDist(0, n - 1);

    vector<int> current = randomInitialRoute(n, rng);
    double currentCost = routeLength(current, dist);

    vector<int> best = current;
    double bestCost = currentCost;

    /*
        tabuUntil[i][j] oznacza numer iteracji, do której ruch (i, j) jest tabu.
        Przechowujemy ruch 2-opt, czyli odwrócenie fragmentu od i do j.
    */
    vector<vector<int>> tabuUntil(n, vector<int>(n, 0));

    vector<double> history;

    int iterationsWithoutImprovement = 0;
    int iterationsDone = 0;

    for (int iteration = 1; iteration <= params.maxIterations; iteration++) {
        iterationsDone = iteration;

        int bestI = -1;
        int bestJ = -1;
        double bestCandidateCost = numeric_limits<double>::max();
        double bestCandidateDelta = 0.0;

        auto considerMove = [&](int i, int j) {
            if (i > j) {
                swap(i, j);
            }

            if (i == j) {
                return;
            }

            /*
                Odwrócenie całego cyklu nie daje realnie nowego rozwiązania.
            */
            if (i == 0 && j == n - 1) {
                return;
            }

            double delta = calculate2OptDelta(current, dist, i, j);
            double candidateCost = currentCost + delta;

            bool isTabu = tabuUntil[i][j] > iteration;

            /*
                Kryterium aspiracji:
                ruch tabu może zostać wykonany, jeśli daje nowy najlepszy wynik globalny.
            */
            bool aspiration = candidateCost < bestCost;

            if (isTabu && !aspiration) {
                return;
            }

            if (candidateCost < bestCandidateCost) {
                bestCandidateCost = candidateCost;
                bestCandidateDelta = delta;
                bestI = i;
                bestJ = j;
            }
        };

        if (params.candidateSample <= 0) {
            /*
                Deterministyczne przeszukiwanie całego sąsiedztwa.
                Dla n < 1000 może być OK, ale dla dużych danych będzie wolniejsze.
            */
            for (int i = 0; i < n; i++) {
                for (int j = i + 1; j < n; j++) {
                    considerMove(i, j);
                }
            }
        } else {
            /*
                Losowa próbka sąsiedztwa.
                Dobre dla większych danych.
            */
            for (int k = 0; k < params.candidateSample; k++) {
                int i = posDist(rng);
                int j = posDist(rng);

                while (i == j) {
                    j = posDist(rng);
                }

                considerMove(i, j);
            }
        }

        /*
            Może się zdarzyć, że nie znaleziono żadnego ruchu,
            np. przy bardzo małej instancji i silnym tabu.
        */
        if (bestI == -1 || bestJ == -1) {
            break;
        }

        reverse(current.begin() + bestI, current.begin() + bestJ + 1);
        currentCost += bestCandidateDelta;

        /*
            Dodajemy wykonany ruch do tabu.
            Po tabuTenure iteracjach będzie znowu dozwolony.
        */
        tabuUntil[bestI][bestJ] = iteration + params.tabuTenure;

        if (currentCost < bestCost) {
            best = current;
            bestCost = currentCost;
            iterationsWithoutImprovement = 0;
        } else {
            iterationsWithoutImprovement++;
        }

        history.push_back(bestCost);

        if (iterationsWithoutImprovement >= params.noImproveLimit) {
            break;
        }
    }

    RunResult result;
    result.bestRoute = best;
    result.bestCost = bestCost;
    result.history = history;
    result.iterationsDone = iterationsDone;
    result.seed = seed;

    return result;
}

void saveRunsCSV(
    const vector<RunResult>& results,
    const string& filename
) {
    ofstream file(filename);

    if (!file) {
        throw runtime_error("Nie mozna utworzyc pliku: " + filename);
    }

    file << "run,seed,bestCost,iterationsDone\n";

    for (int i = 0; i < (int)results.size(); i++) {
        file << i + 1 << ","
             << results[i].seed << ","
             << results[i].bestCost << ","
             << results[i].iterationsDone << "\n";
    }
}

void runTabuSearch(
    const vector<Point>& points,
    const Params& params
) {
    int n = points.size();

    vector<vector<double>> dist = buildDistanceMatrix(points);

    vector<RunResult> results;
    results.reserve(params.runs);

    cout << fixed << setprecision(4);

    cout << "========================================\n";
    cout << "Tabu Search dla TSP\n";
    cout << "========================================\n";
    cout << "Liczba wczytanych wierzcholkow: " << n << "\n";
    cout << "Maksymalna liczba iteracji: " << params.maxIterations << "\n";
    cout << "Limit iteracji bez poprawy: " << params.noImproveLimit << "\n";
    cout << "Dlugosc tabu: " << params.tabuTenure << "\n";

    if (params.candidateSample <= 0) {
        cout << "Sasiedztwo: pelne 2-opt\n";
    } else {
        cout << "Sasiedztwo: losowa probka, rozmiar = "
             << params.candidateSample << "\n";
    }

    cout << "Liczba prob uruchomienia: " << params.runs << "\n";
    cout << "Seed bazowy: " << params.seed << "\n";
    cout << "========================================\n\n";

    for (int run = 0; run < params.runs; run++) {
        unsigned runSeed = params.seed + run * 1000003u;

        RunResult result = tabuSearchSingleRun(
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
             << " | iteracje = " << result.iterationsDone
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
    cout << "Najlepszy uzyskany koszt: " << bestResult.bestCost << "\n";
    cout << "Sredni koszt z prob: " << average << "\n";
    cout << "Najlepsza proba numer: " << bestRunIndex + 1 << "\n";
    cout << "Seed najlepszej proby: " << bestResult.seed << "\n";
    cout << "Iteracje najlepszej proby: " << bestResult.iterationsDone << "\n\n";

    cout << "Najlepsza trasa, numery wierzcholkow z pliku:\n";

    for (int idx : bestResult.bestRoute) {
        cout << points[idx].id << " ";
    }

    cout << points[bestResult.bestRoute[0]].id << "\n\n";

    cout << "Zapisano pliki:\n";
    cout << "  " << params.bestRouteFile << "   - najlepsza trasa\n";
    cout << "  " << params.pointsFile << "       - punkty\n";
    cout << "  " << params.historyFile << "      - historia najlepszej proby\n";
    cout << "  " << params.runsFile << "         - wyniki wszystkich prob\n";
}

void printUsage(const string& programName) {
    cout << "Uzycie:\n";
    cout << "  " << programName << " plik.tsp [parametry]\n\n";

    cout << "Parametry opcjonalne:\n";
    cout << "  --iterations N         maksymalna liczba iteracji, np. 10000\n";
    cout << "  --no-improve N         stop po N iteracjach bez poprawy, np. 2000\n";
    cout << "  --tabu N               dlugosc listy tabu, np. 50\n";
    cout << "  --sample N             rozmiar losowej probki sasiedztwa, np. 1000\n";
    cout << "                         wartosc 0 oznacza pelne sasiedztwo 2-opt\n";
    cout << "  --runs N               liczba niezaleznych prob, np. 100\n";
    cout << "  --seed N               seed bazowy, np. 123\n";
    cout << "  --history FILE         plik CSV z historia najlepszej proby\n";
    cout << "  --route FILE           plik CSV z najlepsza trasa\n";
    cout << "  --points FILE          plik CSV z punktami\n";
    cout << "  --runs-file FILE       plik CSV z wynikami prob\n\n";

    cout << "Przyklad dla malych danych, pelne sasiedztwo:\n";
    cout << "  " << programName
         << " wi29.tsp --iterations 10000 --no-improve 2000 "
         << "--tabu 50 --sample 0 --runs 1 --seed 123\n\n";

    cout << "Przyklad dla wiekszych danych, losowa probka:\n";
    cout << "  " << programName
         << " big.tsp --iterations 20000 --no-improve 5000 "
         << "--tabu 100 --sample 5000 --runs 100 --seed 123\n";
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

        if (arg == "--iterations") {
            requireValue(arg);
            p.maxIterations = stoi(argv[++i]);
        } else if (arg == "--no-improve") {
            requireValue(arg);
            p.noImproveLimit = stoi(argv[++i]);
        } else if (arg == "--tabu") {
            requireValue(arg);
            p.tabuTenure = stoi(argv[++i]);
        } else if (arg == "--sample") {
            requireValue(arg);
            p.candidateSample = stoi(argv[++i]);
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

    if (p.maxIterations <= 0) {
        throw runtime_error("Liczba iteracji musi byc dodatnia.");
    }

    if (p.noImproveLimit <= 0) {
        throw runtime_error("Limit iteracji bez poprawy musi byc dodatni.");
    }

    if (p.tabuTenure <= 0) {
        throw runtime_error("Dlugosc tabu musi byc dodatnia.");
    }

    if (p.candidateSample < 0) {
        throw runtime_error("Rozmiar probki sasiedztwa nie moze byc ujemny.");
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

        runTabuSearch(points, params);

    } catch (const exception& e) {
        cerr << "\nBlad: " << e.what() << "\n\n";
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
