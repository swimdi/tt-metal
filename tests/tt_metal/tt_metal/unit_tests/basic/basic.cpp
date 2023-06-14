#include <algorithm>
#include <functional>
#include <random>

#include "doctest.h"
#include "multi_device_fixture.hpp"
#include "single_device_fixture.hpp"
#include "tt_metal/host_api.hpp"
#include "tt_metal/hostdevcommon/common_runtime_address_map.h"  // FIXME: Should remove dependency on this
#include "tt_metal/test_utils/env_vars.hpp"
#include "tt_metal/test_utils/print_helpers.hpp"
#include "tt_metal/test_utils/stimulus.hpp"
using namespace tt;

namespace unit_tests::basic {
// Ping a set number of bytes into the specified address of L1
bool l1_ping(
    tt_metal::Device* device, const size_t& byte_size, const size_t& l1_byte_address, const CoreCoord& grid_size) {
    bool pass = true;
    ////////////////////////////////////////////////////////////////////////////
    //                      Execute Application
    ////////////////////////////////////////////////////////////////////////////
    auto inputs = tt::test_utils::generate_uniform_random_vector<uint32_t>(0, UINT32_MAX, byte_size / sizeof(uint32_t));
    for (int y = 0; y < grid_size.y; y++) {
        for (int x = 0; x < grid_size.x; x++) {
            CoreCoord dest_core({.x = static_cast<size_t>(x), .y = static_cast<size_t>(y)});
            tt_metal::WriteToDeviceL1(device, dest_core, l1_byte_address, inputs);
        }
    }
    for (int y = 0; y < grid_size.y; y++) {
        for (int x = 0; x < grid_size.x; x++) {
            CoreCoord dest_core({.x = static_cast<size_t>(x), .y = static_cast<size_t>(y)});
            std::vector<uint32_t> dest_core_data;
            tt_metal::ReadFromDeviceL1(device, dest_core, l1_byte_address, byte_size, dest_core_data);
            pass &= (dest_core_data == inputs);
            if (not pass) {
                cout << "Mismatch at Core: " << dest_core.str() << std::endl;
            }
        }
    }
    return pass;
}

// Ping a set number of bytes into the specified address of DRAM
bool dram_ping(
    tt_metal::Device* device,
    const size_t& byte_size,
    const size_t& dram_byte_address,
    const unsigned int& num_channels) {
    bool pass = true;
    ////////////////////////////////////////////////////////////////////////////
    //                      Execute Application
    ////////////////////////////////////////////////////////////////////////////
    auto inputs = tt::test_utils::generate_uniform_random_vector<uint32_t>(0, UINT32_MAX, byte_size / sizeof(uint32_t));
    for (unsigned int channel = 0; channel < num_channels; channel++) {
        tt_metal::WriteToDeviceDRAMChannel(device, channel, dram_byte_address, inputs);
    }
    for (unsigned int channel = 0; channel < num_channels; channel++) {
        std::vector<uint32_t> dest_channel_data;
        tt_metal::ReadFromDeviceDRAMChannel(device, channel, dram_byte_address, byte_size, dest_channel_data);
        pass &= (dest_channel_data == inputs);
        if (not pass) {
            cout << "Mismatch at Channel: " << channel << std::endl;
        }
    }
    return pass;
}

// load_blank_kernels into all cores and ensure all cores hit mailbox values
bool load_all_blank_kernels(tt_metal::Device* device) {
    bool pass = true;
    CoreCoord grid_size = device->logical_grid_size();
    constexpr int INIT_VALUE = 42;
    constexpr int DONE_VALUE = 1;
    const std::unordered_map<string, uint32_t> mailbox_addresses = {
        {"BRISC", MEM_TEST_MAILBOX_ADDRESS + MEM_MAILBOX_BRISC_OFFSET},
        {"TRISC0", MEM_TEST_MAILBOX_ADDRESS + MEM_MAILBOX_TRISC0_OFFSET},
        {"TRISC1", MEM_TEST_MAILBOX_ADDRESS + MEM_MAILBOX_TRISC1_OFFSET},
        {"TRISC2", MEM_TEST_MAILBOX_ADDRESS + MEM_MAILBOX_TRISC2_OFFSET},
        {"NCRISC", MEM_TEST_MAILBOX_ADDRESS + MEM_MAILBOX_NCRISC_OFFSET},
    };
    ////////////////////////////////////////////////////////////////////////////
    //                      Application Setup
    ////////////////////////////////////////////////////////////////////////////
    tt_metal::Program program = tt_metal::Program();
    ////////////////////////////////////////////////////////////////////////////
    //                      Compile Application
    ////////////////////////////////////////////////////////////////////////////
    pass &= tt_metal::CompileProgram(device, program);
    ////////////////////////////////////////////////////////////////////////////
    //                      Execute Application
    ////////////////////////////////////////////////////////////////////////////
    pass &= tt_metal::ConfigureDeviceWithProgram(device, program);
    pass &= tt_metal::LaunchKernels(device, program);
    return pass;
}
}  // namespace unit_tests::basic

TEST_SUITE(
    "MultiDeviceTest" *
    doctest::description("Basic device tests should just test simple APIs and shouldn't take more than 1s per chip, "
                         "but can scale beyond for many devices.") *
    doctest::timeout(5)) {
    TEST_CASE("Multi Device Initialize and Teardown" * doctest::timeout(2)) {
        auto arch = tt::get_arch_from_string(tt::test_utils::get_env_arch_name());
        const size_t num_devices = tt::tt_metal::Device::detect_num_available_devices();
        REQUIRE(num_devices > 0);
        std::vector<tt::tt_metal::Device*> devices;

        for (unsigned int id = 0; id < num_devices; id++) {
            devices.push_back(tt::tt_metal::CreateDevice(arch, id));
            REQUIRE(tt::tt_metal::InitializeDevice(devices.at(id)));
        }
        for (unsigned int id = 0; id < num_devices; id++) {
            REQUIRE(tt::tt_metal::CloseDevice(devices.at(id)));
        }
    }
    TEST_CASE("Multi Device Load Blank Kernels" * doctest::timeout(2)) {
        auto arch = tt::get_arch_from_string(tt::test_utils::get_env_arch_name());
        const size_t num_devices = tt::tt_metal::Device::detect_num_available_devices();
        REQUIRE(num_devices > 0);
        std::vector<tt::tt_metal::Device*> devices;

        for (unsigned int id = 0; id < num_devices; id++) {
            devices.push_back(tt::tt_metal::CreateDevice(arch, id));
            REQUIRE(tt::tt_metal::InitializeDevice(devices.at(id)));
        }
        for (unsigned int id = 0; id < num_devices; id++) {
            unit_tests::basic::load_all_blank_kernels(devices.at(id));
        }
        for (unsigned int id = 0; id < num_devices; id++) {
            REQUIRE(tt::tt_metal::CloseDevice(devices.at(id)));
        }
    }
    TEST_CASE_FIXTURE(unit_tests::MultiDeviceFixture, "Ping all legal dram channels") {
        for (unsigned int id = 0; id < num_devices_; id++) {
            auto device_ = devices_.at(id);
            SUBCASE("Low Address Dram") {
                size_t start_byte_address = 0;
                REQUIRE(unit_tests::basic::dram_ping(device_, 4, start_byte_address, device_->num_dram_channels()));
                REQUIRE(unit_tests::basic::dram_ping(device_, 12, start_byte_address, device_->num_dram_channels()));
                REQUIRE(unit_tests::basic::dram_ping(device_, 16, start_byte_address, device_->num_dram_channels()));
                REQUIRE(unit_tests::basic::dram_ping(device_, 1024, start_byte_address, device_->num_dram_channels()));
                REQUIRE(
                    unit_tests::basic::dram_ping(device_, 2 * 1024, start_byte_address, device_->num_dram_channels()));
                REQUIRE(
                    unit_tests::basic::dram_ping(device_, 32 * 1024, start_byte_address, device_->num_dram_channels()));
            }
            SUBCASE("High Address Dram") {
                size_t start_byte_address = device_->dram_bank_size() - 32 * 1024;
                REQUIRE(unit_tests::basic::dram_ping(device_, 4, start_byte_address, device_->num_dram_channels()));
                REQUIRE(unit_tests::basic::dram_ping(device_, 12, start_byte_address, device_->num_dram_channels()));
                REQUIRE(unit_tests::basic::dram_ping(device_, 16, start_byte_address, device_->num_dram_channels()));
                REQUIRE(unit_tests::basic::dram_ping(device_, 1024, start_byte_address, device_->num_dram_channels()));
                REQUIRE(
                    unit_tests::basic::dram_ping(device_, 2 * 1024, start_byte_address, device_->num_dram_channels()));
                REQUIRE(
                    unit_tests::basic::dram_ping(device_, 32 * 1024, start_byte_address, device_->num_dram_channels()));
            }
        }
    }
    TEST_CASE_FIXTURE(unit_tests::MultiDeviceFixture, "Ping all legal dram channels + illegal channel") {
        for (unsigned int id = 0; id < num_devices_; id++) {
            auto device_ = devices_.at(id);
            auto num_channels = device_->num_dram_channels() + 1;
            size_t start_byte_address = 0;
            REQUIRE_THROWS_WITH(
                unit_tests::basic::dram_ping(device_, 4, start_byte_address, num_channels),
                doctest::Contains("Bounds-Error"));
        }
    }

    TEST_CASE_FIXTURE(unit_tests::MultiDeviceFixture, "Ping all legal l1 cores") {
        for (unsigned int id = 0; id < num_devices_; id++) {
            auto device_ = devices_.at(id);
            SUBCASE("Low Address L1") {
                size_t start_byte_address = UNRESERVED_BASE;  // FIXME: Should remove dependency on
                                                              // hostdevcommon/common_runtime_address_map.h header.
                REQUIRE(unit_tests::basic::l1_ping(device_, 4, start_byte_address, device_->logical_grid_size()));
                REQUIRE(unit_tests::basic::l1_ping(device_, 12, start_byte_address, device_->logical_grid_size()));
                REQUIRE(unit_tests::basic::l1_ping(device_, 16, start_byte_address, device_->logical_grid_size()));
                REQUIRE(unit_tests::basic::l1_ping(device_, 1024, start_byte_address, device_->logical_grid_size()));
                REQUIRE(
                    unit_tests::basic::l1_ping(device_, 2 * 1024, start_byte_address, device_->logical_grid_size()));
                REQUIRE(
                    unit_tests::basic::l1_ping(device_, 32 * 1024, start_byte_address, device_->logical_grid_size()));
            }
            SUBCASE("High Address L1") {
                size_t start_byte_address = device_->l1_size() - 32 * 1024;
                REQUIRE(unit_tests::basic::l1_ping(device_, 4, start_byte_address, device_->logical_grid_size()));
                REQUIRE(unit_tests::basic::l1_ping(device_, 12, start_byte_address, device_->logical_grid_size()));
                REQUIRE(unit_tests::basic::l1_ping(device_, 16, start_byte_address, device_->logical_grid_size()));
                REQUIRE(unit_tests::basic::l1_ping(device_, 1024, start_byte_address, device_->logical_grid_size()));
                REQUIRE(
                    unit_tests::basic::l1_ping(device_, 2 * 1024, start_byte_address, device_->logical_grid_size()));
                REQUIRE(
                    unit_tests::basic::l1_ping(device_, 32 * 1024, start_byte_address, device_->logical_grid_size()));
            }
        }
    }

    TEST_CASE_FIXTURE(unit_tests::MultiDeviceFixture, "Ping all legal l1 + illegal cores") {
        for (unsigned int id = 0; id < num_devices_; id++) {
            auto device_ = devices_.at(id);
            auto grid_size = device_->logical_grid_size();
            grid_size.x++;
            grid_size.y++;
            size_t start_byte_address = UNRESERVED_BASE;  // FIXME: Should remove dependency on
                                                          // hostdevcommon/common_runtime_address_map.h header.
            REQUIRE_THROWS_WITH(
                unit_tests::basic::l1_ping(device_, 4, start_byte_address, grid_size),
                doctest::Contains("Bounds-Error"));
        }
    }
}

TEST_SUITE(
    "SingleDeviceTest" *
    doctest::description("Basic device tests should just test simple APIs and shouldn't take more than 1s") *
    doctest::timeout(1)) {
    TEST_CASE("Single Device Initialize and Teardown") {
        auto arch = tt::get_arch_from_string(tt::test_utils::get_env_arch_name());
        tt::tt_metal::Device* device;
        const unsigned int pcie_id = 0;
        device = tt::tt_metal::CreateDevice(arch, pcie_id);
        REQUIRE(tt::tt_metal::InitializeDevice(device));
        REQUIRE(tt::tt_metal::CloseDevice(device));
    }

    TEST_CASE("Single Device Load Blank Kernels") {
        auto arch = tt::get_arch_from_string(tt::test_utils::get_env_arch_name());
        tt::tt_metal::Device* device;
        const unsigned int pcie_id = 0;
        device = tt::tt_metal::CreateDevice(arch, pcie_id);
        REQUIRE(tt::tt_metal::InitializeDevice(device));
        REQUIRE(tt::tt_metal::CloseDevice(device));
    }
    TEST_CASE_FIXTURE(unit_tests::SingleDeviceFixture, "Ping all legal dram channels") {
        SUBCASE("Low Address Dram") {
            size_t start_byte_address = 0;
            REQUIRE(unit_tests::basic::dram_ping(device_, 4, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 12, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 16, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 1024, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 2 * 1024, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 32 * 1024, start_byte_address, device_->num_dram_channels()));
        }
        SUBCASE("High Address Dram") {
            size_t start_byte_address = device_->dram_bank_size() - 32 * 1024;
            REQUIRE(unit_tests::basic::dram_ping(device_, 4, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 12, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 16, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 1024, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 2 * 1024, start_byte_address, device_->num_dram_channels()));
            REQUIRE(unit_tests::basic::dram_ping(device_, 32 * 1024, start_byte_address, device_->num_dram_channels()));
        }
    }
    TEST_CASE_FIXTURE(unit_tests::SingleDeviceFixture, "Ping all legal dram channels + illegal channel") {
        auto num_channels = device_->num_dram_channels() + 1;
        size_t start_byte_address = 0;
        REQUIRE_THROWS_WITH(
            unit_tests::basic::dram_ping(device_, 4, start_byte_address, num_channels),
            doctest::Contains("Bounds-Error"));
    }

    TEST_CASE_FIXTURE(unit_tests::SingleDeviceFixture, "Ping all legal l1 cores") {
        SUBCASE("Low Address L1") {
            size_t start_byte_address = UNRESERVED_BASE;  // FIXME: Should remove dependency on
                                                          // hostdevcommon/common_runtime_address_map.h header.
            REQUIRE(unit_tests::basic::l1_ping(device_, 4, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 12, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 16, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 1024, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 2 * 1024, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 32 * 1024, start_byte_address, device_->logical_grid_size()));
        }
        SUBCASE("High Address L1") {
            size_t start_byte_address = device_->l1_size() - 32 * 1024;
            REQUIRE(unit_tests::basic::l1_ping(device_, 4, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 12, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 16, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 1024, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 2 * 1024, start_byte_address, device_->logical_grid_size()));
            REQUIRE(unit_tests::basic::l1_ping(device_, 32 * 1024, start_byte_address, device_->logical_grid_size()));
        }
    }

    TEST_CASE_FIXTURE(unit_tests::SingleDeviceFixture, "Ping all legal l1 + illegal cores") {
        auto grid_size = device_->logical_grid_size();
        grid_size.x++;
        grid_size.y++;
        size_t start_byte_address =
            UNRESERVED_BASE;  // FIXME: Should remove dependency on hostdevcommon/common_runtime_address_map.h header.
        REQUIRE_THROWS_WITH(
            unit_tests::basic::l1_ping(device_, 4, start_byte_address, grid_size), doctest::Contains("Bounds-Error"));
    }
}
