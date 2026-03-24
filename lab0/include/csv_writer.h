#ifndef CSV_WRITER_H
#define CSV_WRITER_H

#include <string>

#include "types.h"

void writeResultsCsv(const std::string& outputPath,
                     const TspInstance& instance,
                     const RandomExperimentResult& randomResult,
                     const MstResult& mstResult,
                     const TourResult& mstTour,
                     unsigned long long seed);

std::string defaultOutputPath(const std::string& inputPath);

#endif
