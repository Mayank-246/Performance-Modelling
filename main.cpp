#include "fifo_switch.h"
#include "voq_optimal.h"
#include "islip_switch.h"

#include <iostream>
#include <vector>

using namespace std;

int main() {

    // Input trace as specified in the assignment
    // Format: {pid, arrival_time, source_input, dest_output}
    vector<Packet> trace = {
        {"p1",  0, 0, 0}, {"p2",  0, 0, 1}, {"p3",  0, 1, 0}, {"p4",  0, 1, 2},
        {"p5",  0, 2, 0}, {"p6",  1, 0, 2}, {"p7",  1, 2, 1}, {"p8",  2, 1, 1},
        {"p9",  2, 2, 2}, {"p10", 3, 0, 1}, {"p11", 3, 1, 0}, {"p12", 3, 2, 1},
        {"p13", 4, 0, 0}, {"p14", 4, 1, 2}, {"p15", 4, 2, 2}, {"p16", 5, 0, 2},
        {"p17", 5, 1, 1}, {"p18", 5, 2, 0}
    };

    const int NUM_INPUTS  = 3;
    const int NUM_OUTPUTS = 3;

    // Part 1: Standard FIFO with HoL blocking
    FIFOSwitch fifo_sw(NUM_INPUTS, NUM_OUTPUTS, trace);
    int t_fifo = fifo_sw.run();

    // Part 2: VOQ with optimal exhaustive search
    OptimalVOQSwitch voq_sw(trace, NUM_INPUTS, NUM_OUTPUTS);
    int t_voq = voq_sw.run();

    // Part 3: VOQ with iSLIP scheduling
    ISLIPSwitch islip_sw(trace, NUM_INPUTS, NUM_OUTPUTS);
    int t_islip = islip_sw.run();

    cout << "Total service time  FIFO          : " << t_fifo   << " slots\n";
    cout << "Total service time  VOQ (optimal) : " << t_voq    << " slots\n";
    cout << "Total service time  iSLIP         : " << t_islip  << " slots\n";

    return 0;
}
