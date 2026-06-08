#include <bits/stdc++.h>
using namespace std;

struct Point {
    int id;
    double x, y;
};

struct Params {
    double startTemp = 10000.0;
    double cooling = 0.97;
    int trialsPerEpoch = 1000;
    int maxEpochs = 1000;
    int noImproveLimit = 150;
    unsigned seed = chrono::steady_clock::now().time_since_epoch().count();

    string historyFile = "history.csv";
    string bestRouteFile = "best_route.csv";
    string pointsFile = "points.csv";
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
    Oblicza zmianę długości trasy dla ruchu 2-opt w czasie O(1).

    Odwracamy fragment trasy od i do j.

    Przed:
        a - b ... c - d

    Po:
        a - c ... b - d

    Delta:
        nowe krawędzie - stare krawędzie
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
        file << p.id << ","
             << p.x << ","
             << p.y << "\n";
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

void simulatedAnnealing(
    const vector<Point>& points,
    const Params& params
) {
    int n = points.size();

    mt19937 rng(params.seed);
    uniform_real_distribution<double> realDist(0.0, 1.0);
    uniform_int_distribution<int> posDist(0, n - 1);

    vector<vector<double>> dist = buildDistanceMatrix(points);

    vector<int> current = randomInitialRoute(n, rng);
    double currentCost = routeLength(current, dist);

    vector<int> best = current;
    double bestCost = currentCost;

    double temperature = params.startTemp;
    int epochsWithoutImprovement = 0;

    ofstream history(params.historyFile);

    if (!history) {
        throw runtime_error("Nie mozna utworzyc pliku: " + params.historyFile);
    }

    history << "epoch,temperature,currentCost,bestCost,acceptedWorse,acceptedBetter\n";

    cout << fixed << setprecision(4);

    cout << "========================================\n";
    cout << "Symulowane wyzarzanie dla TSP\n";
    cout << "Wersja zoptymalizowana: 2-opt delta O(1)\n";
    cout << "========================================\n";
    cout << "Liczba wczytanych wierzcholkow: " << n << "\n";
    cout << "Temperatura poczatkowa: " << params.startTemp << "\n";
    cout << "Wspolczynnik chlodzenia: " << params.cooling << "\n";
    cout << "Liczba prob w epoce: " << params.trialsPerEpoch << "\n";
    cout << "Maksymalna liczba epok: " << params.maxEpochs << "\n";
    cout << "Limit epok bez poprawy: " << params.noImproveLimit << "\n";
    cout << "Seed: " << params.seed << "\n";
    cout << "========================================\n\n";

    cout << "Startowy koszt trasy: " << currentCost << "\n\n";

    for (int epoch = 1; epoch <= params.maxEpochs; epoch++) {
        bool improvedInEpoch = false;

        int acceptedWorse = 0;
        int acceptedBetter = 0;

        for (int trial = 0; trial < params.trialsPerEpoch; trial++) {
            int i = posDist(rng);
            int j = posDist(rng);

            while (i == j) {
                j = posDist(rng);
            }

            if (i > j) {
                swap(i, j);
            }

            /*
                Dla pełnego cyklu unikamy przypadku i = 0 oraz j = n - 1.
                Taki ruch odwracałby prawie całą trasę i mógłby dawać
                nieintuicyjne zachowanie przy delcie z domknięciem cyklu.
            */
            if (i == 0 && j == n - 1) {
                continue;
            }

            double delta = calculate2OptDelta(current, dist, i, j);
            double candidateCost = currentCost + delta;

            bool accept = false;

            if (delta < 0) {
                accept = true;
                acceptedBetter++;
            } else {
                double probability = exp(-delta / temperature);

                if (realDist(rng) < probability) {
                    accept = true;
                    acceptedWorse++;
                }
            }

            if (accept) {
                reverse(current.begin() + i, current.begin() + j + 1);
                currentCost = candidateCost;

                if (currentCost < bestCost) {
                    best = current;
                    bestCost = currentCost;
                    improvedInEpoch = true;
                }
            }
        }

        history << epoch << ","
                << temperature << ","
                << currentCost << ","
                << bestCost << ","
                << acceptedWorse << ","
                << acceptedBetter << "\n";

        if (improvedInEpoch) {
            epochsWithoutImprovement = 0;
        } else {
            epochsWithoutImprovement++;
        }

        if (epoch % 10 == 0 || improvedInEpoch) {
            cout << "Epoka: " << setw(5) << epoch
                 << " | T = " << setw(12) << temperature
                 << " | aktualny = " << setw(12) << currentCost
                 << " | najlepszy = " << setw(12) << bestCost
                 << " | lepsze zaakceptowane = " << setw(5) << acceptedBetter
                 << " | gorsze zaakceptowane = " << setw(5) << acceptedWorse
                 << "\n";
        }

        temperature *= params.cooling;

        if (temperature < 1e-9) {
            cout << "\nStop: temperatura spadla prawie do zera.\n";
            break;
        }

        if (epochsWithoutImprovement >= params.noImproveLimit) {
            cout << "\nStop: brak poprawy przez "
                 << params.noImproveLimit << " epok.\n";
            break;
        }
    }

    saveBestRouteCSV(points, best, params.bestRouteFile);
    savePointsCSV(points, params.pointsFile);

    cout << "\n========================================\n";
    cout << "Wyniki koncowe\n";
    cout << "========================================\n";
    cout << "Najlepszy koszt trasy: " << bestCost << "\n\n";

    cout << "Najlepsza trasa, numery wierzcholkow z pliku:\n";

    for (int idx : best) {
        cout << points[idx].id << " ";
    }

    cout << points[best[0]].id << "\n\n";

    cout << "Zapisano pliki:\n";
    cout << "  " << params.historyFile << "      - historia kosztu i temperatury\n";
    cout << "  " << params.bestRouteFile << "   - najlepsza trasa do narysowania\n";
    cout << "  " << params.pointsFile << "       - wszystkie punkty\n";
}

void printUsage(const string& programName) {
    cout << "Uzycie:\n";
    cout << "  " << programName << " plik.tsp [parametry]\n\n";

    cout << "Parametry opcjonalne:\n";
    cout << "  --temp X              temperatura poczatkowa, np. 10000\n";
    cout << "  --cooling X           wspolczynnik chlodzenia z zakresu (0,1), np. 0.97\n";
    cout << "  --trials N            liczba prob w jednej epoce, np. 1000\n";
    cout << "  --epochs N            maksymalna liczba epok, np. 1000\n";
    cout << "  --no-improve N        stop po N epokach bez poprawy, np. 150\n";
    cout << "  --seed N              ziarno generatora losowego, np. 123\n";
    cout << "  --history FILE        plik CSV z historia, domyslnie history.csv\n";
    cout << "  --route FILE          plik CSV z najlepsza trasa, domyslnie best_route.csv\n";
    cout << "  --points FILE         plik CSV z punktami, domyslnie points.csv\n\n";

    cout << "Przyklad:\n";
    cout << "  " << programName
         << " wi29.tsp --temp 10000 --cooling 0.97 --trials 100000 "
         << "--epochs 1000 --no-improve 150 --seed 123\n";
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

        if (arg == "--temp") {
            requireValue(arg);
            p.startTemp = stod(argv[++i]);
        } else if (arg == "--cooling") {
            requireValue(arg);
            p.cooling = stod(argv[++i]);
        } else if (arg == "--trials") {
            requireValue(arg);
            p.trialsPerEpoch = stoi(argv[++i]);
        } else if (arg == "--epochs") {
            requireValue(arg);
            p.maxEpochs = stoi(argv[++i]);
        } else if (arg == "--no-improve") {
            requireValue(arg);
            p.noImproveLimit = stoi(argv[++i]);
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
        } else {
            throw runtime_error("Nieznany parametr: " + arg);
        }
    }

    if (p.startTemp <= 0) {
        throw runtime_error("Temperatura poczatkowa musi byc dodatnia.");
    }

    if (p.cooling <= 0 || p.cooling >= 1) {
        throw runtime_error("Wspolczynnik chlodzenia musi byc z przedzialu (0, 1).");
    }

    if (p.trialsPerEpoch <= 0) {
        throw runtime_error("Liczba prob w epoce musi byc dodatnia.");
    }

    if (p.maxEpochs <= 0) {
        throw runtime_error("Liczba epok musi byc dodatnia.");
    }

    if (p.noImproveLimit <= 0) {
        throw runtime_error("Limit epok bez poprawy musi byc dodatni.");
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

        simulatedAnnealing(points, params);

    } catch (const exception& e) {
        cerr << "\nBlad: " << e.what() << "\n\n";
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
