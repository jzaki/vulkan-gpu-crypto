#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#include <stdint.h>
typedef uint32_t uint;
#define GET_DATA(buf) (buf).data()
#define SHARED_FN inline
#else
// GLSL compatibility
#define GET_DATA(buf) (buf).data
#define SHARED_FN
#endif

// Shared Indexing Function: (batch, row, col) -> linear index
SHARED_FN uint IX3D(uint b, uint r, uint c, uint rows, uint cols) {
    return (b * rows * cols) + (r * cols) + c;
}

const uint BATCH = 32;
const uint M = 128;
const uint K = 128;
const uint N = 128;

const uint MSM_POINTS = 1 << 16;  // 65536 points

struct MatrixParams {
    uint batch;
    uint m;
    uint k;
    uint n;
};

struct MsmParams {
    uint points;
};

#endif  // COMMON_H
