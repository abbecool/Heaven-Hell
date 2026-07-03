#include "TestSupport.hpp"
#include "physics/RandomArray.hpp"

#include <array>

namespace {

using TestSupport::require;

void testDeterministicSeed()
{
    const std::vector<int> first = generateRandomArray(16, 12345, -10, 10);
    const std::vector<int> second = generateRandomArray(16, 12345, -10, 10);
    require(first == second, "matching seeds produced different values");
}

void testSizeAndRange()
{
    const std::vector<int> values = generateRandomArray(100, 42, -3, 7);
    require(values.size() == 100, "requested size was not preserved");

    for (const int value : values) {
        require(value >= -3 && value <= 7, "generated value was outside the requested range");
    }
}

void testZeroSize()
{
    const std::vector<int> values = generateRandomArray(0, 42, 1, 10);
    require(values.empty(), "zero-size request did not return an empty vector");
}

constexpr std::array Tests = {
    TestSupport::TestCase{"deterministic_seed", testDeterministicSeed},
    TestSupport::TestCase{"size_and_range", testSizeAndRange},
    TestSupport::TestCase{"zero_size", testZeroSize}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
