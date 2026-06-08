import pandas as pd
import matplotlib.pyplot as plt
import sys

history = pd.read_csv("history.csv")
# read name from arguments 
name = sys.argv[1] if len(sys.argv) > 1 else "unknown"

plt.figure()
plt.plot(history["epoch"], history["bestCost"], label="Najlepszy koszt")
plt.plot(history["epoch"], history["currentCost"], label="Aktualny koszt", alpha=0.6)
plt.xlabel("Epoka")
plt.ylabel("Długość trasy")
plt.title("Zmiana jakości rozwiązania w czasie")
plt.legend()
plt.grid(True)
plt.savefig("cost_history_" +name+ ".png", dpi=300)

plt.figure()
plt.plot(history["epoch"], history["temperature"])
plt.xlabel("Epoka")
plt.ylabel("Temperatura")
plt.title("Spadek temperatury")
plt.grid(True)
plt.savefig("temperature_" +name+".png", dpi=300)

route = pd.read_csv("best_route.csv")

plt.figure()

# Odwrócone osie: x <-> y
plt.plot(route["y"], route["x"], marker="o")

# Labele zostają takie same
plt.xlabel("X")
plt.ylabel("Y")
plt.title("Najlepsza znaleziona trasa")
plt.grid(True)
plt.axis("equal")
#plt.gca().invert_yaxis()
#plt.gca().invert_xaxis()
plt.savefig("best_route_"+name+".png", dpi=300)
