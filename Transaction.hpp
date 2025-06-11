#ifndef TRANSACTION_HPP
#define TRANSACTION_HPP
#include <string>
using namespace std;

struct Transaction {
    string transaction_id;
    string timestamp;
    string sender_account;
    string receiver_account;
    double amount;
    string transaction_type;
    string merchant_category;
    string location;
    string device_used;
    bool is_fraud;
    string fraud_type;
    string time_since_last_transaction;
    string spending_deviation_score;
    double velocity_score;
    double geo_anomaly_score;
    string payment_channel;
    string ip_address;
    string device_hash;
};

#endif
