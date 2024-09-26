#include "RandomArray.h"

// Function to generate an array of random integers
std::vector<int> generateRandomArray(int size, int seed, int minValue, int maxValue) {
    // Create a random number engine and seed it
    std::mt19937 engine(seed);

    // Create a distribution range
    std::uniform_int_distribution<int> dist(minValue, maxValue);

    // Create a vector to hold the random numbers
    std::vector<int> randomArray(size);

    // Fill the vector with random numbers
    for (int& num : randomArray) {
        num = dist(engine);
    }

    return randomArray;
}