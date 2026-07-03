// CTest invokes this runner once per named Vec2 operation.
#include "TestSupport.hpp"
#include "physics/Vec2.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string_view>

namespace {

using TestSupport::require;

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001f)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

void requireVec2(const Vec2& value, float x, float y)
{
    require(nearlyEqual(value.x, x), "unexpected x value");
    require(nearlyEqual(value.y, y), "unexpected y value");
}

void testDefaultConstructor()
{
    const Vec2 value;
    requireVec2(value, 0.0f, 0.0f);
}

void testFloatConstructor()
{
    requireVec2(Vec2{1.5f, -2.5f}, 1.5f, -2.5f);
}

void testIntegerConstructor()
{
    requireVec2(Vec2{1, -2}, 1.0f, -2.0f);
}

void testMixedConstructor()
{
    requireVec2(Vec2{1, 2.5f}, 1.0f, 2.5f);
    requireVec2(Vec2{1.5f, -2}, 1.5f, -2.0f);
}

void testJsonConstructor()
{
    const json value = {{"x", 7.5f}, {"y", -4.25f}};
    requireVec2(Vec2{value}, 7.5f, -4.25f);
}

void testJsonScalarConstructor()
{
    requireVec2(Vec2{json(7.5f), json(-4.25f)}, 7.5f, -4.25f);
}

void testEquality()
{
    require(Vec2{1.0f, 2.0f} == Vec2{1.0f, 2.0f}, "equal vectors compare unequal");
}

void testGreaterEqual()
{
    require(Vec2{3.0f, 4.0f} >= Vec2{1.0f, 4.0f}, "greater-equal comparison failed");
    require(!(Vec2{1.0f, 2.0f} >= Vec2{2.0f, 1.0f}), "greater-equal comparison ignored x");
}

void testInequality()
{
    require(Vec2{1.0f, 2.0f} != Vec2{1.0f, 3.0f}, "different vectors compare equal");
}

void testAddition()
{
    requireVec2(Vec2{1.0f, 2.0f} + Vec2{3.0f, 4.0f}, 4.0f, 6.0f);
}

void testSubtraction()
{
    requireVec2(Vec2{3.0f, 4.0f} - Vec2{1.0f, 2.0f}, 2.0f, 2.0f);
}

void testVectorMultiplication()
{
    requireVec2(Vec2{2.0f, 3.0f} * Vec2{4.0f, 5.0f}, 8.0f, 15.0f);
}

void testScalarMultiplication()
{
    requireVec2(Vec2{2.0f, 3.0f} * 2.5f, 5.0f, 7.5f);
}

void testVectorDivision()
{
    requireVec2(Vec2{8.0f, 15.0f} / Vec2{2.0f, 5.0f}, 4.0f, 3.0f);
}

void testScalarDivision()
{
    requireVec2(Vec2{8.0f, 15.0f} / 2.0f, 4.0f, 7.5f);
}

void testVectorModulo()
{
    requireVec2(Vec2{9.5f, 8.0f} % Vec2{4.0f, 3.0f}, 1.5f, 2.0f);
}

void testScalarModulo()
{
    requireVec2(Vec2{9.5f, 8.0f} % 4, 1.5f, 0.0f);
}

void testIncrement()
{
    Vec2 value{1.0f, 2.0f};
    ++value;
    requireVec2(value, 2.0f, 3.0f);
}

void testDecrement()
{
    Vec2 value{1.0f, 2.0f};
    --value;
    requireVec2(value, 0.0f, 1.0f);
}

void testAddAssign()
{
    Vec2 value{1.0f, 2.0f};
    value += Vec2{3.0f, 4.0f};
    requireVec2(value, 4.0f, 6.0f);
}

void testSubtractAssign()
{
    Vec2 value{4.0f, 6.0f};
    value -= Vec2{3.0f, 4.0f};
    requireVec2(value, 1.0f, 2.0f);
}

void testMultiplyAssign()
{
    Vec2 value{2.0f, 3.0f};
    value *= 2.5f;
    requireVec2(value, 5.0f, 7.5f);
}

void testDivideAssign()
{
    Vec2 value{8.0f, 15.0f};
    value /= 2.0f;
    requireVec2(value, 4.0f, 7.5f);
}

void testDistance()
{
    require(nearlyEqual(Vec2{1.0f, 2.0f}.dist(Vec2{4.0f, 6.0f}), 5.0f), "distance was incorrect");
}

void testIsNull()
{
    require(Vec2{}.isNull(), "zero vector was not null");
    require(!Vec2{0.0f, 0.1f}.isNull(), "non-zero vector was null");
}

void testLength()
{
    require(nearlyEqual(Vec2{3.0f, 4.0f}.length(), 5.0f), "length was incorrect");
}

void testAbsoluteValue()
{
    requireVec2(Vec2{-3.0f, 4.0f}.abs_elem(), 3.0f, 4.0f);
}

void testNormalization()
{
    requireVec2(Vec2{3.0f, 4.0f}.norm(), 0.6f, 0.8f);
}

void testZeroNormalization()
{
    requireVec2(Vec2{}.norm(), 0.0f, 0.0f);
}

void testScaledNormalization()
{
    requireVec2(Vec2{3.0f, 4.0f}.norm(10.0f), 6.0f, 8.0f);
    requireVec2(Vec2{}.norm(10.0f), 0.0f, 0.0f);
}

void testAngle()
{
    require(nearlyEqual(Vec2{0.0f, 1.0f}.angle(), 90.0f), "angle was incorrect");
}

void testMainDirection()
{
    requireVec2(Vec2{5.0f, 2.0f}.mainDir(), 5.0f, 0.0f);
    requireVec2(Vec2{2.0f, -5.0f}.mainDir(), 0.0f, -5.0f);
    requireVec2(Vec2{4.0f, -4.0f}.mainDir(), 4.0f, 0.0f);
}

void testToInteger()
{
    Vec2 value{3.9f, -2.1f};
    requireVec2(value.toInt(), 3.0f, -3.0f);
}

void testSmaller()
{
    Vec2 smaller{1.0f, 2.0f};
    Vec2 larger{2.0f, 3.0f};
    Vec2 narrower{1.0f, 5.0f};
    Vec2 comparison{2.0f, 4.0f};
    require(smaller.smaller(larger), "smaller comparison failed");
    require(!larger.smaller(smaller), "smaller comparison ignored both components");
    require(narrower.smaller(comparison), "smaller comparison ignored one smaller component");
}

void testGreater()
{
    Vec2 smaller{1.0f, 2.0f};
    Vec2 larger{2.0f, 3.0f};
    Vec2 wider{5.0f, 1.0f};
    Vec2 comparison{4.0f, 2.0f};
    require(larger.greater(smaller), "greater comparison failed");
    require(!smaller.greater(larger), "greater comparison ignored both components");
    require(wider.greater(comparison), "greater comparison ignored one greater component");
}

class ScopedCoutRedirect {
    std::streambuf* m_originalBuffer = nullptr;

public:
    explicit ScopedCoutRedirect(std::streambuf* replacement)
        : m_originalBuffer(std::cout.rdbuf(replacement)) {}

    ~ScopedCoutRedirect()
    {
        std::cout.rdbuf(m_originalBuffer);
    }
};

void testPrint()
{
    std::ostringstream output;
    ScopedCoutRedirect redirect(output.rdbuf());
    Vec2{1.5f, -2.25f}.print("position");
    require(output.str() == "position: 1.5, -2.25\n", "print output was incorrect");
}

void testHasPositive()
{
    Vec2 value{1.0f, -2.0f};
    require(value.hasPositive(), "positive component was not detected");
}

void testHasNegative()
{
    Vec2 value{1.0f, -2.0f};
    require(value.hasNegative(), "negative component was not detected");
}

void testIsPositive()
{
    Vec2 value{1.0f, 2.0f};
    Vec2 zeroY{1.0f, 0.0f};
    require(value.isPositive(), "positive vector was not detected");
    require(!zeroY.isPositive(), "zero component counted as positive");
}

void testIsNegative()
{
    Vec2 value{-1.0f, -2.0f};
    Vec2 zeroY{-1.0f, 0.0f};
    require(value.isNegative(), "negative vector was not detected");
    require(!zeroY.isNegative(), "zero component counted as negative");
}

void testFromJson()
{
    const json serialized = {{"x", 7.5f}, {"y", -4.25f}};
    requireVec2(serialized.get<Vec2>(), 7.5f, -4.25f);
}

constexpr std::array Tests = {
    TestSupport::TestCase{"default_constructor", testDefaultConstructor},
    TestSupport::TestCase{"float_constructor", testFloatConstructor},
    TestSupport::TestCase{"integer_constructor", testIntegerConstructor},
    TestSupport::TestCase{"mixed_constructor", testMixedConstructor},
    TestSupport::TestCase{"json_constructor", testJsonConstructor},
    TestSupport::TestCase{"json_scalar_constructor", testJsonScalarConstructor},
    TestSupport::TestCase{"equality", testEquality},
    TestSupport::TestCase{"greater_equal", testGreaterEqual},
    TestSupport::TestCase{"inequality", testInequality},
    TestSupport::TestCase{"addition", testAddition},
    TestSupport::TestCase{"subtraction", testSubtraction},
    TestSupport::TestCase{"vector_multiplication", testVectorMultiplication},
    TestSupport::TestCase{"scalar_multiplication", testScalarMultiplication},
    TestSupport::TestCase{"vector_division", testVectorDivision},
    TestSupport::TestCase{"scalar_division", testScalarDivision},
    TestSupport::TestCase{"vector_modulo", testVectorModulo},
    TestSupport::TestCase{"scalar_modulo", testScalarModulo},
    TestSupport::TestCase{"increment", testIncrement},
    TestSupport::TestCase{"decrement", testDecrement},
    TestSupport::TestCase{"add_assign", testAddAssign},
    TestSupport::TestCase{"subtract_assign", testSubtractAssign},
    TestSupport::TestCase{"multiply_assign", testMultiplyAssign},
    TestSupport::TestCase{"divide_assign", testDivideAssign},
    TestSupport::TestCase{"distance", testDistance},
    TestSupport::TestCase{"is_null", testIsNull},
    TestSupport::TestCase{"length", testLength},
    TestSupport::TestCase{"absolute_value", testAbsoluteValue},
    TestSupport::TestCase{"normalization", testNormalization},
    TestSupport::TestCase{"zero_normalization", testZeroNormalization},
    TestSupport::TestCase{"scaled_normalization", testScaledNormalization},
    TestSupport::TestCase{"angle", testAngle},
    TestSupport::TestCase{"main_direction", testMainDirection},
    TestSupport::TestCase{"to_integer", testToInteger},
    TestSupport::TestCase{"smaller", testSmaller},
    TestSupport::TestCase{"greater", testGreater},
    TestSupport::TestCase{"print", testPrint},
    TestSupport::TestCase{"has_positive", testHasPositive},
    TestSupport::TestCase{"has_negative", testHasNegative},
    TestSupport::TestCase{"is_positive", testIsPositive},
    TestSupport::TestCase{"is_negative", testIsNegative},
    TestSupport::TestCase{"from_json", testFromJson}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
