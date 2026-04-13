#!/usr/bin/env bash
# run_all.sh  —  compile, simulate, then generate graphs

set -e

echo "=== Compiling ==="
g++ -O2 -std=c++17 main.cpp -o switch_sim

echo "=== Running simulation ==="
mkdir -p logs backlogs
./switch_sim

echo "=== Generating plots ==="
PYTHON=$(command -v python3 || command -v python)
$PYTHON plot_backlog.py
$PYTHON plot_service_time.py

echo "=== Done ==="
echo "Outputs: backlog_plot.png  service_time_plot.png"
