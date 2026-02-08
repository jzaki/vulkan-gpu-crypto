#include "cpu_msm.h"
#include "blst.h"
#include <cstring>
#include <vector>

void cpu_msm(const std::vector<float>& points_raw, const std::vector<float>& scalars_raw, std::vector<float>& result_raw, const MsmParams& params) {
    const blst_p1_affine* points = reinterpret_cast<const blst_p1_affine*>(points_raw.data());
    const blst_scalar* scalars = reinterpret_cast<const blst_scalar*>(scalars_raw.data());
    
    std::vector<const blst_p1_affine*> point_ptrs(params.points);
    std::vector<const byte*> scalar_ptrs(params.points);
    
    for (size_t i = 0; i < params.points; ++i) {
        point_ptrs[i] = &points[i];
        scalar_ptrs[i] = reinterpret_cast<const byte*>(&scalars[i]);
    }

    blst_p1 result;
    size_t scratch_size = blst_p1s_mult_pippenger_scratch_sizeof(params.points);
    std::vector<limb_t> scratch(scratch_size / sizeof(limb_t) + 1);

    blst_p1s_mult_pippenger(&result, point_ptrs.data(), params.points, scalar_ptrs.data(), 255, scratch.data());

    blst_p1_affine result_affine;
    blst_p1_to_affine(&result_affine, &result);
    
    if (result_raw.size() * sizeof(float) >= sizeof(blst_p1_affine)) {
        memcpy(result_raw.data(), &result_affine, sizeof(blst_p1_affine));
    }
}
