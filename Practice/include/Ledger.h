#pragma once
#include "SQLData.h"  // Include the header for SQLData class

// Class Ledger represents a ledger that manages user, seed list, coin data, and SQL interactions
class Ledger
{
private:
    User& user;           // Reference to the User object, representing the current user
    SQLData& sqlData;     // Reference to SQLData object for database operations
    SeedList& seedList;   // Reference to SeedList object for managing seed phrases
    Coin& coin;           // Reference to Coin object for handling coin data
    std::string dbName = "MyLedgerData.db";  // Database name (SQLite file)
    bool userLoad;        // Flag to check if the user has been successfully loaded

public:
    // Constructor that initializes the Ledger class with references to the User, SQLData, SeedList, and Coin objects
    Ledger(User& other, SQLData& sqlD, SeedList& sL, Coin& c);

    // Method to verify the password entered by the user
    bool enterPassword(const char* password);

    // Method to check if the user’s password is correct (returns true if correct)
    bool checkUserPassword();

    // Method to enter user credentials and seed size via ImGui
    bool enterUser_ImGui(const char* password, int seedSize);

    // Method to search for a password in the SQL table and return true if found
    bool findPasswordInSQLTable(const char* password);

    // Method to search for a seed phrase in the SQL table and return true if found
    bool findSeedPhraseInSQLTable(const char* seed);

    // Method to return the seed phrase as a string
    std::string getSeedPhrase();
};
