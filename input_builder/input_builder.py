import argparse
import enum
import re

from pathlib import Path


class Method(enum.Enum):
    IMC = "IMC"

    def from_string(string_form):
        for method in Method:
            if method.value == string_form:
                return method
        raise ValueError(f"Unknown method: {string_form}")


class DomainDecompositionType(enum.Enum):
    PARTICLE_PASS = "PARTICLE_PASS"
    REPLICATED = "REPLICATED"
    METIS = "METIS"

    def from_string(string_form):
        for dd in DomainDecompositionType:
            if dd.value == string_form:
                return dd
        raise ValueError(f"Unknown domain decomposition: {string_form}")


class MeshDecompositionType(enum.Enum):
    METIS = "METIS"
    CUBE = "CUBE"
    NAIVE = "NAIVE"

    def from_string(string_form):
        for md in MeshDecompositionType:
            if md.value == string_form:
                return md
        raise ValueError(f"Unknown mesh decomposition: {string_form}")


class CommonConfig:
    """
    Encapsulate <common> + <debug_options> parameters.
    We store the template XML inside this class, separate from the constructor,
    for clarity and readability.
    """

    def __init__(
        self,
        method: Method,
        t_start: float,
        t_stop: float,
        dt_start: float,
        t_mult: float,
        dt_max: float,
        photons: int,
        seed: int,
        tilt: bool,
        stratified_sampling: bool,
        use_gpu_transporter: bool,
        dd_transport_type: DomainDecompositionType,
        n_omp_threads: int,
        mesh_decomposition: MeshDecompositionType,
        batch_size: int,
        particle_message_size: int,
        write_silo: bool,
    ):
        if isinstance(method, Method):
            self.method = method
        elif isinstance(method, str):
            self.method = Method.from_string(method)
        else:
            raise ValueError("method must be a Method enum or string value.")

        self.t_start = t_start
        self.t_stop = t_stop
        self.dt_start = dt_start
        self.t_mult = t_mult
        self.dt_max = dt_max
        self.photons = photons
        self.seed = seed
        self.tilt = tilt
        self.stratified_sampling = stratified_sampling
        self.use_gpu_transporter = use_gpu_transporter

        if isinstance(dd_transport_type, DomainDecompositionType):
            self.dd_transport_type = dd_transport_type
        elif isinstance(dd_transport_type, str):
            self.dd_transport_type = DomainDecompositionType.from_string(
                dd_transport_type
            )
        else:
            raise ValueError(
                "dd_transport_type must be a DomainDecompositionType enum or string value."
            )

        self.n_omp_threads = n_omp_threads

        if isinstance(mesh_decomposition, MeshDecompositionType):
            self.mesh_decomposition = mesh_decomposition
        elif isinstance(mesh_decomposition, str):
            self.mesh_decomposition = MeshDecompositionType.from_string(
                mesh_decomposition
            )
        else:
            raise ValueError(
                "mesh_decomposition must be a MeshDecompositionType enum or string value."
            )

        self.batch_size = batch_size
        self.particle_message_size = particle_message_size
        self.output_frequency = 1
        self.write_silo = write_silo

        self.print_verbose = False
        self.print_mesh_info = False

    def validate_mesh_decomposition(self, nx, ny, nz):
        """
        Optional feasibility check for mesh vs. domain decomposition.
        Adjust or remove as needed.
        """
        if self.dd_transport_type == DomainDecompositionType.REPLICATED:
            total_cells = nx * ny * nz
            if total_cells > 1_000_000:
                raise RuntimeError(
                    "REPLICATED domain decomposition with >1e6 cells might be infeasible."
                )

    def as_dict(self):
        """
        Converts all config fields into the string placeholders expected by the template.
        """
        return {
            "common": {
                "method": self.method.value,
                "t_start": str(self.t_start),
                "t_stop": str(self.t_stop),
                "dt_start": str(self.dt_start),
                "t_mult": str(self.t_mult),
                "dt_max": str(self.dt_max),
                "photons": str(self.photons),
                "seed": str(self.seed),
                "tilt": str(self.tilt).upper(),
                "stratified_sampling": str(self.stratified_sampling).upper(),
                "use_gpu_transporter": str(self.use_gpu_transporter).upper(),
                "dd_transport_type": self.dd_transport_type.value,
                "n_omp_threads": str(self.n_omp_threads),
                "mesh_decomposition": self.mesh_decomposition.value,
                "batch_size": str(self.batch_size),
                "particle_message_size": str(self.particle_message_size),
                "output_frequency": str(self.output_frequency),
                "write_silo": str(self.write_silo).upper(),
            },
            "debug_options": {
                "print_verbose": str(self.print_verbose).upper(),
                "print_mesh_info": str(self.print_mesh_info).upper(),
            },
        }

    def build_xml_string(self, physical):
        comment_line_pattern = re.compile(r"<!--.*-->")
        single_line_pattern = re.compile(r"<.*>.*</.*>")
        scope_end_pattern = re.compile(r"</.*>")
        scope_begin_pattern = re.compile(r"<.*>")

        indent = 0
        physical_lines = ""
        for line in physical.readlines():
            if line.strip() == "":
                continue
            if re.match(comment_line_pattern, line.strip("\n")):
                physical_lines += f"{'\t'*indent}{line.strip("\n")}\n"
            elif re.match(single_line_pattern, line.strip("\n")):
                physical_lines += f"{'\t'*indent}{line.strip("\n")}\n"
            elif re.match(scope_end_pattern, line.strip("\n")):
                indent -= 1
                physical_lines += f"{'\t'*indent}{line.strip("\n")}\n"
            elif re.match(scope_begin_pattern, line):
                physical_lines += f"{'\t'*indent}{line.strip("\n")}\n"
                indent += 1
            else:
                raise RuntimeError(f"Unrecognized line format: {line}.")

        return f"<prototype>\n{self._build_xml_string(self.as_dict(), 0)}{physical_lines}</prototype>"

    def _build_xml_string(self, item, indent):
        if isinstance(item, dict):
            to_ret = ""
            for member in item.items():
                to_ret += self._build_xml_string(member, indent)
            return to_ret
        elif isinstance(item, tuple):
            key, value = item
            if isinstance(value, dict):
                to_ret = f"{'\t'*indent}<{key}>\n"
                to_ret += self._build_xml_string(value, indent + 1)
                to_ret += f"{'\t'*indent}</{key}>\n"
                return to_ret
            elif isinstance(value, str):
                return f"{'\t'*indent}<{key}>{value}</{key}>\n"
            else:
                raise ValueError(
                    f"Invalid value type in entry {key}: {value}."
                )
        else:
            raise ValueError(f"Invalid type in item: {item}.")


def parse_input():
    parser = argparse.ArgumentParser(description="Build Branson input files.")
    parser.add_argument(
        "path_to_physical", type=str, help="Path to physical snippet."
    )
    parser.add_argument(
        "path_to_output", type=str, help="Path to output file."
    )
    parser.add_argument(
        "--method",
        type=str,
        default="IMC",
        help="The transport method to use.",
        choices=[m.value for m in Method],
    )
    parser.add_argument(
        "--t_start", type=float, default=0.0, help="Start time."
    )
    parser.add_argument(
        "--t_stop", type=float, default=0.020, help="Stop time."
    )
    parser.add_argument(
        "--dt_start", type=float, default=0.001, help="Initial time step."
    )
    parser.add_argument(
        "--t_mult", type=float, default=1.0, help="Time step multiplier."
    )
    parser.add_argument(
        "--dt_max", type=float, default=1.0, help="Maximum time step."
    )
    parser.add_argument(
        "--photons", type=int, default=250_000_000, help="Number of photons."
    )
    parser.add_argument("--seed", type=int, default=14706, help="Random seed.")
    parser.add_argument(
        "--tilt",
        action="store_true",
        default=False,
        help="Whether to tilt the source.",
    )
    parser.add_argument(
        "--stratified_sampling",
        action="store_true",
        default=False,
        help="Whether to use stratified sampling.",
    )
    parser.add_argument(
        "--use_gpu_transporter",
        action="store_true",
        default=False,
        help="Whether to use the GPU transporter.",
    )
    parser.add_argument(
        "--dd_transport_type",
        type=str,
        default="PARTICLE_PASS",
        help="Domain decomposition transport type.",
        choices=[dd.value for dd in DomainDecompositionType],
    )
    parser.add_argument(
        "--n_omp_threads",
        type=int,
        default=1,
        help="Number of OpenMP threads.",
    )
    parser.add_argument(
        "--mesh_decomposition",
        type=str,
        default="METIS",
        help="Mesh decomposition strategy.",
        choices=[md.value for md in MeshDecompositionType],
    )
    parser.add_argument(
        "--batch_size",
        type=int,
        default=5000,
        help="Batch size for particle transport.",
    )
    parser.add_argument(
        "--particle_message_size",
        type=int,
        default=10000,
        help="Particle message size.",
    )
    parser.add_argument(
        "--write_silo",
        action="store_true",
        default=False,
        help="Whether to write SILO output.",
    )
    return vars(parser.parse_args())


if __name__ == "__main__":
    args = parse_input()
    physical_path = Path(args.pop("path_to_physical"))
    output_path = Path(args.pop("path_to_output"))
    config = CommonConfig(**args)

    with open(physical_path, "r") as physical:
        with open(output_path, "w") as output:
            output.write(config.build_xml_string(physical))

    print(f"Wrote to output file: {output_path}")
