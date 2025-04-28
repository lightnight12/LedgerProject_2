#include "Coin.h"

// Default constructor for Coin
Coin::Coin()
    : coinName(""),   // Initialize coinName as an empty string
    prise(0.0f)      // Initialize prise as 0.0 (default price)
{
}

// Parameterized constructor for Coin
Coin::Coin(std::string n, float p)
    : coinName(n),    // Initialize coinName with the provided name (n)
    prise(p)         // Initialize prise with the provided price (p)
{
}
