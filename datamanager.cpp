#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <ctime>
using namespace std;

// -------------------- TRAIN STRUCT --------------------
struct Train {
    string trainNo;
    string trainName;
    string from;
    string to;
    string arr;
    string dep;
    string stop;
    set<string> classes;
};

// -------------------- BOOKING STRUCT --------------------
struct Booking {
    string pnr;
    string name;
    int age;
    string trainNo;
    string trainName;
    string classType;
    int seatNo;
    int fare;
    string departure;
};

// -------------------- DATABASE CLASS --------------------
class Database {
public:
    vector<Train> trains;
    vector<Booking> bookings;
    map<string, int> seatCapacity;
    map<string, int> fares;

    string trainFile;
    string bookingFile;

    Database() {
        trainFile = "trains.csv";
        bookingFile = "bookings.csv";

        seatCapacity["1A"] = 20;
        seatCapacity["2A"] = 40;
        seatCapacity["3A"] = 60;
        seatCapacity["3E"] = 70;
        seatCapacity["SL"] = 120;
        seatCapacity["CC"] = 80;
        seatCapacity["2S"] = 100;

        fares["1A"] = 2000;
        fares["2A"] = 1500;
        fares["3A"] = 1100;
        fares["3E"] = 900;
        fares["SL"] = 400;
        fares["CC"] = 700;
        fares["2S"] = 300;

        srand(time(0));
    }

    // Trim spaces
    string trim(string s) {
        int start = s.find_first_not_of(" \t\r\n");
        if (start == string::npos) return "";
        int end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    // Split line by delimiter
    vector<string> split(string line, char d) {
        vector<string> result;
        string word = "";
        for (int i = 0; i < (int)line.size(); i++) {
            if (line[i] == d) {
                result.push_back(trim(word));
                word = "";
            } else {
                word += line[i];
            }
        }
        result.push_back(trim(word));
        return result;
    }

    string toLower(string s) {
        for (int i = 0; i < (int)s.size(); i++) {
            s[i] = tolower(s[i]);
        }
        return s;
    }

    // ---------------- LOAD TRAINS ----------------
    bool loadTrains() {
        trains.clear();
        ifstream file(trainFile.c_str());
        if (!file.is_open()) return false;

        string line;
        bool skip = true;

        while (getline(file, line)) {
            if (skip) { skip = false; continue; }
            if (line == "") continue;

            vector<string> p = split(line, ',');
            if (p.size() < 8) continue;

            Train t;
            t.trainNo = p[0];
            t.trainName = p[1];
            t.from = p[2];
            t.to = p[3];
            t.arr = p[4];
            t.dep = p[5];
            t.stop = p[6];

            stringstream ss(p[7]);
            string c;
            while (ss >> c) {
                t.classes.insert(c);
            }

            trains.push_back(t);
        }
        return true;
    }

    // ---------------- LOAD BOOKINGS ----------------
    bool loadBookings() {
        bookings.clear();
        ifstream file(bookingFile.c_str());
        if (!file.is_open()) return true;

        string line;
        bool skip = true;

        while (getline(file, line)) {
            if (skip) { skip = false; continue; }
            if (line == "") continue;

            vector<string> p = split(line, ',');
            if (p.size() < 9) continue;

            Booking b;
            b.pnr = p[0];
            b.name = p[1];
            b.age = atoi(p[2].c_str());
            b.trainNo = p[3];
            b.trainName = p[4];
            b.classType = p[5];
            b.seatNo = atoi(p[6].c_str());
            b.fare = atoi(p[7].c_str());
            b.departure = p[8];

            bookings.push_back(b);
        }
        return true;
    }

    // ---------------- SAVE BOOKINGS ----------------
    bool saveBookings() {
        ofstream file(bookingFile.c_str());
        if (!file.is_open()) return false;

        file << "pnr,name,age,trainNo,trainName,classType,seatNo,fare,departure\n";

        for (int i = 0; i < bookings.size(); i++) {
            Booking b = bookings[i];
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

    // ---------------- SEARCH BY NAME ----------------
    vector<Train> searchByName(string key) {
        vector<Train> result;
        string low = toLower(key);

        for (int i = 0; i < trains.size(); i++) {
            string name = toLower(trains[i].trainName);
            if (name.find(low) != string::npos) {
                result.push_back(trains[i]);
            }
        }
        return result;
    }

    // ---------------- FIND BY NUMBER ----------------
    const Train* findByNumber(string no) {
        for (int i = 0; i < trains.size(); i++) {
            if (trains[i].trainNo == no) {
                return &trains[i];
            }
        }
        return NULL;
    }

    // ---------------- BOOKED COUNT ----------------
    int bookedCount(string trainNo, string cls) {
        int count = 0;
        for (int i = 0; i < bookings.size(); i++) {
            if (bookings[i].trainNo == trainNo &&
                bookings[i].classType == cls) {
                count++;
            }
        }
        return count;
    }

    int nextSeat(string trainNo, string cls) {
        return bookedCount(trainNo, cls) + 1;
    }

    // ---------------- SIMPLE PNR GENERATOR ----------------
    string generatePNR() {
        int r = rand() % 900000 + 100000;
        stringstream ss;
        ss << r;
        return ss.str();
    }

    // ---------------- ADD BOOKING ----------------
    bool addBooking(Booking b) {
        bookings.push_back(b);
        return saveBookings();
    }

    // ---------------- CANCEL BOOKING ----------------
    bool cancel(string pnr) {
        for (int i = 0; i < bookings.size(); i++) {
            if (bookings[i].pnr == pnr) {
                bookings.erase(bookings.begin() + i);
                return saveBookings();
            }
        }
        return false;
    }
};

// -------------------- MAIN --------------------
int main() {
    Database db;
    db.loadTrains();
    db.loadBookings();

    cout << "Database Loaded Successfully.\n";
    return 0;
}
