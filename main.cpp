#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include "transaction.hpp"

using namespace std;

const int MAX_TRANSACTIONS = 100000;
Transaction* card = new Transaction[MAX_TRANSACTIONS];
Transaction* ach = new Transaction[MAX_TRANSACTIONS];
Transaction* upi = new Transaction[MAX_TRANSACTIONS];
Transaction* wire = new Transaction[MAX_TRANSACTIONS];
int cardCount = 0, achCount = 0, upiCount = 0, wireCount = 0;

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

Transaction parseTransaction(const string& line) {
    stringstream ss(line);
    string cell;
    Transaction t;

    getline(ss, t.transaction_id, ',');
    getline(ss, t.timestamp, ',');
    getline(ss, t.sender_account, ',');
    getline(ss, t.receiver_account, ',');
    getline(ss, cell, ','); t.amount = cell.empty() ? 0.0 : stod(cell);
    getline(ss, t.transaction_type, ',');
    getline(ss, t.merchant_category, ',');
    getline(ss, t.location, ',');
    getline(ss, t.device_used, ',');
    getline(ss, cell, ','); for (char& c : cell) c = tolower(c);
    t.is_fraud = (cell == "true");
    getline(ss, t.fraud_type, ',');
    getline(ss, t.time_since_last_transaction, ',');
    getline(ss, t.spending_deviation_score, ',');
    getline(ss, cell, ','); t.velocity_score = cell.empty() ? 0.0 : stod(cell);
    getline(ss, cell, ','); t.geo_anomaly_score = cell.empty() ? 0.0 : stod(cell);
    getline(ss, t.payment_channel, ',');
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
    getline(file, line); // skip header
    cardCount = achCount = upiCount = wireCount = 0;

    while (getline(file, line)) {
        if (line.empty() || count(line.begin(), line.end(), ',') < 17)
            continue;

        Transaction t = parseTransaction(line);

        if (t.payment_channel == "card" && cardCount < MAX_TRANSACTIONS)
            card[cardCount++] = t;
        else if (t.payment_channel == "ACH" && achCount < MAX_TRANSACTIONS)
            ach[achCount++] = t;
        else if (t.payment_channel == "UPI" && upiCount < MAX_TRANSACTIONS)
            upi[upiCount++] = t;
        else if (t.payment_channel == "wire_transfer" && wireCount < MAX_TRANSACTIONS)
            wire[wireCount++] = t;
    }

    file.close();

    cout << "\nLoaded Transactions:\n";
    cout << "Card: " << cardCount
         << " | ACH: " << achCount
         << " | UPI: " << upiCount
         << " | Wire Transfer: " << wireCount << endl;
}

// Helpers for Search
string toLower(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

void displaySearchMenu() {
    cout << "\n========= SEARCH MENU =========\n";
    cout << "1-18: Search by specific field\n";
    cout << "19: All-rounder search (any field)\n";
    cout << "20: Back to Main Menu\n";
    cout << "Choose an option: ";
}

void searchTransactions(const string& searchField, const string& searchTermLower, int page) {
    int shown = 0, startIdx = page * 10;
    bool foundAny = false;

    for (int i = 0; i < cardCount; i++) {
        string value = toLower(
            (searchField == "transaction_id") ? card[i].transaction_id :
            (searchField == "timestamp") ? card[i].timestamp :
            (searchField == "sender_account") ? card[i].sender_account :
            (searchField == "receiver_account") ? card[i].receiver_account :
            (searchField == "amount") ? to_string(card[i].amount) :
            (searchField == "transaction_type") ? card[i].transaction_type :
            (searchField == "merchant_category") ? card[i].merchant_category :
            (searchField == "location") ? card[i].location :
            (searchField == "device_used") ? card[i].device_used :
            (searchField == "is_fraud") ? (card[i].is_fraud ? "true" : "false") :
            (searchField == "fraud_type") ? card[i].fraud_type :
            (searchField == "time_since_last_transaction") ? card[i].time_since_last_transaction :
            (searchField == "spending_deviation_score") ? card[i].spending_deviation_score :
            (searchField == "velocity_score") ? to_string(card[i].velocity_score) :
            (searchField == "geo_anomaly_score") ? to_string(card[i].geo_anomaly_score) :
            (searchField == "payment_channel") ? card[i].payment_channel :
            (searchField == "ip_address") ? card[i].ip_address :
            (searchField == "device_hash") ? card[i].device_hash : "");

        if (value.find(searchTermLower) != string::npos) {
            if (shown >= startIdx && shown < startIdx + 10)
                printTransaction(card[i]);
            shown++;
            foundAny = true;
        }
    }

    if (!foundAny)
        cout << "No results found. Check for typos.\n";
    else if (shown <= startIdx)
        cout << "No more results.\n";
}

void handleSearchMenu() {
    int searchChoice;
    do {
        displaySearchMenu();
        cin >> searchChoice;

        if (searchChoice == 20) break;

        string field;
        switch (searchChoice) {
            case 1: field = "transaction_id"; break;
            case 2: field = "timestamp"; break;
            case 3: field = "sender_account"; break;
            case 4: field = "receiver_account"; break;
            case 5: field = "amount"; break;
            case 6: field = "transaction_type"; break;
            case 7: field = "merchant_category"; break;
            case 8: field = "location"; break;
            case 9: field = "device_used"; break;
            case 10: field = "is_fraud"; break;
            case 11: field = "fraud_type"; break;
            case 12: field = "time_since_last_transaction"; break;
            case 13: field = "spending_deviation_score"; break;
            case 14: field = "velocity_score"; break;
            case 15: field = "geo_anomaly_score"; break;
            case 16: field = "payment_channel"; break;
            case 17: field = "ip_address"; break;
            case 18: field = "device_hash"; break;
            case 19: field = "all"; break;
            default: cout << "Invalid choice.\n"; continue;
        }

        string searchTerm;
        cout << "Enter search term (case-insensitive): ";
        cin.ignore();
        getline(cin, searchTerm);
        string searchTermLower = toLower(searchTerm);

        int page = 0;
        char nav;
        do {
            searchTransactions(field, searchTermLower, page);
            cout << "\n[N]ext Page | [P]revious Page | [B]ack to Search Menu: ";
            cin >> nav;
            nav = tolower(nav);
            if (nav == 'n') page++;
            else if (nav == 'p' && page > 0) page--;
        } while (nav != 'b');
    } while (true);
}

// Bucket sort by location (A-Z)
void bucketSortByLocation(Transaction* arr, int& count, bool reverse = false) {
    const string cities[] = {
        "Berlin", "Dubai", "London", "New York", "Singapore",
        "Sydney", "Tokyo", "Toronto"
    };
    const int NUM_BUCKETS = sizeof(cities) / sizeof(cities[0]);

    Transaction** buckets = new Transaction*[NUM_BUCKETS];
    int* bucketSizes = new int[NUM_BUCKETS]();
    int* bucketCaps = new int[NUM_BUCKETS];

    for (int i = 0; i < NUM_BUCKETS; i++) {
        bucketCaps[i] = count;
        buckets[i] = new Transaction[bucketCaps[i]];
    }

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < NUM_BUCKETS; j++) {
            if (arr[i].location == cities[j]) {
                buckets[j][bucketSizes[j]++] = arr[i];
                break;
            }
        }
    }

    int idx = 0;
    if (!reverse) {
        for (int i = 0; i < NUM_BUCKETS; i++)
            for (int j = 0; j < bucketSizes[i]; j++)
                arr[idx++] = buckets[i][j];
    } else {
        for (int i = NUM_BUCKETS - 1; i >= 0; i--)
            for (int j = 0; j < bucketSizes[i]; j++)
                arr[idx++] = buckets[i][j];
    }

    for (int i = 0; i < NUM_BUCKETS; i++)
        delete[] buckets[i];
    delete[] buckets;
    delete[] bucketSizes;
    delete[] bucketCaps;
}

void showFirst5(Transaction* arr, int count) {
    for (int i = 0; i < 5 && i < count; ++i)
        printTransaction(arr[i]);
}

void handleSortMenu() {
    int sortChoice;
    do {
        cout << "\n========= SORT MENU =========\n";
        cout << "1. Sort all by location (A-Z)\n";
        cout << "2. Sort all by location (Z-A)\n";
        cout << "3. Back to Main Menu\n";
        cout << "Choose an option: ";
        cin >> sortChoice;

        if (sortChoice == 3) break;

        bool reverse = (sortChoice == 2);
        bucketSortByLocation(card, cardCount, reverse);
        bucketSortByLocation(ach, achCount, reverse);
        bucketSortByLocation(upi, upiCount, reverse);
        bucketSortByLocation(wire, wireCount, reverse);

        cout << "\n--- First 5 Card Transactions ---\n";
        showFirst5(card, cardCount);

        cout << "\n--- First 5 ACH Transactions ---\n";
        showFirst5(ach, achCount);

        cout << "\n--- First 5 UPI Transactions ---\n";
        showFirst5(upi, upiCount);

        cout << "\n--- First 5 Wire Transfer Transactions ---\n";
        showFirst5(wire, wireCount);

    } while (true);
}

#include <cstdlib>
#include <ctime>

// Show random transactions
void showRandomSamples() {
    srand(time(0));
    Transaction* all[4] = { card, ach, upi, wire };
    int counts[4] = { cardCount, achCount, upiCount, wireCount };

    cout << "\n--- Random 5 Transactions ---\n";
    for (int i = 0; i < 5; i++) {
        int channelIdx = rand() % 4;
        if (counts[channelIdx] == 0) continue;
        int idx = rand() % counts[channelIdx];
        printTransaction(all[channelIdx][idx]);
    }
}

// Show top 5 of each
void showTop5All() {
    cout << "\n--- First 5 Card Transactions ---\n";
    showFirst5(card, cardCount);

    cout << "\n--- First 5 ACH Transactions ---\n";
    showFirst5(ach, achCount);

    cout << "\n--- First 5 UPI Transactions ---\n";
    showFirst5(upi, upiCount);

    cout << "\n--- First 5 Wire Transfer Transactions ---\n";
    showFirst5(wire, wireCount);
}

void handleTestingMenu() {
    int testChoice;
    do {
        cout << "\n========= TESTING MENU =========\n";
        cout << "1. Confirm data load (counts)\n";
        cout << "2. Show random transaction samples\n";
        cout << "3. Show top 5 of each type\n";
        cout << "4. Back to Main Menu\n";
        cout << "Choose an option: ";
        cin >> testChoice;

        switch (testChoice) {
            case 1:
                cout << "\nCard: " << cardCount
                     << " | ACH: " << achCount
                     << " | UPI: " << upiCount
                     << " | Wire Transfer: " << wireCount << endl;
                break;
            case 2: showRandomSamples(); break;
            case 3: showTop5All(); break;
            case 4: break;
            default: cout << "Invalid choice.\n";
        }
    } while (testChoice != 4);
}



// Main Menu
void displayMainMenu() {
    cout << "\n========= MAIN MENU =========\n";
    cout << "1. Search\n";
    cout << "2. Sort\n";
    cout << "3. Testing\n";
    cout << "4. Exit\n";
    cout << "Choose an option: ";
}

int main() {
    const string filename = "financial_fraud_detection.csv";
    loadData(filename);

    int mainChoice;
    do {
        displayMainMenu();
        cin >> mainChoice;

        switch (mainChoice) {
            case 1: handleSearchMenu(); break;
            case 2: handleSortMenu(); break;
            case 3: handleTestingMenu(); break;
            case 4: cout << "Exiting program.\n"; break;
            default: cout << "Invalid choice. Try again.\n";
        }
    } while (mainChoice != 4);

    delete[] card;
    delete[] ach;
    delete[] upi;
    delete[] wire;

    return 0;
}
