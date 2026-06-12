#ifndef PRECICEADAPTER_H
#define PRECICEADAPTER_H
#include "Interface.H"

// Conjugate Heat Transfer module
#include "CHT/CHT.H"

// Fluid-Structure Interaction module
#include "FSI/FSI.H"

// Fluid-Fluid module
#include "FF/FF.H"

#include "module-generic/Generic.H"

// NOTE: If you want to couple a new variable, include your module's header here.
// You also need to include it in the Make/files file.
// In case you use additional OpenFOAM symbols, you may also need to specify
// the respective libraries in the Make/options.

// OpenFOAM header files
#include "Time.H"
#include "fvMesh.H"

// preCICE Solver Interface
#include <precice/precice.hpp>

// CoSimIO header file
#include "co_sim_io.hpp"
#include "co_sim_io_mpi.hpp"

using CoSimIOError = std::runtime_error;

namespace preciceAdapter
{

class Adapter
{

private:
    //- Structure of the configuration of each coupling interface.
    //  Every interface needs to know the coupling mesh, the OpenFOAM
    //  patches that consist the coupling surface and the kinds
    //  of data that are exchanged.
    struct InterfaceConfig
    {
        std::string MeshName;
        std::string LocationsType;
        bool MeshConnectivity;
        std::vector<std::string> PatchNames;
        std::vector<std::string> CellSetNames;
        std::vector<struct FieldConfig> WriteData;
        std::vector<struct FieldConfig> ReadData;
    };

    //- Configuration interfaces
    std::vector<struct InterfaceConfig> mInterfacesConfig;

#ifdef ADAPTER_ENABLE_TIMINGS
    //- Timers
    clockValue time_in_config_read;
    clockValue time_in_co_sim_io_construct;
    clockValue time_in_mesh_setup;
    clockValue time_in_initialize;
    clockValue time_in_checkpointing_setup;
    clockValue time_in_write;
    clockValue time_in_advance;
    clockValue time_in_read;
    clockValue time_in_checkpointing_read;
    clockValue time_in_checkpointing_write;
    clockValue time_in_write_results;
    clockValue time_in_finalize;
#endif

    //- Member variables required for CoSimIO
    std::string mConnectionName;

    // Control variables
    bool mStrongCoupling = false;
    bool mFirstOneToGo = false;
    double mEndTimeOfStepWindow;
    bool mSolveRequested;
    bool mCouplingOngoing = true;
    bool mRepeatTimeStep = false;
    std::string mLastControlSignal;

    //- OpenFOAM runTime object
    const Foam::Time& mRunTime;

    //- OpenFOAM fvMesh object
    const Foam::fvMesh& mMesh;

    // Configuration parameters used in the Adapter

    //- preCICE configuration file name
    Foam::word mCoSimIOConfigFilename;

    //- preCICE participant name
    Foam::word mParticipantName;

    //- Switch for checkpointing
    bool mCheckpointing = false;

    //- Moving mesh checkpointing
    bool mMeshCheckPointed = false;

    //- Type of timestep (fixed, adjustable)
    bool mAdjustableTimestep;

    //- Should the (fixed) timestep be stored and used?
    bool mUseStoredTimestep = false;

    //- Switch to enable the ConjugateHeatTransfer module
    bool mCHTEnabled = false;

    //- Switch to enable the FluidStructureInteraction module
    bool mFSIEnabled = false;

    //- Switch to enable the FluidFluid module
    bool mFFenabled = false;

    // NOTE: Add a switch for your new module here

    //- Switch to enable General module
    bool mGenericModuleEnabled = false;

    //- Interfaces
    std::vector<Interface*> mInterfaces;

    //- preCICE solver interface
    precice::Participant* mPrecice = nullptr;

    //- preCICE solver interface initialized
    bool mCoSimIOInitialized = false;

    //- Conjugate Heat Transfer module object
    CHT::ConjugateHeatTransfer* mCHT = nullptr;

    //- Fluid-Structure Interaction module object
    FSI::FluidStructureInteraction* mFSI = nullptr;

    //- Fluid-Fluid module object
    FF::FluidFluid* mFF = nullptr;

    //- Generic module object
    Generic::GenericInterface* mGeneric = nullptr;

    // NOTE: Add here a pointer for your new module object

    // Timesteps

    //- Timestep used by the solver
    double mTimeStepSolver;

    //- Stored (fixed) timestep
    double mTimeStepStored;

    // Checkpointing

    //- Checkpointed time (value)
    Foam::scalar mCouplingIterationTimeValue;

    //- Checkpointed time (index)
    Foam::label mCouplingIterationTimeIndex;

    // Vectors of pointers to the checkpointed mesh fields and their copies

    //- Checkpointed mesh points, oldPoints
    Foam::pointField* mMeshPoints = nullptr;
    Foam::pointField* mMeshOldPoints = nullptr;

    //- Checkpointed surfaceScalarField mesh fields (phi)
    std::vector<Foam::surfaceScalarField*> mMeshSurfaceScalarFields;

    //- Checkpointed surfaceScalarField mesh fields (copies)
    std::vector<Foam::surfaceScalarField*> mMeshSurfaceScalarFieldCopies;

    // Vectors of pointers to the checkpointed fields and their copies

    //- Checkpointed volScalarField fields
    std::vector<Foam::volScalarField*> mVolScalarFields;

    //- Checkpointed volScalarField fields (copies)
    std::vector<Foam::volScalarField*> mVolScalarFieldCopies;

    //- Checkpointed volVectorField fields
    std::vector<Foam::volVectorField*> mVolVectorFields;

    //- Checkpointed volVectorField fields (copies)
    std::vector<Foam::volVectorField*> mVolVectorFieldCopies;

    //- Checkpointed surfaceScalarField fields
    std::vector<Foam::surfaceScalarField*> mSurfaceScalarFields;

    //- Checkpointed surfaceScalarField fields (copies)
    std::vector<Foam::surfaceScalarField*> mSurfaceScalarFieldCopies;

    //- Checkpointed surfaceVectorField fields
    std::vector<Foam::surfaceVectorField*> mSurfaceVectorFields;

    //- Checkpointed surfaceVectorField fields (copies)
    std::vector<Foam::surfaceVectorField*> mSurfaceVectorFieldCopies;

    //- Checkpointed pointScalarField fields
    std::vector<Foam::pointScalarField*> mPointScalarFields;

    //- Checkpointed pointScalarField fields (copies)
    std::vector<Foam::pointScalarField*> mPointScalarFieldCopies;

    //- Checkpointed pointVectorField fields
    std::vector<Foam::pointVectorField*> mPointVectorFields;

    //- Checkpointed pointVectorField fields (copies)
    std::vector<Foam::pointVectorField*> mPointVectorFieldCopies;

    //- Checkpointed volTensorField fields
    std::vector<Foam::volTensorField*> mVolTensorFields;

    //- Checkpointed volTensorField fields (copies)
    std::vector<Foam::volTensorField*> mVolTensorFieldCopies;

    //- Checkpointed surfaceTensorField fields
    std::vector<Foam::surfaceTensorField*> mSurfaceTensorFields;

    //- Checkpointed surfaceTensorField fields (copies)
    std::vector<Foam::surfaceTensorField*> mSurfaceTensorFieldCopies;

    //- Checkpointed pointTensorField fields
    std::vector<Foam::pointTensorField*> mPointTensorFields;

    //- Checkpointed pointTensorField fields (copies)
    std::vector<Foam::pointTensorField*> mPointTensorFieldCopies;

    //- Checkpointed volSymmTensorField fields
    std::vector<Foam::volSymmTensorField*> mVolSymmTensorFields;

    //- Checkpointed volSymmTensorField fields (copies)
    std::vector<Foam::volSymmTensorField*> mVolSymmTensorFieldCopies;


    // NOTE: Declare additional vectors for any other types required.

    // Configuration

    //- Read the adapter's configuration file
    void ConfigFileRead();

    //- Helper to read a list of field configurations, which can contain either words, dictionaries, or both
    void ReadFieldConfigs(const std::string& listName, Foam::ITstream& stream, std::vector<FieldConfig>& configs);

    //- Check the adapter's configuration file
    bool ConfigFileCheck(const std::string adapterConfigFileName);

    // Methods communicating with CoSimIO
    void ConnectSolverToCoSimIO();

    void DisconnectSolverFromCoSimIO();

    //- Initialize preCICE and exchange the first data
    void Initialize();

    //- Finalize and destroy preCICE
    void Finalize();

    //- Advance preCICE
    void Advance();

    //- Read the coupling data at each interface
    void ReadCouplingData(double relativeReadTime);

    //- Write the coupling data at each interface
    void WriteCouplingData();

    //- Adjust the timestep of the solver according to preCICE
    // and read data associated to the calculated time step length
    void CheckSolverTimeStepAndReadData();

    //- Determine if the coupling is still happening
    bool IsCouplingOngoing();

    //- Determine if the coupling timestep has been completed
    bool IsCouplingTimeWindowComplete();

    //- Determine if a checkpoint must be read
    bool RequiresReadingCheckpoint();

    //- Determine if a checkpoint must be written
    bool RequiresWritingCheckpoint();

    // Methods for checkpointing

    //- Configure the mesh checkpointing
    void SetupMeshCheckpointing();

    //- Configure the checkpointing
    void SetupCheckpointing();

    //- Remove checkpointed fields which are not used by OpenFOAM anymore
    void PruneCheckpointedFields();


    //- Make a copy of the runTime object
    void StoreCheckpointTime();

    //- Restore the copy of the runTime object
    void ReloadCheckpointTime();

    //- Store the locations of the mesh points
    void StoreMeshPoints();

    //- Restore the locations of the mesh points
    void ReloadMeshPoints();

    // Add mesh checkpoint fields, depending on the type

    //- Add a surfaceScalarField mesh field (phi)
    void AddMeshCheckpointField(Foam::surfaceScalarField& field);

    //- Add a volScalarField to checkpoint
    void AddCheckpointField(volScalarField* field);

    //- Add a volVectorField to checkpoint
    void AddCheckpointField(volVectorField* field);

    //- Add a surfaceScalarField to checkpoint
    void AddCheckpointField(surfaceScalarField* field);

    //- Add a surfaceVectorField to checkpoint
    void AddCheckpointField(surfaceVectorField* field);

    //- Add a pointScalarField to checkpoint
    void AddCheckpointField(pointScalarField* field);

    //- Add a pointVectorField to checkpoint
    void AddCheckpointField(pointVectorField* field);

    // NOTE: Add here methods to add other object types to checkpoint,
    // if needed.

    //- Add a volTensorField to checkpoint
    void AddCheckpointField(volTensorField* field);

    //- Add a surfaceTensorField to checkpoint
    void AddCheckpointField(surfaceTensorField* field);

    //- Add a pointTensorField to checkpoint
    void AddCheckpointField(pointTensorField* field);

    //- Add a volSymmTensorField to checkpoint
    void AddCheckpointField(volSymmTensorField* field);

    //- Read the checkpoint - restore the mesh fields and time
    void ReadMeshCheckpoint();

    //- Read the checkpoint - restore the fields and time
    void ReadCheckpoint();

    //- Write the checkpoint - store the mesh fields and time
    void WriteMeshCheckpoint();

    //- Write the checkpoint - store the fields and time
    void WriteCheckpoint();

    //- Destroy the preCICE interface and delete the allocated
    //  memory in a proper way. Called by the destructor.
    void Teardown();

    // Import the control info
    CoSimIO::Info ImportControlInfo();

    // Process the control info
    void ProcessControlInfo(const CoSimIO::Info& rInfo);

public:
    // Methods called by the functionObject

    //- Constructor
    Adapter(const Foam::Time& runTime, const Foam::fvMesh& mesh);

    //- Setup the adapter's configuration
    void configure();

    //- Called by the functionObject's execute()
    void execute();

    //- Called by the functionObject's adjustTimeStep()
    void adjustTimeStep();

    //- Called by the functionObject's end()
    void end();

    //- Destructor
    ~Adapter();
};

}

#endif
