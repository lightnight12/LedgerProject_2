#pragma once
#include "Coin.h"
#include <vector>
#include <random>

// Seed structure representing each word in the seed phrase
struct Seed
{
    std::string data;  // The seed word
    Seed* next;        // Pointer to the next seed in the list
    Seed* prev;        // Pointer to the previous seed in the list

    // Constructor to initialize a seed with a value and set next and prev to nullptr
    Seed(std::string value) : data(std::move(value)), next(nullptr), prev(nullptr) {}
};

// SeedList class represents a list of Seed objects (seed words)
class SeedList
{
private:
    Seed* head;          // Pointer to the first Seed in the list
    Seed* tail;          // Pointer to the last Seed in the list
    size_t size;         // Number of elements in the list
    size_t targetSize;   // Target size for the seed list (for generating seed phrase)

    // Random number generator and distribution for generating random seed words
    std::mt19937 rd;
    std::uniform_int_distribution<> device;

    // List of all possible seed words
    std::vector<std::string> allSeedWords =
    {
        "abandon", "ability", "absorb",
        "baby", "balance", "basket",
        "cable", "camera", "cannon",
        "damage", "dancer", "december",
        "eager", "early", "echo",
        "fabric", "face", "fancy",
        "galaxy", "garage", "gather",
        "habit", "hammer", "harmony",
        "ice", "idea", "impact",
        "jacket", "january", "jazz",
        "kangaroo", "keen", "kidney",
        "label", "ladder", "language",
        "machine", "magic", "mango",
        "naive", "name", "nation",
        "oak", "object", "ocean",
        "package", "paddle", "palace",
        "quality", "quantum", "quarter",
        "rabbit", "raccoon", "random",
        "saddle", "salad", "sample",
        "table", "tackle", "talent",
        "umbrella", "unable", "uniform",
        "vacuum", "valley", "value",
        "wagon", "wait", "wander",
        "xenon", "xerox", "xray",
        "yard", "year", "yellow",
        "zebra", "zero", "zone"
    };

    // Predefined sizes for different lists (not used in the constructor yet)
    const size_t sizeList1;
    const size_t sizeList2;
    const size_t sizeList3;

public:
    // Constructor that takes the target size for the seed list
    SeedList(size_t s);

    // Destructor to free the dynamically allocated memory for the seed list
    ~SeedList();

    // Method to generate a seed phrase with a specific number of words
    bool generateSeedPhrase(int size);

    // Method to add a new seed word to the end of the list
    void push_back(const std::string& value);

    // Method to print the seed words in the list
    void print();

    // Method to clear the seed list (remove all elements)
    void clear();

    // Method to get the seed phrase as a vector of strings
    std::vector<std::string> getSeedPhrase() const;

    // Method to get the seed phrase as a single string
    std::string getStringSeedPhrase();
};
