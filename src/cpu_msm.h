#ifndef CPU_MSM_H
#define CPU_MSM_H

#include <vector>

#include "common.h"

void cpu_msm(const std::vector<float>& points, const std::vector<float>& scalars,
             std::vector<float>& result, const MsmParams& params);

#endif  // CPU_MSM_H
