// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "tt_lib_bindings.hpp"
#include "tt_dnn/op_library/bmm/bmm_op.hpp"
#include "tt_dnn/op_library/nlp_tms/nlp_tms.hpp"
#include "tt_dnn/op_library/compute_kernel_config.hpp"

namespace tt::tt_metal::detail
{
    void TensorModuleCustomAndBMMOPs( py::module & m_tensor)
    {
        // matrix multiplication
        m_tensor.def("bmm_tilize_untilize", &bmm_tilize_untilize,
            py::arg("a").noconvert(), py::arg("b").noconvert(), py::arg("bias").noconvert(), py::arg("out_dt").noconvert(),
            py::arg("a_height_nblocks").noconvert(), py::arg("a_width_nblocks").noconvert(), py::arg("b_width_nblocks").noconvert(),
            py::arg("a_block_height_ntiles").noconvert(), py::arg("a_block_width_ntiles").noconvert(), py::arg("b_block_width_ntiles").noconvert(),
            py::arg("out_subblock_height_ntiles").noconvert(), py::arg("out_subblock_width_ntiles").noconvert(),
            py::arg("tilize_in0").noconvert(), py::arg("untilize_out").noconvert(), py::arg("has_bias").noconvert(),
            py::arg("kernel_config").noconvert() = std::nullopt, R"doc(
            Perform a batched matmul ``A x B`` with two tensors, where batch and channel dims match.
            This op also supports tiling tensor A and untiling the output.

            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------|
            | Argument                      | Description                                           | Data type | Valid range | Required |
            +===============================+=======================================================+===========+=============+==========+
            | a                             | LHS matmul operand                                    | Tensor    |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | b                             | RHS matmul operand                                    | Tensor    |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | a_height_nblocks              | Number of blocks along A's height                     | uint32_t  |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | a_width_nblocks               | Number of blocks along A's width (= along B's height) | uint32_t  |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | b_width_nblocks               | Number of blocks along B's width                      | uint32_t  |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | a_block_height_ntiles         | Number of tiles along height of an A block            | uint32_t  |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | a_block_width_ntiles          | Number of tiles along width of an A block             | uint32_t  |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | b_block_width_ntiles          | Number of tiles along width of a B block              | uint32_t  |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | out_subblock_height_ntiles    | Height of subblocks on height for output              | uint32_t  |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
            | out_subblock_width_ntiles     | Number of subblocks on width for output               | uint32_t  |             | Yes      |
            +-------------------------------+-------------------------------------------------------+-----------+-------------+----------+
        )doc");
        // *** matrix multiplication ***
        m_tensor.def(
                "matmul",
        [](const Tensor& input_a,
           const Tensor& input_b,
           const MemoryConfig& output_mem_config,
           std::optional<const DeviceComputeKernelConfig> kernel_config,
           const bool untilize_out) {
            return tt::operations::primary::matmul(input_a, input_b, /*bias=*/std::nullopt, /*program_config=*/std::nullopt, output_mem_config, /*output_dtype=*/std::nullopt, kernel_config, untilize_out);
        },
            py::arg("input_a").noconvert(), py::arg("input_b").noconvert(), py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, py::arg("kernel_config").noconvert() = std::nullopt, py::arg("untilize_out").noconvert() = false, R"doc(
            Perform a non-batched matrix multiplication ``arg0 x arg1`` with two tensors.

            Both input tensors must have BFLOAT16 data type.

            Output tensor will have BFLOAT16 data type.

            .. csv-table::
                :header: "Argument", "Description", "Data type", "Valid range", "Required"

                "input_a", "First tensor to multiply", "Tensor", "Tensor of shape [1, 1, Y, S]", "Yes"
                "input_b", "Second tensor to multiply", "Tensor", "Tensor of shape [1, 1, S, X]", "Yes"
                "output_mem_config", "Layout of tensor in TT Accelerator device memory banks", "MemoryConfig", "Default is interleaved in DRAM", "No"
        )doc");

        m_tensor.def("bmm",
        [](const Tensor& input_a,
           const Tensor& input_b,
           const MemoryConfig& output_mem_config,
           std::optional<const DeviceComputeKernelConfig> kernel_config,
           const bool untilize_out) {
            return tt::operations::primary::matmul(input_a, input_b, /*bias=*/std::nullopt, /*program_config=*/std::nullopt, output_mem_config, /*output_dtype=*/std::nullopt, kernel_config, untilize_out, /*user_core_coord=*/std::nullopt, /*user_fused_activation=*/std::nullopt, /*input_b_is_batched=*/true);
        },
            py::arg("input_a").noconvert(), py::arg("input_b").noconvert(), py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, py::arg("kernel_config").noconvert() = std::nullopt, py::arg("untilize_out").noconvert() = false,  R"doc(
            Perform a batched matmul ``arg0 x arg1`` with two tensors, where batch dims match.

            Both input tensors must have BFLOAT16 data type.

            Output tensor will have BFLOAT16 data type.

            .. csv-table::
                :header: "Argument", "Description", "Data type", "Valid range", "Required"

                "input_a", "First tensor to multiply", "Tensor", "Tensor of shape [W, Z, Y, S]", "Yes"
                "input_b", "Second tensor to multiply", "Tensor", "Tensor of shape [W, Z, S, X]", "Yes"
                "output_mem_config", "Layout of tensor in TT Accelerator device memory banks", "MemoryConfig", "Default is interleaved in DRAM", "No"
        )doc");

        // Custom Falcon matmuls/bmms
        m_tensor.def("falcon_fused_qkv_matmul", &falcon_fused_qkv_matmul,
            py::arg().noconvert(), py::arg().noconvert(), py::arg("bias").noconvert() = std::nullopt, py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, py::arg("output_dtype").noconvert() = std::nullopt, R"doc(
            Perform a falcon_fused_qkv non-batched matmul ``A x B`` with two tensors.
        )doc");
        m_tensor.def("falcon_selfout_matmul", &falcon_selfout_matmul,
            py::arg().noconvert(), py::arg().noconvert(), py::arg("bias").noconvert() = std::nullopt, py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, py::arg("output_dtype").noconvert() = std::nullopt, R"doc(
            Perform a falcon_selfout non-batched matmul ``A x B`` with two tensors.
        )doc");
        m_tensor.def("falcon_dense_4h_to_h_matmul", &falcon_dense_4h_to_h_matmul,
            py::arg().noconvert(), py::arg().noconvert(), py::arg("bias").noconvert() = std::nullopt, py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, py::arg("output_dtype").noconvert() = std::nullopt, py::arg("packer_l1_acc").noconvert() = std::nullopt, R"doc(
            Perform a falcon_dense_4h_to_h non-batched matmul ``A x B`` with two tensors.
        )doc");
        m_tensor.def("falcon_dense_h_to_4h_matmul", &falcon_dense_h_to_4h_matmul,
            py::arg().noconvert(), py::arg().noconvert(), py::arg("bias").noconvert() = std::nullopt, py::arg("fused_activation") = std::nullopt, py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, py::arg("output_dtype").noconvert() = std::nullopt, R"doc(
            Perform a falcon_dense_h_to_4h non-batched matmul ``A x B`` with two tensors. This invokes the MULTI_CORE matmul parallelization. This parallelization does not support bias option yet.
        )doc");
        m_tensor.def("falcon_lm_head_matmul", &falcon_lm_head_matmul,
            py::arg().noconvert(), py::arg().noconvert(), py::arg("bias").noconvert() = std::nullopt, py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, py::arg("output_dtype").noconvert() = std::nullopt, R"doc(
            Perform a falcon_lm_head non-batched matmul ``A x B`` with two tensors. This invokes the MULTI_CORE matmul parallelization. This parallelization does not support bias option yet.
        )doc");

        // Custom Generic NLP TMs
        // This op should support arbitrary B and S divisible by 32 on DRAM; on L1, might error out due to space
        m_tensor.def("nlp_create_qkv_heads_falcon7b", &nlp_create_qkv_heads_falcon7b,
            py::arg().noconvert(), py::arg("output_mem_config") = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, R"doc(
            Shuffles [B, 1, S, 4672] fused qkv matrix into 3 heads with shapes [B, 71, S, 64], [B, 1, S, 64], and [B, 1, S, 64].
        )doc");
        m_tensor.def("nlp_create_qkv_heads_decode", &nlp_create_qkv_heads_decode,
            py::arg("input").noconvert(), py::arg("num_heads").noconvert(), py::arg("num_kv_heads").noconvert() = std::nullopt, py::arg("output_mem_config") = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, R"doc(
            Shuffles [1, S=1, B=32, head_dim * (num_heads + 2*num_kv_heads)] fused qkv matrix into Q, K, and V heads with shape [S, B, num_heads, head_dim] for Q and [S, B, num_kv_heads, head_dim] for K and V, where num_heads and num_kv_heads will be padded to nearest 32. Input must be sharded, B=32 and S=1.
        )doc");
        m_tensor.def("nlp_concat_heads_decode", &nlp_concat_heads_decode,
            py::arg().noconvert(), py::arg("num_heads").noconvert() = std::nullopt, R"doc(
            Shuffles [S=1, B=32, 32(num_heads), head_dim] tensor into tensor with shape [S=1, 1, B=32, num_heads * head_dim]. num_heads should be specified and be less than 32; the op will assume the input padded num_heads to 32 and will unpad it. The output is default width sharded by num heads.
        )doc");
        m_tensor.def("nlp_kv_cache_load_slice", &nlp_kv_cache_load_slice,
            py::arg("input").noconvert(), py::arg("seq_len_start"), py::arg("seq_len_end"), R"doc(
            Unpad TT INTERLEAVED, TILE layout Tensor into a height sharded tensor. Typically used to unpad the KV cache from [B,n_heads,max_seq_length,head_dim] (or [n_heads,B,max_seq_length,head_dim]) into [B,n_heads,S,head_dim] (or [n_heads,B,S,head_dim]), where S = seq_len_end-seq_len_start. seq_len_start and seq_len_end are the start and end of the sequence length to unpad, and must be multiples of 32.
            Returns an output tensor that is height sharded on B x n_heads corees (note the B and n_heads dims are interchangeable), where each shard is [S, head_dim].
        )doc");
        // More general implementation, but perf might be worse since the cbs are very small and writer calls noc_async_write_barrier() a lot
        m_tensor.def("nlp_create_qkv_heads", &nlp_create_qkv_heads,
            py::arg("input").noconvert(), py::arg("input_kv").noconvert() = std::nullopt, py::arg("num_heads").noconvert(), py::arg("num_kv_heads").noconvert() = std::nullopt, py::arg("transpose_k_heads").noconvert() = true, py::arg("output_mem_config") = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, R"doc(
            Shuffles [B, 1, S, 3 * head_dim * num_heads] fused qkv matrix into 3 Q, K, and V heads with shapes [B, num_heads, S, head_dim], [B, num_kv_heads, head_dim, S], and [B, num_kv_heads, S, head_dim]. If optional ``input_kv`` tensor is provided, K and V will be created from ``input_kv`` and ``input`` should have shape [B, 1, S, head_dim * num_heads] instead. ``num_kv_heads`` defaults to ``num_heads`` if not provided. An additional transpose along the last two dims is performed by default for K heads, but this can be skipped with ``transpose_k_heads=false``.
        )doc");
        m_tensor.def("nlp_concat_heads", &nlp_concat_heads,
            py::arg().noconvert(), py::arg("output_mem_config") = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, R"doc(
            Shuffles [B, num_heads, S, head_dim] tensor into tensor with shape [B, 1, S, num_heads * head_dim].
        )doc");

        // Custom Resnet matmuls
        m_tensor.def("resnet_matmul", &resnet_matmul,
            py::arg().noconvert(), py::arg().noconvert(), py::arg("bias").noconvert() = std::nullopt, py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG, py::arg("output_dtype").noconvert() = std::nullopt, py::arg("math_fidelity").noconvert() = MathFidelity::LoFi, R"doc(
            Perform a resnet_matmul with fused bias.
        )doc");

        m_tensor.def("create_qkv_heads", &create_qkv_heads,
        py::arg("input").noconvert(),
        py::arg("num_q_heads").noconvert(),
        py::arg("num_kv_heads").noconvert() = std::nullopt,
        py::arg("transpose_k_heads").noconvert() = false,
        py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG,
        R"doc(
        Splits a [B, 1, Seq_len, H] fused qkv matrix (where H is num_kv_heads * (num_q_heads/num_kv_heads + 2) * head_dim) into a Q tensor [B, num_q_heads, Seq_len, head_dim], K tensor [B, num_kv_heads, Seq_len, head_dim] (with the last two dims transposed if applicable) and V tensor [B, num_kv_heads, Seq_len, head_dim].
        )doc");

        m_tensor.def("create_qkv_heads_from_separate_tensors", &create_qkv_heads_from_separate_tensors,
        py::arg("input_q").noconvert(),
        py::arg("input_kv").noconvert(),
        py::arg("num_q_heads").noconvert(),
        py::arg("num_kv_heads").noconvert() = std::nullopt,
        py::arg("transpose_k_heads").noconvert() = false,
        py::arg("output_mem_config").noconvert() = operation::DEFAULT_OUTPUT_MEMORY_CONFIG,
        R"doc(
        Splits a [B, 1, Seq_len, H] q matrix and fused kv matrix (where H is num_q_heads * head_dim for q and num_kv_heads * head_dim * 2 for kv) into a Q tensor [B, num_q_heads, Seq_len, head_dim], K tensor [B, num_kv_heads, Seq_len, head_dim] (with the last two dims transposed if applicable) and V tensor [B, num_kv_heads, Seq_len, head_dim].
        )doc");
    }

}
