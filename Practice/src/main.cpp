#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>
#include <windows.h>
#include <direct.h> 
#include <iomanip>
#include <format> 
#include <filesystem>
#include <fstream>
#include <errno.h>  
#include <cstring>
#include <iostream>
#include <random>
#include <chrono>
#include <thread>

#include "Ledger.h"

#pragma region DX9_GLOBAL_DATA
// Global variables for managing Direct3D 9 and ImGui state
static LPDIRECT3D9 g_pD3D = nullptr; // Main Direct3D interface — used to create the rendering device
static LPDIRECT3DDEVICE9 g_pd3dDevice = nullptr; // Rendering device — used to draw the ImGui interface
static bool g_DeviceLost = false; // Flag indicating if the device is lost (e.g., when changing resolution or using alt+tab)
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0; // Variables for storing new window size (used when resizing)
static D3DPRESENT_PARAMETERS g_d3dpp = {}; // Direct3D presentation parameters (format, buffers, depth, etc.)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam); // Windows message handler for ImGui + DirectX9

// Main message handling function for Windows
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))     // Pass events to ImGui if they concern it (e.g., key presses, mouse events, etc.)
        return true;

    switch (msg)
    {
    case WM_SIZE:
        // If the window is minimized — do nothing
        if (wParam == SIZE_MINIMIZED)
            return 0;

        // Store the new window size to use later for resizing the rendering device
        g_ResizeWidth = (UINT)LOWORD(lParam);
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
        // Block access to the system menu via Alt (to prevent flickering menu)
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        // Exit the application when the main window is closed
        ::PostQuitMessage(0);
        return 0;
    }

    // Pass all other messages to the default Windows handler
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
#pragma endregion


const int windowWidth = 800;  // The width of the window
const int windowHeight = 600; // The height of the window

class UI_Render
{
private:
    User user;                // User object that stores information about the user
    SQLData sqlData;          // SQLData object that handles database interaction
    SeedList seedList;        // SeedList object for managing the seed phrase
    Coin coin;                // Coin object to represent the coin in use
    Ledger ledger;            // Ledger object for managing user wallet and coins

    std::list<Coin> coins;    // A list to store coins available in the wallet
    Currency currency;        // Currency object containing supported currencies
    float SEND_AMOUNT = 0.0f; // Amount to send (in cryptocurrency)
    std::string SEND_COIN_NAME = ""; // Name of the coin to be sent
    float newGlobalAmount = 0.0f; // New global amount for cryptocurrency

    enum class MenuState
    {
        MS_None,              // No menu state selected

        MS_Login,             // Login screen
        MS_RestartPasswod,    // Screen to restart the password
        MS_EnterNewUser,      // Screen to enter new user details

        MS_ChooseSeedLength,  // Screen to choose the seed phrase length
        MS_ShowSeedPhrase,    // Screen to display the generated seed phrase
        MS_ShowUserID,        // Screen to display the user ID

        MS_UserView,          // User view screen (home screen after login)
        MS_AddMoneyInWallet,  // Screen to add money to the wallet
        MS_BuyCoins,          // Screen to buy coins
        MS_WidthdrawCoin,     // Screen to withdraw a coin
        MS_EnterCryptoAccount,// Screen to enter crypto account

        MS_UserCoins,         // User's coins screen
        MS_UserWallet,        // User's wallet screen
    };
    MenuState CurrentState;  // Current menu state

public:
    // Constructor, methods, and additional logic will follow here

#pragma region DEVICE
    bool CreateDeviceD3D(HWND hWnd) // Create Direct3D9 render device
    {
        // Create the main Direct3D object
        // If failed, return false (function ends with failure)
        if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
            return false;

        // Zero out the presentation parameters structure (to avoid garbage in memory)
        ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));

        // Set presentation parameters:
        g_d3dpp.Windowed = TRUE;                                // Work in windowed mode
        g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;             // Swap buffer is discarded each frame
        g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;              // Format is automatically chosen based on current desktop settings
        g_d3dpp.EnableAutoDepthStencil = TRUE;                  // Enable automatic depth/stencil buffer creation
        g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;            // Depth/stencil buffer format — 16-bit
        g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Vertical sync (1 frame per monitor cycle)

        // Create rendering device
        // Parameters: first adapter, device type, window handle, vertex processing, render settings, output device
        if (g_pD3D->CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_HAL,
            hWnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING,
            &g_d3dpp,
            &g_pd3dDevice) < 0)
            return false; // If device creation fails — return false

        return true; // Everything succeeded
    }

    void CleanupDeviceD3D()
    {
        // If the device was created — release it (frees GPU resources)
        if (g_pd3dDevice)
        {
            g_pd3dDevice->Release(); // Decreases the reference count of the object (COM)
            g_pd3dDevice = nullptr;  // Nullify to avoid dangling pointers
        }

        // If the Direct3D object was created — release it as well
        if (g_pD3D)
        {
            g_pD3D->Release(); // Clean up the Direct3D9 API
            g_pD3D = nullptr;  // Nullify
        }
    }

    void ResetDevice()
    {
        // Invalidate ImGui-related Direct3D9 objects (textures, shaders, buffers)
        ImGui_ImplDX9_InvalidateDeviceObjects();

        // Reset (restart) the graphics device with the current parameters (e.g., updated window size)
        HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);

        // If the device could not be reset — throw an error (only in debug mode)
        if (hr == D3DERR_INVALIDCALL)
            IM_ASSERT(0);

        // After a successful reset — recreate ImGui objects
        ImGui_ImplDX9_CreateDeviceObjects();
    }
#pragma endregion

    
#pragma region CONTROL

    // Login function: handles user login logic
    void Login(char* password)
    {
        ImGui::Begin("Login");
        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        // Input for the password
        ImGui::InputText("Password", password, 128, ImGuiInputTextFlags_Password);

        // Button to submit the login attempt
        if (ImGui::Button("Login"))
        {
            if (password && password[0] != '\0') // Check if password is not empty
            {
                // Check if the password is in the SQL table
                if (!ledger.findPasswordInSQLTable(password))
                {
                    CurrentState = MenuState::MS_RestartPasswod; // If not found, go to password reset state
                }
                else
                {
                    // If password is correct, get the userID from the database
                    user.userID = sqlData.getSQLUserID(password);
                    if (!user.userID.empty())
                    {
                        std::cout << user.userID;
                    }

                    CurrentState = MenuState::MS_UserView; // Proceed to user dashboard
                }
            }
        }

        ImGui::EndChild();
        ImGui::End();
    }

    // Restart Password function: allows user to reset their password or enter seed phrase
    void RestartPassword()
    {
        static bool openSeedPhraseField = false;
        static char tempSeedPhrase[300];

        ImGui::Begin("Account Recovery");

        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        ImGui::Text("User not found. Choose an option:");

        // Buttons to either create a new user or enter seed phrase
        if (ImGui::Button("Create New User"))
        {
            CurrentState = MenuState::MS_ChooseSeedLength; // Go to seed length selection
        }

        if (ImGui::Button("Enter Seed Phrase"))
        {
            openSeedPhraseField = true;
        }

        if (openSeedPhraseField)
        {
            // Input for seed phrase
            ImGui::InputText("Seed Phrase", tempSeedPhrase, IM_ARRAYSIZE(tempSeedPhrase));

            // Button to confirm the seed phrase and log in
            if (ImGui::Button("Confirm"))
            {
                if (ledger.findSeedPhraseInSQLTable(tempSeedPhrase)) // Validate the seed phrase
                {
                    CurrentState = MenuState::MS_UserView; // Proceed to user view
                    openSeedPhraseField = false;
                    memset(tempSeedPhrase, 0, sizeof(tempSeedPhrase)); // Clear seed phrase input
                }
            }
        }

        if (ImGui::Button("Back"))
        {
            CurrentState = MenuState::MS_Login; // Go back to login
            openSeedPhraseField = false;
            memset(tempSeedPhrase, 0, sizeof(tempSeedPhrase)); // Clear seed phrase input
        }

        ImGui::EndChild();
        ImGui::End();
    }

    // Enter New User function: allows the creation of a new user
    void EnterNewUser(char* password)
    {
        ImGui::Begin("New User");

        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        static char tempPassword[128] = {}; // Temporary password for new user

        ImGui::InputText("Password", tempPassword, 128, ImGuiInputTextFlags_Password); // Input for new password

        if (ImGui::Button("Login"))
        {
            if (password) // If password is valid
            {
                strncpy_s(password, 128, tempPassword, _TRUNCATE); // Set the password
                memset(tempPassword, 0, 128); // Clear temporary password field
                CurrentState = MenuState::MS_ChooseSeedLength; // Move to seed length selection
            }
        }

        ImGui::EndChild();
        ImGui::End();
    }

    // Choose Seed Length function: allows the user to select seed phrase length
    void ChooseSeedLength(char* password)
    {
        ImGui::Begin("Choose Seed Phrase Length");

        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        static int seedLength = 0;

        ImGui::Text("Select seed phrase length:");
        ImGui::RadioButton("12 words", &seedLength, 12); // Option for 12-word seed phrase
        ImGui::RadioButton("16 words", &seedLength, 16); // Option for 16-word seed phrase
        ImGui::RadioButton("24 words", &seedLength, 24); // Option for 24-word seed phrase

        if (ImGui::Button("Generate"))
        {
            if (password) // If password is valid, generate the seed phrase
            {
                ledger.enterUser_ImGui(password, seedLength);
                CurrentState = MenuState::MS_ShowSeedPhrase; // Proceed to show the seed phrase
            }
        }

        if (ImGui::Button("Back"))
        {
            CurrentState = MenuState::MS_Login; // Go back to login screen
        }

        ImGui::EndChild();
        ImGui::End();
    }

    // Show Seed Phrase function: shows the generated seed phrase to the user
    void ShowSeedPhrase()
    {
        ImGui::Begin("Seed Phrase (Write it on the paper)");

        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        std::string seedPhrase = ledger.getSeedPhrase(); // Get the generated seed phrase
        ImGui::Text("%s", seedPhrase.c_str()); // Display the seed phrase

        if (ImGui::Button("Confirm"))
        {
            CurrentState = MenuState::MS_UserView; // Confirm the seed phrase and go to user view
        }

        ImGui::EndChild();
        ImGui::End();
    }

    // Show UserID function: shows the userID on the screen
    void ShowUserID()
    {
        ImGui::Begin("Seed Phrase (Write it on the paper)");

        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);
        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        std::string userIDText = user.userID;
        ImGui::Text("%s", userIDText.c_str()); // Display the userID

        if (ImGui::Button("Return"))
        {
            CurrentState = MenuState::MS_UserView; // Go back to user dashboard
        }

        ImGui::EndChild();
        ImGui::End();
    }

    // User View function: shows the user dashboard with options to manage the wallet, buy coins, etc.
    void UserView(char* password)
    {
        static bool openGraph = false;
        static float values[60] = {}; // Placeholder for graph values
        static float base[] = { 1.0f, 3.0f, 2.5f, 5.0f, 4.0f, 6.0f };
        static int index = 0;
        static double lastUpdate = 0.0;
        static float currentValue = 1.0f;

        ImGui::Begin("User Dashboard");

        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        // Buttons for different user actions: adding money, viewing wallet, buying coins, etc.
        if (ImGui::Button("Add Money in Wallet"))
        {
            CurrentState = MenuState::MS_AddMoneyInWallet;
        }
        else if (ImGui::Button("My Wallet"))
        {
            CurrentState = MenuState::MS_UserWallet;
        }
        else if (ImGui::Button("My coins"))
        {
            CurrentState = MenuState::MS_UserCoins;
        }
        else if (ImGui::Button("Buy coins"))
        {
            CurrentState = MenuState::MS_BuyCoins;
        }
        else if (ImGui::Button("Widthdraw coins"))
        {
            CurrentState = MenuState::MS_WidthdrawCoin;
        }
        else if (ImGui::Button("ShowUserID"))
        {
            CurrentState = MenuState::MS_ShowUserID;
        }
        else if (ImGui::Button("Logout"))
        {
            CurrentState = MenuState::MS_Login;
            memset(password, 0, 128); // Clear password
        }

        ImGui::EndChild();
        ImGui::End();
    }

    void AddMoneyInWallet()
    {
        // Begin the "Add Currency" window
        ImGui::Begin("Add Currency");

        // Set the window size to the desired dimensions
        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        // Create a child window for content
        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        // Static variables to store the amount to add and the selected currency
        static float amountToAdd = 0.0f;
        static std::string selectedCurrency;

        // Loop through all available currencies and create a button for each
        for (auto& it : currency.currentCurrency)
        {
            // If the button is clicked, set the selected currency
            if (ImGui::Button(it.c_str()))
            {
                selectedCurrency = it;
            }
        }

        // If a currency is selected, show a slider to select the amount to add
        if (!selectedCurrency.empty())
        {
            // Slider label dynamically changes based on the selected currency
            std::string sliderLabel = "Amount to Add in " + selectedCurrency;
            ImGui::Text("Select amount to add in %s:", selectedCurrency.c_str());
            ImGui::SliderFloat(sliderLabel.c_str(), &amountToAdd, 0.0f, 1000.0f);

            // If the "Confirm" button is clicked, add the amount to the wallet
            if (ImGui::Button("Confirm"))
            {
                // Check if the currency exists in the user's balance
                if (sqlData.valuteExists(user.userID, selectedCurrency, SQLData::DataBaseState::DBS_BALANCE))
                {
                    // If exists, update the user's balance with the new amount
                    float currentAmount = sqlData.getCurrentValuteAmount(user.userID, selectedCurrency, SQLData::DataBaseState::DBS_BALANCE);
                    float updatedAmount = currentAmount + amountToAdd;
                    sqlData.updateValuteAmount(user.userID, selectedCurrency, updatedAmount, SQLData::DataBaseState::DBS_BALANCE);
                }
                else
                {
                    // If currency doesn't exist, insert it with the added amount
                    sqlData.insertValute(user.userID, selectedCurrency, amountToAdd, SQLData::DataBaseState::DBS_BALANCE);
                }

                // Log the amount added and reset the values
                std::cout << "Added " << amountToAdd << " " << selectedCurrency << " to wallet" << std::endl;
                amountToAdd = 0.0f;
                selectedCurrency.clear();
            }
        }

        // Button to return to the previous menu
        if (ImGui::Button("Return"))
        {
            CurrentState = MenuState::MS_UserView;
        }

        // End child window and main window
        ImGui::EndChild();
        ImGui::End();
    }
    void BuyCoins()
    {
        // Begin the "Buy Crypto" window
        ImGui::Begin("Buy Crypto");

        // Set the window size to the desired dimensions
        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        // Create a child window for content
        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        // Static variables to store the selected coin and the quantities
        static std::string selectedCoinName;
        static std::map<std::string, float> coinQuantities;

        // Variable to show error messages if needed
        static bool showErrorMsg = false;

        // Loop through all available coins and create a button for each
        for (const auto& coin : coins)
        {
            std::string buttonLabel = "Buy " + coin.coinName;

            // If the button is clicked, set the selected coin
            if (ImGui::Button(buttonLabel.c_str()))
            {
                selectedCoinName = coin.coinName;
            }

            // When a coin is selected, display a slider to set the amount in dollars
            if (selectedCoinName == coin.coinName)
            {
                std::string sliderLabel = "Amount in $ for " + coin.coinName;

                // Initialize the amount for the coin if it's not already set
                if (coinQuantities.find(coin.coinName) == coinQuantities.end())
                {
                    coinQuantities[coin.coinName] = 1.0f;
                }

                // Retrieve the amount in dollars for the selected coin
                float& amountInDollars = coinQuantities[coin.coinName];
                ImGui::SliderFloat(sliderLabel.c_str(), &amountInDollars, 1.0f, 100.0f);

                // Calculate the amount of coins the user will receive based on the price
                float coinsToReceive = amountInDollars / coin.prise;

                // Show the price and the calculated amount of coins
                ImGui::Text("Price: $%.2f for 1 %s", coin.prise, coin.coinName.c_str());
                ImGui::Text("You will receive: %.6f %s", coinsToReceive, coin.coinName.c_str());

                // If the "Confirm Purchase" button is clicked
                if (ImGui::Button("Confirm Purchase"))
                {
                    // Check if the user has enough balance in USD
                    if (sqlData.getCurrentValuteAmount(user.userID, "USD", SQLData::DataBaseState::DBS_BALANCE) > 0.0f)
                    {
                        // If enough balance, check if the user has sufficient funds
                        if (sqlData.getCurrentValuteAmount(user.userID, "USD", SQLData::DataBaseState::DBS_BALANCE) >= amountInDollars)
                        {
                            // Check if the coin already exists in the user's wallet
                            if (sqlData.valuteExists(user.userID, coin.coinName, SQLData::DataBaseState::DBS_COINS))
                            {
                                // If the coin exists, update the amount
                                float currentAmount = sqlData.getCurrentValuteAmount(user.userID, coin.coinName, SQLData::DataBaseState::DBS_COINS);
                                float newAmount = currentAmount + coinsToReceive;
                                sqlData.updateValuteAmount(user.userID, coin.coinName, newAmount, SQLData::DataBaseState::DBS_COINS);

                                // Deduct the amount from the user's USD balance
                                float currenMoneyAmount = sqlData.getCurrentValuteAmount(user.userID, "USD", SQLData::DataBaseState::DBS_BALANCE);
                                float newMoneyAmount = currenMoneyAmount - amountInDollars;
                                sqlData.updateValuteAmount(user.userID, "USD", newMoneyAmount, SQLData::DataBaseState::DBS_BALANCE);
                            }
                            else
                            {
                                // If the coin doesn't exist, insert it into the user's wallet
                                sqlData.insertValute(user.userID, selectedCoinName, coinsToReceive, SQLData::DataBaseState::DBS_COINS);

                                // Deduct the amount from the user's USD balance
                                float currenMoneyAmount = sqlData.getCurrentValuteAmount(user.userID, "USD", SQLData::DataBaseState::DBS_BALANCE);
                                float newMoneyAmount = currenMoneyAmount - amountInDollars;
                                sqlData.updateValuteAmount(user.userID, "USD", newMoneyAmount, SQLData::DataBaseState::DBS_BALANCE);
                            }

                            // Clear the selected coin and log the purchase
                            selectedCoinName.clear();
                            std::cout << "Purchased " << coinsToReceive << " of " << coin.coinName << std::endl;
                        }
                        else
                        {
                            // If not enough funds, show an error message
                            showErrorMsg = true;
                            if (showErrorMsg)
                            {
                                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Not enough funds to complete the purchase.");
                            }
                        }
                    }
                }
            }

            ImGui::Separator();
        }

        // Button to return to the previous menu
        if (ImGui::Button("Return"))
        {
            showErrorMsg = false;
            CurrentState = MenuState::MS_UserView;
        }

        // End child window and main window
        ImGui::EndChild();
        ImGui::End();
    }
    void WidthdrawCoin()
    {
        static int selectedCoinIndex = -1; // Static variable to remember the selected coin index

        ImGui::Begin("Withdraw Crypto");

        // Set the window size for the withdraw window
        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        // Create a child window that occupies the full size of the parent window
        ImVec2 child_size(windowSize.x, windowSize.y);
        ImGui::BeginChild("ChildWindow", child_size, true);

        // Get the list of user's coins
        auto coins = sqlData.getUserCoins(user.userID);

        // Begin the table for displaying the coins and their amounts
        if (ImGui::BeginTable("CoinsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
        {
            // Set up the columns for the table (Coin name and Coin Amount)
            ImGui::TableSetupColumn("Coin");
            ImGui::TableSetupColumn("Amount");
            ImGui::TableHeadersRow();

            // Loop through each coin and display it in the table
            for (int i = 0; i < coins.size(); ++i)
            {
                const auto& coin = coins[i];

                ImGui::TableNextRow();

                // Display the coin name with selectable option
                ImGui::TableSetColumnIndex(0);
                bool isSelected = (selectedCoinIndex == i);
                if (ImGui::Selectable(coin.first.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    selectedCoinIndex = i; // Select the coin when clicked
                }

                // Display the amount of the selected coin
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.4f", coin.second); // Display amount with 4 decimal precision
            }

            ImGui::EndTable();
        }

        // If a coin is selected, show withdrawal options
        if (selectedCoinIndex >= 0 && selectedCoinIndex < coins.size())
        {
            const auto& selectedCoin = coins[selectedCoinIndex];

            ImGui::Separator(); // Separator for better layout

            // Display the selected coin and its amount
            ImGui::Text("Selected: %s (%.4f)", selectedCoin.first.c_str(), selectedCoin.second);

            // Input for the amount to withdraw
            static float amountToWithdraw = 0.0f;
            ImGui::InputFloat("Amount to withdraw", &amountToWithdraw, 0.01f, 1.0f, "%.4f");

            // Withdraw button logic
            if (ImGui::Button("Withdraw"))
            {
                // Check if the withdrawal amount is less than or equal to the available balance
                if (amountToWithdraw <= selectedCoin.second)
                {
                    // Update the global amount after withdrawal
                    float currentCount = selectedCoin.second;
                    float newAmount = currentCount - amountToWithdraw;
                    newGlobalAmount = newAmount;

                    // Set up the coin and amount for the transaction
                    SEND_AMOUNT = amountToWithdraw;
                    SEND_COIN_NAME = selectedCoin.first;

                    // Log the withdrawal action
                    std::cout << "Withdrawing " << amountToWithdraw << " of " << selectedCoin.first << std::endl;

                    // Reset the selected coin and transition to the next state
                    selectedCoinIndex = -1;
                    CurrentState = MenuState::MS_EnterCryptoAccount; // Move to the next step: entering crypto account
                }
                else
                {
                    // Inform the user if the withdrawal amount exceeds the balance
                    std::cout << "Not enough balance!" << std::endl;
                }
            }
        }

        // Return button to go back to the user view
        if (ImGui::Button("Return"))
        {
            CurrentState = MenuState::MS_UserView; // Return to the user view
        }

        // End the child window and the main window
        ImGui::EndChild();
        ImGui::End();
    }
    void EnterCryptoAccount()
    {
        ImGui::Begin("Send Crypto");

        // Set the window size for the send crypto window
        ImVec2 windowSize(windowWidth, windowHeight);
        ImGui::SetWindowSize(windowSize);

        // Create a child window that occupies the full size of the parent window
        ImVec2 child_size(windowSize.x, windowHeight);
        ImGui::BeginChild("ChildWindow", child_size, true);

        static char cryptoAccount[128] = ""; // Input buffer for the crypto account

        // Input for the crypto account address
        ImGui::InputText("Enter crypto account", cryptoAccount, sizeof(cryptoAccount));

        // Display the entered account
        ImGui::Text("Entered Account: %s", cryptoAccount);

        ImGui::Separator(); // Separator for layout

        // Display the amount and coin name that is being sent
        ImGui::Text("Sending: %.4f of %s", SEND_AMOUNT, SEND_COIN_NAME.c_str());

        // Send button logic
        if (ImGui::Button("Send"))
        {
            // Check if the entered crypto account exists in the database
            if (sqlData.findUserID(cryptoAccount))
            {
                // Perform the transaction: send the coins to the specified account
                sqlData.sendCoinsToUser(cryptoAccount, SEND_COIN_NAME, SEND_AMOUNT);

                // Update the user's coin balance after sending the amount
                sqlData.updateValuteAmount(user.userID, SEND_COIN_NAME, newGlobalAmount, SQLData::DataBaseState::DBS_COINS);

                // Log the transaction
                std::cout << "Sent " << SEND_AMOUNT << " " << SEND_COIN_NAME << " to " << cryptoAccount << std::endl;
            }
            else
            {
                // Inform the user if the crypto account is not found
                std::cout << "User not found!" << std::endl;
            }

            // Return to the user view after the transaction
            CurrentState = MenuState::MS_UserView;
        }

        // Cancel button to return to the user view without sending
        if (ImGui::Button("Cancel"))
        {
            CurrentState = MenuState::MS_UserView; // Return to the user view
        }

        // End the child window and the main window
        ImGui::EndChild();
        ImGui::End();
    }

    void UserCoins()
    {
        // Begin the ImGui window titled "My Coins"
        ImGui::Begin("My Coins");
        ImGui::SetWindowSize(ImVec2(windowWidth, windowHeight)); // Set the window size

        // Begin a child window inside the main window
        ImGui::BeginChild("ChildWindow", ImVec2(0, 0), true);

        // Return button to go back to the user view
        if (ImGui::Button("Return"))
        {
            CurrentState = MenuState::MS_UserView; // Transition to the user view state
        }

        // Fetch the user's coins from the database
        std::vector<std::pair<std::string, float>> userCoins = sqlData.getUserCoins(user.userID);

        // Loop through each coin and display it along with the amount
        for (const auto& coin : userCoins)
        {
            ImGui::Text("%s - %.8f", coin.first.c_str(), coin.second); // Display coin name and its amount
        }

        // End the child window and the main window
        ImGui::EndChild();
        ImGui::End();
    }
    void UserWallet()
    {
        // Begin the ImGui window titled "My Wallet"
        ImGui::Begin("My Wallet");
        ImGui::SetWindowSize(ImVec2(windowWidth, windowHeight)); // Set the window size

        // Begin a child window inside the main window
        ImGui::BeginChild("ChildWindow", ImVec2(0, 0), true);

        // Return button to go back to the user view
        if (ImGui::Button("Return"))
        {
            CurrentState = MenuState::MS_UserView; // Transition to the user view state
        }

        // Fetch the user's balance from the database (wallet money)
        std::vector<std::pair<std::string, float>> walletMoney = sqlData.getUserBalance(user.userID);

        // Loop through each item in the wallet and display it (e.g., balance and coin name)
        for (const auto& money : walletMoney)
        {
            ImGui::Text("%s - %.2f", money.first.c_str(), money.second); // Display balance and its value
        }

        // End the child window and the main window
        ImGui::EndChild();
        ImGui::End();
    }
    void SwitchFunc(char* password)
    {
        // Switch statement to handle different menu states
        switch (CurrentState)
        {
        case MenuState::MS_None:
            break; // Do nothing if the state is MS_None

        case MenuState::MS_Login:
            Login(password); // Call Login function if the state is MS_Login
            break;
        case MenuState::MS_RestartPasswod:
            RestartPassword(); // Call RestartPassword function if the state is MS_RestartPasswod
            break;
        case MenuState::MS_EnterNewUser:
            EnterNewUser(password); // Call EnterNewUser function if the state is MS_EnterNewUser
            break;

        case MenuState::MS_ChooseSeedLength:
            ChooseSeedLength(password); // Call ChooseSeedLength function if the state is MS_ChooseSeedLength
            break;
        case MenuState::MS_ShowSeedPhrase:
            ShowSeedPhrase(); // Call ShowSeedPhrase function if the state is MS_ShowSeedPhrase
            break;
        case MenuState::MS_ShowUserID:
            ShowUserID(); // Call ShowUserID function if the state is MS_ShowUserID
            break;

        case MenuState::MS_UserView:
            UserView(password); // Call UserView function if the state is MS_UserView
            break;
        case MenuState::MS_AddMoneyInWallet:
            AddMoneyInWallet(); // Call AddMoneyInWallet function if the state is MS_AddMoneyInWallet
            break;
        case MenuState::MS_BuyCoins:
            BuyCoins(); // Call BuyCoins function if the state is MS_BuyCoins
            break;
        case MenuState::MS_WidthdrawCoin:
            WidthdrawCoin(); // Call WidthdrawCoin function if the state is MS_WidthdrawCoin
            break;
        case MenuState::MS_UserCoins:
            UserCoins(); // Call UserCoins function if the state is MS_UserCoins
            break;
        case MenuState::MS_UserWallet:
            UserWallet(); // Call UserWallet function if the state is MS_UserWallet
            break;

        case MenuState::MS_EnterCryptoAccount:
            EnterCryptoAccount(); // Call EnterCryptoAccount function if the state is MS_EnterCryptoAccount
            break;
        default:
            break; // Do nothing if the state is unknown or not specified
        }
    }
#pragma endregion

public:
    UI_Render()
        // Constructor for the UI_Render class
        : ledger(user, sqlData, seedList, coin), seedList(12), CurrentState(MenuState::MS_Login)  // Initialize member variables and set the initial state
    {
        // Adding some predefined coins with their respective values (this could be dynamic in a full implementation)
        coins.push_back(Coin("Bitcoin", 63250.0f));     // Bitcoin with a value of 63250.0
        coins.push_back(Coin("Ethereum", 3100.0f));     // Ethereum with a value of 3100.0
        coins.push_back(Coin("Dogecoin", 0.14f));       // Dogecoin with a value of 0.14
        coins.push_back(Coin("Litecoin", 85.0f));       // Litecoin with a value of 85.0
        coins.push_back(Coin("Cardano", 0.45f));        // Cardano with a value of 0.45
    }

    void Update()
    {
#pragma region RenderWindow
        // === [1] Registering the Window Class === //
        WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
                           GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                           L"ImGui Example", nullptr };

        ::RegisterClassExW(&wc); // Register the window class

        // === [2] Defining the Desired Window Size === //

        // === [3] Adjusting the Window Size Considering the Border === //
        // Now we account for the border, title bar, and other elements
        RECT windowRect = { 0, 0, windowWidth, windowHeight };
        AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE); // Calculate the correct size

        // === [4] Creating the Window === //
        HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Ledger Project",
            WS_OVERLAPPEDWINDOW, 100, 100,
            windowRect.right - windowRect.left, // Window width considering the border
            windowRect.bottom - windowRect.top, // Window height considering the border
            nullptr, nullptr, wc.hInstance, nullptr);

        // === [5] Initializing the Graphics Device === //
        if (!CreateDeviceD3D(hwnd)) // If the device fails to create
        {
            CleanupDeviceD3D(); // Clean up in case of an error
            ::UnregisterClassW(wc.lpszClassName, wc.hInstance); // Unregister the window class
            return;
        }

        // === [6] Showing and Updating the Window === //
        ::ShowWindow(hwnd, SW_SHOWDEFAULT); // Show the window
        ::UpdateWindow(hwnd); // Update the window after showing it

        // === [7] Initializing ImGui === //
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable keyboard navigation
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable gamepad navigation
        ImGui::StyleColorsDark();                             // Dark theme

        ImGui_ImplWin32_Init(hwnd);   // Win32 initialization
        ImGui_ImplDX9_Init(g_pd3dDevice); // DX9 initialization

        // === [8] Data Initialization === //
#pragma endregion

#pragma region RenderCycle
        // === [7] Main Render Cycle === //
        bool done = false;
        while (!done)
        {
            // [7.1] Handling messages from Windows
            MSG msg;
            while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
                if (msg.message == WM_QUIT)
                    done = true;
            }
            if (done)
                break;

            // [7.2] Handling device loss (e.g., screen resizing or mode change)
            if (g_DeviceLost)
            {
                HRESULT hr = g_pd3dDevice->TestCooperativeLevel();
                if (hr == D3DERR_DEVICELOST)
                {
                    ::Sleep(10); // Wait until the device is recovered
                    continue;
                }
                if (hr == D3DERR_DEVICENOTRESET)
                    ResetDevice(); // Attempt to reset the device
                g_DeviceLost = false;
            }

            // [7.3] Window resizing
            if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
            {
                g_d3dpp.BackBufferWidth = g_ResizeWidth;
                g_d3dpp.BackBufferHeight = g_ResizeHeight;
                g_ResizeWidth = g_ResizeHeight = 0;
                ResetDevice();
            }

            // === [8] Starting ImGui Frame === //
            ImGui_ImplDX9_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            static char password[128];
            SwitchFunc(password);

            // === [10] Ending ImGui Frame === //
            ImGui::EndFrame();

            // Setting Direct3D render states
            g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

            // Clearing the screen with a color
            D3DCOLOR clear_col_dx = D3DCOLOR_RGBA(
                (int)(0.45f * 255.0f),
                (int)(0.55f * 255.0f),
                (int)(0.60f * 255.0f),
                (int)(1.00f * 255.0f));
            g_pd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
                clear_col_dx, 1.0f, 0);

            // Drawing the frame
            if (g_pd3dDevice->BeginScene() >= 0)
            {
                ImGui::Render(); // Generate the final data
                ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData()); // Render it
                g_pd3dDevice->EndScene();
            }

            // Present the frame
            HRESULT result = g_pd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
            if (result == D3DERR_DEVICELOST)
                g_DeviceLost = true;
        }
#pragma endregion

#pragma region FinishRender
        // === [11] ImGui Cleanup + Shutdown === //
        ImGui_ImplDX9_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();

        CleanupDeviceD3D(); // Release Direct3D resources
        ::DestroyWindow(hwnd); // Close the window
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance); // Unregister the window class
#pragma endregion
    }
};

int main(int, char**)
{
    UI_Render ui;  // Create an instance of the UI_Render class
    ui.Update();   // Update the UI

    return 0;      // Exit the program with a success code (0)
}
