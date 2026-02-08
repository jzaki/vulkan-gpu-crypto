#include "cpu_matmul.h"

void cpu_matmul(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C, const MatrixParams& params) {
    for (uint b = 0; b < params.batch; ++b) {
        for (uint i = 0; i < params.m; ++i) {
            for (uint j = 0; j < params.n; ++j) {
                float sum = 0.0f;
                for (uint k = 0; k < params.k; ++k) {
                    sum += A[IX3D(b, i, k, params.m, params.k)] * 
                           B[IX3D(b, k, j, params.k, params.n)];
                }
                C[IX3D(b, i, j, params.m, params.n)] = sum;
            }
        }
    }
}

