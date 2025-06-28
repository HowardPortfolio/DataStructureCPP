#ifndef ARRAYTRANSACTIONSTORE_HPP
#define ARRAYTRANSACTIONSTORE_HPP

#include "Transaction.hpp"

#define MAX_TRANSACTIONS 10000

class ArrayTransactionStore
{
private:
    Transaction transactions[MAX_TRANSACTIONS];
    int count;

public:
    ArrayTransactionStore() : count(0) {}

    void add(const Transaction &t)
    {
        if (count < MAX_TRANSACTIONS)
            transactions[count++] = t;
    }

    void swap(int i, int j)
    {
        if (i >= 0 && j >= 0 && i < count && j < count)
        {
            Transaction temp = transactions[i];
            transactions[i] = transactions[j];
            transactions[j] = temp;
        }
    }

    int size() const { return count; }
    Transaction get(int index) const { return transactions[index]; }
    void clear() { count = 0; }
};

#endif