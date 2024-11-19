// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "device_fixture.hpp"
#include "gtest/gtest.h"
#include "tt_metal/detail/tt_metal.hpp"
#include "tt_metal/host_api.hpp"
#include "tt_metal/common/constants.hpp"
#include "tt_metal/test_utils/stimulus.hpp"

using namespace tt::tt_metal;

namespace basic_tests::l1::sharded {

struct L1Config {
    uint32_t num_cores_height = 2;
    uint32_t num_cores_width = 1;
    uint32_t num_tiles_per_core_height = 2;
    uint32_t num_tiles_per_core_width = 2;
    uint32_t element_size = 2;
    uint32_t size_bytes = 1 * num_cores_height * num_tiles_per_core_height * tt::constants::TILE_HEIGHT *
                          num_cores_width * num_tiles_per_core_width * tt::constants::TILE_WIDTH * element_size;
    uint32_t page_size_bytes = tt::constants::TILE_HW * element_size;
    tt::DataFormat l1_data_format = tt::DataFormat::Float16_b;
    TensorMemoryLayout buffer_layout = TensorMemoryLayout::HEIGHT_SHARDED;

    bool sharded = true;
    ShardSpecBuffer shard_spec() const {
        return ShardSpecBuffer(
            CoreRangeSet(std::set<CoreRange>(
                {CoreRange(CoreCoord(0, 0), CoreCoord(0, num_cores_height * num_cores_width - 1))})),
            {(uint32_t)num_tiles_per_core_height * tt::constants::TILE_HEIGHT,
             (uint32_t)num_tiles_per_core_width * tt::constants::TILE_WIDTH},
            ShardOrientation::ROW_MAJOR,
            false,
            {tt::constants::TILE_HEIGHT, tt::constants::TILE_WIDTH},
            {1 * num_cores_height * num_tiles_per_core_height * num_cores_height,
             num_tiles_per_core_width * num_cores_width});
    }
};

namespace local_test_functions {

/// @brief does host -> L1 -> host and makes sure its the same data
/// @param device
/// @param test_config - Configuration of the test -- see struct
/// @return
bool l1_buffer_read_write(Device* device, const L1Config& test_config) {
    bool pass = true;

    auto buffer = test_config.sharded ? CreateBuffer(tt::tt_metal::ShardedBufferConfig{
                                            .device = device,
                                            .size = test_config.size_bytes,
                                            .page_size = test_config.page_size_bytes,
                                            .buffer_layout = test_config.buffer_layout,
                                            .shard_parameters = test_config.shard_spec()})
                                      : CreateBuffer(tt::tt_metal::BufferConfig{
                                            .device = device,
                                            .size = test_config.size_bytes,
                                            .page_size = test_config.page_size_bytes,
                                            .buffer_layout = test_config.buffer_layout});

    auto input =
        tt::test_utils::generate_uniform_random_vector<uint32_t>(0, 100, test_config.size_bytes / sizeof(uint32_t));

    tt::tt_metal::detail::WriteToBuffer(buffer, input);

    tt::Cluster::instance().l1_barrier(device->id());
    std::vector<uint32_t> output;
    tt::tt_metal::detail::ReadFromBuffer(buffer, output);
    pass &= (output == input);

    if (!pass) {
        if (input.size() != output.size()) {
            std::cout << "Different size of input and output, input.size() = " << input.size() << " output.size() "
                      << output.size() << std::endl;
        }
        int smaller_size = std::min<int>(input.size(), output.size());
        auto entries_per_page = test_config.page_size_bytes / (sizeof(uint32_t));
        for (int i = 0; i < smaller_size; i++) {
            if (input[i] != output[i]) {
                std::cout << "mismatch on page: " << i / entries_per_page << " entry index: " << i % entries_per_page
                          << " with input being " << std::hex << input[i] << " and output being " << output[i]
                          << std::dec << std::endl;
            }
        }
    }

    return pass;
}

}  // end namespace local_test_functions

TEST_F(DeviceFixture, TestInterleavedReadWrite) {
    for (unsigned int id = 0; id < num_devices_; id++) {
        L1Config test_config;
        test_config.buffer_layout = TensorMemoryLayout::INTERLEAVED;
        test_config.sharded = false;
        EXPECT_TRUE(local_test_functions::l1_buffer_read_write(this->devices_.at(id), test_config));
    }
}

TEST_F(DeviceFixture, TestHeightShardReadWrite) {
    for (unsigned int id = 0; id < num_devices_; id++) {
        L1Config test_config;
        EXPECT_TRUE(local_test_functions::l1_buffer_read_write(this->devices_.at(id), test_config));
    }
}

TEST_F(DeviceFixture, TestWidthShardReadWrite) {
    for (unsigned int id = 0; id < num_devices_; id++) {
        L1Config test_config;
        test_config.buffer_layout = TensorMemoryLayout::WIDTH_SHARDED;
        EXPECT_TRUE(local_test_functions::l1_buffer_read_write(this->devices_.at(id), test_config));
    }
}

TEST_F(DeviceFixture, TestUnorderedHeightShardReadWrite) {
    std::vector<CoreCoord> cores = {CoreCoord(1, 0), CoreCoord(1, 3), CoreCoord(1, 4), CoreCoord(6, 6),
                                    CoreCoord(6, 5), CoreCoord(6, 4), CoreCoord(6, 3), CoreCoord(6, 2),
                                    CoreCoord(6, 1), CoreCoord(6, 0), CoreCoord(5, 0), CoreCoord(5, 1),
                                    CoreCoord(5, 2), CoreCoord(5, 3), CoreCoord(5, 4), CoreCoord(5, 5),
                                    CoreCoord(5, 6), CoreCoord(2, 4), CoreCoord(2, 3), CoreCoord(2, 0)};
    std::vector<CoreRange> core_ranges;
    core_ranges.reserve(cores.size());
    for (const auto& core : cores) {
        core_ranges.emplace_back(core, core);
    }
    ShardSpecBuffer shard_spec = ShardSpecBuffer(
        CoreRangeSet(core_ranges),
        {tt::constants::TILE_HEIGHT, tt::constants::TILE_WIDTH},
        ShardOrientation::ROW_MAJOR,
        false,
        {tt::constants::TILE_HEIGHT, tt::constants::TILE_WIDTH},
        {(uint32_t)cores.size(), 1});
    for (unsigned int id = 0; id < num_devices_; id++) {
        auto device = this->devices_.at(id);
        std::vector<CoreCoord> physical_cores;
        physical_cores.reserve(cores.size());
        for (const auto& core : cores) {
            physical_cores.push_back(device->worker_core_from_logical_core(core));
        }
        uint32_t page_size = tt::constants::TILE_HW * sizeof(uint32_t);
        uint32_t total_size = cores.size() * page_size;
        auto buffer = CreateBuffer(tt::tt_metal::ShardedBufferConfig{
            .device = device,
            .size = total_size,
            .page_size = page_size,
            .buffer_layout = TensorMemoryLayout::HEIGHT_SHARDED,
            .shard_parameters = shard_spec});
        auto input = tt::test_utils::generate_uniform_random_vector<uint32_t>(0, 100, total_size / sizeof(uint32_t));

        tt::tt_metal::detail::WriteToBuffer(buffer, input);
        tt::Cluster::instance().l1_barrier(device->id());
        auto input_it = input.begin();
        for (const auto& physical_core : physical_cores) {
            auto readback = tt::llrt::read_hex_vec_from_core(device->id(), physical_core, buffer->address(), page_size);
            EXPECT_TRUE(std::equal(input_it, input_it + tt::constants::TILE_HW, readback.begin()));
            input_it += tt::constants::TILE_HW;
        }
        std::vector<uint32_t> output;
        tt::tt_metal::detail::ReadFromBuffer(buffer, output);
        EXPECT_EQ(input, output);
    }
}

}  // end namespace basic_tests::l1::sharded