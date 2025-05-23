#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include "transaction.hpp"

using namespace std;

//LOWER THIS VALUE IF IT TAKES TOO LONG TO TEST
const int MAX_TRANSACTIONS = 5000001;

Transaction* card = new Transaction[MAX_TRANSACTIONS];
Transaction* ach = new Transaction[MAX_TRANSACTIONS];
Transaction* upi = new Transaction[MAX_TRANSACTIONS];
Transaction* wire = new Transaction[MAX_TRANSACTIONS];
Transaction* other = new Transaction[MAX_TRANSACTIONS];

int cardCount = 0, achCount = 0, upiCount = 0, wireCount = 0, otherCount = 0;

void printTransaction(const Transaction& t) {
    cout << fixed << setprecision(2);
    cout << "ID: " << t.transaction_id
         << " | Timestamp: " << t.timestamp
         << " | Sender: " << t.sender_account
         << " | Receiver: " << t.receiver_account
         << " | Amount: " << t.amount
         << " | Type: " << t.transaction_type
         << " | Location: " << t.location
         << " | Fraud: " << (t.is_fraud ? "YES" : "NO")
         << " | Channel: " << t.payment_channel
         << " | Fraud Type: " << (t.is_fraud ? t.fraud_type : "N/A");

    if (!t.time_since_last_transaction.empty()) {
        try {
            double val = stod(t.time_since_last_transaction);
            cout << " | Time Since Last Txn: " << val;
        } catch (...) {
            cout << " | Time Since Last Txn: Invalid";
        }
    }

    cout << endl;
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

    getline(ss, cell, ',');
    for (char& c : cell) c = tolower(c);
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
        cerr << "Error opening file." << endl;
        return;
    }

    string line;
    getline(file, line);

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
        else if (
            t.payment_channel != "card" &&
            t.payment_channel != "ACH" &&
            t.payment_channel != "UPI" &&
            t.payment_channel != "wire_transfer" &&
            otherCount < MAX_TRANSACTIONS
        )
            other[otherCount++] = t;
    }

    file.close();

    cout << "\nLoaded Transactions:\n";
    cout << "Card: " << cardCount
         << " | ACH: " << achCount
         << " | UPI: " << upiCount
         << " | Wire Transfer: " << wireCount
         << " | Other: " << otherCount << endl;

    cout << "\nLast transaction parsed:\n";
    if (otherCount > 0) printTransaction(other[otherCount - 1]);
    else if (wireCount > 0) printTransaction(wire[wireCount - 1]);
    else if (upiCount > 0) printTransaction(upi[upiCount - 1]);
    else if (achCount > 0) printTransaction(ach[achCount - 1]);
    else if (cardCount > 0) printTransaction(card[cardCount - 1]);
    else cout << "No transactions loaded.\n";
}

void bucketSortByLocation(Transaction* arr, int& count) {
    const string cities[] = {
        "Berlin", "Dubai", "London", "New York", "Singapore",
        "Sydney", "Tokyo", "Toronto"
    };
    const int NUM_BUCKETS = sizeof(cities) / sizeof(cities[0]);

    Transaction** buckets = new Transaction*[NUM_BUCKETS];
    int* bucketSizes = new int[NUM_BUCKETS]();
    int* bucketCaps = new int[NUM_BUCKETS];

    for (int i = 0; i < NUM_BUCKETS; i++) {
        bucketCaps[i] = count; // worst case
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
    for (int i = 0; i < NUM_BUCKETS; i++) {
        for (int j = 0; j < bucketSizes[i]; j++) {
            arr[idx++] = buckets[i][j];
        }
    }

    for (int i = 0; i < NUM_BUCKETS; i++)
        delete[] buckets[i];
    delete[] buckets;
    delete[] bucketSizes;
    delete[] bucketCaps;
}

void displayMenu() {
    cout << "\n========= Transaction Menu =========\n";
    cout << "1. Show first 5 Card transactions\n";
    cout << "2. Show first 5 ACH transactions\n";
    cout << "3. Show first 5 UPI transactions\n";
    cout << "4. Show first 5 Wire Transfer transactions\n";
    cout << "5. Show first 5 Other transactions\n";
    cout << "6. Sort ALL groups by Location (Bucket Sort)\n";
    cout << "7. Exit\n";
    cout << "Choose an option: ";
}

void showTransactions(Transaction* arr, int count) {
    if (count == 0) {
        cout << "No transactions found in this group.\n";
        return;
    }

    for (int i = 0; i < 5 && i < count; ++i) {
        printTransaction(arr[i]);
    }
}

int main() {
    loadData("financial_fraud_detection.csv");

    int choice;
    do {
        displayMenu();
        cin >> choice;

        switch (choice) {
            case 1: showTransactions(card, cardCount); break;
            case 2: showTransactions(ach, achCount); break;
            case 3: showTransactions(upi, upiCount); break;
            case 4: showTransactions(wire, wireCount); break;
            case 5: showTransactions(other, otherCount); break;
            case 6:
                bucketSortByLocation(card, cardCount);
                bucketSortByLocation(ach, achCount);
                bucketSortByLocation(upi, upiCount);
                bucketSortByLocation(wire, wireCount);
                bucketSortByLocation(other, otherCount);
                cout << "All groups sorted by location (bucket sort).\n";
                break;
            case 7:
                cout << "Exiting...\n";
                break;
            default:
                cout << "Invalid choice. Try again.\n";
        }
    } while (choice != 7);

    delete[] card;
    delete[] ach;
    delete[] upi;
    delete[] wire;
    delete[] other;

    return 0;
}
