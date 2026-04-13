#ifndef VOQ_OPTIMAL_H
#define VOQ_OPTIMAL_H

#include <iostream>
#include <vector>
#include <deque>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <fstream>
#include "packet.h"

using namespace std;

/*
 * OptimalVOQSwitch
 * ----------------
 * Each input port maintains N separate Virtual Output Queues (one per
 * destination). An exhaustive DFS search with pruning finds the matching
 * sequence that minimises the total service time (time until all packets
 * are delivered).
 */
struct OptimalVOQSwitch {

    int N_in, N_out;              // switch dimensions
    vector<Packet> pkt_trace;     // full packet trace

    // voqs[i][j] = queue of packets at input i destined for output j
    // Indexed for DFS; rebuilt from arrivals[] each run.
    vector<vector<deque<Packet>>> voqs;

    // arrivals[t] = list of packets that arrive at time t
    vector<vector<Packet>> arrivals;

    // DFS state
    int   best_time;
    vector<vector<Packet>> best_schedule;
    unordered_map<string, int> memo;  // state -> earliest time seen

    ofstream result_log;

    // ---- Constructor ----
    OptimalVOQSwitch(const vector<Packet>& trace, int n_in, int n_out)
        : pkt_trace(trace), N_in(n_in), N_out(n_out), best_time(1000)
    {
        // Pre-bucket packets by arrival time
        int horizon = 0;
        for (auto& p : pkt_trace) horizon = max(horizon, p.arrival);
        arrivals.resize(horizon + 50);
        for (auto& p : pkt_trace) arrivals[p.arrival].push_back(p);

        result_log.open("logs/slots_voq.txt");
    }

    // ---- Public entry point ----
    int run() {
        voqs.assign(N_in, vector<deque<Packet>>(N_out));
        vector<vector<Packet>> schedule;
        dfs(0, voqs, 0, schedule);

        // Write optimal schedule
        result_log << "Optimal Schedule (min service time = "
                   << best_time << "):\n";
        for (int t = 0; t < (int)best_schedule.size(); t++) {
            result_log << "t=" << t << ": ";
            if (best_schedule[t].empty()) {
                result_log << "(idle)\n";
            } else {
                for (auto& p : best_schedule[t])
                    result_log << p.pid << " Input Port" << p.in_port
                               << "->Output Port" << p.out_port << "  ";
                result_log << "\n";
            }
        }

        write_backlog();
        return best_time;
    }

private:

    // True when every VOQ is empty
    bool queues_empty(const vector<vector<deque<Packet>>>& q) const {
        for (int i = 0; i < N_in; i++)
            for (int j = 0; j < N_out; j++)
                if (!q[i][j].empty()) return false;
        return true;
    }

    // Compact state key for memoisation
    string make_key(const vector<vector<deque<Packet>>>& q, int idx) const {
        string key = to_string(idx) + "|";
        for (int i = 0; i < N_in; i++)
            for (int j = 0; j < N_out; j++)
                key += to_string(q[i][j].size()) + ",";
        return key;
    }

    // Generate all valid bipartite matchings for the current VOQ state.
    // A matching is a set of (input, output) pairs with no conflicts.
    // Results are sorted descending by cardinality (try larger matchings first).
    vector<vector<Packet>> enumerate_matchings(
        const vector<vector<deque<Packet>>>& q) const
    {
        vector<vector<Packet>> result;

        // For a 3×3 switch enumerate all choice combinations
        for (int c0 = -1; c0 < N_out; c0++) {
            for (int c1 = -1; c1 < N_out; c1++) {
                for (int c2 = -1; c2 < N_out; c2++) {

                    vector<int> chosen = {c0, c1, c2};
                    bool ok = true;

                    // Each chosen output must have a packet waiting
                    for (int i = 0; i < N_in && ok; i++)
                        if (chosen[i] != -1 && q[i][chosen[i]].empty())
                            ok = false;

                    // No two inputs may share an output
                    for (int i = 0; i < N_in && ok; i++)
                        for (int j = i + 1; j < N_in && ok; j++)
                            if (chosen[i] != -1 && chosen[i] == chosen[j])
                                ok = false;

                    if (!ok) continue;

                    vector<Packet> match;
                    for (int i = 0; i < N_in; i++)
                        if (chosen[i] != -1)
                            match.push_back(q[i][chosen[i]].front());

                    if (!match.empty()) result.push_back(match);
                }
            }
        }

        // Prefer larger matchings to prune faster
        sort(result.begin(), result.end(),
             [](const vector<Packet>& a, const vector<Packet>& b) {
                 return a.size() > b.size();
             });
        return result;
    }

    // DFS over time slots; prunes branches that cannot beat best_time
    void dfs(int t,
             vector<vector<deque<Packet>>>& q,
             int n_arrived,
             vector<vector<Packet>>& schedule)
    {
        // Lower-bound pruning: even perfectly packing the remaining
        // packets cannot finish faster than this many additional slots
        int remaining = 0;
        for (int i = 0; i < N_in; i++)
            for (int j = 0; j < N_out; j++)
                remaining += (int)q[i][j].size();
        remaining += (int)(pkt_trace.size() - n_arrived);

        int lower_bound = (remaining + N_in - 1) / N_in;
        if (t + lower_bound >= best_time) return;

        // Memoisation: skip if we reached this state at an earlier time
        string key = make_key(q, n_arrived);
        if (memo.count(key) && memo[key] <= t) return;
        memo[key] = t;

        // Enqueue packets that arrive at time t
        for (auto& p : arrivals[t]) {
            q[p.in_port][p.out_port].push_back(p);
            n_arrived++;
        }

        // Goal: all packets arrived and all queues empty
        if (n_arrived == (int)pkt_trace.size() && queues_empty(q)) {
            if (t < best_time) {
                best_time = t;
                best_schedule = schedule;
            }
            return;
        }

        // Idle slot when nothing is queued yet
        if (queues_empty(q)) {
            schedule.push_back({});
            dfs(t + 1, q, n_arrived, schedule);
            schedule.pop_back();
            return;
        }

        // Try every valid matching
        for (auto& match : enumerate_matchings(q)) {
            // Apply matching
            for (auto& p : match)
                q[p.in_port][p.out_port].pop_front();

            schedule.push_back(match);
            dfs(t + 1, q, n_arrived, schedule);
            schedule.pop_back();

            // Undo matching
            for (auto it = match.rbegin(); it != match.rend(); ++it)
                q[it->in_port][it->out_port].push_front(*it);
        }
    }

    // Re-simulate best_schedule to compute per-slot backlog
    void write_backlog() {
        vector<vector<deque<Packet>>> q(N_in, vector<deque<Packet>>(N_out));
        int idx = 0;
        ofstream bf("backlogs/voq_backlog.txt");

        for (int t = 0; t < (int)best_schedule.size(); t++) {
            // Arrivals
            for (auto& p : arrivals[t]) {
                q[p.in_port][p.out_port].push_back(p);
                idx++;
            }
            // Forward
            for (auto& p : best_schedule[t])
                q[p.in_port][p.out_port].pop_front();

            // Count remaining
            int depth = 0;
            for (int i = 0; i < N_in; i++)
                for (int j = 0; j < N_out; j++)
                    depth += (int)q[i][j].size();

            bf << t << " " << depth << "\n";
        }
    }
};

#endif // VOQ_OPTIMAL_H
