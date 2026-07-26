# The OpenFOAM adapter

## Overview

### Description
This adapter enables communication between the OpenFOAM solver and the CoSimIO library from Kratos Multiphysics. It is based on the OpenFOAM-preCICE adapter and adapts its functionality to use CoSimIO for data exchange. This adapter is a fork of the [OpenFOAM-preCICE adapter](https://precice.org/adapter-openfoam-overview.html). It supports fluid-structure interaction (fluid part), conjugate heat transfer (fluid and solid parts), and fluid-fluid simulations.

### Capabilities
This adapter can read/write the following fields in a surface coupling setup:

- Temperature (read + write)
- Temperature surface-normal gradient (read + write)
- Heat flux (read + write)
- Sink temperature (read + write)
- Heat transfer coefficient (read + write)
- Force (read + write)
- Stress (write)
- Displacement (read + write)
- Displacement delta (read)
- Pressure (read + write)
- Pressure surface-normal gradient (read + write)
- Velocity (read + write)
- Velocity surface-normal gradient (read + write)
- Phase fraction (alpha) (read + write)
- Phase fraction (alpha) gradient (read + write)
- Phase flux (phi) (read + write)

In addition, the adapter supports the following fields in a volume coupling setup:

- Temperature (write)
- Pressure (write)
- Velocity (read + write)


## How to get the adapter?
The adapter depends on OpenFOAM v2512, CoSimIO, and preCICE. The following provides a complete guideline to get the adapter and its dependencies.

**1. Clone the adapter's repository**

```bash
git clone https://github.com/juancamarotti/OpenFOAM_CoSimIO-Adapter.git
```

**2. Clone CoSimIO and build it with MPI support**

```bash
git clone https://github.com/KratosMultiphysics/CoSimIO.git

cd CoSimIO

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCO_SIM_IO_BUILD_MPI=ON -DCO_SIM_IO_BUILD_TESTING=OFF

cmake --build build --parallel "$(nproc)"
```

**3. Install OpenFOAM solver version v2512**
```bash
# Add the repository
curl -s https://dl.openfoam.com/add-debian-repo.sh | sudo bash

# Update the repository information
sudo apt-get update

# Install OpenFOAM v2512
sudo apt-get install openfoam2512-default
```

**⚠️ Warning:** The adapter currently supports **only OpenFOAM v2512**. Please install this version before proceeding.

**4. Get preCICE, build and install**

```bash
git clone https://github.com/precice/precice.git

cd precice

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local" -DPRECICE_FEATURE_PETSC_MAPPING=OFF

cmake --build build --parallel "$(nproc)"
cmake --install build
```

**5. Load OpenFOAM environment**

```bash
source /usr/lib/openfoam/openfoam2512/etc/bashrc
```

**6. Set environment variables**

```bash
export COSIMIO_ROOT=/path/to/CoSimIO
export PKG_CONFIG_PATH=path/to/precice_build
export LD_LIBRARY_PATH=path/to/CoSimIO_build
```
Please change ```<path/to/CoSimIO/SourceFiles>``` accordingly. Besides, ```$PKG_CONFIG_PATH``` must include the directory containing ```libprecice.pc```, while ```LD_LIBRARY_PATH``` must include the directories containing the shared libraries ```libco_sim_io.so``` and ```libco_sim_io_mpi.so``` so that the runtime linker can locate them.

**7. At adapter's directory, run ```Allwmake``` build file**

From the adapter root directory, execute:
```bash
chmod +x ./Allwmake

./Allwmake
```

## Configuration
This section summarizes how to write the CoSimIODict file, set the boundary conditions, and activate the adapter in the controlDict.



