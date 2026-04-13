#ifndef ISLIP_SWITCH_H
#define ISLIP_SWITCH_H

#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include "packet.h"

using namespace std;

/*
 * ISLIPSwitch
 * -----------
 * Implements the iSLIP matching algorithm on top of Virtual Output Queues.
 * Each time slot runs up to 3 Request-Grant-Accept (RGA) iterations.
 * Round-robin arbiters at each output and each input break ties fairly
 * and advance their pointers only when a match is accepted.
 */
struct ISLIPSwitch {

    int N;                              // switch dimension (N×N)
    vector<Packet> pkt_trace;
    vector<vector<queue<Packet>>> voq;  // voq[i][j]: input i -> output j

    // Round-robin pointers for arbiters
    vector<int> grant_ptr;   // per-output: next input to consider
    vector<int> accept_ptr;  // per-input:  next output to consider

    // Per-slot match results (-1 = unmatched)
    vector<int> matched_out;  // matched_out[i] = output granted to input i
    vector<int> matched_in;   // matched_in[j]  = input accepted by output j

    ofstream detail_log;

    int clock;
    int total_delivered;

    // ---- Constructor ----
    ISLIPSwitch(const vector<Packet>& trace, int n_in, int n_out)
        : N(n_in), pkt_trace(trace), clock(0), total_delivered(0)
    {
        voq.resize(N, vector<queue<Packet>>(N));
        grant_ptr.assign(N, 0);
        accept_ptr.assign(N, 0);
        matched_out.resize(N);
        matched_in.resize(N);
        detail_log.open("logs/slots_islip.txt");
    }

    // ---- Public entry point ----
    int run() {
        vector<pair<int,int>> backlog_samples;
        int total = (int)pkt_trace.size();

        while (total_delivered < total) {
            detail_log << "=== t=" << clock << " ===\n";

            enqueue_arrivals();
            run_islip();
            int sent = forward_packets();

            if (sent == 0)
                detail_log << "  (no transmissions)\n";
            else
                detail_log << "  Forwarded this slot: " << sent << "\n";

            // Sample backlog
            int depth = 0;
            for (int i = 0; i < N; i++)
                for (int j = 0; j < N; j++)
                    depth += (int)voq[i][j].size();
            backlog_samples.push_back({clock, depth});

            clock++;
        }

        ofstream bf("backlogs/islip_backlog.txt");
        for (auto& s : backlog_samples)
            bf << s.first << " " << s.second << "\n";

        return clock;
    }

private:

    // Push all packets with arrival <= clock into their VOQs
    void enqueue_arrivals() {
        for (auto& p : pkt_trace) {
            if (p.arrival == clock) {
                voq[p.in_port][p.out_port].push(p);
                detail_log << "  Arrived: Packet " << p.pid
                           << "  Input Port " << p.in_port << "->Output Port " << p.out_port << "\n";
            }
        }
    }

    // iSLIP: three Request-Grant-Accept rounds
    void run_islip() {
        // Clear per-slot match arrays
        fill(matched_out.begin(), matched_out.end(), -1);
        fill(matched_in.begin(),  matched_in.end(),  -1);

        for (int round = 0; round < 3; round++) {

            // ---- REQUEST ----
            // Each unmatched input requests every output for which it
            // has a non-empty VOQ and which is still unmatched.
            vector<vector<int>> requests(N);  // requests[j] = list of inputs
            for (int i = 0; i < N; i++) {
                if (matched_out[i] != -1) continue;
                for (int j = 0; j < N; j++)
                    if (!voq[i][j].empty() && matched_in[j] == -1)
                        requests[j].push_back(i);
            }

            // ---- GRANT ----
            // Each unmatched output grants the first requesting input
            // found starting from its round-robin pointer.
            vector<int> granted(N, -1);
            for (int j = 0; j < N; j++) {
                if (matched_in[j] != -1 || requests[j].empty()) continue;

                for (int k = 0; k < N; k++) {
                    int candidate = (grant_ptr[j] + k) % N;
                    for (int req : requests[j]) {
                        if (req == candidate) {
                            granted[j] = candidate;
                            break;
                        }
                    }
                    if (granted[j] != -1) break;
                }
            }

            // ---- ACCEPT ----
            // Each unmatched input accepts the first grant found
            // starting from its round-robin pointer, then both pointers advance.
            for (int i = 0; i < N; i++) {
                if (matched_out[i] != -1) continue;

                for (int k = 0; k < N; k++) {
                    int j = (accept_ptr[i] + k) % N;
                    if (granted[j] == i) {
                        matched_out[i] = j;
                        matched_in[j]  = i;
                        // Advance pointers past the accepted ports
                        accept_ptr[i] = (j + 1) % N;
                        grant_ptr[j]  = (i + 1) % N;
                        break;
                    }
                }
            }
        }
    }

    // Send the matched packets, return count forwarded
    int forward_packets() {
        int sent = 0;
        for (int i = 0; i < N; i++) {
            int j = matched_out[i];
            if (j == -1 || voq[i][j].empty()) continue;

            Packet p = voq[i][j].front();
            voq[i][j].pop();
            detail_log << "  Sent: Packet " << p.pid
                       << "  Input Port " << i << "->Output Port " << j << "\n";
            total_delivered++;
            sent++;
        }
        return sent;
    }
};

#endif // ISLIP_SWITCH_H
