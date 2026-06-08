#!/usr/bin/env python3
"""Create report plots from one or more Lab 1 CSV result files.

This script mirrors the structure of the Lab 0 plotting scripts:
- read the CSV files produced by `lab1_experiment`
- generate cross-instance comparison charts for the three exercises
- generate one best-tour comparison figure per instance

Examples
--------
python3 plot_results.py data/*.csv
python3 plot_results.py results/*.csv --output-dir plots_lab1 --annotate
"""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple

import matplotlib.pyplot as plt


SummaryKey = Tuple[str, str]
SummaryMap = Dict[SummaryKey, float]

ALGORITHMS: list[tuple[str, str, str]] = [
    ("ls_full_invert", "Exercise 1", "#4C72B0"),
    ("ls_sampled_invert", "Exercise 2", "#55A868"),
    ("mst_seeded_ls", "Exercise 3", "#C44E52"),
]

COMPARISON_PLOTS: list[tuple[str, str, str, str]] = [
    ("avg_final_length", "Average Final Length", "Tour length", "avg_final_length.png"),
    (
        "avg_improvement_steps",
        "Average Improvement Steps",
        "Accepted improvements",
        "avg_improvement_steps.png",
    ),
    ("best_length", "Best Length", "Tour length", "best_length.png"),
]


class CsvFormatError(Exception):
    """Raised when the input CSV does not match the expected schema."""


@dataclass(frozen=True)
class TourPoint:
    step: int
    node_id: int
    x: float
    y: float


@dataclass
class InstanceData:
    instance: str
    summary: SummaryMap
    tours: dict[str, list[TourPoint]]


def safe_float(text: str) -> float:
    text = (text or "").strip()
    if text == "":
        raise ValueError("empty numeric field")
    return float(text)


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def read_results(csv_path: Path) -> InstanceData:
    summary: SummaryMap = {}
    tours: dict[str, list[TourPoint]] = {}
    instance_name = csv_path.stem

    with csv_path.open("r", newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        required = {
            "row_type",
            "instance",
            "algorithm",
            "metric",
            "value",
            "step",
            "node_id",
            "x",
            "y",
        }
        missing = required.difference(reader.fieldnames or [])
        if missing:
            raise CsvFormatError(
                f"{csv_path}: missing required columns: {', '.join(sorted(missing))}"
            )

        for row in reader:
            row_type = (row.get("row_type") or "").strip()
            instance = (row.get("instance") or "").strip()
            if instance:
                instance_name = instance

            if row_type == "summary":
                algorithm = (row.get("algorithm") or "").strip()
                metric = (row.get("metric") or "").strip()
                try:
                    value = safe_float(row.get("value") or "")
                except ValueError:
                    continue
                summary[(algorithm, metric)] = value
                continue

            if row_type != "tour":
                continue

            algorithm = (row.get("algorithm") or "").strip()
            metric = (row.get("metric") or "").strip()
            if metric != "best_tour":
                continue

            step_text = (row.get("step") or "").strip()
            node_id_text = (row.get("node_id") or "").strip()
            x_text = (row.get("x") or "").strip()
            y_text = (row.get("y") or "").strip()
            if not step_text or not node_id_text or not x_text or not y_text:
                continue

            tours.setdefault(algorithm, []).append(
                TourPoint(
                    step=int(step_text),
                    node_id=int(node_id_text),
                    x=float(x_text),
                    y=float(y_text),
                )
            )

    for algorithm_points in tours.values():
        algorithm_points.sort(key=lambda point: point.step)

    return InstanceData(instance=instance_name, summary=summary, tours=tours)


def require_summary_metric(
    instance_data: InstanceData,
    algorithm: str,
    metric: str,
) -> float:
    value = instance_data.summary.get((algorithm, metric))
    if value is None:
        raise CsvFormatError(
            f"{instance_data.instance}: missing summary metric {algorithm}/{metric}"
        )
    return value


def save_grouped_metric_plot(
    instances: list[InstanceData],
    metric: str,
    label: str,
    ylabel: str,
    output_path: Path,
    title_prefix: str,
) -> None:
    if not instances:
        raise ValueError("no instances to plot")

    labels = [instance.instance for instance in instances]
    x_positions = list(range(len(labels)))
    width = 0.24

    fig, ax = plt.subplots(figsize=(max(9, len(labels) * 1.5), 6))

    for offset_index, (algorithm, algorithm_label, color) in enumerate(ALGORITHMS):
        values = [require_summary_metric(instance, algorithm, metric) for instance in instances]
        offset = (offset_index - 1) * width
        bar_positions = [x + offset for x in x_positions]
        bars = ax.bar(bar_positions, values, width=width, label=algorithm_label, color=color)

        for bar in bars:
            height = bar.get_height()
            ax.text(
                bar.get_x() + bar.get_width() / 2.0,
                height,
                f"{height:.0f}",
                ha="center",
                va="bottom",
                fontsize=8,
                rotation=90,
            )

    ax.set_title(f"{title_prefix} - {label}")
    ax.set_xlabel("Instance")
    ax.set_ylabel(ylabel)
    ax.set_xticks(x_positions)
    ax.set_xticklabels(labels)
    ax.grid(axis="y", linestyle="--", alpha=0.4)
    ax.legend()

    fig.tight_layout()
    fig.savefig(output_path, dpi=200, bbox_inches="tight")
    plt.close(fig)


def save_mst_weight_plot(
    instances: list[InstanceData],
    output_path: Path,
    title_prefix: str,
) -> None:
    labels = [instance.instance for instance in instances]
    values = [require_summary_metric(instance, "mst", "weight") for instance in instances]
    x_positions = list(range(len(labels)))

    fig, ax = plt.subplots(figsize=(max(9, len(labels) * 1.3), 6))
    bars = ax.bar(x_positions, values, color="#8172B2")

    for bar in bars:
        height = bar.get_height()
        ax.text(
            bar.get_x() + bar.get_width() / 2.0,
            height,
            f"{height:.0f}",
            ha="center",
            va="bottom",
            fontsize=8,
            rotation=90,
        )

    ax.set_title(f"{title_prefix} - MST Weight")
    ax.set_xlabel("Instance")
    ax.set_ylabel("Weight")
    ax.set_xticks(x_positions)
    ax.set_xticklabels(labels)
    ax.grid(axis="y", linestyle="--", alpha=0.4)

    fig.tight_layout()
    fig.savefig(output_path, dpi=200, bbox_inches="tight")
    plt.close(fig)


def plot_best_tour(
    ax: plt.Axes,
    instance: InstanceData,
    algorithm: str,
    title: str,
    annotate: bool,
) -> None:
    points = instance.tours.get(algorithm, [])
    if not points:
        ax.set_title(f"{title}\nmissing tour")
        ax.text(0.5, 0.5, "No best_tour rows", ha="center", va="center", transform=ax.transAxes)
        ax.set_axis_off()
        return

    xs = [point.x for point in points]
    ys = [point.y for point in points]
    node_ids = [point.node_id for point in points]

    best_length = require_summary_metric(instance, algorithm, "best_length")

    ax.plot(xs, ys, linewidth=1.2, color="#2F2F2F")
    ax.scatter(xs[:-1], ys[:-1], s=16, color="#1F77B4", zorder=3)

    if annotate:
        for point in points[:-1]:
            ax.annotate(
                str(point.node_id),
                (point.x, point.y),
                fontsize=7,
                xytext=(3, 3),
                textcoords="offset points",
            )

    ax.set_title(f"{title}\nbest = {best_length:.0f}, nodes = {len(node_ids) - 1}")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_aspect("equal", adjustable="datalim")
    ax.grid(True, alpha=0.3)


def save_best_tour_figure(
    instance: InstanceData,
    output_path: Path,
    annotate: bool,
) -> None:
    fig, axes = plt.subplots(1, len(ALGORITHMS), figsize=(18, 6))

    if len(ALGORITHMS) == 1:
        axes = [axes]

    for ax, (algorithm, label, _color) in zip(axes, ALGORITHMS):
        plot_best_tour(ax, instance, algorithm, label, annotate)

    fig.suptitle(f"{instance.instance} - best tours by algorithm", fontsize=14)
    fig.tight_layout()
    fig.savefig(output_path, dpi=200, bbox_inches="tight")
    plt.close(fig)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Generate report plots from one or more Lab 1 CSV files."
    )
    parser.add_argument(
        "csvs",
        nargs="+",
        type=Path,
        help="CSV files produced by lab1_experiment",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("plots_report"),
        help="Directory for generated PNG files (default: plots_report)",
    )
    parser.add_argument(
        "--title-prefix",
        type=str,
        default="Lab 1 heuristic comparison",
        help="Prefix used in plot titles",
    )
    parser.add_argument(
        "--annotate",
        action="store_true",
        help="Annotate node IDs on best-tour figures",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    for csv_path in args.csvs:
        if not csv_path.exists():
            raise SystemExit(f"CSV file not found: {csv_path}")

    instances = [read_results(path) for path in args.csvs]

    summary_dir = args.output_dir / "summary"
    tours_dir = args.output_dir / "tours"
    ensure_dir(summary_dir)
    ensure_dir(tours_dir)

    generated: list[Path] = []

    for metric, label, ylabel, filename in COMPARISON_PLOTS:
        out_path = summary_dir / filename
        save_grouped_metric_plot(
            instances,
            metric,
            label,
            ylabel,
            out_path,
            args.title_prefix,
        )
        generated.append(out_path)

    mst_plot_path = summary_dir / "mst_weight.png"
    save_mst_weight_plot(instances, mst_plot_path, args.title_prefix)
    generated.append(mst_plot_path)

    for instance in instances:
        out_path = tours_dir / f"{instance.instance}_best_tours.png"
        save_best_tour_figure(instance, out_path, args.annotate)
        generated.append(out_path)

    print("Generated plots:")
    for path in generated:
        print(path)

    print("Instances included:")
    for instance in instances:
        print(f"- {instance.instance}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
