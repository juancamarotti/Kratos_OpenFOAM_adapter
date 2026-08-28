import KratosMultiphysics as KM
from KratosMultiphysics.CoSimulationApplication.co_simulation_analysis import CoSimulationAnalysis

import matplotlib.pyplot as plt


class CustomCoSimulationAnalysis(CoSimulationAnalysis):
    def __init__(self, parameters):
        super().__init__(parameters)

        self.time_history = []
        self.disp_x_history_A = []
        self.disp_x_history_B = []

    def OutputSolutionStep(self):
        super().OutputSolutionStep()

        solver = self._GetSolver()

        if hasattr(solver, "model") and hasattr(solver.model, "solver_wrappers"):
            structure_solver = solver.model.solver_wrappers.get("structure")

            if structure_solver and hasattr(structure_solver, "model"):
                sub_model = structure_solver.model
                model_part_name = "Structure"

                if sub_model.HasModelPart(model_part_name):
                    model_part = sub_model.GetModelPart(model_part_name)
                else:
                    raise RuntimeError(f'ModelPart "{model_part_name}" not found.')
            else:
                raise RuntimeError("Structure solver not found.")
        else:
            raise RuntimeError("Solver model or solver wrappers not found.")

        wet_interface_sub_model_part = model_part.GetSubModelPart("StructureInterface2D_StructureInterface")
        current_time = wet_interface_sub_model_part.ProcessInfo[KM.TIME]

        # Only do this the first time
        if not hasattr(self, "tracking_nodes_initialized"):
            self.tracking_nodes_initialized = True

            self.point_A = (0.6, 0.2) # tip mid point
            self.point_B = (0.15, 0.2)

            def get_closest_node(target_point):
                min_dist = float("inf")
                closest_node = None

                for node in wet_interface_sub_model_part.Nodes:
                    dist = (
                        (node.X - target_point[0]) ** 2
                        + (node.Y - target_point[1]) ** 2
                    )

                    if dist < min_dist:
                        min_dist = dist
                        closest_node = node

                if closest_node is None:
                    raise RuntimeError("No nodes found in wet interface sub model part.")

                return closest_node

            self.node_A = get_closest_node(self.point_A)
            self.node_B = get_closest_node(self.point_B)

            print(
                f"Tracking node A: Id={self.node_A.Id}, "
                f"coordinates=({self.node_A.X}, {self.node_A.Y}, {self.node_A.Z})"
            )
            print(
                f"Tracking node B: Id={self.node_B.Id}, "
                f"coordinates=({self.node_B.X}, {self.node_B.Y}, {self.node_B.Z})"
            )

        # --- Evaluate displacement at A ---
        solution_A = self.node_A.GetSolutionStepValue(KM.DISPLACEMENT_Y)

        self.time_history.append(current_time)
        self.disp_x_history_A.append(solution_A)

        # --- Evaluate displacement at B ---
        solution_B = self.node_B.GetSolutionStepValue(KM.DISPLACEMENT_Y)

        self.disp_x_history_B.append(solution_B)

    def Finalize(self):
        super().Finalize()

        plt.rcParams["text.usetex"] = True
        plt.rcParams["font.family"] = "serif"

        plt.figure()
        plt.plot(self.time_history, self.disp_x_history_A, label=r"$u_x$ at Point A")
        plt.plot(self.time_history, self.disp_x_history_B, label=r"$u_x$ at Point B")

        plt.xlabel(r"\textbf{Time} [s]")
        plt.ylabel(r"\textbf{Displacement} $u_y$ [m]")
        plt.title(
            r"\textbf{Displacement vs Time (Closest Node)}",
            fontsize=14
        )

        plt.grid(True)
        plt.legend(loc="best")

        plt.savefig("displacement_vs_time.pdf", dpi=300, bbox_inches="tight")
        plt.show()


if __name__ == "__main__":

    with open("ProjectParametersCoSim.json", "r") as parameter_file:
        parameters = KM.Parameters(parameter_file.read())

    simulation = CustomCoSimulationAnalysis(parameters)
    simulation.Run()