#ifndef PACKET_H
#define PACKET_H

#include <string>

// Represents a single packet flowing through the switch fabric
struct Packet {
    std::string pid;    // unique packet identifier (e.g. "p1")
    int arrival;          // time slot at which the packet arrives
    int in_port;          // source input port index
    int out_port;         // destination output port index
};

#endif // PACKET_H
