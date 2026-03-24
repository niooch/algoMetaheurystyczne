#!/usr/bin/env python3
"""Plot the MST and the MST+DFS tour for a TSPLIB EUC_2D instance.

This script mirrors the logic used in the modular C++ project:
- Euclidean distance rounded to nearest integer (TSPLIB EUC_2D style)
- Prim's algorithm starting from node 0
- DFS preorder on the MST, visiting neighbors in ascending index order

It can work from the .tsp file alone. Optionally, if a results CSV produced by the
C++ program is supplied, the script will also try to read the saved `mst_dfs` tour
from that file and compare it against the recomputed one.

Examples
--------
python3 plot_mst.py dj38.tsp
python3 plot_mst.py dj38.tsp --csv dj38_results.csv
python3 plot_mst.py dj38.tsp --output-dir plots --show --annotate
"""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple

import matplotlib.pyplot as plt


@dataclass(frozen=True)
class Node:
    node_id: int
    x: float
    y: float


@dataclass
class TspInstance:
    name: str
    nodes: List[Node]


@dataclass
class MstResult:
    parent: List[int]
    adjacency: List[List[int]]
    weight: int


def read_tsplib(path: Path) -> TspInstance:
    name = path.stem
    nodes: List[Node] = []
    in_section = False

    with path.open("r", encoding="utf-8") as f:
        for raw_line in f:
            line = raw_line.strip()
            if not line:
                continue

            if not in_section:
                if line.startswith("NAME") and ":" in line:
                    name = line.split(":", 1)[1].strip() or name
                if line == "NODE_COORD_SECTION":
                    in_section = True
                continue

            if line == "EOF":
                break

            parts = line.split()
            if len(parts) < 3:
                continue

            node_id = int(parts[0])
            x = float(parts[1])
            y = float(parts[2])
            nodes.append(Node(node_id=node_id, x=x, y=y))

    if not nodes:
        raise ValueError(f"No nodes found in file: {path}")

    return TspInstance(name=name, nodes=nodes)


def euc2d_distance(a: Node, b: Node) -> int:
    dx = a.x - b.x
    dy = a.y - b.y
    return int(round(math.sqrt(dx * dx + dy * dy)))


def build_distance_matrix(nodes: Sequence[Node]) -> List[List[int]]:
    n = len(nodes)
    dist = [[0] * n for _ in range(n)]
    for i in range(n):
        for j in range(i + 1, n):
            d = euc2d_distance(nodes[i], nodes[j])
            dist[i][j] = d
            dist[j][i] = d
    return dist


def prim_mst(dist: Sequence[Sequence[int]]) -> MstResult:
    n = len(dist)
    inf = 10**18
    min_key = [inf] * n
    parent = [-1] * n
    used = [False] * n
    min_key[0] = 0

    for _ in range(n):
        u = -1
        for v in range(n):
            if not used[v] and (u == -1 or min_key[v] < min_key[u]):
                u = v
        if u == -1:
            raise RuntimeError("Graph appears disconnected.")

        used[u] = True
        for v in range(n):
            if not used[v] and dist[u][v] < min_key[v]:
                min_key[v] = dist[u][v]
                parent[v] = u

    adjacency = [[] for _ in range(n)]
    weight = 0
    for v in range(1, n):
        p = parent[v]
        adjacency[p].append(v)
        adjacency[v].append(p)
        weight += dist[p][v]

    for neighbors in adjacency:
        neighbors.sort()

    return MstResult(parent=parent, adjacency=adjacency, weight=weight)


def dfs_preorder(adjacency: Sequence[Sequence[int]], start: int = 0) -> List[int]:
    visited = [False] * len(adjacency)
    order: List[int] = []

    def dfs(u: int) -> None:
        visited[u] = True
        order.append(u)
        for v in adjacency[u]:
            if not visited[v]:
                dfs(v)

    dfs(start)
    return order


def compute_tour_length(order: Sequence[int], dist: Sequence[Sequence[int]]) -> int:
    if not order:
        return 0
    total = 0
    for i in range(len(order) - 1):
        total += dist[order[i]][order[i + 1]]
    total += dist[order[-1]][order[0]]
    return total


def read_mst_dfs_tour_from_csv(csv_path: Path) -> Optional[List[int]]:
    order: List[Tuple[int, int]] = []
    with csv_path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row.get("row_type") != "tour":
                continue
            if row.get("algorithm") != "mst_dfs":
                continue
            step_text = row.get("step", "").strip()
            node_id_text = row.get("node_id", "").strip()
            if not step_text or not node_id_text:
                continue
            order.append((int(step_text), int(node_id_text)))

    if not order:
        return None

    order.sort()
    node_ids = [node_id for _, node_id in order]

    # The CSV repeats the first vertex at the end for plotting; remove the closing repeat.
    if len(node_ids) >= 2 and node_ids[0] == node_ids[-1]:
        node_ids.pop()

    # Convert 1-based node IDs from the file/CSV into 0-based indices.
    return [node_id - 1 for node_id in node_ids]


def parse_summary_metrics(csv_path: Path) -> Dict[Tuple[str, str], str]:
    metrics: Dict[Tuple[str, str], str] = {}
    with csv_path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row.get("row_type") != "summary":
                continue
            algorithm = row.get("algorithm", "")
            metric = row.get("metric", "")
            value = row.get("value", "")
            metrics[(algorithm, metric)] = value
    return metrics


def mst_edges_from_parent(parent: Sequence[int]) -> List[Tuple[int, int]]:
    return [(p, v) for v, p in enumerate(parent) if v != 0]


def closed_cycle(order: Sequence[int]) -> List[int]:
    if not order:
        return []
    return list(order) + [order[0]]


def plot_mst(
    instance: TspInstance,
    mst: MstResult,
    output_path: Path,
    annotate: bool = False,
) -> None:
    fig, ax = plt.subplots(figsize=(10, 8))

    edges = mst_edges_from_parent(mst.parent)
    for u, v in edges:
        a = instance.nodes[u]
        b = instance.nodes[v]
        ax.plot([a.x, b.x], [a.y, b.y], linewidth=1.5)

    xs = [node.x for node in instance.nodes]
    ys = [node.y for node in instance.nodes]
    ax.scatter(xs, ys, s=28, zorder=3)

    if annotate:
        for idx, node in enumerate(instance.nodes):
            ax.annotate(str(node.node_id), (node.x, node.y), fontsize=8, xytext=(4, 4), textcoords="offset points")

    ax.set_title(f"{instance.name} - Minimum Spanning Tree (weight = {mst.weight})")
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_aspect("equal", adjustable="datalim")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(output_path, dpi=200, bbox_inches="tight")
    plt.close(fig)


def plot_mst_with_dfs_cycle(
    instance: TspInstance,
    mst: MstResult,
    dfs_order: Sequence[int],
    tour_length: int,
    output_path: Path,
    annotate: bool = False,
) -> None:
    fig, ax = plt.subplots(figsize=(10, 8))

    # Draw MST faintly in the background.
    edges = mst_edges_from_parent(mst.parent)
    for u, v in edges:
        a = instance.nodes[u]
        b = instance.nodes[v]
        ax.plot([a.x, b.x], [a.y, b.y], linewidth=1.0, alpha=0.35)

    cycle = closed_cycle(dfs_order)
    for i in range(len(cycle) - 1):
        a = instance.nodes[cycle[i]]
        b = instance.nodes[cycle[i + 1]]
        ax.plot([a.x, b.x], [a.y, b.y], linewidth=2.0)

    xs = [node.x for node in instance.nodes]
    ys = [node.y for node in instance.nodes]
    ax.scatter(xs, ys, s=28, zorder=3)

    start = instance.nodes[dfs_order[0]]
    ax.scatter([start.x], [start.y], s=80, marker="*", zorder=4)

    if annotate:
        for visit_step, idx in enumerate(dfs_order):
            node = instance.nodes[idx]
            label = f"{node.node_id} ({visit_step})"
            ax.annotate(label, (node.x, node.y), fontsize=8, xytext=(4, 4), textcoords="offset points")

    ax.set_title(
        f"{instance.name} - MST + DFS cycle "
        f"(MST weight = {mst.weight}, tour length = {tour_length})"
    )
    ax.set_xlabel("x")
    ax.set_ylabel("y")
    ax.set_aspect("equal", adjustable="datalim")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(output_path, dpi=200, bbox_inches="tight")
    plt.close(fig)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Draw the MST and the MST+DFS cycle for a TSPLIB EUC_2D instance."
    )
    parser.add_argument("input_tsp", type=Path, help="Path to the input .tsp file")
    parser.add_argument("--csv", type=Path, default=None, help="Optional results CSV from the C++ program")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("plots"),
        help="Directory where PNG files will be saved (default: plots)",
    )
    parser.add_argument(
        "--prefix",
        type=str,
        default=None,
        help="Optional filename prefix (default: instance name)",
    )
    parser.add_argument(
        "--annotate",
        action="store_true",
        help="Annotate nodes with IDs / DFS visit order",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="Open the figures interactively after saving",
    )
    args = parser.parse_args()

    instance = read_tsplib(args.input_tsp)
    dist = build_distance_matrix(instance.nodes)
    mst = prim_mst(dist)
    recomputed_dfs_order = dfs_preorder(mst.adjacency, start=0)
    recomputed_tour_length = compute_tour_length(recomputed_dfs_order, dist)

    dfs_order = recomputed_dfs_order
    source_label = "recomputed from TSP"
    summary_metrics: Dict[Tuple[str, str], str] = {}

    if args.csv is not None:
        if not args.csv.exists():
            raise FileNotFoundError(f"CSV file not found: {args.csv}")
        summary_metrics = parse_summary_metrics(args.csv)
        csv_order = read_mst_dfs_tour_from_csv(args.csv)
        if csv_order is not None:
            dfs_order = csv_order
            source_label = f"loaded from CSV: {args.csv.name}"

    tour_length = compute_tour_length(dfs_order, dist)

    args.output_dir.mkdir(parents=True, exist_ok=True)
    prefix = args.prefix or instance.name
    mst_png = args.output_dir / f"{prefix}_mst.png"
    cycle_png = args.output_dir / f"{prefix}_mst_dfs_cycle.png"

    plot_mst(instance, mst, mst_png, annotate=args.annotate)
    plot_mst_with_dfs_cycle(
        instance,
        mst,
        dfs_order,
        tour_length,
        cycle_png,
        annotate=args.annotate,
    )

    print(f"Instance: {instance.name}")
    print(f"Nodes: {len(instance.nodes)}")
    print(f"MST weight (recomputed): {mst.weight}")
    print(f"DFS tour source: {source_label}")
    print(f"MST + DFS tour length: {tour_length}")

    if args.csv is not None:
        mst_csv_weight = summary_metrics.get(("mst", "mst_weight"))
        mst_csv_tour = summary_metrics.get(("mst_dfs", "tour_length"))
        if mst_csv_weight is not None:
            print(f"MST weight (CSV): {mst_csv_weight}")
        if mst_csv_tour is not None:
            print(f"MST + DFS tour length (CSV): {mst_csv_tour}")
        if dfs_order != recomputed_dfs_order:
            print("Note: CSV tour order differs from the recomputed DFS preorder.")

    print(f"Saved: {mst_png}")
    print(f"Saved: {cycle_png}")

    if args.show:
        img1 = plt.imread(mst_png)
        plt.figure(figsize=(10, 8))
        plt.imshow(img1)
        plt.axis("off")
        plt.title(mst_png.name)

        img2 = plt.imread(cycle_png)
        plt.figure(figsize=(10, 8))
        plt.imshow(img2)
        plt.axis("off")
        plt.title(cycle_png.name)
        plt.show()


if __name__ == "__main__":
    main()
