#ifndef COMMON_H
#define COMMON_H

#ifdef __cplusplus
#include <stdint.h>
typedef uint32_t uint;
#define GET_DATA(buf) (buf).data()
#else
#define GET_DATA(buf) (buf).data
#endif


// Shared Indexing Macro: (batch, row, col) -> linear index
#define IX3D(b, r, c, rows, cols) (((b) * (rows) * (cols)) + ((r) * (cols)) + (c))

const uint BATCH = 32;
const uint M = 128;
const uint K = 128;
const uint N = 128;

struct MatrixParams {
    uint batch;
    uint m;
    uint k;
    uint n;
};


#endif // COMMON_H
