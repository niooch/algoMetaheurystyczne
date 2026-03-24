#ifndef MST_H
#define MST_H

#include <vector>

#include "types.h"

MstResult primMst(const std::vector<std::vector<long long>>& dist);
TourResult mstDfsTour(const std::vector<std::vector<long long>>& dist);

#endif
