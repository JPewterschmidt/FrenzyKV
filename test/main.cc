#include "gtest/gtest.h"
#include "koios/task_scheduler_concept.h"
#include "koios/runtime.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <filesystem>

#include "spdlog/spdlog.h"
#include "frenzykv/options.h"

namespace fs = ::std::filesystem;

int main(int argc, char** argv)
{
    nlohmann::json config_j;
    ::std::cout << fs::current_path() << ::std::endl;
    ::std::ifstream ifs{ "./test-config.json" };
    bool running_with_user_config{};
    if (ifs)
    {
        ifs >> config_j;
        ::std::cout << argv[0] << ": ./test-config.json Got it." << ::std::endl;
        frenzykv::set_global_options(config_j);
        running_with_user_config = true;
    }
    else
    {
        ::std::cout << argv[0] << ": Unittests will run with defualt config." << ::std::endl;
    }

    ::testing::InitGoogleTest(&argc, argv);
    koios::runtime_init(11);
    auto result = RUN_ALL_TESTS();
    koios::runtime_exit();
    spdlog::info("Working directory: {}", fs::current_path().string());
    if (running_with_user_config)
        spdlog::info("running with user config : ./test-config.json");
    return result;
}
