#pragma once
#include "Seed.h"
#include "list"
#include "map"

// Structure representing available currencies
struct Currency
{
    std::list<std::string> currentCurrency;  // List of supported currencies

    // Constructor that initializes the list of supported currencies
    Currency()
    {
        currentCurrency.clear(); // Clear the list (in case it's already populated)

        // Add various currencies to the list
        currentCurrency.push_back("USD");  // US Dollar
        currentCurrency.push_back("EUR");  // Euro
        currentCurrency.push_back("GBP");  // British Pound
        currentCurrency.push_back("JPY");  // Japanese Yen
        currentCurrency.push_back("UAH");  // Ukrainian Hryvnia
        currentCurrency.push_back("CAD");  // Canadian Dollar
        currentCurrency.push_back("AUD");  // Australian Dollar
        currentCurrency.push_back("CHF");  // Swiss Franc
        currentCurrency.push_back("CNY");  // Chinese Yuan
        currentCurrency.push_back("INR");  // Indian Rupee
    }
};

// Structure representing a user
struct User
{
    std::string userID;  // User ID (generated randomly)
    std::string password;  // User's password
    std::mt19937 rd;  // Random number generator engine
    std::uniform_int_distribution<> device;  // Uniform distribution for randomness
    std::vector<std::string> seedPhrase;  // A seed phrase (not yet implemented)

    // List of all possible characters that can be part of the user ID or password
    std::vector<std::string> allCharSymbols =
    {
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
        "k", "l", "m", "n", "o", "p", "q", "r", "s", "t",
        "u", "v", "w", "x", "y", "z",
        "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
        "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T",
        "U", "V", "W", "X", "Y", "Z",
        "!", "@", "#", "$", "%", "^", "&", "*", "(", ")",
        "_", "+", "=", "{", "}", "[", "]", "|", "\\", ":",
        ";", "\"", "'", "<", ">", ",", ".", "?", "/"
    };

    // Method to generate a random user ID by shuffling characters
    std::string generateUserID()
    {
        std::string allCharSymbolsString = "";

        // Concatenate all characters into a single string
        for (const auto& symbol : allCharSymbols)
        {
            allCharSymbolsString += symbol;
        }

        // Shuffle the characters to create randomness
        std::shuffle(allCharSymbolsString.begin(), allCharSymbolsString.end(), rd);

        // Return a random user ID with 10 characters
        std::string userID = allCharSymbolsString.substr(0, 10);

        return userID;
    }

    // Constructor initializing the random number generator
    User()
        : rd(std::random_device{}())  // Initialize random device
    {
    }

};
