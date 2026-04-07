#include <catch2/catch_test_macros.hpp>
#include "undo_redo.h"

TEST_CASE("undo_redo empty undo returns false", "[undo_redo]") {
    undo_redo<int> ur;
    int result = 0;
    REQUIRE_FALSE(ur.undo(result, 0));
}

TEST_CASE("undo_redo empty redo returns false", "[undo_redo]") {
    undo_redo<int> ur;
    int result = 0;
    REQUIRE_FALSE(ur.redo(result, 0));
}

TEST_CASE("undo_redo basic undo restores last put", "[undo_redo]") {
    undo_redo<int> ur;
    ur.put(10);
    ur.put(20);
    int result = 0;
    REQUIRE(ur.undo(result, 30));
    REQUIRE(result == 20);
}

TEST_CASE("undo_redo undo then redo", "[undo_redo]") {
    undo_redo<int> ur;
    ur.put(10);
    ur.put(20);
    int result = 0;
    ur.undo(result, 30); // result=20, redo=[30]
    REQUIRE(ur.redo(result, 20));
    REQUIRE(result == 30);
}

TEST_CASE("undo_redo put clears redo stack", "[undo_redo]") {
    undo_redo<int> ur;
    ur.put(10);
    ur.put(20);
    int result = 0;
    ur.undo(result, 30);  // redo=[30]
    ur.put(40);           // should clear redo
    REQUIRE_FALSE(ur.redo(result, 40));
}

TEST_CASE("undo_redo multiple sequential undos", "[undo_redo]") {
    undo_redo<int> ur;
    ur.put(10);
    ur.put(20);
    ur.put(30);
    int result = 0;
    ur.undo(result, 40); REQUIRE(result == 30);
    ur.undo(result, 30); REQUIRE(result == 20);
    ur.undo(result, 20); REQUIRE(result == 10);
    REQUIRE_FALSE(ur.undo(result, 10));
}

TEST_CASE("undo_redo respects max size by evicting oldest", "[undo_redo]") {
    undo_redo<int> ur(3);
    ur.put(1);
    ur.put(2);
    ur.put(3);
    ur.put(4); // evicts 1; stack is [2, 3, 4]
    int result = 0;
    ur.undo(result, 5); REQUIRE(result == 4);
    ur.undo(result, 4); REQUIRE(result == 3);
    ur.undo(result, 3); REQUIRE(result == 2);
    REQUIRE_FALSE(ur.undo(result, 2)); // 1 was evicted
}

TEST_CASE("undo_redo redo restores current state to undo stack", "[undo_redo]") {
    undo_redo<int> ur;
    ur.put(10);
    int result = 0;
    ur.undo(result, 20);  // result=10, undo=[], redo=[20]
    ur.redo(result, 10);  // result=20, undo=[10], redo=[]
    REQUIRE(ur.undo(result, 20));
    REQUIRE(result == 10);
}
