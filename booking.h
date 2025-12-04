#ifndef BOOKING_H
#define BOOKING_H

#include <string>
using std::string;

// Simple POD structure to store booking details
struct Booking {
    string pnr;
    string name;
    int age = 0;

    string trainNo;
    string trainName;

    string classType;
    int seatNo = 0;

    int fare = 0;
    string departure;
};

#endif // BOOKING_H
