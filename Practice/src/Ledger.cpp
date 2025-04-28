#include "Ledger.h"

// Constructor for the Ledger class, initializing necessary components and setting up the database
Ledger::Ledger(User& other, SQLData& sqlD, SeedList& sL, Coin& c)
    : user(other), sqlData(sqlD), seedList(sL), coin(c), userLoad(false)
{
    // Attempt to open the SQLite database
    if (sqlData.open(dbName))
    {
        // Create tables if they don't already exist
        sqlData.createTable(SQLData::DataBaseState::DBS_DATA);
        sqlData.createTable(SQLData::DataBaseState::DBS_COINS);
        sqlData.createTable(SQLData::DataBaseState::DBS_BALANCE);
    }
    else
    {
        std::cerr << "Failed to open database.\n";  // Error handling for failed database connection
    }
}

// Function to verify if the entered password exists in the database (returns false if the password is found)
bool Ledger::enterPassword(const char* password)
{
    if (!sqlData.findPassword(password).empty())
    {
        return false; // Password is found, return false (as in, it matches)
    }

    return true; // Password is not found in the database, return true
}

// Function to check if the entered password matches the user's stored password
bool Ledger::checkUserPassword()
{
    std::string tempPassword;
    std::cout << "Repeat your password: ";
    std::cin >> tempPassword;

    // Compare the entered password with the user's stored password
    return tempPassword == user.password;
}

// Function to initialize a new user with a generated user ID, password, and seed phrase
bool Ledger::enterUser_ImGui(const char* password, int seedSize)
{
    user.userID = user.generateUserID();  // Generate a random user ID
    user.password = password;             // Assign the entered password
    seedList.generateSeedPhrase(seedSize); // Generate a seed phrase of the specified size

    // Store the generated seed phrase in the user object
    user.seedPhrase = seedList.getSeedPhrase();

    // Insert user data into the database
    sqlData.insertData(user.userID, user.password, seedList.getStringSeedPhrase());

    // Optionally print the seed phrase to the console for verification (can be omitted later for security)
    seedList.print();

    return true; // Indicate that the user was successfully entered
}

// Function to search for the password in the SQL table and return false if not found
bool Ledger::findPasswordInSQLTable(const char* password)
{
    // Check if the password is found in the database (if it's empty, it means not found)
    if (sqlData.findPassword(password) == "")
    {
        return false; // Password not found
    }

    return true; // Password found
}

// Function to search for the seed phrase in the SQL table and return false if not found
bool Ledger::findSeedPhraseInSQLTable(const char* seed)
{
    std::cout << seed << std::endl;

    // Optionally print all seed phrases for debugging (could be omitted later for security reasons)
    sqlData.printAllSeeds();

    // Check if the seed is found in the database
    if (sqlData.findSeed(seed) == "")
    {
        return false; // Seed not found
    }

    return true; // Seed found
}

// Function to return the current seed phrase as a string
std::string Ledger::getSeedPhrase()
{
    return seedList.getStringSeedPhrase();
}
