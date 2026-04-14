#ifndef LAB1_RESULTS_WRITER_H
#define LAB1_RESULTS_WRITER_H

#include <string>

#include "experiment_types.h"

void writeLab1ResultsCsv(const std::string& outputPath,
                         const TspInstance& instance,
                         const Lab1Results& results);

std::string defaultLab1OutputPath(const std::string& inputPath);

#endif
