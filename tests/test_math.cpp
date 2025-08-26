#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

int add(int a, int b) {
    return a + b;
}

TEST_CASE("Addition works", "[math]") {
    REQUIRE(add(2, 2) == 4);
    REQUIRE(add(-1, 1) == 0);
}
