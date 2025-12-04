#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <string>
#include <vector>
#include <map>

using std::string;
using std::vector;
using std::map;

#include "train.h"
#include "booking.h"

class DataManager {
public:
    DataManager();

    void setTrainFilePath(const string &path);
    void setBookingFilePath(const string &path);

    bool loadTrains();
    bool loadBookings();
    bool saveBookings() const;

    const vector<Train>& trains() const { return m_trains; }
    const vector<Booking>& bookings() const { return m_bookings; }

    vector<Train> searchTrainsByRoute(const string &from, const string &to) const;
    vector<Train> searchTrainsByName(const string &namePart) const;
    const Train* findTrainByNumber(const string &trainNo) const;

    vector<Booking> findBookingsByPNR(const string &pnr) const;

    int bookedCount(const string &trainNo, const string &classType) const;
    int nextSeatNo(const string &trainNo, const string &classType) const;

    string generatePNR() const;

    const map<string, int>& seatCapacity() const { return m_seatCapacity; }
    const map<string, int>& fares() const { return m_fares; }

    bool addBooking(const Booking &booking);
    bool cancelBooking(const string &pnr, Booking *removed = nullptr);

private:
    string m_trainFilePath;
    string m_bookingFilePath;

    vector<Train>   m_trains;
    vector<Booking> m_bookings;

    map<string, int> m_seatCapacity;
    map<string, int> m_fares;

    vector<string> split(const string &s, char delim) const;
    string trim(const string &s) const;
    string toUpper(const string &s) const;
    string toLower(const string &s) const;
};

#endif
