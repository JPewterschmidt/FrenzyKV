#include "gtest/gtest.h"

#include "frenzykv/persistent/level.h"

using namespace frenzykv;

TEST(level, naming)
{
    uuid id_expected;
    level_t l_expected = 10;
    auto name = name_a_sst(l_expected, id_expected);
    auto l_id_opt = retrive_level_and_id_from_sst_name(name);
    ASSERT_TRUE(l_id_opt.has_value());
    const auto& l  = l_id_opt->first;
    const auto& id = l_id_opt->second;
    ASSERT_EQ(l, l_expected);
    ASSERT_EQ(id, id_expected) << id_expected.to_string();
}
