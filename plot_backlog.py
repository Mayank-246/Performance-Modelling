"""
plot_backlog.py
---------------
Graph 2 (Assignment Part 4): Line chart of total packets remaining in
the switch (backlog) over time for all three scheduling strategies.
"""

import matplotlib.pyplot as plt
import os


def read_backlog(filepath):
    slots, counts = [], []
    with open(filepath) as fh:
        for line in fh:
            t, b = map(int, line.split())
            slots.append(t)
            counts.append(b)
    return slots, counts


base = os.path.dirname(__file__)

t_fifo,  b_fifo  = read_backlog(os.path.join(base, "backlogs/fifo_backlog.txt"))
t_voq,   b_voq   = read_backlog(os.path.join(base, "backlogs/voq_backlog.txt"))
t_islip, b_islip = read_backlog(os.path.join(base, "backlogs/islip_backlog.txt"))

fig, ax = plt.subplots(figsize=(8, 5))

ax.plot(t_fifo,  b_fifo,  marker="o", linewidth=1.8, label="FIFO")
ax.plot(t_voq,   b_voq,   marker="s", linewidth=1.8, label="VOQ Optimal")
ax.plot(t_islip, b_islip, marker="^", linewidth=1.8, label="iSLIP")

ax.set_xlabel("Time Slot (t)")
ax.set_ylabel("Packets Remaining in Switch")
ax.set_title("Backlog vs. Time  —  3×3 Switch Simulation")
ax.legend()
ax.grid(True, linestyle="--", alpha=0.6)

plt.tight_layout()
plt.savefig("backlog_plot.png", dpi=150)
plt.show()
print("Saved: backlog_plot.png")
