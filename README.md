# The OpenFOAM adapter

**Requirements**: OpenFOAM v2512, CoSimIO built with MPI support, CMake, and a compatible C++ compiler.


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
The adapter depends on OpenFOAM v2512 and CoSimIO. The following provides a complete guideline to get the adapter and its dependencies.

**1. Clone the adapter's repository**

```bash
git clone https://github.com/juancamarotti/OpenFOAM_CoSimIO-Adapter.git
```

**2. Clone, build, and install CoSimIO with MPI support**

```bash
git clone https://github.com/KratosMultiphysics/CoSimIO.git

cd CoSimIO

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCO_SIM_IO_BUILD_MPI=ON -DCO_SIM_IO_BUILD_TESTING=OFF

cmake --build build --parallel "$(nproc)"

cmake --install build
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


**4. Load OpenFOAM environment**

```bash
source /usr/lib/openfoam/openfoam2512/etc/bashrc
```

**5. Set environment variables**

```bash
export COSIMIO_ROOT=/path/to/CoSimIO
export LD_LIBRARY_PATH=path/to/CoSimIO_build
```
Please change ```<path/to/CoSimIO>``` accordingly. Besides, ```LD_LIBRARY_PATH``` must include the directories containing the shared libraries ```libco_sim_io.so``` and ```libco_sim_io_mpi.so``` so that the runtime linker can locate them.

**6. At adapter's directory, run ```Allwmake``` build file**

From the adapter root directory, execute:
```bash
chmod +x ./Allwmake

./Allwmake
```

## Configuration

This section summarizes how to write the ```CoSimIODict``` file, set the boundary conditions, and activate the adapter in the ```controlDict```.

### CoSimIODict
The adapter is configured via the file ```system/CoSimIODict```. This file is an OpenFOAM dictionary with the following structure:

```
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      CoSimIODict;
}

participant Fluid;

modules (<moduleName>);

interfaces
{
    <InterfaceName>
    {
        mesh              <meshName>;
        patches           (<boundaryPatchName>);
        locations         <interfaceMeshLocation>;
        
        ReadData
        (
            <readFieldData>
        );
        
        WriteData
        (
            <writeFieldData>
        );
    };
};

<moduleName>
{
    <quantityKeyword> <quantityName> <dimensionList> <value>;
}
```
The ```moduleName``` selects the coupling module used by the adapter. Available modules include FSI for fluid-structure interaction, CHT for conjugate heat transfer, and FF for fluid-fluid coupling.
<!--participants-->
The ```interfaces``` block determines the interfaces available in the co-simulation. For each interface, following properties have to be determined:
- ```InterfaceName``` can be set arbitrarily
- ```meshName``` <!-- participant and meshName are originally required to correspond with preciceConfig.xml. How about here? ProjectParametersCoSim.json?-->
- ```patches``` specifies the list of names of boundary patches that participate in the co-simulation. Each specified patch must exist in the OpenFOAM mesh boundary, and the corresponding field files in the ```0/``` directory must provide appropriate boundary conditions for that patch.
- The ```locations``` field determines the position where the interface mesh is defined on the cell. Default value of ```locations``` is ```FaceCenters```, while other values can be set for ```interfaceMeshLocation```, including ```FaceNodes``` and ```VolumeCenters```.
- ```ReadData``` and ```WriteData``` sections define which field will be exchanged between the two solvers. Available fields include ```Temperature```,  ```Heat-Flux```, ```Sink-Temperature```, and ```Heat-Transfer-Coefficient```. Postfixed names following a hyphen can be added to distinguish multiple data sets of the same type (e.g. ```Temperature-Domain1```). For FSI module, ```WriteData``` values also include ```Force``` and ```Stress``` for fluid participants and ```Displacement``` for solid participants, while ```ReadData``` takes also ```Displacement``` and ```DisplacementDelta``` for fluid participants and ```Force``` and ```Stress``` for solid participants.
- Specific settings when using certain values for ```ReadData``` and/or ```WriteData``` for each boundary module has to be satisfied:

    - ```CHT``` module: for ```readData(Temperature)```, use ```type fixedValue``` for the interface in ```0/T```. For ```readData(Heat-Flux)```, use ```type fixedGradient``` for the interface in ```0/T```. For ```readData(Sink-Temperature)``` or ```Heat-Transfer-Coefficient```, use ```type mixed``` for the interface in ```0/T```
    - ```FSI``` module: for ```ReadData``` values ```Displacement``` or ```DisplacementDelta```, you need the following:
        
        - ```type movingWallVelocity``` for the interface (e.g., ```flap```) in ```0/U```
        - type ```fixedValue``` for the interface (e.g., ```flap```) in the ```0/pointDisplacement```, and
        - ```solver displacementLaplacian``` in the ```constant/dynamicMeshDict```

    - ```FF``` module supports reading and writing ```Pressure```, ```Velocity```, ```PressureGradient```, ```VelocityGradient```, ```FlowTemperature```, ```FlowTemperatureGradient```, ```Alpha```, ```AlphaGradient``` and the face flux ```Phi```. Similarly to the ```CHT``` module, you need a ```fixedValue``` boundary condition of the respective primary field in order to read and apply values, and a ```fixedGradient``` boundary condition of the respective gradient field in order to read and apply gradients.

- The last section defines additional properties for specific solvers. More information can be obtained via [preCICE-OpenFoam-Adapter documentation](https://precice.org/adapter-openfoam-config.html#additional-properties-for-some-solvers).

An overview of entries required for ```CoSimIODict``` configuration can be found in the following table:

| Entry         | Description                                                                                                         |
| ------------- | ------------------------------------------------------------------------------------------------------------------- |
| `participant` | Mandatory participant-name entry. Currently stored by the adapter but not used to establish the CoSimIO connection. |
| `modules`     | Specifies the enabled coupling modules (`FSI`, `CHT`, `FF`, etc.).                                                  |
| `interfaces`  | Defines one or more coupling interfaces.                                                                            |
| `mesh`        | Name/identifier of the coupling mesh exported through CoSimIO.                                                      |
| `patches`     | OpenFOAM boundary patches belonging to the interface.                                                               |
| `locations`   | Specifies whether coupling locations are face centers, face nodes, or volume centers.                               |
| `ReadData`    | Quantities imported from the coupled solver.                                                                        |
| `WriteData`   | Quantities exported to the coupled solver.                                                                          |


### controlDict

A ```controlDict``` parameter dictionary has the following general format:

```
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    location    "system";
    object      controlDict;
}

// Parameters definition

functions
{
    CoSimIO_Adapter
    {
        type CoSimIOAdapterFunctionObject;
        errors strict; // Available since OpenFOAM v2012
    }

    //Additional OpenFOAM function objects declaration
}
```

The parameters to be defined in the parameter definition are listed in the following table:

| Parameter                       | Meaning                                                                                                                       |
| ------------------------------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `application <solver>;`         | Specifies the OpenFOAM solver/application to run.                                                                             |
| `startFrom <start_option>;`     | Specifies how the simulation start time is selected, e.g. from `startTime` or the latest available time directory.            |
| `startTime <time>;`             | Defines the initial simulation time when `startFrom startTime;` is used.                                                      |
| `stopAt <stop_option>;`         | Specifies the condition used to stop the simulation, e.g. when `endTime` is reached.                                          |
| `endTime <time>;`               | Defines the final simulation time.                                       |
| `deltaT <time_step>;`           | Defines the simulation time-step size.                                                                                        |
| `writeControl <write_control>;` | Specifies how output writing is triggered, e.g. by time steps or simulation time.                                             |
| `writeInterval <interval>;`     | Defines the interval between output writes according to the selected `writeControl`.                                          |
| `purgeWrite <number>;`          | Specifies how many previous output time directories are retained. A value of `0` keeps all written directories.               |
| `writeFormat <format>;`         | Specifies the output file format, typically `ascii` or `binary`.                                                              |
| `writePrecision <digits>;`      | Defines the numerical precision used when writing field data.                                                                 |
| `writeCompression <option>;`    | Enables or disables compression of output files.                                                                              |
| `timeFormat <format>;`          | Specifies the numeric format used for simulation-time values and time-directory names.                                        |
| `timePrecision <digits>;`       | Defines the numerical precision used for simulation-time values and directory names.                                          |
| `libs ("<library_name>");`      | Dynamically loads one or more shared libraries required by additional runtime functionality, such as custom function objects. |


```functions``` section defines the function objects that will be executed during runtime. ```CoSimIO_Adapter``` function object call is a must, since it establishes the connection between ```CoSimIO``` and ```OpenFOAM```. Other OpenFOAM function objects can also be called per need. Generic OpenFOAM function-object controls can be found at [OpenFOAM function object controls documentation](https://doc.openfoam.com/2312/tools/post-processing/function-objects/). 

### Examples

#### FSI Mok

```CoSimIODict``` configuration

```
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      CoSimIODict;
}

participant Fluid;

modules (FSI);

interfaces
{
  interface_flap
  {
    mesh              interface_flap;
    patches           (Mok);
    locations         FaceCenters;
    
    ReadData
    (
        Displacement-Flap
    );
    
    WriteData
    (
        Force-Flap
    );
  };
};

FSI
{
  rho rho [1 -3 0 0 0 0 0] 956.0;
}
```

```controlDict``` configuration

```
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    location    "system";
    object      controlDict;
}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

application         pimpleFoam;

startFrom           startTime;
//startFrom           latestTime;

startTime           0;

stopAt              endTime;

endTime             25.0;

deltaT              0.05;

writeControl        timeStep;

writeInterval       10;

purgeWrite          0;

writeFormat         ascii;

writePrecision      12;

writeCompression    off;

timeFormat          general;

timePrecision       8;

libs ("libCoSimIOAdapterFunctionObject.so");
functions
{
    CoSimIO_Adapter
    {
        type CoSimIOAdapterFunctionObject;
        errors strict; // Available since OpenFOAM v2012
    }

    // Function object "CourantNo" to see courant number
    Co1
    {
        type                CourantNo;
        libs                ("libfieldFunctionObjects.so");
        executeControl      timeStep;
        executeInterval     50;
        writeControl        writeTime;
    }

    // Function object "forces" to calcuate the total force on interface
    forces
    {
        type                forces;
        libs                ( "libforces.so" );
        patches             (Mok);
        rho                 rhoInf;
        log                 true;
        rhoInf              956.0;
        CofR                (0 0 0);
        writeControl    timeStep;
        writeInterval   1;
    }

    residuals
    {
        type            residuals;
        libs            ("libutilityFunctionObjects.so");
        writeControl    timeStep;
        writeInterval   100;
        fields          (p U);
    }


    probes
    {
        type            probes;
        libs            ("libsampling.so");

        writeControl    writeTime;     // or timeStep if you want every step
        writeInterval   100;

        fields          (U p);         // add whatever you want to sample

        probeLocations
        (
            (0.4965 0.25 0.0)
        );
    }

    writePointDisplacement
    {
        type            writeObjects;
        libs            ("libutilityFunctionObjects.so");

        writeControl    timeStep;
        writeInterval   100;

        objects         (pointDisplacement);
    }


}
```


#### FSI Turek

```CoSimIODict``` configuration

```
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    object      CoSimIODict;
}

participant Fluid;

modules (FSI);

interfaces
{
  interface_flap
  {
    mesh              interface_flap;
    patches           (flap);
    locations         FaceCenters;
    
    ReadData
    (
        Displacement-Flap
    );
    
    WriteData
    (
        Force-Flap
    );
  };
};

FSI
{
  rho rho [1 -3 0 0 0 0 0] 1000.0;
}
```

```ControlDict``` configuration
```
FoamFile
{
    version     2.0;
    format      ascii;
    class       dictionary;
    location    "system";
    object      controlDict;
}
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

application         pimpleFoam;

startFrom           startTime;
//startFrom           latestTime;

startTime           0;

stopAt              endTime;

endTime             25.0;

deltaT              0.005;

writeControl        timeStep;

writeInterval       100;

purgeWrite          0;

writeFormat         ascii;

writePrecision      12;

writeCompression    off;

timeFormat          general;

timePrecision       8;

libs ("libCoSimIOAdapterFunctionObject.so");
functions
{
    CoSimIO_Adapter
    {
        type CoSimIOAdapterFunctionObject;
        errors strict; // Available since OpenFOAM v2012
    }

    // Function object "CourantNo" to see courant number
    Co1
    {
        type                CourantNo;
        libs                ("libfieldFunctionObjects.so");
        executeControl      timeStep;
        executeInterval     100;
        writeControl        writeTime;
    }

    // Function object "forces" to calcuate the total force on interface
    forces
    {
        type                forces;
        libs                ( "libforces.so" );
        patches             (flap);
        rho                 rhoInf;
        log                 true;
        rhoInf              1000;
        CofR                (0 0 0);
        writeControl    timeStep;
        writeInterval   1;
    }

    residuals
    {
        type            residuals;
        libs            ("libutilityFunctionObjects.so");
        writeControl    timeStep;
        writeInterval   100;
        fields          (p U);
    }


    probes
    {
        type            probes;
        libs            ("libsampling.so");

        writeControl    writeTime;     // or timeStep if you want every step
        writeInterval   100;

        fields          (U p);         // add whatever you want to sample

        probeLocations
        (
            (0.4965 0.25 0.0)
        );
    }

    writePointDisplacement
    {
        type            writeObjects;
        libs            ("libutilityFunctionObjects.so");

        writeControl    timeStep;
        writeInterval   100;

        objects         (pointDisplacement);
    }
}
```



## References

OpenCFD Ltd. (2024, January 2). *Function objects*. OpenFOAM Documentation. https://doc.openfoam.com/2312/tools/post-processing/function-objects/

preCICE. (2026, June 30). *Configure the OpenFOAM adapter*. https://precice.org/adapter-openfoam-config.html#additional-properties-for-some-solvers

Sastre i Rienitz, E. (2026, August 17). *Coupling Kratos Multiphysics with OpenFOAM for strongly coupled FSI problems* [Presentation slides]. Technische Universität München. https://github.com/juancamarotti/OpenFOAM_CoSimIO-Adapter/blob/develop/Coupling_Kratos_Multiphysics_with_OpenFOAM.pdf