// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ckernel_sfpu_identity.h"
#include "llk_math_eltwise_unary_sfpu_0_param.h"
#include "llk_math_eltwise_unary_sfpu_init.h"

namespace ckernel {

// New LLK SFPU APIs

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_identity(uint dst_index, int vector_mode = (int)VectorMode::RC) {
    llk_math_eltwise_unary_sfpu_0_param<APPROXIMATE>(
        ckernel::sfpu::calculate_identity<APPROXIMATE, 8>,
        ckernel::sfpu::calculate_identity<APPROXIMATE, 8>,
        dst_index,
        vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_identity_uint32(uint dst_index, int vector_mode = (int)VectorMode::RC) {
    llk_math_eltwise_unary_sfpu_0_param<APPROXIMATE>(
        ckernel::sfpu::calculate_identity_uint<APPROXIMATE, 8>,
        ckernel::sfpu::calculate_identity_uint<APPROXIMATE, 8>,
        dst_index,
        vector_mode);
}

template <bool APPROXIMATE>
inline void llk_math_eltwise_unary_sfpu_identity_init() {
    llk_math_eltwise_unary_sfpu_init<SfpuType::unused, APPROXIMATE>();
}

}  // namespace ckernel
