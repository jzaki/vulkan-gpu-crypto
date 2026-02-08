#ifndef CPU_MATMUL_H
#define CPU_MATMUL_H

#include "common.h"
#include <vector>

void cpu_matmul(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, const MatrixParams& params);

#endif // CPU_MATMUL_H
