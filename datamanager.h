#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <vector>
#include <map>
#include <string>
#include "train.h"
#include "booking.h"

class DataManager {
public:
    DataManager();

    void setTrainFilePath(const std::string &path);
    void setBookingFilePath(const std::string &path);

    bool loadTrains();
    bool loadBookings();
    bool saveBookings() const;

    const std::vector<Train> &trains() const { return m_trains; }
    const std::vector<Booking> &bookings() const { return m_bookings; }

    std::vector<Train> searchTrainsByRoute(const std::string &from, const std::string &to) const;
    std::vector<Train> searchTrainsByName(const std::string &namePart) const;
    const Train *findTrainByNumber(const std::string &trainNo) const;
    std::vector<Booking> findBookingsByPNR(const std::string &pnr) const;

    int bookedCount(const std::string &trainNo, const std::string &classType) const;
    int nextSeatNo(const std::string &trainNo, const std::string &classType) const;
    std::string generatePNR() const;

    const std::map<std::string, int> &seatCapacity() const { return m_seatCapacity; }
    const std::map<std::string, int> &fares() const { return m_fares; }

    bool addBooking(const Booking &booking);
    bool cancelBooking(const std::string &pnr, Booking *removed = nullptr);

private:
    std::string m_trainFilePath;
    std::string m_bookingFilePath;
    std::vector<Train> m_trains;
    std::vector<Booking> m_bookings;

    std::map<std::string, int> m_seatCapacity;
    std::map<std::string, int> m_fares;

    std::vector<std::string> split(const std::string &s, char delim) const;
    std::string trim(const std::string &s) const;
    std::string toUpper(const std::string &s) const;
    std::string toLower(const std::string &s) const;
};

#endif // DATAMANAGER_H
