# Directory structure
```
assignment/
├── backlogs/
├── logs/
├── Result_graphs/
├── fifo_switch.h
├── voq_optimal.h
├── islip_switch.h
├── packet.h
├── main.cpp
├── plot_backlog.py
├── plot_service_time.py
├── run_all.sh
└── README.md
```
# Dependencies
- g++ (C++17 or later)
- python3 (or python — older versions may not be compatible)
- Linux (shell script and g++ tested on Linux/ Windows with WSL)

# Execution
Open terminal/shell in the repo which `run_all.sh` exists
```bash
chmod +x run_all.sh
./run_all.sh
```

# Results
- Logs are written to the logs/ directory, labelled by the corresponding part of the assignment
- Backlog data is written to the backlogs/ directory
- Results or graphs are located in `Result_graphs/` folder
