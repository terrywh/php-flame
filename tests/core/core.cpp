#define BOOST_TEST_MODULE core-test
#include <boost/test/unit_test.hpp>

#include "../../src/core/basic_snowflake.h"

BOOST_AUTO_TEST_SUITE(core)

BOOST_AUTO_TEST_CASE(basic_snowflake_node_id_should_remain_unchanged) {
    BOOST_TEST(sizeof(basic_snowflake) == 16);
    basic_snowflake snow {111, 1000000000000ll};
    int64_t id1 = snow.next_id();
    int64_t id2 = snow.next_id();
    BOOST_TEST((id1 & (0x3FFl << 42)) >> 42 == 111); // little endian
}

BOOST_AUTO_TEST_CASE(basic_snowflake_unique_id_should_be_monotone_increasing) {
    BOOST_TEST(sizeof(basic_snowflake) == 16);
    basic_snowflake snow {111, 1000000000000ll};
    int64_t id1 = snow.next_id(), id2 = snow.next_id(), id3 = snow.next_id();
    BOOST_TEST(id1 < id2);
    BOOST_TEST(id2 < id3);
}

BOOST_AUTO_TEST_SUITE_END()