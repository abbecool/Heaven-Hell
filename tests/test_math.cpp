// #define CATCH_CONFIG_MAIN
// #include <catch2/catch_all.hpp>

// #include "./physics/Vec2.h"

// int add(int a, int b) {
//     return a + b;
// }

// TEST_CASE("Addition works", "[math]") {
//     REQUIRE(add(2, 2) == 4);
//     REQUIRE(add(-1, 1) == 0);
// }

// TEST_CASE("Vec2 basic operations", "[Vec2]") {
//     Vec2 a(1.0f, 2.0f);
//     Vec2 b(3.0f, 4.0f);

//     SECTION("Addition") {
//         Vec2 c = a + b;
//         REQUIRE(c.x == Approx(4.0f));
//         REQUIRE(c.y == Approx(6.0f));
//     }

//     SECTION("Subtraction") {
//         Vec2 d = b - a;
//         REQUIRE(d.x == Approx(2.0f));
//         REQUIRE(d.y == Approx(2.0f));
//     }

//     SECTION("Equality") {
//         Vec2 e(1.0f, 2.0f);
//         REQUIRE(a == e);
//     }

//     SECTION("Assignment") {
//         Vec2 f;
//         f = a;
//         REQUIRE(f.x == Approx(1.0f));
//         REQUIRE(f.y == Approx(2.0f));
//     }
// }