#pragma once
#include <string>
#include <iostream>

// Struct representing a Coin with a name and a price
struct Coin
{
    std::string coinName; // The name of the coin (e.g., "Bitcoin", "Ethereum")
    float prise;          // The price of the coin (e.g., 63250.0 for Bitcoin)

    // Default constructor
    Coin();

    // Parameterized constructor to initialize Coin with a name and price
    Coin(std::string n, float p);
};

