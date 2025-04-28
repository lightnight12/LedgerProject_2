#pragma once
#include <sqlite3.h>
#include "User.h"

class SQLData
{
private:
    sqlite3* db;

#pragma region CREATE_ID_TABLE
    std::string createTableQuery =
        "CREATE TABLE IF NOT EXISTS DATA ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "password TEXT NOT NULL, "
        "seed TEXT NOT NULL, "
        "FOREIGN KEY(user_id) REFERENCES Users(id)"
        ");";
#pragma endregion

#pragma region CREATE_CRYPTO_TABLE
    std::string createTableCrypto =
        "CREATE TABLE IF NOT EXISTS COINS ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "coin_name TEXT NOT NULL, "
        "amount REAL NOT NULL, "
        "FOREIGN KEY(user_id) REFERENCES DATA(id)"
        ");";
#pragma endregion

#pragma region CREATE_MONEY_DATA
    std::string createMoneyBalance =
        "CREATE TABLE IF NOT EXISTS BALANCE ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "user_id INTEGER NOT NULL, "
        "money_name TEXT NOT NULL, "  
        "amount REAL NOT NULL, "
        "FOREIGN KEY(user_id) REFERENCES DATA(id)"
        ");";


#pragma endregion

#pragma region ID_QUERY
    std::string insertQuery = "INSERT INTO DATA (user_id, password, seed) VALUES (?, ?, ?);";
    std::string findSeedQuery = "SELECT password FROM DATA WHERE seed = ?;";
    std::string findSeedQuery2 = "SELECT seed FROM DATA;";
    std::string findPasswordQuery = "SELECT seed FROM DATA WHERE password = ?;";
    std::string printQuery = "SELECT id, seed, password FROM DATA;";
#pragma endregion

#pragma region CRYPTO_QUERY
    std::string updateCoinAmountQuery =
        "UPDATE COINS SET amount = ? WHERE user_id = ? AND coin_name = ?;";
    std::string getCoinsForUserQuery =
        "SELECT coin_name, amount FROM COINS WHERE user_id = ?;";
#pragma endregion


public:
    enum class DataBaseState
    {
        DBS_NONE,
        DBS_DATA,
        DBS_COINS,
        DBS_BALANCE
    };

    DataBaseState dbs;

    SQLData() : db(nullptr), dbs(DataBaseState::DBS_NONE) {}
    ~SQLData() {}
    bool open(const std::string& dbID)
    {
        if (sqlite3_open(dbID.c_str(), &db))
        {
            std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }
        return true;
    }
    bool close()
    {
        if (db)
        {
            sqlite3_close(db);
            db = nullptr;
        }
        return true;
    }
    bool execute(const std::string& query)
    {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db, query.c_str(), nullptr, nullptr, &errMsg);

        if (rc != SQLITE_OK)
        {
            std::cerr << "SQL error: " << errMsg << std::endl;
            sqlite3_free(errMsg);
            return false;
        }

        return true;
    }

    bool createTable(DataBaseState dbs)
    {
        switch (dbs)  
        {
        case DataBaseState::DBS_DATA:
            std::cout << "[DEBUG] Creating DATA table...\n";
            return execute(createTableQuery);  

        case DataBaseState::DBS_COINS:
            std::cout << "[DEBUG] Creating COINS table...\n";
            return execute(createTableCrypto);  

        case DataBaseState::DBS_BALANCE:
            std::cout << "[DEBUG] Creating MONEY BALANCE table...\n";
            return execute(createMoneyBalance);  

        default:
            std::cout << "[ERROR] Invalid table selection\n";
            return false;  
        }
    }

    bool valuteExists(const std::string& userID, const std::string& valuteName, DataBaseState dbs)
    {
        sqlite3_stmt* stmt = nullptr;
        const char* query = nullptr;

        switch (dbs)
        {
        case DataBaseState::DBS_COINS:
            query = "SELECT 1 FROM COINS WHERE user_id = ? AND coin_name = ? LIMIT 1;";
            break;
        case DataBaseState::DBS_BALANCE:
            query = "SELECT 1 FROM BALANCE WHERE user_id = ? AND money_name = ? LIMIT 1;";
            break;
        default:
            return false;
        }

        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "[ERROR] Prepare failed: " << sqlite3_errmsg(db) << "\n";
            return false;
        }

        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, valuteName.c_str(), -1, SQLITE_STATIC);

        bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
        sqlite3_finalize(stmt);
        return exists;
    }

    bool updateValuteAmount(const std::string& userID, const std::string& moneyName, float newAmount, DataBaseState dbs)
    {
        sqlite3_stmt* stmt = nullptr;
        const char* query = nullptr;

        switch (dbs)
        {
        case DataBaseState::DBS_COINS:
            query = "UPDATE COINS SET amount = ? WHERE user_id = ? AND coin_name = ?;";
            break;
        case DataBaseState::DBS_BALANCE:
            query = "UPDATE BALANCE SET amount = ? WHERE user_id = ? AND money_name = ?;";
            break;
        default:
            return false;
        }

        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "[ERROR] Prepare failed: " << sqlite3_errmsg(db) << "\n";
            return false;
        }

        sqlite3_bind_double(stmt, 1, newAmount);
        sqlite3_bind_text(stmt, 2, userID.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, moneyName.c_str(), -1, SQLITE_STATIC);

        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    float getCurrentValuteAmount(const std::string& userID, const std::string& valuteName, DataBaseState dbs)
    {
        sqlite3_stmt* stmt = nullptr;
        const char* query = nullptr;

        switch (dbs)
        {
        case DataBaseState::DBS_COINS:
            query = "SELECT amount FROM COINS WHERE user_id = ? AND coin_name = ?;";
            break;
        case DataBaseState::DBS_BALANCE:
            query = "SELECT amount FROM BALANCE WHERE user_id = ? AND money_name = ?;";
            break;
        default:
            return 0.0f;
        }

        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "[ERROR] Prepare failed: " << sqlite3_errmsg(db) << "\n";
            return 0.0f;
        }

        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, valuteName.c_str(), -1, SQLITE_STATIC);

        float currentAmount = 0.0f;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            currentAmount = static_cast<float>(sqlite3_column_double(stmt, 0));
        }

        sqlite3_finalize(stmt);
        return currentAmount;
    }

    void insertValute(const std::string& userID, const std::string& valuteName, double amount, DataBaseState dbs)
    {
        if (!db)
        {
            std::cerr << "Database not open." << std::endl;
            return;
        }

        sqlite3_stmt* stmt;
        const char* query = nullptr;

        switch (dbs)
        {
        case DataBaseState::DBS_COINS:
            query = "INSERT INTO COINS (user_id, coin_name, amount) VALUES (?, ?, ?);";
            break;
        case DataBaseState::DBS_BALANCE:
            query = "INSERT INTO BALANCE (user_id, money_name, amount) VALUES (?, ?, ?);";
        }

        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Failed to prepare insert valute statement: " << sqlite3_errmsg(db) << "\n";
            return;
        }

        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, valuteName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 3, amount);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            std::cerr << "Failed to insert valute: " << sqlite3_errmsg(db) << "\n";
        }
        else
        {
            std::cout << "[INFO] Valute inserted successfully.\n";
        }

        sqlite3_finalize(stmt);
    }

    void insertData(const std::string& userID, const std::string& password, const std::string& seed)
    {
        if (!db)
        {
            std::cerr << "Database not open." << std::endl;
            return;
        }

        std::string trimmedSeed = trim(seed);

        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, insertQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Failed to prepare insert statement: " << sqlite3_errmsg(db) << "\n";
            return;
        }

        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, trimmedSeed.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            std::cerr << "Failed to insert data: " << sqlite3_errmsg(db) << "\n";
        }
        else
        {
            std::cout << "[INFO] Data inserted successfully.\n";
        }

        sqlite3_finalize(stmt);
    }

    std::string trim(const std::string& str)
    {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos)
            return "";

        size_t last = str.find_last_not_of(' ');
        return str.substr(first, last - first + 1);
    }
    std::string findSeed(const std::string& seed)
    {
        if (!db)
        {
            std::cerr << "Database not open." << std::endl;
            return "";
        }

        std::string trimmedSeed = trim(seed);
        std::cout << "[INFO] Looking for seed: '" << trimmedSeed << "' (length: " << trimmedSeed.length() << ")\n";

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, findSeedQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
            return "";
        }

        sqlite3_bind_text(stmt, 1, trimmedSeed.c_str(), -1, SQLITE_TRANSIENT);

        std::string result;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char* password = sqlite3_column_text(stmt, 0);
            result = reinterpret_cast<const char*>(password);
        }
        else
        {
            std::cout << "[INFO] Seed not found in the database.\n";
        }

        sqlite3_finalize(stmt);
        return result;
    }


    std::string findPassword(const std::string& password)
    {
        if (!db)
        {
            std::cerr << "Database not open\n";
            return "";
        }

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, findPasswordQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
            return "";
        }

        sqlite3_bind_text(stmt, 1, password.c_str(), -1, SQLITE_TRANSIENT);

        std::string result;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char* seed = sqlite3_column_text(stmt, 0);
            result = reinterpret_cast<const char*>(seed);
        }
        else
        {
            std::cout << "Password not found.\n";
        }

        sqlite3_finalize(stmt);
        return result;
    }
    std::string getSQLUserID(const std::string& password)
    {
        std::string getUserIDByPasswordQuery = "SELECT user_id FROM DATA WHERE password = ?;";

        if (!db)
        {
            std::cerr << "Database not open\n";
            return "";
        }

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, getUserIDByPasswordQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << "\n";
            return "";
        }

        sqlite3_bind_text(stmt, 1, password.c_str(), -1, SQLITE_TRANSIENT);

        std::string result;
        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            const unsigned char* seed = sqlite3_column_text(stmt, 0);
            result = reinterpret_cast<const char*>(seed);
        }
        else
        {
            std::cout << "Password not found.\n";
        }

        sqlite3_finalize(stmt);
        return result;
    }

    std::vector<std::pair<std::string, float>> getUserBalance(const std::string& userID)
    {
        std::vector<std::pair<std::string, float>> balances;

        const char* query = "SELECT money_name, amount FROM BALANCE WHERE user_id = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
            return balances;
        }

        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string moneyName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            float amount = static_cast<float>(sqlite3_column_double(stmt, 1));
            balances.emplace_back(moneyName, amount);
        }

        sqlite3_finalize(stmt);
        return balances;
    }
    std::vector<std::pair<std::string, float>> getUserCoins(const std::string& userID)
    {
        std::vector<std::pair<std::string, float>> coins;

        const char* query = "SELECT coin_name, amount FROM COINS WHERE user_id = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare getUserCoins: " << sqlite3_errmsg(db) << std::endl;
            return coins;
        }

        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string coinName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            float amount = static_cast<float>(sqlite3_column_double(stmt, 1));
            coins.emplace_back(coinName, amount);
        }

        sqlite3_finalize(stmt);
        return coins;
    }

    float getUserCoin(const std::string& userID)
    {
        float coinCount = 0.0f;

        std::string query = "SELECT amount FROM COINS WHERE user_id = ? AND coin_name = ?;";

        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Failed to prepare balance query: " << sqlite3_errmsg(db) << std::endl;
            return coinCount;
        }

        // Прив'язуємо значення userID та coinName до запиту
        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            coinCount = static_cast<float>(sqlite3_column_double(stmt, 0));  
        }
        else
        {
            std::cerr << "No balance found for user " << userID << std::endl;
        }

        sqlite3_finalize(stmt);
        return coinCount;
    }

    bool findUserID(const std::string& userID)
    {
        const char* query = "SELECT 1 FROM DATA WHERE user_id = ? LIMIT 1;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare findUserID: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        if (sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
            std::cerr << "Failed to bind parameter in findUserID: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            return false;
        }

        bool exists = false;

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            exists = true;
        }

        sqlite3_finalize(stmt);
        return exists;
    }
    bool sendCoinsToUser(const std::string& userID, const std::string& coinName, float amount)
    {
        const char* findUserQuery = "SELECT id FROM DATA WHERE user_id = ?;";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, findUserQuery, -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "Failed to prepare findUserID: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, userID.c_str(), -1, SQLITE_STATIC);

        std::string realUserID = userID;

        sqlite3_finalize(stmt);

        if (realUserID == "") {
            std::cerr << "User not found!" << std::endl;
            return false;
        }

        std::cout << "Found userID: " << realUserID << std::endl;

        // Тепер шукаємо монету в COINS
        const char* findCoinQuery = "SELECT id FROM COINS WHERE user_id = ? AND coin_name = ?;";
        if (sqlite3_prepare_v2(db, findCoinQuery, -1, &stmt, nullptr) != SQLITE_OK) {
            std::cerr << "Failed to prepare findCoinQuery: " << sqlite3_errmsg(db) << std::endl;
            return false;
        }

        sqlite3_bind_text(stmt, 1, realUserID.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, coinName.c_str(), -1, SQLITE_STATIC);

        bool coinExists = false;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            coinExists = true;
        }

        sqlite3_finalize(stmt);

        if (coinExists)
        {
            // Оновити існуючу монету
            const char* updateQuery = "UPDATE COINS SET amount = amount + ? WHERE user_id = ? AND coin_name = ?;";
            if (sqlite3_prepare_v2(db, updateQuery, -1, &stmt, nullptr) != SQLITE_OK) {
                std::cerr << "Failed to prepare updateQuery: " << sqlite3_errmsg(db) << std::endl;
                return false;
            }

            sqlite3_bind_double(stmt, 1, static_cast<double>(amount));
            sqlite3_bind_text(stmt, 2, realUserID.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, coinName.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to update coin: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_finalize(stmt);
                return false;
            }
            sqlite3_finalize(stmt);
        }
        else
        {
            const char* insertQuery = "INSERT INTO COINS (user_id, coin_name, amount) VALUES (?, ?, ?);";
            if (sqlite3_prepare_v2(db, insertQuery, -1, &stmt, nullptr) != SQLITE_OK) {
                std::cerr << "Failed to prepare insertQuery: " << sqlite3_errmsg(db) << std::endl;
                return false;
            }

            sqlite3_bind_text(stmt, 1, realUserID.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, coinName.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt, 3, static_cast<double>(amount));

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                std::cerr << "Failed to insert coin: " << sqlite3_errmsg(db) << std::endl;
                sqlite3_finalize(stmt);
                return false;
            }
            sqlite3_finalize(stmt);
        }

        return true;
    }


    void printAllSeeds()
    {
        if (!db)
        {
            std::cerr << "[ERROR] Database not open.\n";
            return;
        }

        sqlite3_stmt* stmt = nullptr;

        if (sqlite3_prepare_v2(db, printQuery.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            std::cerr << "[ERROR] Failed to prepare SELECT statement: " << sqlite3_errmsg(db) << "\n";
            return;
        }

        std::cout << "=== [SEED TABLE DUMP] ===\n";

        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* seedText = sqlite3_column_text(stmt, 1);
            const unsigned char* passText = sqlite3_column_text(stmt, 2);

            std::string seedStr = reinterpret_cast<const char*>(seedText);
            std::string passStr = reinterpret_cast<const char*>(passText);

            std::cout << "ID: " << id
                << " | Seed: '" << seedStr << "' (length: " << seedStr.length() << ")"
                << " | Password: '" << passStr << "'\n";
        }

        std::cout << "=========================\n";

        sqlite3_finalize(stmt);
    }





};
