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

ArrayTransactionStore cardStore, achStore, upiStore, wireStore;
LinkedListTransactionStore cardLL, achLL, upiLL, wireLL;

using namespace std;
using namespace std::chrono;

static const std::string CITIES[] = {
    "Berlin", "Dubai", "London", "New York", "Singapore",
    "Sydney", "Tokyo", "Toronto"
};
constexpr int NUM_BUCKETS = sizeof(CITIES) / sizeof(CITIES[0]);


#define MAX_TRANSACTIONS 1000
bool isLinkedMode = false;

// ------------------ Utility Functions ----------------------

void exportStoreToJSON(const string& filename, const ArrayTransactionStore& store) {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Failed to open file for JSON export.\n";
        return;
    }

    out << "[\n";
    for (int i = 0; i < store.size(); ++i) {
        const Transaction& t = store.get(i);
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

void exportLinkedListToJSON(const std::string& filename, const LinkedListTransactionStore& store) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for JSON export.\n";
        return;
    }

    out << "[\n";
    ListNode* curr = store.getHead();
    int index = 0;
    int total = store.size();

    while (curr) {
        const Transaction& t = curr->data;
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
    std::cout << "Exported to " << filename << "\n";
}


void printMemoryUsage() {
    if (!isLinkedMode) {
        size_t total =
            cardStore.size() * sizeof(Transaction) +
            achStore.size() * sizeof(Transaction) +
            upiStore.size() * sizeof(Transaction) +
            wireStore.size() * sizeof(Transaction);
        cout << "[ARRAY] Estimated Memory Usage: " << total << " bytes\n";
    } else {
        size_t total =
            cardLL.size() * (sizeof(Transaction) + sizeof(ListNode*)) +
            achLL.size() * (sizeof(Transaction) + sizeof(ListNode*)) +
            upiLL.size() * (sizeof(Transaction) + sizeof(ListNode*)) +
            wireLL.size() * (sizeof(Transaction) + sizeof(ListNode*));
        cout << "[LINKED LIST] Estimated Memory Usage: " << total << " bytes\n";
    }
}

void printTransaction(const Transaction& t) {
    cout << fixed << setprecision(2);
    cout << "ID: " << t.transaction_id
         << " | Location: " << t.location
         << " | Amount: " << t.amount
         << " | Type: " << t.transaction_type
         << " | Fraud: " << (t.is_fraud ? "YES" : "NO")
         << " | Channel: " << t.payment_channel
         << endl;
}

string toLower(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void paginateArrayResults(const string& title, const ArrayTransactionStore& store,bool& exitEarly) {
    int page = 0;
    char nav;
    const int pageSize = 5;

    do {
        cout << "\n--- " << title << " | Page " << (page + 1) << " ---\n";
        int start = page * pageSize;
        int end = min(start + pageSize, store.size());

        for (int i = start; i < end; ++i) {
            printTransaction(store.get(i));
        }

        if (start >= store.size()) {
            cout << "No more data.\n";
        }

        cout << "\n[N]ext Page | [P]revious Page | [B]ack | [E]xit to Main Menu: ";
    cin >> nav;
    nav = tolower(nav);
    if (nav == 'n') page++;
    else if (nav == 'p' && page > 0) page--;
    else if (nav == 'e') {
        exitEarly = true;
        return;
    }
    } while (nav != 'b');
}


void paginateLinkedListResults(const string& title, const LinkedListTransactionStore& store, bool& exitEarly) {
    int page = 0;
    char nav;
    const int pageSize = 5;

    do {
        cout << "\n--- " << title << " | Page " << (page + 1) << " ---\n";
        int start = page * pageSize;
        int end = start + pageSize;

        int index = 0;
        int shown = 0;
        ListNode* curr = store.getHead();

        while (curr && index < end) {
            if (index >= start) {
                printTransaction(curr->data);
                shown++;
            }
            curr = curr->next;
            index++;
        }

        if (shown == 0)
            cout << "No more data.\n";

        cout << "\n[N]ext Page | [P]revious Page | [B]ack | [E]xit to Main Menu: ";
    cin >> nav;
    nav = tolower(nav);
    if (nav == 'n') page++;
    else if (nav == 'p' && page > 0) page--;
    else if (nav == 'e') {
        exitEarly = true;
        return;
    }
    } while (nav != 'b');
}

// ------------------ LOAD & PARSE ----------------------
Transaction parseTransaction(const string& line) {
    stringstream ss(line);
    string cell;
    Transaction t;

    getline(ss, t.transaction_id, ',');
    getline(ss, t.timestamp, ',');
    getline(ss, t.sender_account, ',');
    getline(ss, t.receiver_account, ',');
    
    getline(ss, cell, ',');
    t.amount = cell.empty() ? 0.0 : stod(cell);

    getline(ss, t.transaction_type, ',');
    t.transaction_type = toLower(t.transaction_type);

    getline(ss, t.merchant_category, ',');
    getline(ss, t.location, ',');
    getline(ss, t.device_used, ',');

    getline(ss, cell, ',');
    for (char& c : cell) c = tolower(c);
    t.is_fraud = (cell == "true");

    getline(ss, t.fraud_type, ',');
    getline(ss, t.time_since_last_transaction, ',');
    getline(ss, t.spending_deviation_score, ',');

    getline(ss, cell, ',');
    t.velocity_score = cell.empty() ? 0.0 : stod(cell);

    getline(ss, cell, ',');
    t.geo_anomaly_score = cell.empty() ? 0.0 : stod(cell);

    getline(ss, t.payment_channel, ',');
    t.payment_channel = toLower(t.payment_channel);

    getline(ss, t.ip_address, ',');
    getline(ss, t.device_hash, ',');

    return t;
}


void loadData(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file.\n";
        return;
    }

    string line;
    getline(file, line);

    cardStore.clear(); achStore.clear(); upiStore.clear(); wireStore.clear();
    cardLL.clear(); achLL.clear(); upiLL.clear(); wireLL.clear();

    while (getline(file, line)) {
        if (line.empty() || count(line.begin(), line.end(), ',') < 17)
            continue;

        Transaction t = parseTransaction(line); 

        const string& channel = t.payment_channel; 

        if (isLinkedMode) {
    if (channel == "card" && cardLL.size() < MAX_TRANSACTIONS) cardLL.add(t);
    else if (channel == "ach" && achLL.size() < MAX_TRANSACTIONS) achLL.add(t);
    else if (channel == "upi" && upiLL.size() < MAX_TRANSACTIONS) upiLL.add(t);
    else if (channel == "wire_transfer" && wireLL.size() < MAX_TRANSACTIONS) wireLL.add(t);
} else {
            if (channel == "card") cardStore.add(t);
            else if (channel == "ach") achStore.add(t);
            else if (channel == "upi") upiStore.add(t);
            else if (channel == "wire_transfer") wireStore.add(t);
        }
    }

    file.close();

    cout << "\nLoaded Transactions:\n";
    if (isLinkedMode) {
        cout << "Card: " << cardLL.size()
             << " | ACH: " << achLL.size()
             << " | UPI: " << upiLL.size()
             << " | Wire Transfer: " << wireLL.size() << endl;
    } else {
        cout << "Card: " << cardStore.size()
             << " | ACH: " << achStore.size()
             << " | UPI: " << upiStore.size()
             << " | Wire Transfer: " << wireStore.size() << endl;
    }
}



void showFirst5(const ArrayTransactionStore& store) {
    for (int i = 0; i < 5 && i < store.size(); ++i)
        printTransaction(store.get(i));
}

void searchByTransactionType(const string& searchTypeLower, int page) {
    int shown = 0, startIdx = page * 10;
    bool foundAny = false;

    if (!isLinkedMode) {
        ArrayTransactionStore* stores[] = { &cardStore, &achStore, &upiStore, &wireStore };
        for (auto& store : stores) {
            for (int i = 0; i < store->size(); i++) {
                string typeLower = toLower(store->get(i).transaction_type);
                if (typeLower.find(searchTypeLower) != string::npos) {
                    if (shown >= startIdx && shown < startIdx + 10)
                        printTransaction(store->get(i));
                    shown++;
                    foundAny = true;
                }
            }
        }
    } else {
        LinkedListTransactionStore* stores[] = { &cardLL, &achLL, &upiLL, &wireLL };
        for (auto& store : stores) {
            ListNode* curr = store->getHead();
            while (curr) {
                string typeLower = toLower(curr->data.transaction_type);
                if (typeLower.find(searchTypeLower) != string::npos) {
                    if (shown >= startIdx && shown < startIdx + 10)
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


void handleSearchMenu() {
    int choice;
    do {
        cout << "\n========= SEARCH MENU =========\n";
        cout << "1. Search by Transaction Type\n";
        cout << "2. Back to Main Menu\n";
        cout << "Choose an option: ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear(); cin.ignore(10000, '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }

        if (choice == 2) return;

        if (choice == 1) {
            cout << "Enter Transaction Type (case-insensitive): ";
            cin.ignore();
            string searchTerm;
            getline(cin, searchTerm);
            string searchTermLower = toLower(searchTerm);

            int page = 0;
            char nav;
            do {
                searchByTransactionType(searchTermLower, page);
                cout << "\nPage " << (page + 1) << " | [N]ext Page | [P]revious Page | [B]ack: ";
                cin >> nav;
                nav = tolower(nav);
                if (nav == 'n') page++;
                else if (nav == 'p' && page > 0) page--;
            } while (nav != 'b');
        } else {
            cout << "Invalid choice. Try again.\n";
        }
    } while (true);
}

void bucketSortByLocation(ArrayTransactionStore& store, bool reverse = false) {
    ArrayTransactionStore* buckets = new ArrayTransactionStore[NUM_BUCKETS];

    for (int i = 0; i < store.size(); ++i) {
        const std::string& loc = store.get(i).location;
        bool found = false;
        for (int j = 0; j < NUM_BUCKETS; ++j) {
            if (loc == CITIES[j]) {
                buckets[j].add(store.get(i));
                found = true;
                break;
            }
        }
        if (!found) {
            cerr << "⚠️  Warning: Unknown location '" << loc << "'\n";
        }
    }

    store.clear();

    if (!reverse) {
        for (int i = 0; i < NUM_BUCKETS; ++i)
            for (int j = 0; j < buckets[i].size(); ++j)
                store.add(buckets[i].get(j));
    } else {
        for (int i = NUM_BUCKETS - 1; i >= 0; --i)
            for (int j = 0; j < buckets[i].size(); ++j)
                store.add(buckets[i].get(j));
    }

    delete[] buckets; 
}




void bucketSortByLocation(LinkedListTransactionStore& store, bool reverse = false) {
    const int LIMIT = min(store.size(), MAX_TRANSACTIONS); 
    LinkedListTransactionStore temp;

    ListNode* curr = store.getHead();
    int copied = 0;
    while (curr && copied < LIMIT) {
        temp.add(curr->data);
        curr = curr->next;
        ++copied;
    }

    LinkedListTransactionStore buckets[NUM_BUCKETS];
    curr = temp.getHead();

    while (curr) {
        for (int j = 0; j < NUM_BUCKETS; ++j) {
            if (curr->data.location == CITIES[j]) {
                buckets[j].add(curr->data);
                break;
            }
        }
        curr = curr->next;
    }

    store.clear();
    if (!reverse) {
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            ListNode* node = buckets[i].getHead();
            while (node) {
                store.add(node->data);
                node = node->next;
            }
        }
    } else {
        for (int i = NUM_BUCKETS - 1; i >= 0; --i) {
            ListNode* node = buckets[i].getHead();
            while (node) {
                store.add(node->data);
                node = node->next;
            }
        }
    }
}



void handleSortMenu() {
    int choice;
    do {
        cout << "\n========= SORT MENU =========\n";
        cout << "1. Sort all by location (A-Z)\n";
        cout << "2. Sort all by location (Z-A)\n";
        cout << "3. Back to Main Menu\n";
        cout << "Choose an option: ";
        cin >> choice;

        if (cin.fail()) {
            cin.clear(); cin.ignore(10000, '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }

        if (choice == 3) return;

        bool reverse = (choice == 2);

        if (!isLinkedMode) {
            auto start = high_resolution_clock::now();

            bucketSortByLocation(cardStore, reverse);
            bucketSortByLocation(achStore, reverse);
            bucketSortByLocation(upiStore, reverse);
            bucketSortByLocation(wireStore, reverse);

            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);
            cout << "\n[ARRAY] Sorting Time: " << duration.count() << " ms\n";
            printMemoryUsage();


            bool exitEarly = false;

            paginateArrayResults("Card Transactions", cardStore, exitEarly);
            if (exitEarly) return;

            paginateArrayResults("ACH Transactions", achStore, exitEarly);
            if (exitEarly) return;

            paginateArrayResults("UPI Transactions", upiStore, exitEarly);
            if (exitEarly) return;

            paginateArrayResults("Wire Transactions", wireStore, exitEarly);

        } else {
            auto start = high_resolution_clock::now();

            bucketSortByLocation(cardLL, reverse);
            bucketSortByLocation(achLL, reverse);
            bucketSortByLocation(upiLL, reverse);
            bucketSortByLocation(wireLL, reverse);

            auto end = high_resolution_clock::now();
            auto duration = duration_cast<milliseconds>(end - start);
            cout << "\n[LINKED LIST] Sorting Time: " << duration.count() << " ms\n";
            printMemoryUsage();


            bool exitEarly = false;

            paginateLinkedListResults("Card Transactions", cardLL, exitEarly);
            if (exitEarly) return;

            paginateLinkedListResults("ACH Transactions", achLL, exitEarly);
            if (exitEarly) return;

            paginateLinkedListResults("UPI Transactions", upiLL, exitEarly);
            if (exitEarly) return;

            paginateLinkedListResults("Wire Transactions", wireLL, exitEarly);


        }

    } while (true);
}



void showRandomSamples() {
    srand(time(0));
    ArrayTransactionStore* stores[] = { &cardStore, &achStore, &upiStore, &wireStore };
    int idx = rand() % 4;

    cout << "\n--- Random 5 Transactions ---\n";
    for (int i = 0; i < 5; i++) {
        int c = rand() % 4;
        if (stores[c]->size() == 0) continue;
        int j = rand() % stores[c]->size();
        printTransaction(stores[c]->get(j));
    }
}


// --------------- Main Menu --------------------
void displayMainMenu() {
    cout << "\n========= MAIN MENU =========\n";
    cout << "1. Search\n";
    cout << "2. Sort\n";
    cout << "3. Export all to JSON\n"; 
    cout << "4. Exit\n";
    cout << "Choose an option: ";
}


int main() {
    int mode;
    cout << "Choose mode: 1 = Array, 2 = Linked List: ";
    cin >> mode;
    isLinkedMode = (mode == 2);

    loadData("financial_fraud_detection.csv");

    int mainChoice;
    do {
        displayMainMenu();
        cin >> mainChoice;

        if (cin.fail()) {
            cin.clear(); cin.ignore(10000, '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }

        switch (mainChoice) {
            case 1:
                handleSearchMenu();
                break;
            case 2:
                handleSortMenu();
                break;
            case 3:
                if (isLinkedMode) {
                    exportLinkedListToJSON("linked_card.json", cardLL);
                    exportLinkedListToJSON("linked_ach.json", achLL);
                    exportLinkedListToJSON("linked_upi.json", upiLL);
                    exportLinkedListToJSON("linked_wire.json", wireLL);
                } else {
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
