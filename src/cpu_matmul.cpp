#include "cpu_matmul.h"

#include <immintrin.h>
#include <omp.h>

void cpu_matmul(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C,
                const MatrixParams& params) {
    const float* A_ptr = A.data();
    const float* B_ptr = B.data();
    float* C_ptr = C.data();

// Parallelize over batch and rows (m)
#pragma omp parallel for collapse(2)
    for (uint b = 0; b < params.batch; ++b) {
        for (uint i = 0; i < params.m; ++i) {
            // Pointer to C[b][i][0]
            float* C_row = C_ptr + b * params.m * params.n + i * params.n;

            for (uint k = 0; k < params.k; ++k) {
                // Value A[b][i][k] broadcasted to AVX register
                float A_val = A_ptr[b * params.m * params.k + i * params.k + k];
                __m256 vec_A = _mm256_set1_ps(A_val);

                // Pointer to B[b][k][0]
                const float* B_row = B_ptr + b * params.k * params.n + k * params.n;

                // Vectorized loop over j
                for (uint j = 0; j < params.n; j += 8) {
                    // Load 8 floats from C (accumulators)
                    __m256 vec_C = _mm256_loadu_ps(C_row + j);

                    // Load 8 floats from B
                    __m256 vec_B = _mm256_loadu_ps(B_row + j);

                    // C += A * B
                    vec_C = _mm256_fmadd_ps(vec_A, vec_B, vec_C);

                    // Store back to C
                    _mm256_storeu_ps(C_row + j, vec_C);
                }
            }
        }
    }
}
