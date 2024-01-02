#include "fmt/core.h"
#include "gtest/gtest.h"
#include "koios/task_scheduler_concept.h"
#include "koios/runtime.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    koios::runtime_init(11);
    auto result = RUN_ALL_TESTS();
    koios::runtime_exit();
    return result;
}
