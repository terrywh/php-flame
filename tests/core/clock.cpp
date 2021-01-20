#include <boost/test/unit_test.hpp>
#include "../../src/core/clock.h"

BOOST_AUTO_TEST_SUITE(core_clock)

BOOST_AUTO_TEST_CASE(standard_string_representation) {
    const core::clock& clock = core::clock::get_const_instance();
    std::string utc = clock.utc(), iso = clock.iso();
    // 形如：2021-01-18T13:32:10.368Z
    BOOST_TEST(utc[ 0] == '2');
    BOOST_TEST(utc[ 1] == '0');
    BOOST_TEST(utc[ 4] == '-');
    BOOST_TEST(utc[10] == 'T');
    BOOST_TEST(utc[13] == ':');
    BOOST_TEST(utc[23] == 'Z');
    // 形如：2021-01-18 13:32:10
    BOOST_TEST(iso[ 0] == '2');
    BOOST_TEST(iso[ 1] == '0');
    BOOST_TEST(iso[ 4] == '-');
    BOOST_TEST(iso[10] == ' ');
    BOOST_TEST(iso[13] == ':');
    BOOST_TEST(iso[16] == ':');
}

BOOST_AUTO_TEST_SUITE_END()