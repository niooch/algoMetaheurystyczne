#include "parser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

std::string trim(const std::string& s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

TspInstance readTsplibFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Could not open input file: " + path);
    }

    TspInstance instance;
    std::string line;
    bool inNodeSection = false;

    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        if (!inNodeSection) {
            if (line.rfind("NAME", 0) == 0) {
                const auto pos = line.find(':');
                if (pos != std::string::npos) {
                    instance.name = trim(line.substr(pos + 1));
                }
            }
            if (line == "NODE_COORD_SECTION") {
                inNodeSection = true;
            }
            continue;
        }

        if (line == "EOF") {
            break;
        }

        std::istringstream iss(line);
        Node node;
        if (iss >> node.id >> node.x >> node.y) {
            instance.nodes.push_back(node);
        }
    }

    if (instance.nodes.empty()) {
        throw std::runtime_error("No nodes found in NODE_COORD_SECTION.");
    }

    if (instance.name.empty()) {
        instance.name = "unnamed";
    }

    return instance;
}
