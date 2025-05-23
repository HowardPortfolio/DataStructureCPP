#ifndef ARRAY_STORE_CPP
#define ARRAY_STORE_CPP

#include "Transaction.hpp"
#include <iostream>

#define MAX_TRANSACTIONS 10000

class ArrayTransactionStore {
private:
    Transaction transactions[MAX_TRANSACTIONS];
    int count;

public:
    ArrayTransactionStore() : count(0) {}

    void add(const Transaction& t) {
        if (count < MAX_TRANSACTIONS)
            transactions[count++] = t;
    }

    int size() const {
        return count;
    }

    Transaction get(int index) const {
        return transactions[index];
    }
};

#endif
