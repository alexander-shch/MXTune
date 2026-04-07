#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "delay.h"

TEST_CASE("delay zero delay is passthrough", "[delay]") {
    delay d;
    d.set_delay(0);
    REQUIRE(d.process(1.0f)  == Catch::Approx(1.0f));
    REQUIRE(d.process(2.0f)  == Catch::Approx(2.0f));
    REQUIRE(d.process(-0.5f) == Catch::Approx(-0.5f));
}

TEST_CASE("delay get_delay returns set value", "[delay]") {
    delay d;
    d.set_delay(100);
    REQUIRE(d.get_delay() == 100u);
}

TEST_CASE("delay of 1 delays by one sample", "[delay]") {
    delay d;
    d.set_delay(1);
    REQUIRE(d.process(1.0f) == Catch::Approx(0.0f)); // returns initial zero
    REQUIRE(d.process(2.0f) == Catch::Approx(1.0f));
    REQUIRE(d.process(3.0f) == Catch::Approx(2.0f));
}

TEST_CASE("delay of 3 delays by three samples", "[delay]") {
    delay d;
    d.set_delay(3);
    d.process(1.0f); // out=0
    d.process(2.0f); // out=0
    d.process(3.0f); // out=0
    REQUIRE(d.process(4.0f) == Catch::Approx(1.0f));
    REQUIRE(d.process(5.0f) == Catch::Approx(2.0f));
    REQUIRE(d.process(6.0f) == Catch::Approx(3.0f));
}

TEST_CASE("delay clamps to max_delay", "[delay]") {
    delay d(100);
    d.set_delay(200);
    REQUIRE(d.get_delay() == 100u);
}

TEST_CASE("delay set_delay resets buffer to zero", "[delay]") {
    delay d;
    d.set_delay(2);
    d.process(5.0f);
    d.set_delay(2); // reset — buffer zeroed
    REQUIRE(d.process(1.0f) == Catch::Approx(0.0f));
    REQUIRE(d.process(2.0f) == Catch::Approx(0.0f));
    REQUIRE(d.process(3.0f) == Catch::Approx(1.0f));
}
