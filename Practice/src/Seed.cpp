#include "Seed.h"

// Constructor for SeedList that initializes member variables
SeedList::SeedList(size_t s)
    : head(nullptr), tail(nullptr), size(0), targetSize(s)  // Initialize pointers and size
    , rd(std::random_device{}()), device(0, 1000)             // Random device and distribution
    , sizeList1(12)                                            // Size options for different seed lists (not used in code yet)
    , sizeList2(16)
    , sizeList3(24)
{
}

// Destructor for SeedList that clears the list
SeedList::~SeedList()
{
    clear();  // Calls the clear method to free memory
}

// Generates a random seed phrase of a specified size
bool SeedList::generateSeedPhrase(int size)
{
    clear();  // Clear the current seed list before generating a new one

    // Create a shuffled copy of all available seed words
    std::vector<std::string> shuffled = allSeedWords;
    std::shuffle(shuffled.begin(), shuffled.end(), rd);  // Shuffle the words randomly

    // Select the first 'size' number of words from the shuffled list and add to the seed list
    for (size_t i = 0; i < size; ++i)
    {
        push_back(shuffled[i]);  // Add the shuffled seed word to the list
    }

    return true;  // Return true to indicate success
}

// Adds a new seed word to the end of the seed list
void SeedList::push_back(const std::string& value)
{
    Seed* newSeed = new Seed(value);  // Create a new seed object with the given value

    if (!head)  // If the list is empty
    {
        head = tail = newSeed;  // Set both head and tail to the new seed
    }
    else
    {
        tail->next = newSeed;   // Link the new seed to the current tail
        newSeed->prev = tail;   // Set the previous pointer of the new seed to the current tail
        tail = newSeed;         // Update the tail to the new seed
    }

    ++size;  // Increment the size of the list
}

// Prints the seed phrase (list of seed words) to the console
void SeedList::print()
{
    if (!head) return;  // If the list is empty, do nothing

    Seed* current = head;  // Start from the head of the list
    while (current)  // Traverse the list
    {
        std::cout << current->data << " ";  // Print each seed word
        current = current->next;           // Move to the next seed
    }
    std::cout << std::endl;  // Print a newline after the seed phrase
}

// Clears the entire seed list by deleting all dynamically allocated memory
void SeedList::clear()
{
    Seed* current = head;  // Start from the head of the list
    while (current)  // Traverse the list
    {
        Seed* toDelete = current;  // Store the current seed node
        current = current->next;   // Move to the next seed
        delete toDelete;           // Delete the current seed node to free memory
    }

    head = tail = nullptr;  // Set head and tail to nullptr, indicating an empty list
    size = 0;               // Reset the size to 0
}

// Returns the seed phrase as a vector of strings
std::vector<std::string> SeedList::getSeedPhrase() const
{
    std::vector<std::string> result;  // Vector to store the seed words
    Seed* current = head;  // Start from the head of the list
    while (current)  // Traverse the list
    {
        result.push_back(current->data);  // Add each seed word to the result vector
        current = current->next;          // Move to the next seed
    }
    return result;  // Return the vector containing the seed words
}

// Returns the seed phrase as a single string
std::string SeedList::getStringSeedPhrase()
{
    std::string str_SeedPhrase;  // String to store the seed phrase
    for (const auto& word : getSeedPhrase())  // Get the seed phrase as a vector and iterate through it
    {
        str_SeedPhrase += word + " ";  // Append each seed word to the string
    }
    return str_SeedPhrase;  // Return the seed phrase as a single string
}
