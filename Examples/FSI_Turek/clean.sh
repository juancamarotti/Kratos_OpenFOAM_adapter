#!/bin/sh
set -e -u

echo "--- Cleaning CoSimulation result files in $(pwd)"
rm -rfv .CoSimIOComm_Openfoam_Adapter_Openfoam_Kratos_Wrapper/
rm -rfv vtk_output/
rm -rfv vtk_output_coupling/
rm -rfv Wall_Structure_partitioned/
rm -rfv vtk_output_structure/

echo "--- Cleaning OpenFOAM result files in $(pwd)"
if [ -n "${WM_PROJECT:-}" ]; then

    # shellcheck disable=SC1091
    . "${WM_PROJECT_DIR}/bin/tools/CleanFunctions"

    cleanCase

    rm -rfv 0/uniform/functionObjects/functionObjectProperties

else
    echo "No OpenFOAM environment is active." >&2
    exit 1
fi

echo "--- Cleaning logfiles"
rm -rfv logopenfoam
rm -rfv logkratos
