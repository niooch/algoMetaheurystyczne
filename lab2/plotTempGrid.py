import argparse
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import re
from pathlib import Path


def parse_best_cost(output: str) -> float:
    """
    Szuka w wyjściu programu linii typu:
    Najlepszy koszt trasy: 12345.6789
    """
    patterns = [
        r"Najlepszy koszt trasy:\s*([0-9]+(?:\.[0-9]+)?)",
        r"Najlepszy uzyskany koszt:\s*([0-9]+(?:\.[0-9]+)?)",
    ]

    for pattern in patterns:
        match = re.search(pattern, output)
        if match:
            return float(match.group(1))

    raise ValueError("Nie znaleziono najlepszego kosztu w wyjściu programu.")


def run_sa(
    executable: str,
    instance: str,
    start_temp: float,
    cooling: float,
    trials: int,
    epochs: int,
    no_improve: int,
    seed: int,
) -> float:
    cmd = [
        executable,
        instance,
        "--temp", str(start_temp),
        "--cooling", str(cooling),
        "--trials", str(trials),
        "--epochs", str(epochs),
        "--no-improve", str(no_improve),
        "--seed", str(seed),
    ]

    print("Uruchamiam:", " ".join(cmd))

    result = subprocess.run(
        cmd,
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode != 0:
        print("STDOUT:")
        print(result.stdout)
        print("STDERR:")
        print(result.stderr)
        raise RuntimeError(f"Program zakończył się błędem dla temp={start_temp}, cooling={cooling}")

    return parse_best_cost(result.stdout)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("instance", help="Plik .tsp, np. wi29.tsp")
    parser.add_argument("--exe", default="./sa_tsp", help="Ścieżka do programu sa_tsp")

    parser.add_argument(
        "--temps",
        nargs="+",
        type=float,
        default=[1000, 5000, 10000, 20000, 50000],
        help="Lista temperatur początkowych",
    )

    parser.add_argument(
        "--coolings",
        nargs="+",
        type=float,
        default=[0.90, 0.93, 0.95, 0.97, 0.98, 0.99, 0.995],
        help="Lista współczynników chłodzenia",
    )

    parser.add_argument("--trials", type=int, default=100000)
    parser.add_argument("--epochs", type=int, default=1000)
    parser.add_argument("--no-improve", type=int, default=150)
    parser.add_argument("--seed", type=int, default=123)

    parser.add_argument("--csv", default="temperature_grid_results.csv")
    parser.add_argument("--out", default="temperature_grid_heatmap.png")

    args = parser.parse_args()

    if not Path(args.exe).exists():
        raise FileNotFoundError(f"Nie znaleziono programu: {args.exe}")

    if not Path(args.instance).exists():
        raise FileNotFoundError(f"Nie znaleziono instancji: {args.instance}")

    rows = []

    total = len(args.temps) * len(args.coolings)
    counter = 0

    for start_temp in args.temps:
        for cooling in args.coolings:
            counter += 1

            print(f"\n[{counter}/{total}] temp={start_temp}, cooling={cooling}")

            best_cost = run_sa(
                executable=args.exe,
                instance=args.instance,
                start_temp=start_temp,
                cooling=cooling,
                trials=args.trials,
                epochs=args.epochs,
                no_improve=args.no_improve,
                seed=args.seed,
            )

            rows.append({
                "start_temp": start_temp,
                "cooling": cooling,
                "best_cost": best_cost,
            })

            pd.DataFrame(rows).to_csv(args.csv, index=False)
            print(f"Wynik: {best_cost}")

    df = pd.DataFrame(rows)
    df.to_csv(args.csv, index=False)

    pivot = df.pivot(
        index="start_temp",
        columns="cooling",
        values="best_cost",
    )

    plt.figure(figsize=(10, 6))

    image = plt.imshow(
        pivot.values,
        aspect="auto",
        origin="lower",
    )

    plt.colorbar(image, label="Najlepszy koszt trasy")

    plt.xticks(
        ticks=np.arange(len(pivot.columns)),
        labels=[str(c) for c in pivot.columns],
        rotation=45,
    )

    plt.yticks(
        ticks=np.arange(len(pivot.index)),
        labels=[str(t) for t in pivot.index],
    )

    plt.xlabel("Cooling")
    plt.ylabel("Temperatura początkowa")
    plt.title("Wpływ temperatury początkowej i cooling na wynik SA")

    for y in range(len(pivot.index)):
        for x in range(len(pivot.columns)):
            value = pivot.values[y, x]
            plt.text(
                x,
                y,
                f"{value:.0f}",
                ha="center",
                va="center",
                fontsize=8,
            )

    plt.tight_layout()
    plt.savefig(args.out, dpi=300)
    plt.show()

    print("\nZapisano:")
    print(args.csv)
    print(args.out)


if __name__ == "__main__":
    main()
