import argparse
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--ox", required=True, help="Plik runs dla crossover OX")
    parser.add_argument("--pmx", required=True, help="Plik runs dla crossover PMX")
    parser.add_argument("--name", default="", help="Suffix nazw wyjściowych")

    args = parser.parse_args()

    if not Path(args.ox).exists():
        raise FileNotFoundError(args.ox)

    if not Path(args.pmx).exists():
        raise FileNotFoundError(args.pmx)

    ox = pd.read_csv(args.ox)
    pmx = pd.read_csv(args.pmx)

    ox["method"] = "OX"
    pmx["method"] = "PMX"

    df = pd.concat([ox, pmx], ignore_index=True)

    suffix = f"_{args.name}" if args.name else ""

    summary = df.groupby("method")["bestCost"].agg(
        ["count", "min", "mean", "max", "std"]
    ).reset_index()

    summary_file = f"ga_crossover_summary{suffix}.csv"
    summary.to_csv(summary_file, index=False)

    plt.figure(figsize=(8, 5))
    plt.boxplot(
        [
            ox["bestCost"],
            pmx["bestCost"],
        ],
        labels=["OX", "PMX"],
    )
    plt.ylabel("Najlepszy koszt")
    plt.title("Porównanie metod krzyżowania GA")
    plt.grid(True, axis="y")
    plt.tight_layout()

    boxplot_file = f"ga_crossover_boxplot{suffix}.png"
    plt.savefig(boxplot_file, dpi=300)

    plt.figure(figsize=(9, 5))
    plt.hist(ox["bestCost"], bins=15, alpha=0.6, label="OX")
    plt.hist(pmx["bestCost"], bins=15, alpha=0.6, label="PMX")
    plt.xlabel("Najlepszy koszt")
    plt.ylabel("Liczba prób")
    plt.title("Rozkład wyników dla OX i PMX")
    plt.legend()
    plt.grid(True, axis="y")
    plt.tight_layout()

    hist_file = f"ga_crossover_histogram{suffix}.png"
    plt.savefig(hist_file, dpi=300)

    print("Podsumowanie:")
    print(summary)
    print("\nZapisano:")
    print(summary_file)
    print(boxplot_file)
    print(hist_file)


if __name__ == "__main__":
    main()
