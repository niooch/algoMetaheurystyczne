import argparse
from pathlib import Path

import pandas as pd
import matplotlib.pyplot as plt


def with_suffix(base_name: str, suffix: str, extension: str) -> str:
    if suffix:
        return f"{base_name}_{suffix}.{extension}"
    return f"{base_name}.{extension}"


def plot_history(history_file: str, output_file: str):
    history = pd.read_csv(history_file)

    plt.figure(figsize=(10, 5))
    plt.plot(history["iteration"], history["bestCost"])
    plt.xlabel("Iteracja")
    plt.ylabel("Najlepszy koszt")
    plt.title("Tabu Search - zmiana najlepszego kosztu w czasie")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(output_file, dpi=300)


def plot_runs_bar(runs_file: str, output_file: str):
    runs = pd.read_csv(runs_file)

    plt.figure(figsize=(10, 5))
    plt.bar(runs["run"], runs["bestCost"])
    plt.axhline(
        runs["bestCost"].mean(),
        linestyle="--",
        label=f"Średnia = {runs['bestCost'].mean():.2f}",
    )
    plt.xlabel("Numer próby")
    plt.ylabel("Najlepszy koszt")
    plt.title("Tabu Search - wynik w kolejnych próbach")
    plt.legend()
    plt.grid(True, axis="y")
    plt.tight_layout()
    plt.savefig(output_file, dpi=300)


def plot_runs_histogram(runs_file: str, output_file: str):
    runs = pd.read_csv(runs_file)

    plt.figure(figsize=(10, 5))
    plt.hist(runs["bestCost"], bins=15)
    plt.axvline(
        runs["bestCost"].mean(),
        linestyle="--",
        label=f"Średnia = {runs['bestCost'].mean():.2f}",
    )
    plt.axvline(
        runs["bestCost"].min(),
        linestyle="-",
        label=f"Najlepszy = {runs['bestCost'].min():.2f}",
    )
    plt.xlabel("Najlepszy koszt w próbie")
    plt.ylabel("Liczba prób")
    plt.title("Tabu Search - rozkład wyników")
    plt.legend()
    plt.grid(True, axis="y")
    plt.tight_layout()
    plt.savefig(output_file, dpi=300)


def plot_best_route(route_file: str, output_file: str, invert_y: bool):
    route = pd.read_csv(route_file)

    plt.figure(figsize=(7, 7))

    plt.plot(route["y"], route["x"], marker="o")

    plt.xlabel("X")
    plt.ylabel("Y")
    plt.title("Tabu Search - najlepsza znaleziona trasa")
    plt.grid(True)
    plt.axis("equal")

    if invert_y:
        plt.gca().invert_yaxis()

    plt.tight_layout()
    plt.savefig(output_file, dpi=300)


def save_summary(runs_file: str, output_file: str):
    runs = pd.read_csv(runs_file)

    best_idx = runs["bestCost"].idxmin()
    best_row = runs.loc[best_idx]

    text = []
    text.append("Podsumowanie wyników Tabu Search")
    text.append("=" * 40)
    text.append(f"Liczba prób: {len(runs)}")
    text.append(f"Najlepszy koszt: {runs['bestCost'].min():.4f}")
    text.append(f"Średni koszt: {runs['bestCost'].mean():.4f}")
    text.append(f"Najgorszy koszt: {runs['bestCost'].max():.4f}")
    text.append(f"Odchylenie standardowe: {runs['bestCost'].std():.4f}")
    text.append("")
    text.append(f"Najlepsza próba: {int(best_row['run'])}")
    text.append(f"Seed najlepszej próby: {int(best_row['seed'])}")
    text.append(f"Iteracje najlepszej próby: {int(best_row['iterationsDone'])}")

    Path(output_file).write_text("\n".join(text), encoding="utf-8")

    print("\n".join(text))


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("--history", default="history.csv")
    parser.add_argument("--runs", default="runs.csv")
    parser.add_argument("--route", default="best_route.csv")

    parser.add_argument(
        "--name",
        default="",
        help="Suffix dodawany do nazw plików wynikowych, np. wi29_tabu_100runs",
    )

    parser.add_argument(
        "--invert-y",
        type=int,
        default=1,
        help="1 = odwróć oś Y na wykresie trasy, 0 = nie odwracaj",
    )

    args = parser.parse_args()

    required_files = [args.history, args.runs, args.route]

    for file in required_files:
        if not Path(file).exists():
            raise FileNotFoundError(f"Nie znaleziono pliku: {file}")

    suffix = args.name.strip()

    history_out = with_suffix("tabu_history", suffix, "png")
    bar_out = with_suffix("tabu_runs_bar", suffix, "png")
    hist_out = with_suffix("tabu_runs_histogram", suffix, "png")
    route_out = with_suffix("tabu_best_route", suffix, "png")
    summary_out = with_suffix("tabu_summary", suffix, "txt")

    plot_history(args.history, history_out)
    plot_runs_bar(args.runs, bar_out)
    plot_runs_histogram(args.runs, hist_out)
    plot_best_route(args.route, route_out, bool(args.invert_y))
    save_summary(args.runs, summary_out)

    print("\nZapisano pliki:")
    print(history_out)
    print(bar_out)
    print(hist_out)
    print(route_out)
    print(summary_out)


if __name__ == "__main__":
    main()
