#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "ring_buffer.h"

TEST_CASE("ring_buffer size from sample rate below 88200", "[ring_buffer]") {
    REQUIRE(ring_buffer::get_size_from_rate(44100) == 2048u);
    REQUIRE(ring_buffer::get_size_from_rate(48000) == 2048u);
}

TEST_CASE("ring_buffer size from sample rate at or above 88200", "[ring_buffer]") {
    REQUIRE(ring_buffer::get_size_from_rate(88200) == 4096u);
    REQUIRE(ring_buffer::get_size_from_rate(96000) == 4096u);
}

TEST_CASE("ring_buffer initial state at 44100", "[ring_buffer]") {
    ring_buffer rb(44100);
    REQUIRE(rb.get_buf_size() == 2048u);
    REQUIRE(rb.get_corr_size() == 1025u); // buf_size / 2 + 1
    REQUIRE(rb.get_idx() == 0u);
}

TEST_CASE("ring_buffer initial state at 96000", "[ring_buffer]") {
    ring_buffer rb(96000);
    REQUIRE(rb.get_buf_size() == 4096u);
    REQUIRE(rb.get_corr_size() == 2049u);
}

TEST_CASE("ring_buffer put stores value and advances index", "[ring_buffer]") {
    ring_buffer rb(44100);
    rb.put(1.5f);
    REQUIRE(rb[0] == Catch::Approx(1.5f));
    REQUIRE(rb.get_idx() == 1u);
    rb.put(2.5f);
    REQUIRE(rb[1] == Catch::Approx(2.5f));
    REQUIRE(rb.get_idx() == 2u);
}

TEST_CASE("ring_buffer index wraps after full cycle", "[ring_buffer]") {
    ring_buffer rb(44100);
    unsigned int size = rb.get_buf_size();
    for (unsigned int i = 0; i < size; i++)
        rb.put(static_cast<float>(i));
    REQUIRE(rb.get_idx() == 0u);
}

TEST_CASE("ring_buffer put overwrites on second cycle", "[ring_buffer]") {
    ring_buffer rb(44100);
    unsigned int size = rb.get_buf_size();
    rb.put(99.0f); // written to [0]
    for (unsigned int i = 1; i < size; i++)
        rb.put(0.0f); // fill the rest
    // now idx wrapped to 0; write again
    rb.put(42.0f);
    REQUIRE(rb[0] == Catch::Approx(42.0f));
}
