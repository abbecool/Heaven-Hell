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
    if (argc > 2) {
        std::cerr << "Expected zero or one test name." << std::endl;
        return 2;
    }

    const auto runTest = [](const TestCase& test) {
        try {
            test.run();
            return true;
        } catch (const std::exception& error) {
            std::cerr << test.name << ": " << error.what() << std::endl;
            return false;
        }
    };

    if (argc == 1) {
        for (const TestCase& test : tests) {
            if (!runTest(test)) {
                return 1;
            }
        }
        return 0;
    }

    const std::string_view requestedTest = argv[1];
    for (const TestCase& test : tests) {
        if (test.name == requestedTest) {
            return runTest(test) ? 0 : 1;
        }
    }

    std::cerr << "Unknown test: " << requestedTest << std::endl;
    return 2;
}

} // namespace TestSupport
