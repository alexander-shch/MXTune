#include <catch2/catch_test_macros.hpp>
#include "midi_msg.h"

TEST_CASE("midi_msg default state", "[midi_msg]") {
    midi_msg msg;
    REQUIRE(msg.get_channel() == 1);
    REQUIRE_FALSE(msg.is_note_on());
    REQUIRE(msg.is_note_off());
    REQUIRE(msg.get_note() == 0);
    REQUIRE(msg.get_velocity() == 0);
}

TEST_CASE("midi_msg note_on sets state", "[midi_msg]") {
    midi_msg msg;
    msg.note_on(3, 60, 100);
    REQUIRE(msg.is_note_on());
    REQUIRE_FALSE(msg.is_note_off());
    REQUIRE(msg.get_channel() == 3);
    REQUIRE(msg.get_note() == 60);
    REQUIRE(msg.get_velocity() == 100);
}

TEST_CASE("midi_msg note_off sets state", "[midi_msg]") {
    midi_msg msg;
    msg.note_on(1, 60, 100);
    msg.note_off(2, 64, 80);
    REQUIRE_FALSE(msg.is_note_on());
    REQUIRE(msg.is_note_off());
    REQUIRE(msg.get_channel() == 2);
    REQUIRE(msg.get_note() == 64);
    REQUIRE(msg.get_velocity() == 80);
}

TEST_CASE("midi_msg set_channel", "[midi_msg]") {
    midi_msg msg;
    msg.set_channel(10);
    REQUIRE(msg.get_channel() == 10);
}

TEST_CASE("midi_msg is_note_on and is_note_off are always opposite", "[midi_msg]") {
    midi_msg msg;
    msg.note_on(1, 60, 100);
    REQUIRE(msg.is_note_on() != msg.is_note_off());
    msg.note_off(1, 60, 100);
    REQUIRE(msg.is_note_on() != msg.is_note_off());
}

TEST_CASE("midi_msg note_on default velocity", "[midi_msg]") {
    midi_msg msg;
    msg.note_on(1, 60);
    REQUIRE(msg.get_velocity() == 80);
}

TEST_CASE("midi_msg note_off default velocity", "[midi_msg]") {
    midi_msg msg;
    msg.note_off(1, 60);
    REQUIRE(msg.get_velocity() == 80);
}
