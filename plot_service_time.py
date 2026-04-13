"""
plot_service_time.py
--------------------
Graph 1 (Assignment Part 4): Bar chart comparing the total service time
(time slots to drain the switch) for FIFO, VOQ Optimal, and iSLIP.

Update the `service_times` dict below after running the simulation.
"""

import matplotlib.pyplot as plt

# coded manually the values from main.cpp
service_times = {
    "FIFO":         11,
    "VOQ Optimal":   7,
    "iSLIP":         8,
}

labels = list(service_times.keys())
values = list(service_times.values())
colors = ["#4C72B0", "#55A868", "#C44E52"]

fig, ax = plt.subplots(figsize=(6, 5))
bars = ax.bar(labels, values, color=colors, width=0.5, edgecolor="white")

for bar, val in zip(bars, values):
    ax.text(bar.get_x() + bar.get_width() / 2,
            val + 0.15, str(val),
            ha="center", va="bottom", fontweight="bold")

ax.set_xlabel("Scheduling Algorithm")
ax.set_ylabel("Total Service Time (slots)")
ax.set_title("Service Time Comparison  —  3×3 Switch")
ax.set_ylim(0, max(values) + 2)
ax.grid(axis="y", linestyle="--", alpha=0.6)

plt.tight_layout()
plt.savefig("service_time_plot.png", dpi=150)
plt.show()
print("Saved: service_time_plot.png")
