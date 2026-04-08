#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "manual_tune.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static void fill_pitch(manual_tune& mt, float t0, float t1, float pitch, float conf = 1.0f)
{
    manual_tune::pitch_node n;
    n.pitch = pitch;
    n.conf  = conf;
    mt.set_inpitch(t0, t1, n);
}

// Fill with tiny alternating pitch variations so snap_key's is_same deduplication
// does not skip every entry, allowing idx_end to advance across the full segment.
// Pass add_terminator=true to append a zero-confidence entry that closes the segment.
static void fill_pitch_varied(manual_tune& mt, float t0, float t1, float pitch,
                               float conf = 1.0f, bool add_terminator = false)
{
    const float step = 0.01f;
    int counter = 0;
    for (float t = t0; t + step * 0.5f <= t1; t += step, ++counter)
    {
        manual_tune::pitch_node n;
        n.pitch = pitch + (counter % 2 == 0 ? 0.0001f : -0.0001f);
        n.conf  = conf;
        float end = t + step;
        if (end > t1) end = t1;
        mt.set_inpitch(t, end, n);
    }
    if (add_terminator)
    {
        manual_tune::pitch_node term;
        term.pitch = 0.0f;
        term.conf  = 0.0f;
        mt.set_inpitch(t1, t1 + 0.01f, term);
    }
}

static std::shared_ptr<manual_tune::tune_node> make_node(float t0, float t1, float p,
                                                          bool is_manual = true)
{
    auto n = std::make_shared<manual_tune::tune_node>();
    n->is_manual   = is_manual;
    n->time_start  = t0;
    n->time_end    = t1;
    n->pitch_start = p;
    n->pitch_end   = p;
    n->amount      = 1.0f;
    return n;
}

static const int32_t ALL_NOTES[12] = {1,1,1,1,1,1,1,1,1,1,1,1};

// ---------------------------------------------------------------------------
// inpitch storage
// ---------------------------------------------------------------------------

TEST_CASE("set/get_inpitch round-trip", "[manual_tune][inpitch]") {
    manual_tune mt;
    fill_pitch(mt, 0.0f, 1.0f, 5.0f);
    auto node = mt.get_inpitch(0.5f);
    REQUIRE(node.pitch == Catch::Approx(5.0f));
    REQUIRE(node.conf  == Catch::Approx(1.0f));
}

TEST_CASE("get_inpitch returns zero node outside written range", "[manual_tune][inpitch]") {
    manual_tune mt;
    auto node = mt.get_inpitch(100.0f);
    REQUIRE(node.pitch == Catch::Approx(0.0f));
    REQUIRE(node.conf  == Catch::Approx(0.0f));
}

TEST_CASE("get_inpitch range returns at least one entry per segment", "[manual_tune][inpitch]") {
    manual_tune mt;
    fill_pitch(mt, 0.0f, 1.0f, 3.0f);
    fill_pitch(mt, 1.0f, 2.0f, 7.0f);
    auto list = mt.get_inpitch(0.0f, 2.0f);
    REQUIRE(list.size() >= 2u);
}

TEST_CASE("clear_inpitch zeroes all stored pitch data", "[manual_tune][inpitch]") {
    manual_tune mt;
    fill_pitch(mt, 0.0f, 2.0f, 7.0f);
    mt.clear_inpitch();
    REQUIRE(mt.get_inpitch(1.0f).pitch == Catch::Approx(0.0f));
}

// ---------------------------------------------------------------------------
// tune_node add / get
// ---------------------------------------------------------------------------

TEST_CASE("add_tune stores node retrievable by time", "[manual_tune][tune]") {
    manual_tune mt;
    auto n = make_node(1.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    auto r = mt.get_tune(1.5f);
    REQUIRE(r != nullptr);
    REQUIRE(r->pitch_start == Catch::Approx(5.0f));
}

TEST_CASE("add_tune rejects node shorter than min_time (10 ms)", "[manual_tune][tune]") {
    manual_tune mt;
    auto n = make_node(1.0f, 1.005f, 5.0f);   // 5 ms
    mt.add_tune(n);
    REQUIRE(mt.get_tune(1.003f) == nullptr);
}

TEST_CASE("get_tune returns null before and after a node", "[manual_tune][tune]") {
    manual_tune mt;
    auto n = make_node(1.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    REQUIRE(mt.get_tune(0.5f) == nullptr);
    REQUIRE(mt.get_tune(2.5f) == nullptr);
}

TEST_CASE("add_tune two non-overlapping nodes both retrievable", "[manual_tune][tune]") {
    manual_tune mt;
    auto n1 = make_node(0.0f, 1.0f, 2.0f);
    auto n2 = make_node(2.0f, 3.0f, 7.0f);
    mt.add_tune(n1);
    mt.add_tune(n2);
    REQUIRE(mt.get_tune(0.5f)->pitch_start == Catch::Approx(2.0f));
    REQUIRE(mt.get_tune(2.5f)->pitch_start == Catch::Approx(7.0f));
}

TEST_CASE("add_tune overlapping node replaces the overlapped region", "[manual_tune][tune]") {
    manual_tune mt;
    auto n1 = make_node(0.0f, 4.0f, 1.0f);
    auto n2 = make_node(1.0f, 3.0f, 9.0f);
    mt.add_tune(n1);
    mt.add_tune(n2);
    REQUIRE(mt.get_tune(2.0f)->pitch_start == Catch::Approx(9.0f));
}

TEST_CASE("get_tune range returns all nodes in window", "[manual_tune][tune]") {
    manual_tune mt;
    auto n1 = make_node(0.0f, 1.0f, 0.0f);
    auto n2 = make_node(2.0f, 3.0f, 2.0f);
    auto n3 = make_node(4.0f, 5.0f, 4.0f);
    mt.add_tune(n1);
    mt.add_tune(n2);
    mt.add_tune(n3);
    REQUIRE(mt.get_tune(0.0f, 6.0f).size() == 3u);
}

TEST_CASE("clear_note removes all nodes", "[manual_tune][tune]") {
    manual_tune mt;
    auto n1 = make_node(0.0f, 1.0f, 0.0f);
    auto n2 = make_node(2.0f, 3.0f, 2.0f);
    mt.add_tune(n1);
    mt.add_tune(n2);
    mt.clear_note();
    REQUIRE(mt.get_tune(0.5f) == nullptr);
    REQUIRE(mt.get_tune(2.5f) == nullptr);
}

TEST_CASE("clear_auto_note removes only auto nodes, keeps manual ones", "[manual_tune][tune]") {
    manual_tune mt;
    auto nm = make_node(0.0f, 1.0f, 0.0f, /*is_manual=*/true);
    auto na = make_node(2.0f, 3.0f, 2.0f, /*is_manual=*/false);
    mt.add_tune(nm);
    mt.add_tune(na);
    mt.clear_auto_note();
    REQUIRE(mt.get_tune(0.5f) != nullptr);   // manual — kept
    REQUIRE(mt.get_tune(2.5f) == nullptr);   // auto — removed
}

// ---------------------------------------------------------------------------
// select_tune — hit-testing (blob-style ±0.5 semitone tolerance)
// ---------------------------------------------------------------------------

TEST_CASE("select_tune hits node when pitch within 0.5 semitones", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    REQUIRE(mt.select_tune(1.0f, 5.4f, pos) != nullptr);
}

TEST_CASE("select_tune misses when pitch beyond 0.5 semitones", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    REQUIRE(mt.select_tune(1.0f, 6.0f, pos) == nullptr);
}

TEST_CASE("select_tune exact pitch hit", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 3.0f);
    mt.add_tune(n);
    uint32_t pos;
    REQUIRE(mt.select_tune(1.0f, 3.0f, pos) != nullptr);
}

TEST_CASE("select_tune returns SELECT_LEFT near start of node", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    mt.select_tune(0.1f, 5.0f, pos);
    REQUIRE(pos == manual_tune::SELECT_LEFT);
}

TEST_CASE("select_tune returns SELECT_RIGHT near end of node", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    mt.select_tune(1.9f, 5.0f, pos);
    REQUIRE(pos == manual_tune::SELECT_RIGHT);
}

TEST_CASE("select_tune returns SELECT_MID in the middle of node", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    mt.select_tune(1.0f, 5.0f, pos);
    REQUIRE(pos == manual_tune::SELECT_MID);
}

TEST_CASE("select_tune returns null outside time range", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(1.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    REQUIRE(mt.select_tune(3.0f, 5.0f, pos) == nullptr);
}

// ---------------------------------------------------------------------------
// snap_key
// ---------------------------------------------------------------------------

TEST_CASE("snap_key creates a node from a stable pitch segment", "[manual_tune][snap]") {
    manual_tune mt;
    fill_pitch_varied(mt, 0.0f, 2.0f, 5.0f, 1.0f, /*add_terminator=*/true);
    mt.snap_key(ALL_NOTES, 0.1f, 0.5f, 0.05f, 0.05f, 1.0f);
    REQUIRE(mt.get_tune(1.0f) != nullptr);
}

TEST_CASE("snap_key node pitch is snapped to nearest semitone", "[manual_tune][snap]") {
    manual_tune mt;
    fill_pitch_varied(mt, 0.0f, 2.0f, 5.4f, 1.0f, /*add_terminator=*/true);
    mt.snap_key(ALL_NOTES, 0.1f, 0.5f, 0.05f, 0.05f, 1.0f);
    auto r = mt.get_tune(1.0f);
    REQUIRE(r != nullptr);
    REQUIRE(r->pitch_start == Catch::Approx(5.0f).margin(0.5f));
}

TEST_CASE("snap_key ignores pitch below confidence threshold", "[manual_tune][snap]") {
    manual_tune mt;
    fill_pitch_varied(mt, 0.0f, 2.0f, 5.0f, 0.3f, /*add_terminator=*/true);  // conf < 0.7
    mt.snap_key(ALL_NOTES, 0.1f, 0.5f, 0.05f, 0.05f, 1.0f);
    REQUIRE(mt.get_tune(1.0f) == nullptr);
}

TEST_CASE("snap_key ignores segments shorter than min_len", "[manual_tune][snap]") {
    manual_tune mt;
    fill_pitch_varied(mt, 0.0f, 0.05f, 5.0f, 1.0f, /*add_terminator=*/true);  // 50 ms < 0.1 s
    mt.snap_key(ALL_NOTES, 0.1f, 0.5f, 0.05f, 0.05f, 1.0f);
    REQUIRE(mt.get_tune(0.025f) == nullptr);
}

TEST_CASE("snap_key creates separate nodes for distinct pitches", "[manual_tune][snap]") {
    manual_tune mt;
    fill_pitch_varied(mt, 0.0f, 1.0f, 2.0f, 1.0f, /*add_terminator=*/true);
    fill_pitch_varied(mt, 1.5f, 2.5f, 7.0f, 1.0f, /*add_terminator=*/true);
    mt.snap_key(ALL_NOTES, 0.1f, 0.3f, 0.05f, 0.05f, 1.0f);
    auto r1 = mt.get_tune(0.5f);
    auto r2 = mt.get_tune(2.0f);
    REQUIRE(r1 != nullptr);
    REQUIRE(r2 != nullptr);
    REQUIRE(r1 != r2);
}

// ---------------------------------------------------------------------------
// undo / redo
// ---------------------------------------------------------------------------

TEST_CASE("undo after add_tune removes the added node", "[manual_tune][history]") {
    manual_tune mt;
    auto n1 = make_node(0.0f, 1.0f, 3.0f);
    auto n2 = make_node(2.0f, 3.0f, 7.0f);
    mt.add_tune(n1);
    mt.add_tune(n2);
    mt.undo();
    REQUIRE(mt.get_tune(2.5f) == nullptr);
    REQUIRE(mt.get_tune(0.5f) != nullptr);
}

TEST_CASE("redo re-applies the undone add_tune", "[manual_tune][history]") {
    manual_tune mt;
    auto n1 = make_node(0.0f, 1.0f, 3.0f);
    auto n2 = make_node(2.0f, 3.0f, 7.0f);
    mt.add_tune(n1);
    mt.add_tune(n2);
    mt.undo();
    mt.redo();
    REQUIRE(mt.get_tune(2.5f) != nullptr);
}

TEST_CASE("undo after clear_note restores all nodes", "[manual_tune][history]") {
    manual_tune mt;
    auto n1 = make_node(0.0f, 1.0f, 3.0f);
    auto n2 = make_node(2.0f, 3.0f, 7.0f);
    mt.add_tune(n1);
    mt.add_tune(n2);
    mt.clear_note();
    mt.undo();
    REQUIRE(mt.get_tune(0.5f) != nullptr);
    REQUIRE(mt.get_tune(2.5f) != nullptr);
}

TEST_CASE("undo with history disabled is a no-op", "[manual_tune][history]") {
    manual_tune mt;
    auto n1 = make_node(0.0f, 1.0f, 3.0f);
    mt.add_tune(n1);
    mt.disable_history();
    auto n2 = make_node(2.0f, 3.0f, 7.0f);
    mt.add_tune(n2);
    mt.undo();
    REQUIRE(mt.get_tune(2.5f) != nullptr);   // undo disabled — node still there
}

// ---------------------------------------------------------------------------
// select / unselect / delete
// ---------------------------------------------------------------------------

TEST_CASE("del_selected removes the selected node", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    mt.select_tune(1.0f, 5.0f, pos);  // selects and temporarily erases from list
    mt.del_selected();                  // discards it
    REQUIRE(mt.get_tune(1.0f) == nullptr);
}

TEST_CASE("unselect_tune writes the node back after selection", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    mt.select_tune(1.0f, 5.0f, pos);
    mt.unselect_tune();                 // commits back
    REQUIRE(mt.get_tune(1.0f) != nullptr);
}

TEST_CASE("unselect_tune clears selection — double unselect is safe", "[manual_tune][select]") {
    manual_tune mt;
    auto n = make_node(0.0f, 2.0f, 5.0f);
    mt.add_tune(n);
    uint32_t pos;
    mt.select_tune(1.0f, 5.0f, pos);
    mt.unselect_tune();
    mt.unselect_tune();  // second call must not crash or corrupt
    REQUIRE(mt.get_tune(1.0f) != nullptr);
}

// ---------------------------------------------------------------------------
// set_vthresh
// ---------------------------------------------------------------------------

TEST_CASE("set_vthresh lower than default allows low-confidence pitch into snap_key", "[manual_tune][snap]") {
    manual_tune mt;
    mt.set_vthresh(0.1f);  // well below the 0.3f confidence we'll use
    fill_pitch_varied(mt, 0.0f, 2.0f, 5.0f, 0.3f, /*add_terminator=*/true);
    mt.snap_key(ALL_NOTES, 0.1f, 0.5f, 0.05f, 0.05f, 1.0f);
    REQUIRE(mt.get_tune(1.0f) != nullptr);
}

TEST_CASE("set_vthresh higher than confidence rejects the pitch", "[manual_tune][snap]") {
    manual_tune mt;
    mt.set_vthresh(0.9f);  // above the 0.8f confidence we'll use
    fill_pitch_varied(mt, 0.0f, 2.0f, 5.0f, 0.8f, /*add_terminator=*/true);
    mt.snap_key(ALL_NOTES, 0.1f, 0.5f, 0.05f, 0.05f, 1.0f);
    REQUIRE(mt.get_tune(1.0f) == nullptr);
}

// ---------------------------------------------------------------------------
// check_key
// ---------------------------------------------------------------------------

TEST_CASE("check_key returns false when no pitch data is stored", "[manual_tune][check_key]") {
    manual_tune mt;
    float weights[12] = {};
    REQUIRE(mt.check_key(weights, 0.1f, 0.5f) == false);
}

TEST_CASE("check_key returns true and populates weights from pitch data", "[manual_tune][check_key]") {
    manual_tune mt;
    // Two distinct semitone pitches — C (0) and D (2) — separated so check_key sees a pitch change
    fill_pitch_varied(mt, 0.0f,  1.0f, 0.0f);   // C
    fill_pitch_varied(mt, 1.01f, 2.0f, 2.0f);   // D
    fill_pitch_varied(mt, 2.01f, 2.1f, 4.0f);   // E (closes the D segment)
    float weights[12] = {};
    REQUIRE(mt.check_key(weights, 0.1f, 0.5f) == true);
    // C (semitone 0 mod 12) and D (semitone 2 mod 12) must have non-zero weight
    REQUIRE(weights[0] > 0.0f);
    REQUIRE(weights[2] > 0.0f);
    // Weights must sum to 1
    float sum = 0.0f;
    for (int i = 0; i < 12; ++i) sum += weights[i];
    REQUIRE(sum == Catch::Approx(1.0f).margin(0.001f));
}
