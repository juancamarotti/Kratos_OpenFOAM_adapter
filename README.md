# The OpenFOAM adapter

## Overview

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

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \ 
    -DCO_SIM_IO_BUILD_MPI=ON \ 
    -DCO_SIM_IO_BUILD_TESTING=OFF

cmake --build build --parallel "$(nproc)"
```

**3. Install OpenFOAM solver version v2512**
```bash
# Add the repository
curl -s https://dl.openfoam.com/add-debian-repo.sh | sudo bash

# Update the repository information
sudo apt-get update

# Install preferred package. Eg,
sudo apt-get install openfoam2512-default
```

**4. Get preCICE, build and install**

```bash
git clone https://github.com/precice/precice.git

cd precice

cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$HOME/.local" \
    -DPRECICE_FEATURE_PETSC_MAPPING=OFF

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
export PKG_CONFIG_PATH="$HOME/.local/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export LD_LIBRARY_PATH="$COSIMIO_ROOT/build:$HOME/.local/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
```
Please change ```<path/to/CoSimIO/SourceFiles>``` accordingly. Besides, ```$PKG_CONFIG_PATH``` must include the directory containing ```libprecice.pc```, while ```LD_LIBRARY_PATH``` must include the directories containing the shared libraries ```libco_sim_io.so``` and ```libco_sim_io_mpi.so``` so that the runtime linker can locate them.

**7. At adapter's directory, run ```Allwmake``` build file**

```bash
chmod +x ./Allwmake

From the adapter root directory, execute:

./Allwmake
```

