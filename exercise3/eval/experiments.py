import subprocess
from pathlib import Path
from math import log10
from itertools import product
import sys

PROJECT_ROOT_DIR = Path("..")
DATA_DIR = PROJECT_ROOT_DIR / "data"
BUILD_DIR = PROJECT_ROOT_DIR / "build"
OUTPUT_DIR = Path(".")


def scientific(n):
    k = int(log10(n))
    if 10 ** k == n:
        return f"1e{k}"
    else:
        return str(n)


def generate_graph(num_components, num_nodes_per_component, num_avg_degree) -> Path:
    graph_construction_executable = BUILD_DIR / "graph_construction"
    assert graph_construction_executable.exists()

    graph = DATA_DIR / f"{scientific(num_components)}x-{scientific(num_nodes_per_component)}-{num_avg_degree}.graph"
    if not graph.exists():
        print(f"\tgraph {graph.name} does not exist")

        components = " ".join(f"{num_nodes_per_component} {num_avg_degree}" for _ in range(num_components))
        cmd = f"{graph_construction_executable} {num_components} {components}"

        print(f"\t{cmd[:64]}...")
        try:
            with graph.open("w") as f:
                subprocess.run(cmd.split(" "), check=True, stdout=f)
        except:
            graph.unlink(missing_ok=True)

    assert graph.exists()
    return graph


def run_experiment(task: str, graph: Path, num_threads: int, thread_range: bool, num_iterations: int, header: bool,
                   output_file):
    assert task in "abcdef"
    suffixes = dict(a="_sequential", b="_b", c="_c", d="_d", e="_e", f="")
    benchmark_executable = BUILD_DIR / ("benchmark_dynamic_connectivity" + suffixes[task])

    assert benchmark_executable.exists()

    cmd = f"{benchmark_executable} -graph {graph} -num-threads {num_threads} -num-iterations {num_iterations}"
    if thread_range:
        cmd += " -thread-range"
    if not header:
        cmd += " -no-header"
    print(cmd)
    timeout = num_iterations * (num_threads if thread_range else 1) * 60 * 5
    subprocess.run(cmd.split(" "), check=True, stdout=output_file, timeout=timeout)


def main():
    num_threads = 4
    num_iterations = 10

    if len(sys.argv) > 1:
        num_threads = int(sys.argv[1])

    exp1_path = OUTPUT_DIR / f"exp1-{num_threads}-{num_iterations}.csv"
    if not exp1_path.exists():
        header = True
        with exp1_path.open("w") as f:
            for t, num_components, num_nodes_per_component, num_avg_degree \
                    in product("abcdef", (10,), (10 ** 4, 5 * 10 ** 4, 10 ** 5, 5 * 10 ** 5, 10 ** 6), (2,)):
                graph = generate_graph(num_components, num_nodes_per_component, num_avg_degree)
                run_experiment(t, graph, 1, False, num_iterations, header, f)
                header = False
                run_experiment(t, graph, num_threads, False, num_iterations, header, f)

    exp2_path = OUTPUT_DIR / f"exp2-{num_threads}-{num_iterations}.csv"
    if not exp2_path.exists():
        header = True
        with (OUTPUT_DIR / f"exp2-{num_threads}-{num_iterations}.csv").open("w") as f:
            for t, num_components, num_nodes_per_component, num_avg_degree \
                    in product("abcdef", (100,), (10 ** 5,), (2,)):
                graph = generate_graph(num_components, num_nodes_per_component, num_avg_degree)
                run_experiment(t, graph, num_threads, True, num_iterations, header, f)
                header = False

    exp3_path = OUTPUT_DIR / f"exp3-{num_threads}-{num_iterations}.csv"
    if not exp3_path.exists():
        header = True
        with (OUTPUT_DIR / f"exp3-{num_threads}-{num_iterations}.csv").open("w") as f:
            for t, num_components, num_nodes_per_component, num_avg_degree \
                    in product("abcdef", (100,), (10 ** 5,), (1.1,)):
                graph = generate_graph(num_components, num_nodes_per_component, num_avg_degree)
                run_experiment(t, graph, num_threads, True, 1, header, f)
                header = False

    exp4_path = OUTPUT_DIR / f"exp4-{num_threads}-{num_iterations}.csv"
    if not exp4_path.exists():
        header = True
        with (OUTPUT_DIR / f"exp4-{num_threads}-{num_iterations}.csv").open("w") as f:
            for t, num_components, num_nodes_per_component, num_avg_degree \
                    in product("abcdef", (1000,), (10 ** 5,), (2,)):
                graph = generate_graph(num_components, num_nodes_per_component, num_avg_degree)
                run_experiment(t, graph, num_threads, True, 1, header, f)
                header = False


if __name__ == '__main__':
    main()
