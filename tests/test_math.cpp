#include <catch2/catch_all.hpp>

#include "physics/Vec2.h"
#include <cmath>

using namespace Catch;

TEST_CASE("Vec2 constructors", "[Vec2]") {
    SECTION("Default constructor") {
        Vec2 v;
        REQUIRE(v.x == Approx(0.0f));
        REQUIRE(v.y == Approx(0.0f));
    }

    SECTION("Float constructor") {
        Vec2 v(3.5f, 4.5f);
        REQUIRE(v.x == Approx(3.5f));
        REQUIRE(v.y == Approx(4.5f));
    }

    SECTION("Integer constructor") {
        Vec2 v(3, 4);
        REQUIRE(v.x == Approx(3.0f));
        REQUIRE(v.y == Approx(4.0f));
    }

    SECTION("Mixed type constructor") {
        Vec2 v1(3, 4.5f);
        Vec2 v2(3.5f, 4);
        REQUIRE(v1.x == Approx(3.0f));
        REQUIRE(v1.y == Approx(4.5f));
        REQUIRE(v2.x == Approx(3.5f));
        REQUIRE(v2.y == Approx(4.0f));
    }
}

TEST_CASE("Vec2 arithmetic operations", "[Vec2]") {
    Vec2 a(2.0f, 3.0f);
    Vec2 b(4.0f, 5.0f);

    SECTION("Addition") {
        Vec2 result = a + b;
        REQUIRE(result.x == Approx(6.0f));
        REQUIRE(result.y == Approx(8.0f));
    }

    SECTION("Subtraction") {
        Vec2 result = a - b;
        REQUIRE(result.x == Approx(-2.0f));
        REQUIRE(result.y == Approx(-2.0f));
    }

    SECTION("Element-wise multiplication") {
        Vec2 result = a * b;
        REQUIRE(result.x == Approx(8.0f));
        REQUIRE(result.y == Approx(15.0f));
    }

    SECTION("Scalar multiplication") {
        Vec2 result = a * 2.0f;
        REQUIRE(result.x == Approx(4.0f));
        REQUIRE(result.y == Approx(6.0f));
    }

    SECTION("Scalar division") {
        Vec2 result = a / 2.0f;
        REQUIRE(result.x == Approx(1.0f));
        REQUIRE(result.y == Approx(1.5f));
    }

    SECTION("Element-wise division") {
        Vec2 result = b / a;
        REQUIRE(result.x == Approx(2.0f));
        REQUIRE(result.y == Approx(5.0f / 3.0f));
    }
}

TEST_CASE("Vec2 comparison operations", "[Vec2]") {
    Vec2 a(2.0f, 3.0f);
    Vec2 b(2.0f, 3.0f);
    Vec2 c(3.0f, 4.0f);

    SECTION("Equality") {
        REQUIRE(a == b);
        REQUIRE_FALSE(a == c);
    }

    SECTION("Inequality") {
        REQUIRE(a != c);
        REQUIRE_FALSE(a != b);
    }

    SECTION("Greater-than or equal") {
        REQUIRE(c >= a);
        REQUIRE(a >= b);
        REQUIRE_FALSE(a >= c);
    }
}

TEST_CASE("Vec2 geometric operations", "[Vec2]") {
    Vec2 a(3.0f, 4.0f);
    Vec2 b(0.0f, 0.0f);

    SECTION("Distance calculation") {
        Vec2 origin(0.0f, 0.0f);
        float distance = a.dist(origin);
        REQUIRE(distance == Approx(5.0f)); // 3-4-5 triangle
    }

    SECTION("Distance between two points") {
        Vec2 p1(1.0f, 1.0f);
        Vec2 p2(4.0f, 5.0f);
        float distance = p1.dist(p2);
        REQUIRE(distance == Approx(5.0f)); // sqrt((4-1)^2 + (5-1)^2) = sqrt(9+16) = 5
    }

    SECTION("Length calculation") {
        float length = a.length();
        REQUIRE(length == Approx(5.0f));
    }

    SECTION("Null vector") {
        REQUIRE(b.isNull());
        REQUIRE_FALSE(a.isNull());
    }

    SECTION("Absolute elements") {
        Vec2 v(-3.0f, -4.0f);
        Vec2 result = v.abs_elem();
        REQUIRE(result.x == Approx(3.0f));
        REQUIRE(result.y == Approx(4.0f));
    }

    SECTION("Normalization") {
        Vec2 normalized = a.norm();
        float expectedLen = a.length();
        REQUIRE(normalized.x == Approx(3.0f / expectedLen));
        REQUIRE(normalized.y == Approx(4.0f / expectedLen));
    }

    SECTION("Normalization to specific length") {
        Vec2 normalized = a.norm(10.0f);
        REQUIRE(normalized.length() == Approx(10.0f));
    }
}

TEST_CASE("Vec2 modulo operations", "[Vec2]") {
    Vec2 a(7.0f, 8.0f);

    SECTION("Scalar modulo") {
        Vec2 result = a % 3;
        REQUIRE(result.x == Approx(1.0f));
        REQUIRE(result.y == Approx(2.0f));
    }

    SECTION("Vector modulo") {
        Vec2 b(3.0f, 3.0f);
        Vec2 result = a % b;
        REQUIRE(result.x == Approx(1.0f));
        REQUIRE(result.y == Approx(2.0f));
    }
}

TEST_CASE("Vec2 compound assignment operators", "[Vec2]") {
    SECTION("Plus equals") {
        Vec2 a(1.0f, 2.0f);
        Vec2 b(3.0f, 4.0f);
        a += b;
        REQUIRE(a.x == Approx(4.0f));
        REQUIRE(a.y == Approx(6.0f));
    }

    SECTION("Minus equals") {
        Vec2 a(5.0f, 7.0f);
        Vec2 b(2.0f, 3.0f);
        a -= b;
        REQUIRE(a.x == Approx(3.0f));
        REQUIRE(a.y == Approx(4.0f));
    }

    SECTION("Multiply equals") {
        Vec2 a(2.0f, 3.0f);
        a *= 2.0f;
        REQUIRE(a.x == Approx(4.0f));
        REQUIRE(a.y == Approx(6.0f));
    }

    SECTION("Divide equals") {
        Vec2 a(4.0f, 6.0f);
        a /= 2.0f;
        REQUIRE(a.x == Approx(2.0f));
        REQUIRE(a.y == Approx(3.0f));
    }
}

TEST_CASE("Vec2 sign and direction checks", "[Vec2]") {
    SECTION("Positive vector") {
        Vec2 v(2.0f, 3.0f);
        REQUIRE(v.isPositive());
        REQUIRE_FALSE(v.isNegative());
        REQUIRE(v.hasPositive());
    }

    SECTION("Negative vector") {
        Vec2 v(-2.0f, -3.0f);
        REQUIRE(v.isNegative());
        REQUIRE_FALSE(v.isPositive());
        REQUIRE(v.hasNegative());
    }

    SECTION("Mixed sign vector") {
        Vec2 v(2.0f, -3.0f);
        REQUIRE(v.hasPositive());
        REQUIRE(v.hasNegative());
        REQUIRE_FALSE(v.isPositive());
        REQUIRE_FALSE(v.isNegative());
    }
}

TEST_CASE("Vec2 comparisons for sorting", "[Vec2]") {
    Vec2 a(2.0f, 3.0f);
    Vec2 b(3.0f, 4.0f);
    Vec2 c(1.0f, 2.0f);

    SECTION("Smaller comparison") {
        REQUIRE(c.smaller(a));
        REQUIRE(a.smaller(b));
        REQUIRE_FALSE(b.smaller(a));
    }

    SECTION("Greater comparison") {
        REQUIRE(b.greater(a));
        REQUIRE(a.greater(c));
        REQUIRE_FALSE(c.greater(a));
    }
}

TEST_CASE("Vec2 edge cases", "[Vec2]") {
    SECTION("Zero vector operations") {
        Vec2 zero(0.0f, 0.0f);
        Vec2 other(3.0f, 4.0f);
        
        REQUIRE(zero.length() == Approx(0.0f));
        REQUIRE(zero.isNull());
        REQUIRE((zero + other) == other);
    }

    SECTION("Very small values") {
        Vec2 tiny(0.0001f, 0.0001f);
        REQUIRE(tiny.length() > 0.0f);
    }

    SECTION("Large values") {
        Vec2 large(1e6f, 1e6f);
        Vec2 result = large * 2.0f;
        REQUIRE(result.x == Approx(2e6f));
        REQUIRE(result.y == Approx(2e6f));
    }
}