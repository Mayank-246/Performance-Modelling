#ifndef FIFO_SWITCH_H
#define FIFO_SWITCH_H

#include <deque>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "packet.h"

using namespace std;

/*
 * FIFOSwitch
 * ----------
 * Simulates a crossbar switch where each input port holds a single
 * shared FIFO queue. Only the head-of-line (HoL) packet may contend
 * for an output port each time slot. Tie-breaking: lowest-numbered
 * input port wins when multiple inputs target the same output.
 */
struct FIFOSwitch {

    // Per-input FIFO queues
    vector<deque<Packet>> input_queues;
    int num_outputs;

    // Packet trace (sorted by arrival time)
    vector<Packet> pkt_trace;
    int trace_idx;        // next unprocessed packet in trace
    int num_delivered;    // packets successfully forwarded
    int clock;            // current time slot

    // Output files
    ofstream hol_log;
    ofstream slot_log;

    // Backlog samples: (time_slot, queue_depth)
    vector<pair<int,int>> backlog_samples;

    // ---- Constructor ----
    FIFOSwitch(int num_in, int num_out, vector<Packet>& pkts)
        : num_outputs(num_out), pkt_trace(pkts),
          trace_idx(0), num_delivered(0), clock(0)
    {
        input_queues.resize(num_in);
        hol_log.open("logs/hol_fifo.txt");
        slot_log.open("logs/slots_fifo.txt");
    }

    // ---- Public entry point ----
    int run() {
        while (num_delivered < (int)pkt_trace.size()) {
            enqueue_arrivals();

            vector<bool> port_blocked(input_queues.size(), false);
            vector<bool> output_busy(num_outputs, false);

            vector<Packet> forwarded = attempt_forwarding(port_blocked, output_busy);
            record_slot(forwarded, port_blocked, output_busy);

            // Sample backlog after forwarding
            int depth = 0;
            for (auto& q : input_queues) depth += (int)q.size();
            backlog_samples.push_back({clock, depth});

            clock++;
        }

        // Write backlog data
        ofstream bf("backlogs/fifo_backlog.txt");
        for (auto& s : backlog_samples)
            bf << s.first << " " << s.second << "\n";

        return clock;
    }

private:

    // Push newly arrived packets onto their input queues
    void enqueue_arrivals() {
        while (trace_idx < (int)pkt_trace.size() &&
               pkt_trace[trace_idx].arrival <= clock)
        {
            int port = pkt_trace[trace_idx].in_port;
            input_queues[port].push_back(pkt_trace[trace_idx]);
            trace_idx++;
        }
    }

    // Try to forward one packet per non-blocked input
    vector<Packet> attempt_forwarding(vector<bool>& port_blocked,
                                      vector<bool>& output_busy)
    {
        vector<Packet> sent;
        for (int i = 0; i < (int)input_queues.size(); i++) {
            if (input_queues[i].empty() || port_blocked[i]) continue;

            int dst = input_queues[i].front().out_port;
            if (!output_busy[dst]) {
                Packet p = input_queues[i].front();
                input_queues[i].pop_front();
                p.arrival = clock;  // reuse field to record departure time
                sent.push_back(p);
                output_busy[dst] = true;
                num_delivered++;
            } else {
                port_blocked[i] = true;
            }
        }
        return sent;
    }

    // Detect and log HoL blocking events
    void check_hol_blocking(const vector<bool>& port_blocked,
                            const vector<bool>& output_busy)
    {
        for (int i = 0; i < (int)input_queues.size(); i++) {
            if (!port_blocked[i] || input_queues[i].empty()) continue;

            for (int k = 1; k < (int)input_queues[i].size(); k++) {
                int candidate_dst = input_queues[i][k].out_port;
                if (!output_busy[candidate_dst]) {
                    hol_log << "[t=" << clock << "] HoL block on I" << i
                            << ": packet " << input_queues[i][k].pid
                            << " (-> O" << candidate_dst << ") is stuck behind "
                            << input_queues[i][0].pid << "\n";
                }
            }
        }
    }

    // Write per-slot forwarding log and detect HoL events
    void record_slot(const vector<Packet>& sent,
                     const vector<bool>& port_blocked,
                     const vector<bool>& output_busy)
    {
        for (auto& p : sent)
            slot_log << "t=" << clock << "  packet " << p.pid
                     << "  Input port " << p.in_port << " -> Output port " << p.out_port << "\n";

        check_hol_blocking(port_blocked, output_busy);
        slot_log << "---\n";
    }
};

#endif // FIFO_SWITCH_H
