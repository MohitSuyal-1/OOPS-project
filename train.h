#ifndef TRAIN_H
#define TRAIN_H

#include <string>
#include <set>
using std::string;
using std::set;

// Structure to store train information
struct Train {
    string trainNo;
    string trainName;

    string from;
    string to;

    string arr;
    string dep;

    string stop;

    // Example: {"1A", "2A", "3A", "SL"}
    set<string> classes;
};

#endif // TRAIN_H
