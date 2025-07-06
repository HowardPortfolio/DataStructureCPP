#ifndef LINKEDLISTTRANSACTIONSTORE_HPP
#define LINKEDLISTTRANSACTIONSTORE_HPP
#include <iostream>
#include <iomanip>
using namespace std;
#include "Transaction.hpp"

struct ListNode
{
    Transaction data;
    ListNode *next;
};

class LinkedListTransactionStore
{
private:
    ListNode *head;
    ListNode *tail;
    int count;

public:
    LinkedListTransactionStore() : head(nullptr), tail(nullptr), count(0) {}
    ~LinkedListTransactionStore() { clear(); }

    void add(const Transaction &t)
    {
        ListNode *node = new ListNode{t, nullptr};
        if (!head)
            head = tail = node;
        else
            tail->next = node, tail = node;
        count++;
    }

    int size() const { return count; }

    ListNode *getHead() const { return head; }

    void setHead(ListNode *newHead)
    {
        head = newHead;
        tail = nullptr;
        count = 0;
        ListNode *curr = head;
        while (curr)
        {
            count++;
            if (!curr->next)
                tail = curr;
            curr = curr->next;
        }
    }

    void clear()
    {
        while (head)
        {
            ListNode *temp = head;
            head = head->next;
            delete temp;
        }
        head = tail = nullptr;
        count = 0;
    }

    void printAll(int max = 5) const
    {
        ListNode *curr = head;
        int shown = 0;
        while (curr && shown < max)
        {
            cout << fixed << setprecision(2);
            cout << "ID: " << curr->data.transaction_id
                 << " | Location: " << curr->data.location
                 << " | Amount: " << curr->data.amount
                 << " | Type: " << curr->data.transaction_type
                 << " | Fraud: " << (curr->data.is_fraud ? "YES" : "NO")
                 << " | Channel: " << curr->data.payment_channel << endl;
            curr = curr->next;
            shown++;
        }
    }
};

#endif
