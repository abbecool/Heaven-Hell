#pragma once

#include <array>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace TestSupport {

inline void require(bool condition, std::string_view message)
{
    if (!condition) {
        throw std::runtime_error(std::string(message));
    }
}

struct TestCase {
    std::string_view name;
    void (*run)();
};

template<size_t TestCount>
int runNamedTest(int argc, char* argv[], const std::array<TestCase, TestCount>& tests)
{
    if (argc != 2) {
        std::cerr << "Expected one test name." << std::endl;
        return 2;
    }

    const std::string_view requestedTest = argv[1];
    for (const TestCase& test : tests) {
        if (test.name != requestedTest) {
            continue;
        }

        try {
            test.run();
            return 0;
        } catch (const std::exception& error) {
            std::cerr << test.name << ": " << error.what() << std::endl;
            return 1;
        }
    }

    std::cerr << "Unknown test: " << requestedTest << std::endl;
    return 2;
}

} // namespace TestSupport
