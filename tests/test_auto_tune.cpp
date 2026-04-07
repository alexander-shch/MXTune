#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "auto_tune.h"

// All 12 semitones in scale, none flagged for snapping (value 0 = in scale but no snap)
static int no_snap[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// C major: C D E F G A B (snapping enabled on each note)
static int c_major[12] = {1, -1, 1, -1, 1, 1, -1, 1, -1, 1, -1, 1};

TEST_CASE("auto_tune passthrough when no snapping active", "[auto_tune]") {
    auto_tune at;
    at.set_note(no_snap);
    // With lowersnap/uppersnap both 0, pitch passes through unchanged
    REQUIRE(at.tune( 0.0f) == Catch::Approx( 0.0f).margin(0.001f));
    REQUIRE(at.tune( 5.0f) == Catch::Approx( 5.0f).margin(0.001f));
    REQUIRE(at.tune(-12.0f) == Catch::Approx(-12.0f).margin(0.001f));
}

TEST_CASE("auto_tune shift adds semitones to output", "[auto_tune]") {
    auto_tune at;
    at.set_note(no_snap);
    at.set_shift(1.0f);
    REQUIRE(at.tune(0.0f) == Catch::Approx(1.0f).margin(0.001f));
    REQUIRE(at.tune(5.0f) == Catch::Approx(6.0f).margin(0.001f));
}

TEST_CASE("auto_tune negative shift", "[auto_tune]") {
    auto_tune at;
    at.set_note(no_snap);
    at.set_shift(-2.0f);
    REQUIRE(at.tune(5.0f) == Catch::Approx(3.0f).margin(0.001f));
}

TEST_CASE("auto_tune pull=1 locks output to fixed pitch", "[auto_tune]") {
    auto_tune at;
    at.set_note(no_snap);
    at.set_pull(1.0f);
    at.set_fixed(5.0f);
    REQUIRE(at.tune( 0.0f) == Catch::Approx(5.0f).margin(0.001f));
    REQUIRE(at.tune(-10.0f) == Catch::Approx(5.0f).margin(0.001f));
}

TEST_CASE("auto_tune output clamped at upper limit", "[auto_tune]") {
    auto_tune at;
    at.set_note(no_snap);
    at.set_shift(100.0f);
    REQUIRE(at.tune(0.0f) <= 24.0f);
}

TEST_CASE("auto_tune output clamped at lower limit", "[auto_tune]") {
    auto_tune at;
    at.set_note(no_snap);
    at.set_shift(-100.0f);
    REQUIRE(at.tune(0.0f) >= -48.0f);
}

TEST_CASE("auto_tune C stays on C in C major", "[auto_tune]") {
    auto_tune at;
    at.set_note(c_major);
    at.set_amount(1.0f);
    REQUIRE(at.tune(0.0f) == Catch::Approx(0.0f).margin(0.01f));
}

TEST_CASE("auto_tune D stays on D in C major", "[auto_tune]") {
    auto_tune at;
    at.set_note(c_major);
    at.set_amount(1.0f);
    REQUIRE(at.tune(2.0f) == Catch::Approx(2.0f).margin(0.01f));
}

TEST_CASE("auto_tune C# is pulled toward adjacent scale notes in C major", "[auto_tune]") {
    auto_tune at;
    at.set_note(c_major);
    at.set_amount(1.0f);
    // C# (1.0) is between C (0) and D (2); result must be between them
    float result = at.tune(1.0f);
    REQUIRE(result >= 0.0f);
    REQUIRE(result <= 2.0f);
}
