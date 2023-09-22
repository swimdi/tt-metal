// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "tt_lib_bindings_tensor.hpp"
#include "tt_lib_bindings_tensor_impl.hpp"
#include "tt_dnn/op_library/eltwise_binary/eltwise_binary_op.hpp"

namespace tt::tt_metal::detail {
    void TensorModuleXaryOPs( py::module & m_tensor){

        // *** eltwise binary ***

        detail::bind_binary_op(m_tensor, "add", add, R"doc(Perform an eltwise-binary add (``{0} + {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "sub", sub, R"doc(Perform an eltwise-binary sub (``{0} - {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "mul", mul, R"doc(Perform an eltwise-binary mul (``{0} * {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "squared_difference", squared_difference, R"doc(Perform an eltwise-binary squared_difference (``{0} - {1}``)^2 on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "logical_and", logical_and, R"doc(Performs the element-wise logical AND of the given input tensors ``{0}`` && ``{1}``, Zeros are treated as False and nonzeros are treated as True.)doc");
        detail::bind_binary_op(m_tensor, "bias_gelu", bias_gelu, R"doc(Perform an eltwise-binary bias_gelu (``{0} + {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "gt", gt, R"doc(Perform an eltwise-binary greater-than (``{0} > {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "lt", lt, R"doc(Perform an eltwise-binary less-than (``{0} < {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "lte", lte, R"doc(Perform an eltwise-binary less-than-or-equal (``{0} <= {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "gte", gte, R"doc(Perform an eltwise-binary greater-than-or-equal (``{0} >= {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "eq", eq, R"doc(Perform an eltwise-binary equal (``{0} == {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "ne", ne, R"doc(Perform an eltwise-binary not-equal (``{0} != {1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "ldexp", ldexp, R"doc(Performs eltwise-binary ldexp (``{0} * 2**{1}``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "logaddexp", logaddexp, R"doc(Perform an eltwise-binary logaddexp (``log(exp({0}) + exp({1}))``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "logaddexp2", logaddexp2, R"doc(Perform an eltwise-binary logaddexp2 (``log2(2^({0}) + 2^({1}))``) on two tensors.)doc");
        detail::bind_binary_op(m_tensor, "logical_or", logical_or, R"doc(Perform an eltwise-binary logical OR (``{0} || {1}``) on two tensors.)doc");

        detail::bind_binary_op(m_tensor, "add_without_autoformat", add_without_autoformat,
            R"doc(Perform an eltwise-binary add (``{0} + {1}``) on two tensors.

            Auto formatting is disabled. Both input tensors must have TILE layout. Output tensor will have TILE layout.)doc"
        );


        // *** eltwise unary ***

        detail::bind_unary_op(m_tensor, "exp", exp, R"doc(Returns a new tensor with the exponential of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "recip", recip, R"doc(Returns a new tensor with the reciprocal of the elements of the input tensor ``recip``.)doc");
        detail::bind_unary_op(m_tensor, "relu", relu, R"doc(Applies the rectified linear unit (ReLU) function to the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "relu6", relu6, R"doc(Returns tensor with the relu6 activation on elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "sqrt", sqrt, R"doc(Returns tensor with the square-root of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "sigmoid", sigmoid, R"doc(Applies the sigmoid function to the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "log", log, R"doc(Returns tensor with the natural logarithm of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "tanh", tanh, R"doc(Returns tensor with the hyperbolic tangent of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "log2", log2, R"doc(Returns tensor with the base 2 logarithm of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "log10", log10, R"doc(Returns tensor with the base 10 logarithm of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "sin", tt::tt_metal::sin, R"doc(Returns tensor with the sine of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "cos", tt::tt_metal::cos, R"doc(Returns tensor with the cosine of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "abs", abs, R"doc(Returns tensor with elementwise absolute value of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "isfinite", isfinite, R"doc(Returns boolean tensor that is True where input tensor ``{0}``, is finite and False elsewhere.)doc");
        detail::bind_unary_op(m_tensor, "isinf", isinf, R"doc(Returns boolean tensor that is True where input tensor ``{0}``, is infinite and False elsewhere.)doc");
        detail::bind_unary_op(m_tensor, "isposinf", isposinf, R"doc(Returns each element of input tensor ``{0}``, is positive infinity or not.)doc");
        detail::bind_unary_op(m_tensor, "isneginf", isneginf, R"doc(Returns each element of input tensor ``{0}``, is negative infinity or not.)doc");
        detail::bind_unary_op(m_tensor, "isnan", isnan, R"doc(Returns boolean tensor that is True where tensor ``{0}``, is NaN and False elsewhere.)doc");
        detail::bind_unary_op(m_tensor, "sign", sign, R"doc(Returns tensor with the elementwise signum of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "square", square, R"doc(Returns tensor with the square of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "eqz", eqz, R"doc(Returns tensor with the result of equal to zero of all of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "nez", nez, R"doc(Returns tensor with the not equal zero of all of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "gtz", gtz, R"doc(Returns tensor with the greater than zero of all of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "ltz", ltz, R"doc(Returns tensor with the less than zero of all of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "gez", gez, R"doc(Returns tensor with the greater than equal zero of all of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "lez", lez, R"doc(Returns tensor with the less than equal zero of all of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "exp2", exp2, R"doc(Returns a new tensor with the exp2 (2 power) of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "expm1", expm1,
            R"doc(Returns a new tensor with the expm1 of the elements of the input tensor ``{0}``.
            expm1 = exp(x) - 1)doc"
        );
        detail::bind_unary_op(m_tensor, "signbit", signbit, R"doc(Applies the signbit function to the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "atan", atan, R"doc(Returns a new tensor with the arctan of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "asin", asin, R"doc(Returns a new tensor with the arcsine of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "acos", acos, R"doc(Returns a new tensor with the arccosine of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "logical_not_unary", logical_not_unary, R"doc(Returns a new tensor with the logical not of the elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "log_sigmoid", &log_sigmoid, R"doc(Applies the logsigmoid function to the elements of the input tensor ``{0}``.)doc");

        detail::bind_unary_op_with_param(
            m_tensor, "gelu", &gelu,
            py::arg("fast_and_approx") = true,
            R"doc(Applies the Gaussian Error Linear Units (GELU) function to the elements of the input tensor ``{0}``.)doc",
            R"doc("Indicate true for approx and fast mode; false for accurate and slow mode", "bool", "default of true")doc"
        );

        detail::bind_unary_op_with_param(
            m_tensor, "erf", &erf,
            py::arg("fast_and_approx") = true,
            R"doc(Computes error function for all elements of the input tensor ``{0}``.)doc",
            R"doc("Indicate true for approx and fast mode; false for accurate and slow mode", "bool", "default of true")doc"
        );
        detail::bind_unary_op_with_param(
            m_tensor, "erfc", &erfc,
            py::arg("fast_and_approx") = true,
            R"doc(Computes complementary error function for all elements of the input tensor ``{0}``.)doc",
            R"doc("Indicate true for approx and fast mode; false for accurate and slow mode", "bool", "default of true")doc"
        );
        detail::bind_unary_op_with_param(
            m_tensor, "rsqrt", &rsqrt,
            py::arg("fast_and_approx") = true,
            R"doc(Returns a new tensor with the reciprocal of the square-root of each of the elements of the input tensor ``{0}``.)doc",
            R"doc("Indicate true for approx and fast mode; false for accurate and slow mode", "bool", "default of true")doc"
        );
        detail::bind_unary_op_with_param(
            m_tensor, "relu_max", relu_max,
            py::arg("upper_limit"),
            R"doc(Returns tensor with the relu max of all of elements of the input tensor ``{0}``. This is equivalent
            to relu_max[x] = relu(min(x, ``{1}``)). It caps off the input to a max value and a min value of 0.)doc",
            R"doc("max value", "float", "")doc"

        );
        detail::bind_unary_op_with_param(
            m_tensor, "relu_min", relu_min,
            py::arg("lower_limit"),
            R"doc(Returns tensor with the relu min of all of elements of the input tensor ``{0}``. This is equivalent
            to relu_min[x] = max(x, ``{1}``). It moves relu function down to carry out operation at minvalue
            instead of the standard 0.)doc",
            R"doc("min value", "float", "")doc"

        );
        detail::bind_unary_op_with_param(
            m_tensor, "elu", elu,
            py::arg("alpha"),
            R"doc(Returns tensor with the elu activation of all of elements of the input tensor ``{0}`` and scale
            factor alpha as ``{1}``. ELU(x) = alpha*(exp(x) - 1) if x < 0 else x.)doc",
            R"doc("alpha value", "float", "")doc"
        );
        detail::bind_unary_op_with_param(
            m_tensor, "heaviside", heaviside,
            py::arg("value"),
            R"doc(Returns tensor with the Heaviside step function of all of elements of the input tensor ``{0}`` and value factor as ``{1}``.

            HEAVISIDE(x) = 0 if x < 0 , 1 if x > 0 , else value.)doc",
            R"doc("value", "float", "")doc"

        );
        detail::bind_unary_op_with_param(
            m_tensor, "power", power,
            py::arg("exponent"),
            R"doc(Returns tensor with the all of elements of the input tensor ``{0}`` raised to ``{1}``.)doc",
            R"doc("exponent value", "int", ">=0")doc"
        );
        detail::bind_unary_op_with_param(
            m_tensor, "leaky_relu", leaky_relu,
            py::arg("slope"),
            R"doc(Returns tensor with the leaky relu of all of elements of the input tensor ``{0}`` with negative slope as ``{1}``.)doc",
            R"doc("slope value", "float", "")doc"
        );
        detail::bind_unary_op_with_param(
            m_tensor, "unary_chain", &unary_chain,
            py::arg("unary_chain"),
            R"doc(Returns tensor with the unary op chain applied to all of elements of the input tensor ``{0}``.)doc",
            R"doc("Unary op chain", "Vector<FusibleActivation>", "At least 1 activation")doc"
        );

        detail::bind_unary_op(m_tensor, "relu_without_autoformat", &relu_without_autoformat,
            R"doc(Applies the rectified linear unit (ReLU) function to the elements of the input tensor ``{0}``.

            Auto formatting is disabled. Input tensor must have TILE layout. Output tensor will have TILE layout.)doc"
        );

        // *** bcast binary tied to unary ***
        detail::bind_unary_op(m_tensor, "add1", &add1, R"doc(Returns tensor with the addition of one with input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "neg", &neg, R"doc(Returns tensor with the negate all of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "deg2rad", &deg2rad, R"doc(Returns tensor with the deg2rad conversion of elements of the input tensor ``{0}``.)doc");
        detail::bind_unary_op(m_tensor, "rad2deg", &rad2deg, R"doc(Returns tensor with the rad2deg conversion of elements of the input tensor ``{0}``.)doc");

    }
}
