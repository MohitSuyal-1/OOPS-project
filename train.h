#ifndef TRAIN_H
#define TRAIN_H

#include <string>
#include <set>

struct Train {
    std::string trainNo;
    std::string trainName;
    std::string from;
    std::string to;
    std::string arr;
    std::string dep;
    std::string stop;
    std::set<std::string> classes; // e.g. {"1A","2A","3A","SL"}
};

#endif // TRAIN_H
