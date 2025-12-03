#include "datamanager.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <ctime>
#include <cctype>

DataManager::DataManager() {
    // Initialize seat capacities
    m_seatCapacity = {
        {"1A", 20}, {"2A", 40}, {"3A", 60}, {"3E", 70},
        {"SL", 120}, {"CC", 80}, {"2S", 100}
    };

    // Initialize fares
    m_fares = {
        {"1A", 2000}, {"2A", 1500}, {"3A", 1100}, {"3E", 900},
        {"SL", 400}, {"CC", 700}, {"2S", 300}
    };
}

void DataManager::setTrainFilePath(const std::string &path) {
    m_trainFilePath = path;
}

void DataManager::setBookingFilePath(const std::string &path) {
    m_bookingFilePath = path;
}

std::vector<std::string> DataManager::split(const std::string &s, char delim) const {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        result.push_back(item);
    }
    return result;
}

std::string DataManager::trim(const std::string &s) const {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string DataManager::toUpper(const std::string &s) const {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string DataManager::toLower(const std::string &s) const {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

bool DataManager::loadTrains() {
    m_trains.clear();

    std::ifstream file(m_trainFilePath);
    if (!file.is_open())
        return false;

    std::string line;
    bool firstLine = true;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty())
            continue;
        if (firstLine) { // skip header
            firstLine = false;
            continue;
        }

        std::vector<std::string> parts = split(line, ',');
        if (parts.size() < 8)
            continue;

        Train t;
        t.trainNo   = trim(parts[0]);
        t.trainName = trim(parts[1]);
        t.from      = trim(parts[2]);
        t.to        = trim(parts[3]);
        t.arr       = trim(parts[4]);
        t.dep       = trim(parts[5]);
        t.stop      = trim(parts[6]);

        // classes like "1A 2A 3A SL"
        std::string classesField = trim(parts[7]);
        std::stringstream ss(classesField);
        std::string token;
        while (ss >> token) {
            t.classes.insert(trim(token));
        }

        m_trains.push_back(t);
    }

    return true;
}

bool DataManager::loadBookings() {
    m_bookings.clear();

    std::ifstream file(m_bookingFilePath);
    if (!file.is_open()) {
        // No bookings yet, not an error
        return true;
    }

    std::string line;
    bool firstLine = true;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty())
            continue;

        if (firstLine) {
            // Skip header if present
            if (line.size() >= 3 && toLower(line.substr(0, 3)) == "pnr") {
                firstLine = false;
                continue;
            }
            firstLine = false;
        }

        std::vector<std::string> parts = split(line, ',');
        if (parts.size() < 9)
            continue;

        Booking b;
        b.pnr        = trim(parts[0]);
        b.name       = trim(parts[1]);
        b.age        = std::stoi(trim(parts[2]));
        b.trainNo    = trim(parts[3]);
        b.trainName  = trim(parts[4]);
        b.classType  = trim(parts[5]);
        b.seatNo     = std::stoi(trim(parts[6]));
        b.fare       = std::stoi(trim(parts[7]));
        b.departure  = trim(parts[8]);

        m_bookings.push_back(b);
    }

    return true;
}

bool DataManager::saveBookings() const {
    std::ofstream file(m_bookingFilePath);
    if (!file.is_open())
        return false;

    // Write header
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

std::vector<Train> DataManager::searchTrainsByRoute(const std::string &from, const std::string &to) const {
    std::vector<Train> result;
    std::string fromUpper = toUpper(from);
    std::string toUpper_ = toUpper(to);

    for (const Train &t : m_trains) {
        if (toUpper(t.from) == fromUpper && toUpper(t.to) == toUpper_) {
            result.push_back(t);
        }
    }

    return result;
}

std::vector<Train> DataManager::searchTrainsByName(const std::string &namePart) const {
    std::vector<Train> result;
    std::string patternLower = toLower(namePart);

    for (const Train &t : m_trains) {
        std::string nameLower = toLower(t.trainName);
        if (nameLower.find(patternLower) != std::string::npos) {
            result.push_back(t);
        }
    }

    return result;
}

const Train *DataManager::findTrainByNumber(const std::string &trainNo) const {
    for (const Train &t : m_trains) {
        if (t.trainNo == trainNo)
            return &t;
    }
    return nullptr;
}

std::vector<Booking> DataManager::findBookingsByPNR(const std::string &pnr) const {
    std::vector<Booking> result;
    for (const Booking &b : m_bookings) {
        if (b.pnr == pnr)
            result.push_back(b);
    }
    return result;
}

int DataManager::bookedCount(const std::string &trainNo, const std::string &classType) const {
    int count = 0;
    for (const Booking &b : m_bookings) {
        if (b.trainNo == trainNo && b.classType == classType)
            ++count;
    }
    return count;
}

int DataManager::nextSeatNo(const std::string &trainNo, const std::string &classType) const {
    return bookedCount(trainNo, classType) + 1;
}

std::string DataManager::generatePNR() const {
    static std::mt19937 rng((unsigned)std::time(nullptr));
    std::uniform_int_distribution<int> dist(100000, 999999);
    return std::to_string(dist(rng));
}

bool DataManager::addBooking(const Booking &booking) {
    m_bookings.push_back(booking);
    return saveBookings();
}

bool DataManager::cancelBooking(const std::string &pnr, Booking *removed) {
    for (auto it = m_bookings.begin(); it != m_bookings.end(); ++it) {
        if (it->pnr == pnr) {
            if (removed)
                *removed = *it;
            m_bookings.erase(it);
            return saveBookings();
        }
    }
    return false;
}
