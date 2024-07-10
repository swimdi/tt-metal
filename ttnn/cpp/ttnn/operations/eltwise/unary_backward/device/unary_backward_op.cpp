// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "ttnn/operations/eltwise/unary_backward/device/unary_backward_op.hpp"

#include "third_party/magic_enum/magic_enum.hpp"
#include "tt_eager/tt_dnn/op_library/bcast/bcast_op.hpp"
#include "tt_eager/tt_dnn/op_library/composite/composite_ops.hpp"
#include "tt_eager/tt_dnn/op_library/eltwise_unary/eltwise_unary_op.hpp"
#include "tt_eager/tt_dnn/op_library/unpad/unpad_op.hpp"
#include "tt_metal/common/constants.hpp"
#include "tt_metal/host_api.hpp"
#include "tt_metal/tools/profiler/op_profiler.hpp"
#include "ttnn/operations/eltwise/unary/unary.hpp"
#include "ttnn/operations/eltwise/binary/binary.hpp"

namespace ttnn::operations::unary_backward {

std::vector<ttnn::Tensor> _mul_bw(
    const Tensor& grad, const Tensor& input, float scalar, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor result = mul_unary(grad, scalar, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _clamp_min_bw(
    const Tensor& grad, const Tensor& input, float min, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor minT = gte_unary(input, min, output_mem_config);
    Tensor result = ttnn::multiply(grad, minT, std::nullopt, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _clamp_max_bw(
    const Tensor& grad, const Tensor& input, float max, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor maxT = lte_unary(input, max, output_mem_config);
    Tensor result = ttnn::multiply(grad, maxT, std::nullopt, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _clamp_bw(
    const Tensor& grad, const Tensor& input, float min, float max, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor minT = gte_unary(input, min, output_mem_config);
    Tensor maxT = lte_unary(input, max, output_mem_config);
    Tensor result = ttnn::logical_and(minT, maxT, std::nullopt, output_mem_config);
    result = ttnn::multiply(grad, result, std::nullopt, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _assign_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    grad_tensor.emplace_back(grad);
    return grad_tensor;
}

std::vector<Tensor> _multigammaln_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor digamma_result = ttnn::multiply(grad, tt::tt_metal::digamma(input, output_mem_config), std::nullopt, output_mem_config);
    Tensor digamma_result_2 = ttnn::multiply(
        grad, tt::tt_metal::digamma(add_unary(-0.5, input, output_mem_config), output_mem_config), std::nullopt, output_mem_config);

    Tensor grad_result = ttnn::add(digamma_result, digamma_result_2, std::nullopt, output_mem_config);

    digamma_result = ttnn::multiply(
        grad, tt::tt_metal::digamma(add_unary(-1.0, input, output_mem_config), output_mem_config), std::nullopt, output_mem_config);
    grad_result = ttnn::add(grad_result, digamma_result, std::nullopt, output_mem_config);

    digamma_result = ttnn::multiply(
        grad, tt::tt_metal::digamma(add_unary(-1.5, input, output_mem_config), output_mem_config), std::nullopt, output_mem_config);
    grad_result = ttnn::add(grad_result, digamma_result, std::nullopt, output_mem_config);

    grad_tensor.emplace_back(grad_result);
    return grad_tensor;
}

std::vector<Tensor> _add_bw(
    const Tensor& grad, const Tensor& input, float alpha, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    grad_tensor.emplace_back(grad);
    return grad_tensor;
}

std::vector<Tensor> _unary_comp_bw(const Tensor& grad, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor zero_grad = tt::tt_metal::zeros_like(grad, output_mem_config);
    grad_tensor.emplace_back(zero_grad);
    return grad_tensor;
}

std::vector<Tensor> _eq_bw(
    const Tensor& grad, const Tensor& input, float other, const MemoryConfig& output_mem_config) {
    return _unary_comp_bw(grad, output_mem_config);
}

std::vector<Tensor> _lgamma_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor grad_result = ttnn::multiply(grad, tt::tt_metal::digamma(input, output_mem_config), std::nullopt, output_mem_config);
    grad_tensor.emplace_back(grad_result);
    return grad_tensor;
}

std::vector<Tensor> _sub_bw(const Tensor& grad, const Tensor& input, float alpha, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    grad_tensor.emplace_back(grad);
    return grad_tensor;
}

std::vector<Tensor> _frac_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    grad_tensor.emplace_back(grad);
    return grad_tensor;
}

std::vector<Tensor> _trunc_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor grad_result = tt::tt_metal::zeros_like(grad, output_mem_config);
    grad_tensor.emplace_back(grad_result);
    return grad_tensor;
}

// return: grad_output * (max_deriv - sign * (z / (1 + z)))
// z = exp(-abs(input))
std::vector<Tensor> _log_sigmoid_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor max_deriv = where(ttnn::ltz(input, output_mem_config), 1, 0, output_mem_config);
    Tensor in_sign = where(ttnn::ltz(input, output_mem_config), 1, -1, output_mem_config);
    Tensor in_abs = ttnn::abs(input, output_mem_config);
    Tensor z = ttnn::exp(ttnn::neg(in_abs, output_mem_config), false, output_mem_config);

    Tensor mul_z = ttnn::multiply(z, ttnn::reciprocal((ttnn::add(z, 1.0f, std::nullopt, output_mem_config)), output_mem_config), std::nullopt, output_mem_config);

    Tensor mul_sign = ttnn::multiply(in_sign, mul_z, std::nullopt, output_mem_config);
    Tensor sub_max = ttnn::subtract(max_deriv, mul_sign, std::nullopt, output_mem_config);

    Tensor grad_result = ttnn::multiply(grad, sub_max, std::nullopt, output_mem_config);
    grad_tensor.emplace_back(grad_result);
    return grad_tensor;
}


std::vector<Tensor> _fill_zero_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor result = tt::tt_metal::zeros_like(grad, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _i0_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    float t_inf = std::numeric_limits<float>::infinity();
    Tensor value = ttnn::multiply(
        ttnn::multiply(ttnn::i0(input, output_mem_config), ttnn::reciprocal(input, output_mem_config), std::nullopt, output_mem_config),
        0.5,
        std::nullopt,
        output_mem_config);
    Tensor result = where(
        ttnn::ltz(input, output_mem_config),
        ttnn::multiply(grad,
            ttnn::subtract(ttnn::neg(ttnn::i0(input, output_mem_config), output_mem_config), value, std::nullopt, output_mem_config),
            std::nullopt,
            output_mem_config),
        ttnn::multiply(grad,
            ttnn::subtract(ttnn::i0(input, output_mem_config), value, std::nullopt, output_mem_config),
            std::nullopt,
            output_mem_config),
        output_mem_config);
    result = where(
        ttnn::ge(ttnn::abs(ttnn::i0(input, output_mem_config), output_mem_config), 3.4e+38, std::nullopt, output_mem_config),
        t_inf,
        result,
        output_mem_config);
    result =
        where(ttnn::ge(ttnn::abs(result, output_mem_config), 3.4e+38, std::nullopt, output_mem_config), t_inf, result, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _tan_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor tan_result = ttnn::tan(input, output_mem_config);
    Tensor result =
        ttnn::multiply(grad, ttnn::add(ttnn::square(tan_result, output_mem_config), 1.0f, std::nullopt, output_mem_config), std::nullopt, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

// grad(sigmoid) = grad*(1 - sigmoid(x))*sigmoid(x)
std::vector<Tensor> _sigmoid_bw(
    const Tensor& grad,
    const Tensor& input,
    const MemoryConfig& output_mem_config = operation::DEFAULT_OUTPUT_MEMORY_CONFIG) {
    std::vector<Tensor> grad_tensor;
    Tensor sig_result = ttnn::sigmoid(input, output_mem_config);
    Tensor rsub_term = ttnn::rsub(sig_result, 1.0f, output_mem_config);
    Tensor prod_term_1 = ttnn::multiply(sig_result, rsub_term, std::nullopt, output_mem_config);
    Tensor prod_term_2 = ttnn::multiply(prod_term_1, grad, std::nullopt, output_mem_config);
    grad_tensor.emplace_back(prod_term_2);
    return grad_tensor;
}

std::vector<Tensor> _rsqrt_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor rsqrt_result = ttnn::power(ttnn::rsqrt(input, true, output_mem_config), 3, output_mem_config);
    Tensor result = ttnn::multiply(ttnn::multiply(grad, rsqrt_result, std::nullopt, output_mem_config), -0.5, std::nullopt, output_mem_config);
    float t_inf = std::numeric_limits<float>::infinity();
    result = where(ttnn::eqz(input, output_mem_config), t_inf, result, output_mem_config);
    float t_nan = std::nanf("");
    result = where(ttnn::ltz(input, output_mem_config), t_nan, result, output_mem_config);
    result = where(
        ttnn::logical_and(ttnn::eqz(input, output_mem_config), ttnn::eqz(grad, output_mem_config), std::nullopt, output_mem_config),
        t_nan,
        result,
        output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _neg_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor result = ttnn::neg(grad, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _relu_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor result = ttnn::multiply(ttnn::gtz(input, output_mem_config), grad, std::nullopt, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}

std::vector<Tensor> _logit_bw(const Tensor& grad, const Tensor& input, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor grad_result =
        ttnn::multiply(grad,
            ttnn::reciprocal(ttnn::multiply(input, ttnn::rsub(input, 1.0f, output_mem_config), std::nullopt, output_mem_config)),
            std::nullopt,
            output_mem_config);
        ttnn::ge(input, 0.0f, std::nullopt, output_mem_config),
        ttnn::le(input, 1.0f, std::nullopt, output_mem_config),
        std::nullopt,
        output_mem_config);
    grad_result = where(
        ttnn::eq(status, tt::tt_metal::ones_like(input, output_mem_config), std::nullopt, output_mem_config), grad_result, std::nanf(""));
    grad_result = where(
        ttnn::logical_or(
            ttnn::eq(input, 0.0, std::nullopt, output_mem_config),
            ttnn::eq(input, 1.0, std::nullopt, output_mem_config),
            std::nullopt,
            output_mem_config),
        ttnn::multiply(ttnn::sign(grad, output_mem_config), std::numeric_limits<float>::infinity(), std::nullopt, output_mem_config),
        grad_result,
        output_mem_config);
    grad_tensor.emplace_back(grad_result);
    return grad_tensor;
}


std::vector<Tensor> _hardshrink_bw(
    const Tensor& grad, const Tensor& input_tensor, float lambd, const MemoryConfig& output_mem_config) {
    std::vector<Tensor> grad_tensor;
    Tensor hardshrink_result = hardshrink(input_tensor, lambd, output_mem_config);
    Tensor result = where(eqz(hardshrink_result, output_mem_config), 0.0f, grad, output_mem_config);
    grad_tensor.emplace_back(result);
    return grad_tensor;
}


std::function<std::vector<ttnn::Tensor>(const Tensor&, const Tensor&, const MemoryConfig&)> UnaryBackwardFunction::get_function_type1(UnaryBackwardOpType OpType){
    switch (OpType) {
        case UnaryBackwardOpType::ASSIGN_BW:
            return _assign_bw;
        case UnaryBackwardOpType::MULTIGAMMALN_BW:
            return _lgamma_bw;
        case UnaryBackwardOpType::FRAC_BW:
            return _frac_bw;
        case UnaryBackwardOpType::TRUNC_BW:
            return _trunc_bw;
        case UnaryBackwardOpType::LOG_SIGMOID_BW:
            return _log_sigmoid_bw;
        case UnaryBackwardOpType::FILL_ZERO_BW:
            return _fill_zero_bw;
        case UnaryBackwardOpType::I0_BW:
            return _i0_bw;
        case UnaryBackwardOpType::TAN_BW:
            return _tan_bw;
        case UnaryBackwardOpType::SIGMOID_BW:
            return _sigmoid_bw;
        case UnaryBackwardOpType::RSQRT_BW:
            return _rsqrt_bw;
        case UnaryBackwardOpType::NEG_BW:
            return _neg_bw;
        case UnaryBackwardOpType::RELU_BW:
            return _relu_bw;
        case UnaryBackwardOpType::LOGIT_BW:
            return _logit_bw;
        default:
            TT_ASSERT(false && "Undefined op type");
            return 0;
    }
}

std::function<std::vector<ttnn::Tensor>(const Tensor&, const Tensor&, float, const MemoryConfig&)> UnaryBackwardFunction::get_function_type1_w_float(UnaryBackwardOpType OpType){
    switch (OpType) {
        case UnaryBackwardOpType::MUL_BW:
            return _mul_bw;
        case UnaryBackwardOpType::CLAMP_MIN_BW:
            return _clamp_min_bw;
        case UnaryBackwardOpType::CLAMP_MAX_BW:
            return _clamp_max_bw;
        case UnaryBackwardOpType::ADD_BW:
            return _add_bw;
        case UnaryBackwardOpType::EQ_BW:
            return _eq_bw;
        case UnaryBackwardOpType::SUB_BW:
            return _sub_bw;
        case UnaryBackwardOpType::HARDSHRINK_BW:
            return _hardshrink_bw;
        default:
            TT_ASSERT(false && "Undefined op type");
            return 0;
    }
}

std::function<std::vector<ttnn::Tensor>(const Tensor&, const Tensor&, float, float, const MemoryConfig&)> UnaryBackwardFunction::get_function_type1_w_two_float(UnaryBackwardOpType OpType){
    switch (OpType) {
        case UnaryBackwardOpType::CLAMP_BW:
            return _clamp_bw;
        default:
            TT_ASSERT(false && "Undefined op type");
            return 0;
    }
}

}  // namespace ttnn::operations::unary