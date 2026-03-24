#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "types.h"

std::string trim(const std::string& s);
TspInstance readTsplibFile(const std::string& path);

#endif
