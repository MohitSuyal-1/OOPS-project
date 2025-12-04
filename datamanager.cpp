#include "datamanager.h"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <ctime>

using std::string;
using std::vector;
using std::ifstream;
using std::ofstream;
using std::stringstream;

// ------------------------------------------------------
// Constructor – Initialize Seats & Fares
// ------------------------------------------------------
DataManager::DataManager() {
    m_seatCapacity = {
        {"1A", 20}, {"2A", 40}, {"3A", 60}, {"3E", 70},
        {"SL", 120}, {"CC", 80}, {"2S", 100}
    };

    m_fares = {
        {"1A", 2000}, {"2A", 1500}, {"3A", 1100}, {"3E", 900},
        {"SL", 400}, {"CC", 700}, {"2S", 300}
    };
}

// ------------------------------------------------------
// File Path Setters
// ------------------------------------------------------
void DataManager::setTrainFilePath(const string &path) {
    m_trainFilePath = path;
}

void DataManager::setBookingFilePath(const string &path) {
    m_bookingFilePath = path;
}

// ------------------------------------------------------
// Utility Functions
// ------------------------------------------------------
vector<string> DataManager::split(const string &s, char delim) const {
    vector<string> result;
    stringstream ss(s);
    string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

string DataManager::trim(const string &s) const {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";

    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

string DataManager::toUpper(const string &s) const {
    string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

string DataManager::toLower(const string &s) const {
    string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// ------------------------------------------------------
// Load Trains from CSV
// ------------------------------------------------------
bool DataManager::loadTrains() {
    m_trains.clear();

    ifstream file(m_trainFilePath);
    if (!file.is_open()) return false;

    string line;
    bool first = true;

    while (getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (first) {
            first = false;  // Skip CSV header
            continue;
        }

        vector<string> parts = split(line, ',');
        if (parts.size() < 8) continue;

        Train t;
        t.trainNo   = trim(parts[0]);
        t.trainName = trim(parts[1]);
        t.from      = trim(parts[2]);
        t.to        = trim(parts[3]);
        t.arr       = trim(parts[4]);
        t.dep       = trim(parts[5]);
        t.stop      = trim(parts[6]);

        // Parse class list ("1A 2A 3A SL")
        stringstream ss(trim(parts[7]));
        string token;
        while (ss >> token) {
            t.classes.insert(trim(token));
        }

        m_trains.push_back(t);
    }
    return true;
}

// ------------------------------------------------------
// Load Bookings from CSV
// ------------------------------------------------------
bool DataManager::loadBookings() {
    m_bookings.clear();

    ifstream file(m_bookingFilePath);
    if (!file.is_open()) return true;  // No bookings yet

    string line;
    bool first = true;

    while (getline(file, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (first) {
            if (line.rfind("pnr", 0) == 0) { // header
                first = false;
                continue;
            }
            first = false;
        }

        vector<string> parts = split(line, ',');
        if (parts.size() < 9) continue;

        Booking b;
        b.pnr        = trim(parts[0]);
        b.name       = trim(parts[1]);
        b.age        = stoi(trim(parts[2]));
        b.trainNo    = trim(parts[3]);
        b.trainName  = trim(parts[4]);
        b.classType  = trim(parts[5]);
        b.seatNo     = stoi(trim(parts[6]));
        b.fare       = stoi(trim(parts[7]));
        b.departure  = trim(parts[8]);

        m_bookings.push_back(b);
    }
    return true;
}

// ------------------------------------------------------
// Save Bookings to CSV
// ------------------------------------------------------
bool DataManager::saveBookings() const {
    ofstream file(m_bookingFilePath);
    if (!file.is_open()) return false;

    file << "pnr,name,age,trainNo,trainName,classType,seatNo,fare,departure\n";

    for (const Booking &b : m_bookings) {
        file << b.pnr << ","
             << b.name << ","
             << b.age << ","
             << b.trainNo << ","
             << b.trainName << ","
             << b.classType << ","
             << b.seatNo << ","
             << b.fare << ","
             << b.departure << "\n";
    }

    return true;
}

// ------------------------------------------------------
// Search Functions
// ------------------------------------------------------
vector<Train> DataManager::searchTrainsByRoute(const string &from, const string &to) const {
    vector<Train> result;
    string f = toUpper(from);
    string t = toUpper(to);

    for (const Train &tr : m_trains) {
        if (toUpper(tr.from) == f && toUpper(tr.to) == t)
            result.push_back(tr);
    }
    return result;
}

vector<Train> DataManager::searchTrainsByName(const string &namePart) const {
    vector<Train> result;
    string key = toLower(namePart);

    for (const Train &tr : m_trains) {
        if (toLower(tr.trainName).find(key) != string::npos)
            result.push_back(tr);
    }
    return result;
}

const Train* DataManager::findTrainByNumber(const string &trainNo) const {
    for (const Train &t : m_trains) {
        if (t.trainNo == trainNo)
            return &t;
    }
    return nullptr;
}

// ------------------------------------------------------
// Booking Management
// ------------------------------------------------------
vector<Booking> DataManager::findBookingsByPNR(const string &pnr) const {
    vector<Booking> result;
    for (const Booking &b : m_bookings) {
        if (b.pnr == pnr)
            result.push_back(b);
    }
    return result;
}

int DataManager::bookedCount(const string &trainNo, const string &classType) const {
    int count = 0;
    for (const Booking &b : m_bookings) {
        if (b.trainNo == trainNo && b.classType == classType)
            count++;
    }
    return count;
}

int DataManager::nextSeatNo(const string &trainNo, const string &classType) const {
    return bookedCount(trainNo, classType) + 1;
}

string DataManager::generatePNR() const {
    static std::mt19937 rng((unsigned)time(nullptr));
    std::uniform_int_distribution<int> dist(100000, 999999);
    return std::to_string(dist(rng));
}

bool DataManager::addBooking(const Booking &booking) {
    m_bookings.push_back(booking);
    return saveBookings();
}

bool DataManager::cancelBooking(const string &pnr, Booking *removed) {
    for (auto it = m_bookings.begin(); it != m_bookings.end(); ++it) {
        if (it->pnr == pnr) {
            if (removed) *removed = *it;
            m_bookings.erase(it);
            return saveBookings();
        }
    }
    return false;
}
