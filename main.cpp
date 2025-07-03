#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include "Transaction.hpp"
#include "ArrayTransactionStore.hpp"
#include "LinkedListTransactionStore.hpp"
#include <chrono>

using namespace std;
using namespace std::chrono;

ArrayTransactionStore cardStore, achStore, upiStore, wireStore;
LinkedListTransactionStore cardLL, achLL, upiLL, wireLL;

// Backup stores for original unsorted data
ArrayTransactionStore cardStoreOriginal, achStoreOriginal, upiStoreOriginal, wireStoreOriginal;
LinkedListTransactionStore cardLLOriginal, achLLOriginal, upiLLOriginal, wireLLOriginal;

#define MAX_TRANSACTIONS 500000
bool isLinkedMode = false;

#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>

// ------------------ Utility Functions ----------------------
void exportStoreToJSON(const string &filename, const ArrayTransactionStore &store)
{
    ofstream out(filename);
    if (!out.is_open())
    {
        cerr << "Failed to open file for JSON export.\n";
        return;
    }

    out << "[\n";
    for (int i = 0; i < store.size(); ++i)
    {
        const Transaction &t = store.get(i);
        out << "  {\n"
            << "    \"transaction_id\": \"" << t.transaction_id << "\",\n"
            << "    \"timestamp\": \"" << t.timestamp << "\",\n"
            << "    \"sender_account\": \"" << t.sender_account << "\",\n"
            << "    \"receiver_account\": \"" << t.receiver_account << "\",\n"
            << "    \"amount\": " << t.amount << ",\n"
            << "    \"transaction_type\": \"" << t.transaction_type << "\",\n"
            << "    \"merchant_category\": \"" << t.merchant_category << "\",\n"
            << "    \"location\": \"" << t.location << "\",\n"
            << "    \"device_used\": \"" << t.device_used << "\",\n"
            << "    \"is_fraud\": " << (t.is_fraud ? "true" : "false") << ",\n"
            << "    \"fraud_type\": \"" << t.fraud_type << "\",\n"
            << "    \"time_since_last_transaction\": \"" << t.time_since_last_transaction << "\",\n"
            << "    \"spending_deviation_score\": \"" << t.spending_deviation_score << "\",\n"
            << "    \"velocity_score\": " << t.velocity_score << ",\n"
            << "    \"geo_anomaly_score\": " << t.geo_anomaly_score << ",\n"
            << "    \"payment_channel\": \"" << t.payment_channel << "\",\n"
            << "    \"ip_address\": \"" << t.ip_address << "\",\n"
            << "    \"device_hash\": \"" << t.device_hash << "\"\n"
            << "  }" << (i < store.size() - 1 ? "," : "") << "\n";
    }
    out << "]\n";

    out.close();
    cout << "Exported to " << filename << "\n";
}

void exportLinkedListToJSON(const string &filename, const LinkedListTransactionStore &store)
{
    ofstream out(filename);
    if (!out.is_open())
    {
        cerr << "Failed to open file for JSON export.\n";
        return;
    }

    out << "[\n";
    ListNode *curr = store.getHead();
    int index = 0;
    int total = store.size();

    while (curr)
    {
        const Transaction &t = curr->data;
        out << "  {\n"
            << "    \"transaction_id\": \"" << t.transaction_id << "\",\n"
            << "    \"timestamp\": \"" << t.timestamp << "\",\n"
            << "    \"sender_account\": \"" << t.sender_account << "\",\n"
            << "    \"receiver_account\": \"" << t.receiver_account << "\",\n"
            << "    \"amount\": " << t.amount << ",\n"
            << "    \"transaction_type\": \"" << t.transaction_type << "\",\n"
            << "    \"merchant_category\": \"" << t.merchant_category << "\",\n"
            << "    \"location\": \"" << t.location << "\",\n"
            << "    \"device_used\": \"" << t.device_used << "\",\n"
            << "    \"is_fraud\": " << (t.is_fraud ? "true" : "false") << ",\n"
            << "    \"fraud_type\": \"" << t.fraud_type << "\",\n"
            << "    \"time_since_last_transaction\": \"" << t.time_since_last_transaction << "\",\n"
            << "    \"spending_deviation_score\": \"" << t.spending_deviation_score << "\",\n"
            << "    \"velocity_score\": " << t.velocity_score << ",\n"
            << "    \"geo_anomaly_score\": " << t.geo_anomaly_score << ",\n"
            << "    \"payment_channel\": \"" << t.payment_channel << "\",\n"
            << "    \"ip_address\": \"" << t.ip_address << "\",\n"
            << "    \"device_hash\": \"" << t.device_hash << "\"\n"
            << "  }" << (index < total - 1 ? "," : "") << "\n";

        curr = curr->next;
        index++;
    }

    out << "]\n";
    out.close();
    cout << "Exported to " << filename << "\n";
}

void printSpaceUsage()
{
    if (!isLinkedMode)
    {
        size_t total1 =
            cardStore.size() * sizeof(Transaction) +
            achStore.size() * sizeof(Transaction) +
            upiStore.size() * sizeof(Transaction) +
            wireStore.size() * sizeof(Transaction);
        cout << "[ARRAY] Estimated Space Usage: " << total1 << " bytes\n";
    }
    else
    {
        size_t total2 =
            cardLL.size() * (sizeof(Transaction) + sizeof(ListNode *)) +
            achLL.size() * (sizeof(Transaction) + sizeof(ListNode *)) +
            upiLL.size() * (sizeof(Transaction) + sizeof(ListNode *)) +
            wireLL.size() * (sizeof(Transaction) + sizeof(ListNode *));
        cout << "[LINKED LIST] Estimated Space Usage: " << total2 << " bytes\n";
    }
}

double getRSSMemoryUsage()
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    {
        return pmc.WorkingSetSize / (1024.0 * 1024.0);
    }
    return 0.0;
}

void printMemoryUsageComparison(double rssBefore, double rssAfter)
{
    cout << fixed << setprecision(2);
    cout << "[RSS] RSS Before: " << rssBefore << " MB\n";
    cout << "[RSS] RSS After: " << rssAfter << " MB\n";
    if (rssAfter > rssBefore)
        cout << "[RSS] Memory Usage: " << (rssAfter - rssBefore) << " MB\n";
    else
        cout << "[RSS] Memory Usage: " << (rssBefore - rssAfter) << " MB\n";
}

void printTransaction(const Transaction &t)
{
    cout << fixed << setprecision(2);
    cout << "ID: " << t.transaction_id
         << " | Location: " << t.location
         << " | Amount: " << t.amount
         << " | Type: " << t.transaction_type
         << " | Fraud: " << (t.is_fraud ? "YES" : "NO")
         << " | Channel: " << t.payment_channel
         << endl;
}

string toLower(const string &str)
{
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void paginateArrayResults(const string &title, const ArrayTransactionStore &store, bool &exitEarly)
{
    int page = 0;
    char nav;
    const int pageSize = 5;

    do
    {
        cout << "\n--- " << title << " | Page " << (page + 1) << " ---\n";
        int start = page * pageSize;
        int end = min(start + pageSize, store.size());

        for (int i = start; i < end; ++i)
        {
            printTransaction(store.get(i));
        }

        if (start >= store.size())
        {
            cout << "No more data.\n";
        }

        cout << "\n[N]ext Page | [P]revious Page | [B]ack (Payment Channel) | [E]xit to Main Menu: ";
        cin >> nav;
        nav = tolower(nav);
        if (nav == 'n')
            page++;
        else if (nav == 'p' && page > 0)
            page--;
        else if (nav == 'e')
        {
            exitEarly = true;
            return;
        }
    } while (nav != 'b');
}

void paginateLinkedListResults(const string &title, const LinkedListTransactionStore &store, bool &exitEarly)
{
    int page = 0;
    char nav;
    const int pageSize = 5;

    do
    {
        cout << "\n--- " << title << " | Page " << (page + 1) << " ---\n";
        int start = page * pageSize;
        int end = start + pageSize;

        int index = 0;
        int shown = 0;
        ListNode *curr = store.getHead();

        while (curr && index < end)
        {
            if (index >= start)
            {
                printTransaction(curr->data);
                shown++;
            }
            curr = curr->next;
            index++;
        }

        if (shown == 0)
            cout << "No more data.\n";

        cout << "\n[N]ext Page | [P]revious Page | [B]ack (Payment Channel) | [E]xit to Main Menu: ";
        cin >> nav;
        nav = tolower(nav);
        if (nav == 'n')
            page++;
        else if (nav == 'p' && page > 0)
            page--;
        else if (nav == 'e')
        {
            exitEarly = true;
            return;
        }
    } while (nav != 'b');
}

// Filtered pagination functions for array search results
void paginateFilteredArrayResults(const string &title, const ArrayTransactionStore &store, const string &searchTermLower, bool &exitEarly, high_resolution_clock::time_point start, const string &searchType)
{
    int page = 0;
    char nav;
    const int pageSize = 5;

    do
    {
        cout << "\n--- " << title << " | Page " << (page + 1) << " ---\n";
        int startIdx = page * pageSize;
        int shown = 0;
        int totalMatched = 0;

        for (int i = 0; i < store.size(); ++i)
        {
            string typeLower = toLower(store.get(i).transaction_type);
            if (typeLower.find(searchTermLower) != string::npos)
                totalMatched++;
        }

        int matchIndex = 0;
        for (int i = 0; i < store.size() && shown < pageSize; ++i)
        {
            string typeLower = toLower(store.get(i).transaction_type);
            if (typeLower.find(searchTermLower) != string::npos)
            {
                if (matchIndex >= startIdx)
                {
                    printTransaction(store.get(i));
                    shown++;
                }
                matchIndex++;
            }
        }

        if (shown == 0)
            cout << "No more results.\n";
        else
            cout << "Showing " << shown << " results (Total matches: " << totalMatched << ")\n";

        if (nav != 'n' && nav != 'p' && nav != 'b')
        {
            auto end = high_resolution_clock::now();
            double rssAfter = getRSSMemoryUsage();
            auto duration = duration_cast<milliseconds>(end - start);
            cout << endl;
            cout << "[INFO] " << searchType << " Search Time: " << duration.count() << " ms\n";
            printSpaceUsage();
            printMemoryUsageComparison(getRSSMemoryUsage(), rssAfter);
        }

        cout << "\n[N]ext Page | [P]revious Page | [B]ack (Payment Channel) | [E]xit to Main Menu: ";
        cin >> nav;
        nav = tolower(nav);
        if (nav == 'n')
            page++;
        else if (nav == 'p' && page > 0)
            page--;
        else if (nav == 'e')
        {
            exitEarly = true;
            return;
        }
    } while (nav != 'b');
}

// Filtered pagination functions for linked list search results
void paginateFilteredLinkedListResults(const string &title, const LinkedListTransactionStore &store, const string &searchTermLower, bool &exitEarly, high_resolution_clock::time_point start, const string &searchType)
{
    int page = 0;
    char nav;
    const int pageSize = 5;

    do
    {
        cout << "\n--- " << title << " | Page " << (page + 1) << " ---\n";
        int startIdx = page * pageSize;
        int shown = 0;
        int totalMatched = 0;

        ListNode *curr = store.getHead();
        while (curr)
        {
            string typeLower = toLower(curr->data.transaction_type);
            if (typeLower.find(searchTermLower) != string::npos)
                totalMatched++;
            curr = curr->next;
        }

        int matchIndex = 0;
        curr = store.getHead();
        while (curr && shown < pageSize)
        {
            string typeLower = toLower(curr->data.transaction_type);
            if (typeLower.find(searchTermLower) != string::npos)
            {
                if (matchIndex >= startIdx)
                {
                    printTransaction(curr->data);
                    shown++;
                }
                matchIndex++;
            }
            curr = curr->next;
        }

        if (shown == 0)
            cout << "No more results.\n";
        else
            cout << "Showing " << shown << " results (Total matches: " << totalMatched << ")\n";

        if (nav != 'n' && nav != 'p' && nav != 'b')
        {
            auto end = high_resolution_clock::now();
            double rssAfter = getRSSMemoryUsage();
            auto duration = duration_cast<milliseconds>(end - start);
            cout << endl;
            cout << "[INFO] " << searchType << " Search Time: " << duration.count() << " ms\n";
            printSpaceUsage();
            printMemoryUsageComparison(getRSSMemoryUsage(), rssAfter);
        }

        cout << "\n[N]ext Page | [P]revious Page | [B]ack (Payment Channel) | [E]xit to Main Menu: ";
        cin >> nav;
        nav = tolower(nav);
        if (nav == 'n')
            page++;
        else if (nav == 'p' && page > 0)
            page--;
        else if (nav == 'e')
        {
            exitEarly = true;
            return;
        }
    } while (nav != 'b');
}

// ------------------ LOAD & PARSE ----------------------
Transaction parseTransaction(const string &line)
{
    stringstream ss(line);
    string cell;
    Transaction t;

    getline(ss, t.transaction_id, ',');
    if (t.transaction_id.empty())
        t.transaction_id = "null";

    getline(ss, t.timestamp, ',');
    if (t.timestamp.empty())
        t.timestamp = "null";

    getline(ss, t.sender_account, ',');
    if (t.sender_account.empty())
        t.sender_account = "null";

    getline(ss, t.receiver_account, ',');
    if (t.receiver_account.empty())
        t.receiver_account = "null";

    getline(ss, cell, ',');
    t.amount = cell.empty() ? 0.0 : stod(cell);

    getline(ss, t.transaction_type, ',');
    if (t.transaction_type.empty())
    {
        t.transaction_type = "null";
    }
    else
    {
        t.transaction_type = toLower(t.transaction_type);
    }

    getline(ss, t.merchant_category, ',');
    if (t.merchant_category.empty())
        t.merchant_category = "null";

    getline(ss, t.location, ',');
    if (t.location.empty())
        t.location = "null";

    getline(ss, t.device_used, ',');
    if (t.device_used.empty())
        t.device_used = "null";

    getline(ss, cell, ',');
    if (cell.empty())
    {
        t.is_fraud = false;
    }
    else
    {
        for (char &c : cell)
            c = tolower(c);
        t.is_fraud = (cell == "true");
    }

    getline(ss, t.fraud_type, ',');
    if (t.fraud_type.empty())
        t.fraud_type = "null";

    getline(ss, t.time_since_last_transaction, ',');
    if (t.time_since_last_transaction.empty())
        t.time_since_last_transaction = "null";

    getline(ss, t.spending_deviation_score, ',');
    if (t.spending_deviation_score.empty())
        t.spending_deviation_score = "null";

    getline(ss, cell, ',');
    t.velocity_score = cell.empty() ? 0.0 : stod(cell);

    getline(ss, cell, ',');
    t.geo_anomaly_score = cell.empty() ? 0.0 : stod(cell);

    getline(ss, t.payment_channel, ',');
    if (t.payment_channel.empty())
    {
        t.payment_channel = "null";
    }
    else
    {
        t.payment_channel = toLower(t.payment_channel);
    }

    getline(ss, t.ip_address, ',');
    if (t.ip_address.empty())
        t.ip_address = "null";

    getline(ss, t.device_hash, ',');
    if (t.device_hash.empty())
        t.device_hash = "null";

    return t;
}

void loadData(const string &filename)
{
    ifstream file(filename);
    if (!file.is_open())
    {
        cerr << "Error opening file.\n";
        return;
    }

    string line;
    getline(file, line);

    cardStore.clear();
    achStore.clear();
    upiStore.clear();
    wireStore.clear();
    cardLL.clear();
    achLL.clear();
    upiLL.clear();
    wireLL.clear();

    // Clear backup stores
    cardStoreOriginal.clear();
    achStoreOriginal.clear();
    upiStoreOriginal.clear();
    wireStoreOriginal.clear();
    cardLLOriginal.clear();
    achLLOriginal.clear();
    upiLLOriginal.clear();
    wireLLOriginal.clear();

    int totalTransactionsLoaded = 0;

    while (getline(file, line) && totalTransactionsLoaded < MAX_TRANSACTIONS)
    {
        if (line.empty() || count(line.begin(), line.end(), ',') < 17)
            continue;

        Transaction t = parseTransaction(line);

        const string &channel = t.payment_channel;

        if (channel == "null")
        {
            continue;
        }

        if (isLinkedMode)
        {
            if (channel == "card")
            {
                cardLL.add(t);
                cardLLOriginal.add(t);
                totalTransactionsLoaded++;
            }
            else if (channel == "ach")
            {
                achLL.add(t);
                achLLOriginal.add(t);
                totalTransactionsLoaded++;
            }
            else if (channel == "upi")
            {
                upiLL.add(t);
                upiLLOriginal.add(t);
                totalTransactionsLoaded++;
            }
            else if (channel == "wire_transfer")
            {
                wireLL.add(t);
                wireLLOriginal.add(t);
                totalTransactionsLoaded++;
            }
        }
        else
        {
            if (channel == "card")
            {
                cardStore.add(t);
                cardStoreOriginal.add(t);
                totalTransactionsLoaded++;
            }
            else if (channel == "ach")
            {
                achStore.add(t);
                achStoreOriginal.add(t);
                totalTransactionsLoaded++;
            }
            else if (channel == "upi")
            {
                upiStore.add(t);
                upiStoreOriginal.add(t);
                totalTransactionsLoaded++;
            }
            else if (channel == "wire_transfer")
            {
                wireStore.add(t);
                wireStoreOriginal.add(t);
                totalTransactionsLoaded++;
            }
        }
    }

    file.close();

    cout << "\nLoaded Transactions (Total: " << totalTransactionsLoaded << "):\n";
    if (isLinkedMode)
    {
        cout << "Card: " << cardLL.size()
             << " | ACH: " << achLL.size()
             << " | UPI: " << upiLL.size()
             << " | Wire Transfer: " << wireLL.size() << endl;
    }
    else
    {
        cout << "Card: " << cardStore.size()
             << " | ACH: " << achStore.size()
             << " | UPI: " << upiStore.size()
             << " | Wire Transfer: " << wireStore.size() << endl;
    }
}

void searchByTransactionType(const string &searchTypeLower, int page)
{
    int shown = 0, startIdx = page * 5;
    bool foundAny = false;

    if (!isLinkedMode)
    {
        ArrayTransactionStore stores[] = {cardStoreOriginal, achStoreOriginal, upiStoreOriginal, wireStoreOriginal};
        for (auto &store : stores)
        {
            for (int i = 0; i < store.size(); i++)
            {
                string typeLower = toLower(store.get(i).transaction_type);
                if (typeLower.find(searchTypeLower) != string::npos)
                {
                    if (shown >= startIdx && shown < startIdx + 5)
                        printTransaction(store.get(i));
                    shown++;
                    foundAny = true;
                }
            }
        }
    }
    else
    {
        LinkedListTransactionStore stores[] = {cardLLOriginal, achLLOriginal, upiLLOriginal, wireLLOriginal};
        for (auto &store : stores)
        {
            ListNode *curr = store.getHead();
            while (curr)
            {
                string typeLower = toLower(curr->data.transaction_type);
                if (typeLower.find(searchTypeLower) != string::npos)
                {
                    if (shown >= startIdx && shown < startIdx + 5)
                        printTransaction(curr->data);
                    shown++;
                    foundAny = true;
                }
                curr = curr->next;
            }
        }
    }

    if (!foundAny)
        cout << "No results found.\n";
    else if (shown <= startIdx)
        cout << "No more results.\n";
}

// ---------------- BINARY SEARCH FOR ARRAY ----------------
int binarySearchTransactionType(const ArrayTransactionStore &store, const string &searchTermLower)
{
    int low = 0, high = store.size() - 1;
    while (low <= high)
    {
        int mid = (low + high) / 2;
        string midType = toLower(store.get(mid).transaction_type);

        if (midType == searchTermLower)
            return mid;
        else if (midType < searchTermLower)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}

// ---------------- BINARY SEARCH FOR LINKED LIST ----------------
ListNode *getNodeAtIndex(ListNode *head, int index)
{
    while (index-- && head)
        head = head->next;
    return head;
}

ListNode *binarySearchTransactionType(LinkedListTransactionStore &store, const string &searchTermLower)
{
    int low = 0, high = store.size() - 1;
    while (low <= high)
    {
        int mid = (low + high) / 2;
        ListNode *midNode = getNodeAtIndex(store.getHead(), mid);
        if (!midNode)
            break;

        string midType = toLower(midNode->data.transaction_type);

        if (midType == searchTermLower)
            return midNode;
        else if (midType < searchTermLower)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return nullptr;
}

void handleSearchMenu()
{
    int choice;
    do
    {
        cout << "\n========= SEARCH MENU =========\n";
        cout << "1. Linear Search by Transaction Type\n";
        cout << "2. Binary Search by Transaction Type (After Sorted)\n";
        cout << "3. Back to Main Menu\n";
        cout << "Choose an option: ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }

        if (choice == 3)
            return;

        if (choice == 2)
        {
            cout << "Enter Transaction Type (case-insensitive): ";
            cin.ignore();
            string searchTerm;
            getline(cin, searchTerm);
            string searchTermLower = toLower(searchTerm);

            double rssBefore = getRSSMemoryUsage();
            auto start = high_resolution_clock::now();
            bool found = false;
            bool exitEarly = false;
            if (!isLinkedMode)
            {
                ArrayTransactionStore *stores[] = {&cardStore, &achStore, &upiStore, &wireStore};
                string storeNames[] = {"Card Transactions", "ACH Transactions", "UPI Transactions", "Wire Transactions"};
                for (int i = 0; i < 4 && !exitEarly; ++i)
                {
                    int index = binarySearchTransactionType(*stores[i], searchTermLower);
                    if (index != -1)
                    {
                        found = true;
                    }
                }
            }
            else
            {
                LinkedListTransactionStore *stores[] = {&cardLL, &achLL, &upiLL, &wireLL};
                string storeNames[] = {"Card Transactions", "ACH Transactions", "UPI Transactions", "Wire Transactions"};
                for (int i = 0; i < 4 && !exitEarly; ++i)
                {
                    ListNode *node = binarySearchTransactionType(*stores[i], searchTermLower);
                    if (node)
                    {
                        found = true;
                    }
                }
            }

            if (!isLinkedMode)
            {
                ArrayTransactionStore *stores[] = {&cardStore, &achStore, &upiStore, &wireStore};
                string storeNames[] = {"Card Transactions", "ACH Transactions", "UPI Transactions", "Wire Transactions"};
                for (int i = 0; i < 4 && !exitEarly; ++i)
                {
                    int index = binarySearchTransactionType(*stores[i], searchTermLower);
                    if (index != -1)
                    {
                        paginateFilteredArrayResults(storeNames[i], *stores[i], searchTermLower, exitEarly, start, "Binary");
                    }
                }
            }
            else
            {
                LinkedListTransactionStore *stores[] = {&cardLL, &achLL, &upiLL, &wireLL};
                string storeNames[] = {"Card Transactions", "ACH Transactions", "UPI Transactions", "Wire Transactions"};
                for (int i = 0; i < 4 && !exitEarly; ++i)
                {
                    ListNode *node = binarySearchTransactionType(*stores[i], searchTermLower);
                    if (node)
                    {
                        paginateFilteredLinkedListResults(storeNames[i], *stores[i], searchTermLower, exitEarly, start, "Binary");
                    }
                }
            }
            if (!found)
            {
                cout << "No results found.\n";
            }
        }
        else if (choice == 1)
        {
            cout << "Enter Transaction Type (case-insensitive): ";
            cin.ignore();
            string searchTerm;
            getline(cin, searchTerm);
            string searchTermLower = toLower(searchTerm);

            double rssBefore = getRSSMemoryUsage();
            auto start = high_resolution_clock::now();
            bool found = false;
            if (!isLinkedMode)
            {
                ArrayTransactionStore *stores[] = {&cardStoreOriginal, &achStoreOriginal, &upiStoreOriginal, &wireStoreOriginal};
                for (int s = 0; s < 4 && !found; ++s)
                {
                    for (int i = 0; i < stores[s]->size(); ++i)
                    {
                        if (toLower(stores[s]->get(i).transaction_type).find(searchTermLower) != string::npos)
                        {
                            found = true;
                            break;
                        }
                    }
                }
            }
            else
            {
                LinkedListTransactionStore *stores[] = {&cardLLOriginal, &achLLOriginal, &upiLLOriginal, &wireLLOriginal};
                for (int s = 0; s < 4 && !found; ++s)
                {
                    ListNode *curr = stores[s]->getHead();
                    while (curr && !found)
                    {
                        if (toLower(curr->data.transaction_type).find(searchTermLower) != string::npos)
                        {
                            found = true;
                            break;
                        }
                        curr = curr->next;
                    }
                }
            }

            if (found)
            {
                bool exitEarly = false;
                if (!isLinkedMode)
                {
                    ArrayTransactionStore *stores[] = {&cardStoreOriginal, &achStoreOriginal, &upiStoreOriginal, &wireStoreOriginal};
                    string storeNames[] = {"Card Transactions", "ACH Transactions", "UPI Transactions", "Wire Transactions"};
                    for (int i = 0; i < 4 && !exitEarly; ++i)
                    {
                        bool hasMatches = false;
                        for (int j = 0; j < stores[i]->size(); ++j)
                        {
                            if (toLower(stores[i]->get(j).transaction_type).find(searchTermLower) != string::npos)
                            {
                                hasMatches = true;
                                break;
                            }
                        }
                        if (hasMatches)
                        {
                            paginateFilteredArrayResults(storeNames[i], *stores[i], searchTermLower, exitEarly, start, "Linear");
                        }
                    }
                }
                else
                {
                    LinkedListTransactionStore *stores[] = {&cardLLOriginal, &achLLOriginal, &upiLLOriginal, &wireLLOriginal};
                    string storeNames[] = {"Card Transactions", "ACH Transactions", "UPI Transactions", "Wire Transactions"};
                    for (int i = 0; i < 4 && !exitEarly; ++i)
                    {
                        bool hasMatches = false;
                        ListNode *curr = stores[i]->getHead();
                        while (curr && !hasMatches)
                        {
                            if (toLower(curr->data.transaction_type).find(searchTermLower) != string::npos)
                            {
                                hasMatches = true;
                                break;
                            }
                            curr = curr->next;
                        }
                        if (hasMatches)
                        {
                            paginateFilteredLinkedListResults(storeNames[i], *stores[i], searchTermLower, exitEarly, start, "Linear");
                        }
                    }
                }
            }
            else
            {
                cout << "No results found.\n";
            }
        }
        else
        {
            cout << "Invalid choice. Please try again.\n";
        }

    } while (true);
}

// ---------------- BUCKET SORT FOR ARRAY ----------------
void bucketSortByLocation(ArrayTransactionStore &store, bool reverse = false)
{
    int n = store.size();
    if (n == 0)
        return;

    string *uniqueLocations = new string[n];
    int uniqueCount = 0;
    for (int i = 0; i < n; ++i)
    {
        const string &loc = store.getRef(i).location;
        bool found = false;
        for (int j = 0; j < uniqueCount; ++j)
        {
            if (uniqueLocations[j] == loc)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            uniqueLocations[uniqueCount++] = loc;
        }
    }

    for (int i = 0; i < uniqueCount - 1; ++i)
    {
        int target = i;
        for (int j = i + 1; j < uniqueCount; ++j)
        {
            if (!reverse)
            {
                if (uniqueLocations[j] < uniqueLocations[target])
                    target = j;
            }
            else
            {
                if (uniqueLocations[j] > uniqueLocations[target])
                    target = j;
            }
        }
        if (target != i)
        {
            string tmp = uniqueLocations[i];
            uniqueLocations[i] = uniqueLocations[target];
            uniqueLocations[target] = tmp;
        }
    }

    ArrayTransactionStore *buckets = new ArrayTransactionStore[uniqueCount];
    for (int i = 0; i < n; ++i)
    {
        const string &loc = store.getRef(i).location;
        for (int j = 0; j < uniqueCount; j++)
        {
            if (uniqueLocations[j] == loc)
            {
                buckets[j].add(store.getRef(i));
                break;
            }
        }
    }

    store.clear();
    for (int i = 0; i < uniqueCount; ++i)
    {
        for (int j = 0; j < buckets[i].size(); ++j)
        {
            store.add(buckets[i].get(j));
        }
    }
    delete[] buckets;
    delete[] uniqueLocations;
}

// ---------------- BUCKET SORT FOR LINKED LIST ----------------
void bucketSortByLocation(LinkedListTransactionStore &store, bool reverse = false)
{
    int n = store.size();
    if (n == 0)
        return;

    string *uniqueLocations = new string[n];
    int uniqueCount = 0;
    ListNode *curr = store.getHead();
    while (curr)
    {
        const string &loc = curr->data.location;
        bool found = false;
        for (int j = 0; j < uniqueCount; ++j)
        {
            if (uniqueLocations[j] == loc)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            uniqueLocations[uniqueCount++] = loc;
        }
        curr = curr->next;
    }

    for (int i = 0; i < uniqueCount - 1; ++i)
    {
        int target = i;
        for (int j = i + 1; j < uniqueCount; ++j)
        {
            if (!reverse)
            {
                if (uniqueLocations[j] < uniqueLocations[target])
                    target = j;
            }
            else
            {
                if (uniqueLocations[j] > uniqueLocations[target])
                    target = j;
            }
        }
        if (target != i)
        {
            string tmp = uniqueLocations[i];
            uniqueLocations[i] = uniqueLocations[target];
            uniqueLocations[target] = tmp;
        }
    }

    LinkedListTransactionStore *buckets = new LinkedListTransactionStore[uniqueCount];
    curr = store.getHead();
    while (curr)
    {
        const string &loc = curr->data.location;
        for (int j = 0; j < uniqueCount; ++j)
        {
            if (uniqueLocations[j] == loc)
            {
                buckets[j].add(curr->data);
                break;
            }
        }
        curr = curr->next;
    }

    store.clear();
    for (int i = 0; i < uniqueCount; ++i)
    {
        ListNode *node = buckets[i].getHead();
        while (node)
        {
            store.add(node->data);
            node = node->next;
        }
    }
    delete[] buckets;
    delete[] uniqueLocations;
}

// ---------------- QUICK SORT FOR ARRAY ----------------
int partition(ArrayTransactionStore &store, int low, int high, bool ascending)
{
    const string &pivotLocation = store.getRef(high).location;
    int i = low - 1;

    for (int j = low; j < high; ++j)
    {
        bool condition = ascending
                             ? store.getRef(j).location < pivotLocation
                             : store.getRef(j).location > pivotLocation;

        if (condition)
        {
            i++;
            store.swap(i, j);
        }
    }

    store.swap(i + 1, high);
    return i + 1;
}

void quickSort(ArrayTransactionStore &store, int low, int high, bool ascending = true)
{
    if (low < high)
    {
        int pi = partition(store, low, high, ascending);
        quickSort(store, low, pi - 1, ascending);
        quickSort(store, pi + 1, high, ascending);
    }
}

// ---------------- QUICK SORT FOR LINKED LIST ----------------
ListNode *quickSortSimple(ListNode *head, bool ascending)
{
    if (!head || !head->next)
        return head;

    ListNode *pivot = head;
    ListNode *smallerHead = nullptr;
    ListNode *greaterHead = nullptr;
    ListNode *current = head->next;

    while (current)
    {
        ListNode *next = current->next;

        bool goToSmaller = ascending
                               ? current->data.location < pivot->data.location
                               : current->data.location > pivot->data.location;

        if (goToSmaller)
        {
            current->next = smallerHead;
            smallerHead = current;
        }
        else
        {
            current->next = greaterHead;
            greaterHead = current;
        }

        current = next;
    }

    smallerHead = quickSortSimple(smallerHead, ascending);
    greaterHead = quickSortSimple(greaterHead, ascending);

    pivot->next = greaterHead;

    if (smallerHead)
    {
        ListNode *tail = smallerHead;
        while (tail->next)
            tail = tail->next;
        tail->next = pivot;
        return smallerHead;
    }
    else
    {
        return pivot;
    }
}

void quickSort(LinkedListTransactionStore &store, bool ascending = true)
{
    ListNode *sorted = quickSortSimple(store.getHead(), ascending);
    store.setHead(sorted);
}

// ------------------ SORT MENU ----------------------
void handleSortMenu()
{
    int choice;
    do
    {
        cout << "\n========= SORT MENU =========\n";
        cout << "1. Bucket Sort by Location (A-Z)\n";
        cout << "2. Bucket Sort by Location (Z-A)\n";
        cout << "3. Quick Sort by Location (A-Z)\n";
        cout << "4. Quick Sort by Location (Z-A)\n";
        cout << "5. Back to Main Menu\n";
        cout << "Choose an option: ";
        cin >> choice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }

        if (choice == 5)
            return;

        bool isQuickSort = (choice == 3 || choice == 4);
        bool reverse = (choice == 2 || choice == 4);

        if (!isLinkedMode)
        {
            auto start = high_resolution_clock::now();

            double rssBefore = 0.0, rssAfter = 0.0;
            if (isQuickSort)
            {
                rssBefore = getRSSMemoryUsage();
                quickSort(cardStore, 0, cardStore.size() - 1, !reverse);
                quickSort(achStore, 0, achStore.size() - 1, !reverse);
                quickSort(upiStore, 0, upiStore.size() - 1, !reverse);
                quickSort(wireStore, 0, wireStore.size() - 1, !reverse);
                rssAfter = getRSSMemoryUsage();
            }
            else
            {
                rssBefore = getRSSMemoryUsage();
                bucketSortByLocation(cardStore, reverse);
                bucketSortByLocation(achStore, reverse);
                bucketSortByLocation(upiStore, reverse);
                bucketSortByLocation(wireStore, reverse);
                rssAfter = getRSSMemoryUsage();
            }

            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);

            cout << "\n[ARRAY] ";
            if (isQuickSort)
                cout << "Quick Sort";
            else
                cout << "Bucket Sort";
            cout << " Time: " << duration.count() << " ms\n";
            printSpaceUsage();
            printMemoryUsageComparison(rssBefore, rssAfter);

            bool exitEarly = false;
            paginateArrayResults("Card Transactions", cardStore, exitEarly);
            if (exitEarly)
                return;
            paginateArrayResults("ACH Transactions", achStore, exitEarly);
            if (exitEarly)
                return;
            paginateArrayResults("UPI Transactions", upiStore, exitEarly);
            if (exitEarly)
                return;
            paginateArrayResults("Wire Transactions", wireStore, exitEarly);
        }
        else
        {
            auto start = high_resolution_clock::now();

            double rssBefore = 0.0, rssAfter = 0.0;
            if (isQuickSort)
            {
                rssBefore = getRSSMemoryUsage();
                quickSort(cardLL, !reverse);
                quickSort(achLL, !reverse);
                quickSort(upiLL, !reverse);
                quickSort(wireLL, !reverse);
                rssAfter = getRSSMemoryUsage();
            }
            else
            {
                rssBefore = getRSSMemoryUsage();
                bucketSortByLocation(cardLL, reverse);
                bucketSortByLocation(achLL, reverse);
                bucketSortByLocation(upiLL, reverse);
                bucketSortByLocation(wireLL, reverse);
                rssAfter = getRSSMemoryUsage();
            }
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);

            cout << "\n[LINKED LIST] ";
            if (isQuickSort)
                cout << "Quick Sort";
            else
                cout << "Bucket Sort";
            cout << " Time: " << duration.count() << " ms\n";
            printSpaceUsage();
            printMemoryUsageComparison(rssBefore, rssAfter);

            bool exitEarly = false;
            paginateLinkedListResults("Card Transactions", cardLL, exitEarly);
            if (exitEarly)
                return;
            paginateLinkedListResults("ACH Transactions", achLL, exitEarly);
            if (exitEarly)
                return;
            paginateLinkedListResults("UPI Transactions", upiLL, exitEarly);
            if (exitEarly)
                return;
            paginateLinkedListResults("Wire Transactions", wireLL, exitEarly);
        }

    } while (true);
}

// --------------- Main Menu --------------------
void displayMainMenu()
{
    cout << "\n========= MAIN MENU =========\n";
    cout << "1. Search\n";
    cout << "2. Sort\n";
    cout << "3. Export all to JSON\n";
    cout << "4. Exit\n";
    cout << "Choose an option: ";
}

int main()
{
    int mode;
    cout << "-------------------------------\n";
    cout << "|   Choose mode:              |\n";
    cout << "|   1 = Array                 |\n";
    cout << "|   2 = Linked List           |\n";
    cout << "-------------------------------\n";
    cout << "Enter your choice: ";
    cin >> mode;
    isLinkedMode = (mode == 2);

    loadData("financial_fraud_detection.csv");

    int mainChoice;
    do
    {
        displayMainMenu();
        cin >> mainChoice;

        if (cin.fail())
        {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }

        switch (mainChoice)
        {
        case 1:
            handleSearchMenu();
            break;
        case 2:
            handleSortMenu();
            break;
        case 3:
            if (isLinkedMode)
            {
                exportLinkedListToJSON("linked_card.json", cardLL);
                exportLinkedListToJSON("linked_ach.json", achLL);
                exportLinkedListToJSON("linked_upi.json", upiLL);
                exportLinkedListToJSON("linked_wire.json", wireLL);
            }
            else
            {
                exportStoreToJSON("array_card.json", cardStore);
                exportStoreToJSON("array_ach.json", achStore);
                exportStoreToJSON("array_upi.json", upiStore);
                exportStoreToJSON("array_wire.json", wireStore);
            }
            break;
        case 4:
            cout << "Exiting program.\n";
            break;
        default:
            cout << "Invalid choice. Try again.\n";
        }
    } while (mainChoice != 4);

    return 0;
}
