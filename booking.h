#ifndef BOOKING_H
#define BOOKING_H

#include <string>

struct Booking {
    std::string pnr;
    std::string name;
    int age = 0;
    std::string trainNo;
    std::string trainName;
    std::string classType;
    int seatNo = 0;
    int fare = 0;
    std::string departure;
};

#endif // BOOKING_H
