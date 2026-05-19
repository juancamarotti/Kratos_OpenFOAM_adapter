#include "Adapter.H"
#include "Interface.H"
#include "Utilities.H"

#include "IOstreams.H"
#include <algorithm>

using namespace Foam;

preciceAdapter::Adapter::Adapter(const Time& runTime, const fvMesh& mesh)
: mRunTime(runTime),
  mMesh(mesh)
{
    adapterInfo("Loaded the OpenFOAM-preCICE adapter - v1.3.1.", "info");

    return;
}

void preciceAdapter::Adapter::ReadFieldConfigs(const std::string& listName, Foam::ITstream& stream, std::vector<FieldConfig>& configs)
{
    // Perform check on whether read/WriteData is a list
    if (stream.peek() == token::BEGIN_LIST)
    {
        token _t;
        stream >> _t; // First token is '(', which we throw away

        // Read stream until end of list
        while (stream.peek() != token::END_LIST && !stream.eof())
        {
            // Next token is always a word (the data name)
            word dataName;
            stream >> dataName;

            struct FieldConfig fieldConfig;

            // If next token is '{', we have a dictionary (new schema).
            // We create a dictionary from the stream `dictionary dict(stream);`
            // The dictionary must contain the 'name'.
            // If 'solver_name' is not specified, it defaults to the same as 'name'.
            // 'operation' defaults to 'value'.
            // Note: Currently, the modules FF, CHT, and FSI do not use solver_name/operation.
            if (stream.peek() == token::BEGIN_BLOCK)
            {
                dictionary dict(stream);
                fieldConfig.name = dict.get<word>("name"); // The 'name' entry is mandatory.
                fieldConfig.solver_name = dict.lookupOrDefault<word>("solver_name", fieldConfig.name);
                fieldConfig.operation = dict.lookupOrDefault<word>("operation", "value");
                try
                {
                    fieldConfig.flip_normal = dict.lookupOrDefault<bool>("flip-normal", false);
                }
                catch (const Foam::IOerror& e)
                {
                    adapterInfo("Error parsing 'flip-normal' for field " + dataName + "\n" + e.message(), "error");
                }
            }
            // Else, we have a simple word entry (legacy schema/backwards compatibility).
            else
            {
                fieldConfig.name = dataName;
                fieldConfig.solver_name = "Undefined (legacy mode)";
                fieldConfig.operation = "Undefined (legacy mode)";
                fieldConfig.flip_normal = false;
            }

            configs.push_back(fieldConfig);

            DEBUG(adapterInfo("      - " + dataName));
            DEBUG(adapterInfo("        name: " + fieldConfig.name));
            DEBUG(adapterInfo("        solver_name: " + fieldConfig.solver_name));
            DEBUG(adapterInfo("        operation  : " + fieldConfig.operation));
            DEBUG(adapterInfo("        flip-normal: " + std::string(fieldConfig.flip_normal ? "true" : "false")));
        }
        stream >> _t; // Last token ')'
    }
    else
    {
        adapterInfo(listName + " must be a list", "error");
    }
}

void preciceAdapter::Adapter::ConfigFileRead()
{

    SETUP_TIMER();
    adapterInfo("Reading CoSimIODict...", "info");

    // TODO: static is just a quick workaround to be able
    // to find the dictionary also out of scope (e.g. in KappaEffective).
    // We need a better solution.
    static IOdictionary CoSimIODict(
        IOobject(
            "CoSimIODict",
            mRunTime.system(),
            mMesh,
            IOobject::MUST_READ_IF_MODIFIED,
            IOobject::NO_WRITE));

    // Read and display the preCICE configuration file name
    mCoSimIOConfigFilename = CoSimIODict.get<fileName>("preciceConfig");
    DEBUG(adapterInfo("  precice-config-file : " + mCoSimIOConfigFilename));

    // Read and display the participant name
    mParticipantName = CoSimIODict.get<word>("participant");
    DEBUG(adapterInfo("  participant name    : " + mParticipantName));

    // Read and display the list of modules
    DEBUG(adapterInfo("  modules requested   : "));
    auto modules_ = CoSimIODict.get<wordList>("modules");
    for (const auto& module : modules_)
    {
        DEBUG(adapterInfo("  - " + module + "\n"));

        // Set the modules switches
        if (module == "CHT")
        {
            mCHTEnabled = true;
        }

        if (module == "FSI")
        {
            mFSIEnabled = true;
        }

        if (module == "FF")
        {
            mFFenabled = true;
        }

        if (module == "generic")
        {
            mGenericModuleEnabled = true;
        }
    }

    // Every interface is a subdictionary of "interfaces",
    // each with an arbitrary name. Read all of them and create
    // a list (here: pointer) of dictionaries.
    const auto* interfaceDictPtr = CoSimIODict.findDict("interfaces");
    DEBUG(adapterInfo("  interfaces : "));

    // Check if we found any interfaces
    // and get the details of each interface
    if (!interfaceDictPtr)
    {
        adapterInfo("  Empty list of interfaces", "error");
        return;
    }
    else
    {
        for (const entry& interfaceDictEntry : *interfaceDictPtr)
        {
            if (interfaceDictEntry.isDict())
            {
                const dictionary& interfaceDict = interfaceDictEntry.dict();
                struct InterfaceConfig interfaceConfig;

                interfaceConfig.MeshName = interfaceDict.get<word>("mesh");
                DEBUG(adapterInfo("  - mesh         : " + interfaceConfig.MeshName));

                // By default, assume "faceCenters" as LocationsType
                interfaceConfig.LocationsType = interfaceDict.lookupOrDefault<word>("locations", "faceCenters");
                DEBUG(adapterInfo("    locations    : " + interfaceConfig.LocationsType));

                // By default, assume that no mesh connectivity is required (i.e. no nearest-projection mapping)
                interfaceConfig.MeshConnectivity = interfaceDict.lookupOrDefault<bool>("connectivity", false);
                // Mesh connectivity only makes sense in case of faceNodes, check and raise a warning otherwise
                if (interfaceConfig.MeshConnectivity && (interfaceConfig.LocationsType == "faceCenters" || interfaceConfig.LocationsType == "volumeCenters" || interfaceConfig.LocationsType == "volumeCentres"))
                {
                    DEBUG(adapterInfo("Mesh connectivity is not supported for faceCenters or volumeCenters. \n"
                                      "Please configure the desired interface with the LocationsType faceNodes. \n"
                                      "Have a look in the adapter documentation for detailed information.",
                                      "error"));
                    return;
                }
                DEBUG(adapterInfo("    connectivity : " + std::to_string(interfaceConfig.MeshConnectivity)));

                DEBUG(adapterInfo("    patches      : "));
                auto patches = interfaceDict.get<wordList>("patches");
                for (auto patch : patches)
                {
                    interfaceConfig.PatchNames.push_back(patch);
                    DEBUG(adapterInfo("      - " + patch));
                }

                DEBUG(adapterInfo("    cellSets      : "));
                auto cellSets = interfaceDict.lookupOrDefault<wordList>("cellSets", wordList());

                for (auto cellSet : cellSets)
                {
                    interfaceConfig.CellSetNames.push_back(cellSet);
                    DEBUG(adapterInfo("      - " + cellSet));
                }

                if (!interfaceConfig.CellSetNames.empty() && !(interfaceConfig.LocationsType == "volumeCenters" || interfaceConfig.LocationsType == "volumeCentres"))
                {
                    adapterInfo("Cell sets are not supported for locationType != volumeCenters. \n"
                                "Please configure the desired interface with the LocationsType volumeCenters. \n"
                                "Have a look in the adapter documentation for detailed information.",
                                "error");
                    return;
                }

                if (interfaceDict.found("WriteData"))
                {
                    DEBUG(adapterInfo("    WriteData    : "));
                    ITstream writeDataStream = interfaceDict.lookup("WriteData");
                    ReadFieldConfigs("WriteData", writeDataStream, interfaceConfig.WriteData);
                }

                if (interfaceDict.found("ReadData"))
                {
                    DEBUG(adapterInfo("    ReadData     : "));
                    ITstream readDataStream = interfaceDict.lookup("ReadData");
                    ReadFieldConfigs("ReadData", readDataStream, interfaceConfig.ReadData);
                }

                mInterfacesConfig.push_back(interfaceConfig);
            }
        }
    }

    // NOTE: set the switch for your new module here

    if (mGenericModuleEnabled)
    {
        mGeneric = new Generic::GenericInterface(mMesh);
        if (!mGeneric->configure(CoSimIODict))
        {
            return;
        }
    }

    // If the CHT module is enabled, create it, read the
    // CHT-specific options and configure it.
    if (mCHTEnabled)
    {
        mCHT = new CHT::ConjugateHeatTransfer(mMesh);
        if (!mCHT->configure(CoSimIODict))
        {
            adapterInfo("There was an error while configuring the CHT module",
                        "error");
            return;
        }
    }

    // If the FSI module is enabled, create it, read the
    // FSI-specific options and configure it.
    if (mFSIEnabled)
    {
        mFSI = new FSI::FluidStructureInteraction(mMesh, mRunTime);
        if (!mFSI->configure(CoSimIODict))
        {
            adapterInfo("There was an error while configuring the FSI module",
                        "error");
            return;
        }
    }

    if (mFFenabled)
    {
        mFF = new FF::FluidFluid(mMesh);
        if (!mFF->configure(CoSimIODict))
        {
            adapterInfo("There was an error while configuring the FF module",
                        "error");
            return;
        }
    }

    // NOTE: Create your module and read any options specific to it here

    if (!mCHTEnabled && !mFSIEnabled && !mFFenabled && !mGenericModuleEnabled) // NOTE: Add your new switch here
    {
        adapterInfo("No module is enabled.", "error");
        return;
    }

    // TODO: Loading modules should be implemented in more general way,
    // in order to avoid code duplication. See issue #16 on GitHub.

    ACCUMULATE_TIMER(time_in_config_read_);

    return;
}

void preciceAdapter::Adapter::configure()
try
{
    // Read the adapter's configuration file
    ConfigFileRead();

    // Check the timestep type (fixed vs adjustable)
    DEBUG(adapterInfo("Checking the timestep type (fixed vs adjustable)..."));
    mAdjustableTimestep = mRunTime.controlDict().lookupOrDefault("adjustTimeStep", false);

    if (mAdjustableTimestep)
    {
        DEBUG(adapterInfo("  Timestep type: adjustable."));
    }
    else
    {
        DEBUG(adapterInfo("  Timestep type: fixed."));
    }

    // Construct preCICE
    SETUP_TIMER();
    DEBUG(adapterInfo("Creating the preCICE solver interface..."));
    DEBUG(adapterInfo("  Number of processes: " + std::to_string(Pstream::nProcs())));
    DEBUG(adapterInfo("  MPI rank: " + std::to_string(Pstream::myProcNo())));
    //mPrecice = new precice::Participant(mParticipantName, mCoSimIOConfigFilename, Pstream::myProcNo(), Pstream::nProcs());
    ConnectSolverToCoSimIO();
    std::cout << "Connection successful" << std::endl;
    // exit(0);
    DEBUG(adapterInfo("  preCICE solver interface was created."));

    ACCUMULATE_TIMER(time_in_co_sim_io_construct);

    // Create interfaces
    REUSE_TIMER();
    DEBUG(adapterInfo("Creating interfaces..."));
    for (uint i = 0; i < mInterfacesConfig.size(); i++)
    {
        std::string namePointDisplacement = mFSIEnabled ? mFSI->getPointDisplacementFieldName() : "default";
        std::string nameCellDisplacement = mFSIEnabled ? mFSI->getCellDisplacementFieldName() : "default";
        bool restartFromDeformed = mFSIEnabled ? mFSI->isRestartingFromDeformed() : false;

        Interface* interface = new Interface(*mPrecice, mMesh, mInterfacesConfig.at(i).MeshName, mInterfacesConfig.at(i).LocationsType, mInterfacesConfig.at(i).PatchNames, mInterfacesConfig.at(i).CellSetNames, mInterfacesConfig.at(i).MeshConnectivity, restartFromDeformed, namePointDisplacement, nameCellDisplacement, mConnectionName);
        mInterfaces.push_back(interface);
        DEBUG(adapterInfo("Interface created on mesh " + mInterfacesConfig.at(i).MeshName));

        DEBUG(adapterInfo("Adding coupling data writers..."));
        for (uint j = 0; j < mInterfacesConfig.at(i).WriteData.size(); j++)
        {
            const FieldConfig& fieldConfig = mInterfacesConfig.at(i).WriteData.at(j);
            std::string dataName = fieldConfig.name;

            unsigned int inModules = 0;

            // Add CHT-related coupling data writers
            if (mCHTEnabled && mCHT->addWriters(fieldConfig, interface))
            {
                inModules++;
            }

            // Add FSI-related coupling data writers
            if (mFSIEnabled && mFSI->addWriters(fieldConfig, interface))
            {
                inModules++;
            }

            // Add FF-related coupling data writers
            if (mFFenabled && mFF->addWriters(fieldConfig, interface))
            {
                inModules++;
            }

            // Add generic module coupling data writers
            // Only add Generic interface if not found in other modules
            if (inModules == 0)
            {
                if (mGenericModuleEnabled && mGeneric->addWriters(fieldConfig, interface))
                {
                    inModules++;
                };
            }

            if (inModules == 0)
            {
                adapterInfo("I don't know how to write \"" + dataName
                                + "\". Maybe this is a typo or maybe you need to enable some adapter module?",
                            "error");
            }
            else if (inModules > 1)
            {
                adapterInfo("It looks like more than one modules can write \"" + dataName
                                + "\" and I don't know how to choose. Try disabling one of the modules.",
                            "error");
            }

            // NOTE: Add any coupling data writers for your module here.
        } // end add coupling data writers

        DEBUG(adapterInfo("Adding coupling data readers..."));
        for (uint j = 0; j < mInterfacesConfig.at(i).ReadData.size(); j++)
        {
            const FieldConfig& fieldConfig = mInterfacesConfig.at(i).ReadData.at(j);
            std::string dataName = fieldConfig.name;

            unsigned int inModules = 0;

            // Add CHT-related coupling data readers
            if (mCHTEnabled && mCHT->addReaders(fieldConfig, interface))
            {
                inModules++;
            }

            // Add FSI-related coupling data readers
            if (mFSIEnabled && mFSI->addReaders(fieldConfig, interface))
            {
                inModules++;
            }

            // Add FF-related coupling data readers
            if (mFFenabled && mFF->addReaders(fieldConfig, interface))
            {
                inModules++;
            }

            // Add generic module coupling data readers
            // Only add Generic interface if not found in other modules
            if (inModules == 0)
            {
                if (mGenericModuleEnabled && mGeneric->addReaders(fieldConfig, interface))
                {
                    inModules++;
                }
            }

            if (inModules == 0)
            {
                adapterInfo("I don't know how to read \"" + dataName
                                + "\". Maybe this is a typo or maybe you need to enable some adapter module?",
                            "error");
            }
            else if (inModules > 1)
            {
                adapterInfo("It looks like more than one modules can read \"" + dataName
                                + "\" and I don't know how to choose. Try disabling one of the modules.",
                            "error");
            }

            // NOTE: Add any coupling data readers for your module here.
        } // end add coupling data readers

        // Create the interface's data buffer
        interface->createBuffer();
    }
    ACCUMULATE_TIMER(time_in_mesh_setup);
    std::cout << "THE READERS AND WRITERS WERE CREATED SUCCESSFULLY" << std::endl;

    // Initialize preCICE and exchange the first coupling data
    Initialize();

    // If checkpointing is required, specify the checkpointed fields
    // and write the first checkpoint
    // if (RequiresWritingCheckpoint())
    // {
    //     mCheckpointing = true;

    //     // Setup the checkpointing (find and add fields to checkpoint)
    //     SetupCheckpointing();

    //     // Write checkpoint (for the first iteration)
    //     WriteCheckpoint();
    // }

    // Adjust the timestep for the first iteration, if it is fixed
    // if (!mAdjustableTimestep)
    // {
    //     AdjustSolverTimeStepAndReadData();
    // }

    // If the solver tries to end before the coupling is complete,
    // e.g. because the solver's endTime was smaller or (in implicit
    // coupling) equal with the max-time specified in preCICE,
    // problems may occur near the end of the simulation,
    // as the function object may be called only once near the end.
    // See the implementation of Foam::Time::run() for more details.
    // To prevent this, we set the solver's endTime to "infinity"
    // and let only preCICE control the end of the simulation.
    // This has the side-effect of not triggering the end() method
    // in any function object normally. Therefore, we trigger it
    // when preCICE dictates to stop the coupling.
    adapterInfo(
        "Setting the solver's endTime to infinity to prevent early exits. "
        "Only preCICE will control the simulation's endTime. "
        "Any functionObject's end() method will be triggered by the adapter. "
        "You may disable this behavior in the adapter's configuration.",
        "info");
    const_cast<Time&>(mRunTime).setEndTime(GREAT);

    return;
}
catch (const CoSimIOError& e)
{
    std::exit(EXIT_FAILURE);
}

void preciceAdapter::Adapter::execute()
try
{

    // The solver has already solved the equations for this timestep.
    // Now call the adapter's methods to perform the coupling.

    // TODO add a function which checks if all fields are checkpointed.
    // if (ncheckpointed is nregisterdobjects. )

    // Write the coupling data in the buffer
    WriteCouplingData();
    std::cout << "The data export to co sim io was successful" << std::endl;

    // Advance preCICE
    Advance();

    // Read checkpoint if required
    if (RequiresReadingCheckpoint())
    {
        PruneCheckpointedFields();
        ReadCheckpoint();
    }

    // Write checkpoint if required
    if (RequiresWritingCheckpoint())
    {
        WriteCheckpoint();
    }
    exit(0);

    // As soon as OpenFOAM writes the results, it will not try to write again
    // if the time takes the same value again. Therefore, during an implicit
    // coupling, we write again when the coupling timestep is complete.
    // Check the behavior e.g. by using watch on a result file:
    //     watch -n 0.1 -d ls --full-time Fluid/0.01/T.gz
    SETUP_TIMER();
    if (mCheckpointing && IsCouplingTimeWindowComplete())
    {
        // Check if the time directory already exists
        // (i.e. the solver wrote results that need to be updated)
        if (mRunTime.timePath().type() == fileName::DIRECTORY)
        {
            adapterInfo(
                "The coupling timestep completed. "
                "Writing the updated results.",
                "info");
            const_cast<Time&>(mRunTime).writeNow();
        }
    }
    ACCUMULATE_TIMER(time_in_write_results);

    // Adjust the timestep, if it is fixed
    if (!mAdjustableTimestep)
    {
        AdjustSolverTimeStepAndReadData();
    }

    // If the coupling is not going to continue, tear down everything
    // and stop the simulation.
    if (!IsCouplingOngoing())
    {
        adapterInfo("The coupling completed.", "info");

        // Finalize the preCICE solver interface and delete data
        Finalize();

        // Tell OpenFOAM to stop the simulation.
        // Set the solver's endTime to now. The next evaluation of
        // runTime.run() will be false and the solver will exit.
        const_cast<Time&>(mRunTime).setEndTime(mRunTime.value());
        adapterInfo(
            "The simulation was ended by preCICE. "
            "Calling the end() methods of any functionObject explicitly.",
            "info");
        adapterInfo("Great that you are using the OpenFOAM-preCICE adapter! "
                    "Next to the preCICE library and any other components, please also cite this adapter. "
                    "Find how on https://precice.org/adapter-openfoam-overview.html.",
                    "info");
        const_cast<Time&>(mRunTime).functionObjects().end();
    }

    return;
}
catch (const CoSimIOError& e)
{
    std::exit(EXIT_FAILURE);
}


void preciceAdapter::Adapter::adjustTimeStep()
try
{
    AdjustSolverTimeStepAndReadData();

    return;
}
catch (const CoSimIOError& e)
{
    std::exit(EXIT_FAILURE);
}

void preciceAdapter::Adapter::ReadCouplingData(double relativeReadTime)
{
    SETUP_TIMER();
    DEBUG(adapterInfo("Reading coupling data..."));

    for (uint i = 0; i < mInterfaces.size(); i++)
    {
        mInterfaces.at(i)->readCouplingData(relativeReadTime);
    }

    ACCUMULATE_TIMER(time_in_read);

    return;
}

void preciceAdapter::Adapter::WriteCouplingData()
{
    SETUP_TIMER();
    DEBUG(adapterInfo("Writing coupling data..."));

    for (uint i = 0; i < mInterfaces.size(); i++)
    {
        std::cout << "EXPORTING LOADS TO CO SIM IO" << std::endl;
        mInterfaces.at(i)->writeCouplingData();
    }

    ACCUMULATE_TIMER(time_in_write);

    return;
}

void preciceAdapter::Adapter::ConnectSolverToCoSimIO()
{
    // Connection between openFOAM and Kratos-CoSimulation using CoSimIO (ONLY ONE TIME for multiple interfaces)
    CoSimIO::Info settings;
    settings.Set("my_name", "Openfoam_Adapter");
    settings.Set("connect_to", "Openfoam_Kratos_Wrapper");
    settings.Set("communication_format", "file");
    settings.Set("echo_level", 0);
    settings.Set("version", "1.25");
    CoSimIO::Info connect_info;

    // if(TotalNumOfProcesses == 1)
    // {
    Info << "Running in Serial. Connecting to CoSimulation using File IO" << nl;
    connect_info = CoSimIO::Connect(settings);
    //
    //else{
        // Info << "Running in Parallel. Connecting to CoSimulation using MPI" << nl;
        // connect_info = CoSimIO::ConnectMPI(settings, MPI_COMM_WORLD);
    //}

    //COSIMIO_CHECK_EQUAL(connect_info.Get<int>("connection_status"), CoSimIO::ConnectionStatus::Connected);
    mConnectionName = connect_info.Get<std::string>("connection_name");

    return;
}

void preciceAdapter::Adapter::Initialize()
{
    DEBUG(adapterInfo("Initializing the preCICE solver interface..."));
    SETUP_TIMER();

    // if (mPrecice->requiresInitialData())
    // {
    //     DEBUG(adapterInfo("Initializing preCICE data..."));
    //     WriteCouplingData();
    // }

    // mPrecice->initialize();
    mCoSimIOInitialized = true;
    ACCUMULATE_TIMER(time_in_initialize);

    adapterInfo("preCICE was configured and initialized", "info");

    return;
}

void preciceAdapter::Adapter::Finalize()
{
    if (nullptr != mPrecice && mCoSimIOInitialized && !IsCouplingOngoing())
    {
        DEBUG(adapterInfo("Finalizing the preCICE solver interface..."));

        // Finalize the preCICE solver interface
        SETUP_TIMER();
        mPrecice->finalize();
        ACCUMULATE_TIMER(time_in_finalize);

        mCoSimIOInitialized = false;

        // Delete the solver interface and all the related data
        Teardown();
    }
    else
    {
        adapterInfo("Could not finalize preCICE.", "error");
    }

    return;
}

void preciceAdapter::Adapter::Advance()
{
    DEBUG(adapterInfo("Advancing preCICE..."));

    SETUP_TIMER();
    mPrecice->advance(mTimeStepSolver);
    ACCUMULATE_TIMER(time_in_advance);

    return;
}

void preciceAdapter::Adapter::AdjustSolverTimeStepAndReadData()
{
    DEBUG(adapterInfo("Adjusting the solver's timestep..."));

    // The timestep size that the solver has determined that it wants to use
    double timestepSolverDetermined;

    /* In this method, the adapter overwrites the timestep used by OpenFOAM.
       If the timestep is not adjustable, OpenFOAM will not try to re-estimate
       the timestep or read it again from the controlDict. Therefore, store
       the value that the timestep has is the beginning and try again to use this
       in every iteration.
       // TODO Treat also the case where the user modifies the timestep
       // in the controlDict during the simulation.
    */

    // Is the timestep adjustable or fixed?
    if (!mAdjustableTimestep)
    {
        // Have we already stored the timestep?
        if (!mUseStoredTimestep)
        {
            // Show a warning if runTimeModifiable is set
            if (mRunTime.runTimeModifiable())
            {
                adapterInfo(
                    "You have enabled 'runTimeModifiable' in the "
                    "controlDict. The preciceAdapter does not yet "
                    "fully support this functionality when "
                    "'adjustableTimestep' is not enabled. "
                    "If you modify the 'deltaT' in the controlDict "
                    "during the simulation, it will not be updated.",
                    "warning");
            }

            // Store the value
            mTimeStepStored = mRunTime.deltaT().value();

            // Ok, we stored it once, we will use this from now on
            mUseStoredTimestep = true;
        }

        // Use the stored timestep as the determined solver's timestep
        timestepSolverDetermined = mTimeStepStored;
    }
    else
    {
        // The timestep is adjustable, so OpenFOAM will modify it
        // and therefore we can use the updated value
        timestepSolverDetermined = mRunTime.deltaT().value();
    }

    /* If the solver tries to use a timestep smaller than the one determined
       by preCICE, that means that the solver is trying to subcycle.
       This may not be allowed by the user.
       If the solver tries to use a bigger timestep, then it needs to use
       the same timestep as the one determined by preCICE.
    */
    double tolerance = 1e-14;
    if (mPrecice->getMaxTimeStepSize() - timestepSolverDetermined > tolerance)
    {
        adapterInfo(
            "The solver's timestep is smaller than the "
            "coupling timestep. Subcycling...",
            "info");
        mTimeStepSolver = timestepSolverDetermined;
        if (mFSIEnabled)
        {
            adapterInfo(
                "The adapter does not fully support subcycling for FSI and instabilities may occur.",
                "warning");
        }
    }
    else if (timestepSolverDetermined - mPrecice->getMaxTimeStepSize() > tolerance)
    {
        // In the last time-step, we adjust to dt = 0, but we don't need to trigger the warning here
        if (IsCouplingOngoing())
        {
            adapterInfo(
                "The solver's timestep cannot be larger than the coupling timestep."
                " Adjusting from "
                    + std::to_string(timestepSolverDetermined) + " to " + std::to_string(mPrecice->getMaxTimeStepSize()),
                "warning");
        }
        mTimeStepSolver = mPrecice->getMaxTimeStepSize();
    }
    else
    {
        DEBUG(adapterInfo("The solver's timestep is the same as the "
                          "coupling timestep."));
        mTimeStepSolver = mPrecice->getMaxTimeStepSize();
    }

    // Update the solver's timestep (but don't trigger the adjustDeltaT(),
    // which also triggers the functionObject's adjustTimeStep())
    // TODO: Keep this in mind if any relevant problem appears.
    const_cast<Time&>(mRunTime).setDeltaT(mTimeStepSolver, false);

    DEBUG(adapterInfo("Reading coupling data associated to the calculated time-step size..."));

    // Read the received coupling data from the buffer
    // Fits to an implicit Euler
    ReadCouplingData(mRunTime.deltaT().value());
    return;
}

bool preciceAdapter::Adapter::IsCouplingOngoing()
{
    bool IsCouplingOngoing = false;

    // If the coupling ends before the solver ends,
    // the solver would try to access this method again,
    // giving a segmentation fault if mPrecice
    // was not available.
    if (nullptr != mPrecice)
    {
        IsCouplingOngoing = mPrecice->isCouplingOngoing();
    }

    return IsCouplingOngoing;
}

bool preciceAdapter::Adapter::IsCouplingTimeWindowComplete()
{
    return mPrecice->isTimeWindowComplete();
}

bool preciceAdapter::Adapter::RequiresReadingCheckpoint()
{
    return mPrecice->requiresReadingCheckpoint();
}

bool preciceAdapter::Adapter::RequiresWritingCheckpoint()
{
    return mPrecice->requiresWritingCheckpoint();
}


void preciceAdapter::Adapter::StoreCheckpointTime()
{
    mCouplingIterationTimeIndex = mRunTime.timeIndex();
    mCouplingIterationTimeValue = mRunTime.value();
    DEBUG(adapterInfo("Stored time value t = " + std::to_string(mRunTime.value())));

    return;
}

void preciceAdapter::Adapter::ReloadCheckpointTime()
{
    const_cast<Time&>(mRunTime).setTime(mCouplingIterationTimeValue, mCouplingIterationTimeIndex);
    // TODO also reset the current iteration?!
    DEBUG(adapterInfo("Reloaded time value t = " + std::to_string(mRunTime.value())));

    return;
}

void preciceAdapter::Adapter::StoreMeshPoints()
{
    if (!mMeshPoints)
    {
        DEBUG(adapterInfo("Storing mesh points..."));
        // Add points and oldPoints
        mMeshPoints = new Foam::pointField(mMesh.points());
        mMeshOldPoints = new Foam::pointField(mMesh.oldPoints());
    }

    if (mMesh.moving())
    {
        if (!mMeshCheckPointed)
        {
            // Set up the checkpoint for the mesh flux: meshPhi
            SetupMeshCheckpointing();
            mMeshCheckPointed = true;
        }
        WriteMeshCheckpoint();
    }
}

void preciceAdapter::Adapter::ReloadMeshPoints()
{
    if (!mMesh.moving())
    {
        DEBUG(adapterInfo("Mesh points not moved as the mesh is not moving"));
        return;
    }

    // Reload mesh points
    const_cast<Foam::fvMesh&>(mMesh).movePoints(*mMeshPoints);

    // polyMesh.movePoints will only update oldPoints
    // if (curMotionTimeIndex_ != time().timeIndex())
    const_cast<pointField&>(mMesh.oldPoints()) = *mMeshOldPoints;

    ReadMeshCheckpoint();

    DEBUG(adapterInfo("Moved mesh points to their previous locations."));
}

void preciceAdapter::Adapter::SetupMeshCheckpointing()
{
    // The other mesh <type>Fields:
    //      C
    //      Cf
    //      Sf
    //      magSf
    //      delta
    // are updated by the function fvMesh::movePoints. Only the meshPhi needs checkpointing.
    DEBUG(adapterInfo("Creating a list of the mesh checkpointed fields..."));
    // Add meshPhi (Face motion flux)
    AddMeshCheckpointField(const_cast<surfaceScalarField&>(mMesh.phi()));

    DEBUG(adapterInfo("Added " + mMesh.phi().name() + " to the list of checkpointed fields."));
}


void preciceAdapter::Adapter::SetupCheckpointing()
{
    SETUP_TIMER();

    // Add fields in the checkpointing list - sorted for parallel consistency
    DEBUG(adapterInfo("Adding in checkpointed fields..."));

#undef doLocalCode
#define doLocalCode(GeomFieldType)                                           \
    /* Checkpoint registered GeomFieldType objects */                        \
    for (const word& obj : mMesh.sortedNames<GeomFieldType>())               \
    {                                                                        \
        AddCheckpointField(mMesh.thisDb().getObjectPtr<GeomFieldType>(obj)); \
        DEBUG(adapterInfo("Checkpoint " + obj + " : " #GeomFieldType));      \
    }

    doLocalCode(volScalarField);
    doLocalCode(volVectorField);
    doLocalCode(volTensorField);
    doLocalCode(volSymmTensorField);

    doLocalCode(surfaceScalarField);
    doLocalCode(surfaceVectorField);
    doLocalCode(surfaceTensorField);

    doLocalCode(pointScalarField);
    doLocalCode(pointVectorField);
    doLocalCode(pointTensorField);

    // NOTE: Add here other object types to checkpoint, if needed.

#undef doLocalCode

    ACCUMULATE_TIMER(time_in_checkpointing_setup);
}

void preciceAdapter::Adapter::PruneCheckpointedFields()
{
    // Check if checkpointed fields exist in OpenFOAM registry
    // If not, remove them from the checkpointed fields vector

    word fieldName;
    uint index;
    std::vector<word> regFields;
    std::vector<uint> toRemoveIndices;

#undef doLocalCode
#define doLocalCode(GeomFieldType, GeomField_, GeomFieldCopies_)                                                                              \
    regFields.clear();                                                                                                                        \
    toRemoveIndices.clear();                                                                                                                  \
    index = 0;                                                                                                                                \
    /* Iterate through fields in OpenFOAM registry */                                                                                         \
    for (const word& fieldName : mMesh.sortedNames<GeomFieldType>())                                                                          \
    {                                                                                                                                         \
        regFields.push_back(fieldName);                                                                                                       \
    }                                                                                                                                         \
    /* Iterate through checkpointed fields */                                                                                                 \
    for (GeomFieldType * fieldObj : GeomFieldCopies_)                                                                                         \
    {                                                                                                                                         \
        fieldName = fieldObj->name();                                                                                                         \
        if (std::find(regFields.begin(), regFields.end(), fieldName) == regFields.end())                                                      \
        {                                                                                                                                     \
            toRemoveIndices.push_back(index);                                                                                                 \
        }                                                                                                                                     \
        index += 1;                                                                                                                           \
    }                                                                                                                                         \
    if (!toRemoveIndices.empty())                                                                                                             \
    {                                                                                                                                         \
        /* Iterate in reverse to avoid index shifting */                                                                                      \
        for (auto it = toRemoveIndices.rbegin(); it != toRemoveIndices.rend(); ++it)                                                          \
        {                                                                                                                                     \
            index = *it;                                                                                                                      \
            DEBUG(adapterInfo("Removed " #GeomFieldType " : " + GeomFieldCopies_.at(index)->name() + " from the checkpointed fields list.")); \
            GeomField_.erase(GeomField_.begin() + index);                                                                                     \
            delete GeomFieldCopies_.at(index);                                                                                                \
            GeomFieldCopies_.erase(GeomFieldCopies_.begin() + index);                                                                         \
        }                                                                                                                                     \
    }

    doLocalCode(volScalarField, mVolScalarFields, mVolScalarFieldCopies);
    doLocalCode(volVectorField, mVolVectorFields, mVolVectorFieldCopies);
    doLocalCode(volTensorField, mVolTensorFields, mVolTensorFieldCopies);
    doLocalCode(volSymmTensorField, mVolSymmTensorFields, mVolSymmTensorFieldCopies);

    doLocalCode(surfaceScalarField, mSurfaceScalarFields, mSurfaceScalarFieldCopies);
    doLocalCode(surfaceVectorField, mSurfaceVectorFields, mSurfaceVectorFieldCopies);
    doLocalCode(surfaceTensorField, mSurfaceTensorFields, mSurfaceTensorFieldCopies);

    doLocalCode(pointScalarField, mPointScalarFields, mPointScalarFieldCopies);
    doLocalCode(pointVectorField, mPointVectorFields, mPointVectorFieldCopies);
    doLocalCode(pointTensorField, mPointTensorFields, mPointTensorFieldCopies);

#undef doLocalCode
}

// All mesh checkpointed fields

void preciceAdapter::Adapter::AddMeshCheckpointField(surfaceScalarField& field)
{
    mMeshSurfaceScalarFields.push_back(&field);
    mMeshSurfaceScalarFieldCopies.push_back(new surfaceScalarField(field));
}

void preciceAdapter::Adapter::AddCheckpointField(volScalarField* field)
{
    if (field)
    {
        mVolScalarFields.push_back(field);
        mVolScalarFieldCopies.push_back(new volScalarField(*field));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(volVectorField* field)
{
    if (field)
    {
        mVolVectorFields.push_back(field);
        mVolVectorFieldCopies.push_back(new volVectorField(*field));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(surfaceScalarField* field)
{
    if (field)
    {
        mSurfaceScalarFields.push_back(field);
        mSurfaceScalarFieldCopies.push_back(new surfaceScalarField(*field));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(surfaceVectorField* field)
{
    if (field)
    {
        mSurfaceVectorFields.push_back(field);
        mSurfaceVectorFieldCopies.push_back(new surfaceVectorField(*field));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(pointScalarField* field)
{
    if (field)
    {
        mPointScalarFields.push_back(field);
        mPointScalarFieldCopies.push_back(new pointScalarField(*field));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(pointVectorField* field)
{
    if (field)
    {
        mPointVectorFields.push_back(field);
        mPointVectorFieldCopies.push_back(new pointVectorField(*field));
        // TODO: Old time
        // pointVectorFieldsOld_.push_back(const_cast<pointVectorField&>(field->oldTime())));
        // pointVectorFieldCopiesOld_.push_back(new pointVectorField(field->oldTime()));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(volTensorField* field)
{
    if (field)
    {
        mVolTensorFields.push_back(field);
        mVolTensorFieldCopies.push_back(new volTensorField(*field));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(surfaceTensorField* field)
{
    if (field)
    {
        mSurfaceTensorFields.push_back(field);
        mSurfaceTensorFieldCopies.push_back(new surfaceTensorField(*field));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(pointTensorField* field)
{
    if (field)
    {
        mPointTensorFields.push_back(field);
        mPointTensorFieldCopies.push_back(new pointTensorField(*field));
    }
}

void preciceAdapter::Adapter::AddCheckpointField(volSymmTensorField* field)
{
    if (field)
    {
        mVolSymmTensorFields.push_back(field);
        mVolSymmTensorFieldCopies.push_back(new volSymmTensorField(*field));
    }
}


// NOTE: Add here methods to add other object types to checkpoint, if needed.

void preciceAdapter::Adapter::ReadCheckpoint()
{
    SETUP_TIMER();

    // TODO: To increase efficiency: only the oldTime() fields of the quantities which are used in the time
    //  derivative are necessary. (In general this is only the velocity). Also old information of the mesh
    //  is required.
    //  Therefore, loading the oldTime() and oldTime().oldTime() fields for the other fields can be excluded
    //  for efficiency.
    DEBUG(adapterInfo("Reading a checkpoint..."));

    // Reload the runTime
    ReloadCheckpointTime();

    // Reload the meshPoints (if FSI is enabled)
    if (mFSIEnabled)
    {
        ReloadMeshPoints();
    }

    // Reload all the fields of type volScalarField
    for (uint i = 0; i < mVolScalarFields.size(); i++)
    {
        // Load the volume field
        *(mVolScalarFields.at(i)) == *(mVolScalarFieldCopies.at(i));
        // TODO: Do we need this?
        // *(mVolScalarFields.at(i))->boundaryField() = *(mVolScalarFieldCopies.at(i))->boundaryField();

        int nOldTimes(mVolScalarFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mVolScalarFields.at(i)->oldTime() == mVolScalarFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mVolScalarFields.at(i)->oldTime().oldTime() == mVolScalarFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // Reload all the fields of type volVectorField
    for (uint i = 0; i < mVolVectorFields.size(); i++)
    {
        // Load the volume field
        *(mVolVectorFields.at(i)) == *(mVolVectorFieldCopies.at(i));

        int nOldTimes(mVolVectorFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mVolVectorFields.at(i)->oldTime() == mVolVectorFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mVolVectorFields.at(i)->oldTime().oldTime() == mVolVectorFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // Reload all the fields of type surfaceScalarField
    for (uint i = 0; i < mSurfaceScalarFields.size(); i++)
    {
        *(mSurfaceScalarFields.at(i)) == *(mSurfaceScalarFieldCopies.at(i));

        int nOldTimes(mSurfaceScalarFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mSurfaceScalarFields.at(i)->oldTime() == mSurfaceScalarFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mSurfaceScalarFields.at(i)->oldTime().oldTime() == mSurfaceScalarFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // Reload all the fields of type surfaceVectorField
    for (uint i = 0; i < mSurfaceVectorFields.size(); i++)
    {
        *(mSurfaceVectorFields.at(i)) == *(mSurfaceVectorFieldCopies.at(i));

        int nOldTimes(mSurfaceVectorFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mSurfaceVectorFields.at(i)->oldTime() == mSurfaceVectorFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mSurfaceVectorFields.at(i)->oldTime().oldTime() == mSurfaceVectorFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // Reload all the fields of type pointScalarField
    for (uint i = 0; i < mPointScalarFields.size(); i++)
    {
        *(mPointScalarFields.at(i)) == *(mPointScalarFieldCopies.at(i));

        int nOldTimes(mPointScalarFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mPointScalarFields.at(i)->oldTime() == mPointScalarFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mPointScalarFields.at(i)->oldTime().oldTime() == mPointScalarFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // Reload all the fields of type pointVectorField
    for (uint i = 0; i < mPointVectorFields.size(); i++)
    {
        // Load the volume field
        *(mPointVectorFields.at(i)) == *(mPointVectorFieldCopies.at(i));

        int nOldTimes(mPointVectorFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mPointVectorFields.at(i)->oldTime() == mPointVectorFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mPointVectorFields.at(i)->oldTime().oldTime() == mPointVectorFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // TODO Evaluate if all the tensor fields need to be in here.
    // Reload all the fields of type volTensorField
    for (uint i = 0; i < mVolTensorFields.size(); i++)
    {
        *(mVolTensorFields.at(i)) == *(mVolTensorFieldCopies.at(i));

        int nOldTimes(mVolTensorFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mVolTensorFields.at(i)->oldTime() == mVolTensorFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mVolTensorFields.at(i)->oldTime().oldTime() == mVolTensorFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // Reload all the fields of type surfaceTensorField
    for (uint i = 0; i < mSurfaceTensorFields.size(); i++)
    {
        *(mSurfaceTensorFields.at(i)) == *(mSurfaceTensorFieldCopies.at(i));

        int nOldTimes(mSurfaceTensorFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mSurfaceTensorFields.at(i)->oldTime() == mSurfaceTensorFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mSurfaceTensorFields.at(i)->oldTime().oldTime() == mSurfaceTensorFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // Reload all the fields of type pointTensorField
    for (uint i = 0; i < mPointTensorFields.size(); i++)
    {
        *(mPointTensorFields.at(i)) == *(mPointTensorFieldCopies.at(i));

        int nOldTimes(mPointTensorFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mPointTensorFields.at(i)->oldTime() == mPointTensorFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mPointTensorFields.at(i)->oldTime().oldTime() == mPointTensorFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // TODO volSymmTensorField is new.
    // Reload all the fields of type volSymmTensorField
    for (uint i = 0; i < mVolSymmTensorFields.size(); i++)
    {
        *(mVolSymmTensorFields.at(i)) == *(mVolSymmTensorFieldCopies.at(i));

        int nOldTimes(mVolSymmTensorFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mVolSymmTensorFields.at(i)->oldTime() == mVolSymmTensorFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mVolSymmTensorFields.at(i)->oldTime().oldTime() == mVolSymmTensorFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    // NOTE: Add here other field types to read, if needed.

    DEBUG(adapterInfo("Checkpoint was read. Time = " + std::to_string(mRunTime.value())));

    ACCUMULATE_TIMER(time_in_checkpointing_read);

    return;
}


void preciceAdapter::Adapter::WriteCheckpoint()
{
    SETUP_TIMER();

    DEBUG(adapterInfo("Writing a checkpoint..."));

    // Store the runTime
    StoreCheckpointTime();

    // Store the meshPoints (if FSI is enabled)
    if (mFSIEnabled)
    {
        StoreMeshPoints();
    }

    // Store all the fields of type volScalarField
    for (uint i = 0; i < mVolScalarFields.size(); i++)
    {
        *(mVolScalarFieldCopies.at(i)) == *(mVolScalarFields.at(i));
    }

    // Store all the fields of type volVectorField
    for (uint i = 0; i < mVolVectorFields.size(); i++)
    {
        *(mVolVectorFieldCopies.at(i)) == *(mVolVectorFields.at(i));
    }

    // Store all the fields of type volTensorField
    for (uint i = 0; i < mVolTensorFields.size(); i++)
    {
        *(mVolTensorFieldCopies.at(i)) == *(mVolTensorFields.at(i));
    }

    // Store all the fields of type volSymmTensorField
    for (uint i = 0; i < mVolSymmTensorFields.size(); i++)
    {
        *(mVolSymmTensorFieldCopies.at(i)) == *(mVolSymmTensorFields.at(i));
    }

    // Store all the fields of type surfaceScalarField
    for (uint i = 0; i < mSurfaceScalarFields.size(); i++)
    {
        *(mSurfaceScalarFieldCopies.at(i)) == *(mSurfaceScalarFields.at(i));
    }

    // Store all the fields of type surfaceVectorField
    for (uint i = 0; i < mSurfaceVectorFields.size(); i++)
    {
        *(mSurfaceVectorFieldCopies.at(i)) == *(mSurfaceVectorFields.at(i));
    }

    // Store all the fields of type surfaceTensorField
    for (uint i = 0; i < mSurfaceTensorFields.size(); i++)
    {
        *(mSurfaceTensorFieldCopies.at(i)) == *(mSurfaceTensorFields.at(i));
    }

    // Store all the fields of type pointScalarField
    for (uint i = 0; i < mPointScalarFields.size(); i++)
    {
        *(mPointScalarFieldCopies.at(i)) == *(mPointScalarFields.at(i));
    }

    // Store all the fields of type pointVectorField
    for (uint i = 0; i < mPointVectorFields.size(); i++)
    {
        *(mPointVectorFieldCopies.at(i)) == *(mPointVectorFields.at(i));
    }

    // Store all the fields of type pointTensorField
    for (uint i = 0; i < mPointTensorFields.size(); i++)
    {
        *(mPointTensorFieldCopies.at(i)) == *(mPointTensorFields.at(i));
    }
    // NOTE: Add here other types to write, if needed.

    DEBUG(adapterInfo("Checkpoint for time t = " + std::to_string(mRunTime.value()) + " was stored."));

    ACCUMULATE_TIMER(time_in_checkpointing_write);

    return;
}

void preciceAdapter::Adapter::ReadMeshCheckpoint()
{
    DEBUG(adapterInfo("Reading a mesh checkpoint..."));

    // Only the meshPhi field is here, which is a surfaceScalarField.
    for (uint i = 0; i < mMeshSurfaceScalarFields.size(); i++)
    {
        *(mMeshSurfaceScalarFields.at(i)) == *(mMeshSurfaceScalarFieldCopies.at(i));

        int nOldTimes(mMeshSurfaceScalarFields.at(i)->nOldTimes());
        if (nOldTimes >= 1)
        {
            mMeshSurfaceScalarFields.at(i)->oldTime() == mMeshSurfaceScalarFieldCopies.at(i)->oldTime();
        }
        if (nOldTimes == 2)
        {
            mMeshSurfaceScalarFields.at(i)->oldTime().oldTime() == mMeshSurfaceScalarFieldCopies.at(i)->oldTime().oldTime();
        }
    }

    DEBUG(adapterInfo("Mesh checkpoint was read. Time = " + std::to_string(mRunTime.value())));

    return;
}

void preciceAdapter::Adapter::WriteMeshCheckpoint()
{
    DEBUG(adapterInfo("Writing a mesh checkpoint..."));

    // Store all the fields of type mesh surfaceScalar (phi)
    for (uint i = 0; i < mMeshSurfaceScalarFields.size(); i++)
    {
        *(mMeshSurfaceScalarFieldCopies.at(i)) == *(mMeshSurfaceScalarFields.at(i));
    }

    DEBUG(adapterInfo("Storing mesh points..."));

    // Store mesh points
    // swap pointers
    *(mMeshOldPoints) = *(mMeshPoints);
    *(mMeshPoints) = mMesh.points();

    DEBUG(adapterInfo("Mesh checkpoint for time t = " + std::to_string(mRunTime.value()) + " was stored."));

    return;
}

void preciceAdapter::Adapter::end()
try
{
    // Throw a warning if the simulation exited before the coupling was complete
    if (nullptr != mPrecice && IsCouplingOngoing())
    {
        adapterInfo("The solver exited before the coupling was complete.", "warning");
    }

    return;
}
catch (const CoSimIOError& e)
{
    std::exit(EXIT_FAILURE);
}

void preciceAdapter::Adapter::Teardown()
{
    // If the solver interface was not deleted before, delete it now.
    // Normally it should be deleted when IsCouplingOngoing() becomes false.
    if (nullptr != mPrecice)
    {
        DEBUG(adapterInfo("Destroying the preCICE solver interface..."));
        delete mPrecice;
        mPrecice = nullptr;
    }

    // Delete the preCICE solver interfaces
    if (mInterfaces.size() > 0)
    {
        DEBUG(adapterInfo("Deleting the interfaces..."));
        for (uint i = 0; i < mInterfaces.size(); i++)
        {
            delete mInterfaces.at(i);
        }
        mInterfaces.clear();
    }

    // Delete the copied fields for checkpointing
    if (mCheckpointing)
    {
        DEBUG(adapterInfo("Deleting the checkpoints... "));

        // Fields
        // volScalarFields
        for (uint i = 0; i < mVolScalarFieldCopies.size(); i++)
        {
            delete mVolScalarFieldCopies.at(i);
        }
        mVolScalarFieldCopies.clear();
        // volVector
        for (uint i = 0; i < mVolVectorFieldCopies.size(); i++)
        {
            delete mVolVectorFieldCopies.at(i);
        }
        mVolVectorFieldCopies.clear();
        // surfaceScalar
        for (uint i = 0; i < mSurfaceScalarFieldCopies.size(); i++)
        {
            delete mSurfaceScalarFieldCopies.at(i);
        }
        mSurfaceScalarFieldCopies.clear();
        // surfaceVector
        for (uint i = 0; i < mSurfaceVectorFieldCopies.size(); i++)
        {
            delete mSurfaceVectorFieldCopies.at(i);
        }
        mSurfaceVectorFieldCopies.clear();
        // pointScalar
        for (uint i = 0; i < mPointScalarFieldCopies.size(); i++)
        {
            delete mPointScalarFieldCopies.at(i);
        }
        mPointScalarFieldCopies.clear();
        // pointVector
        for (uint i = 0; i < mPointVectorFieldCopies.size(); i++)
        {
            delete mPointVectorFieldCopies.at(i);
        }
        mPointVectorFieldCopies.clear();

        // Mesh fields
        // meshSurfaceScalar
        for (uint i = 0; i < mMeshSurfaceScalarFieldCopies.size(); i++)
        {
            delete mMeshSurfaceScalarFieldCopies.at(i);
        }
        mMeshSurfaceScalarFieldCopies.clear();

        // volTensorField
        for (uint i = 0; i < mVolTensorFieldCopies.size(); i++)
        {
            delete mVolTensorFieldCopies.at(i);
        }
        mVolTensorFieldCopies.clear();

        // surfaceTensorField
        for (uint i = 0; i < mSurfaceTensorFieldCopies.size(); i++)
        {
            delete mSurfaceTensorFieldCopies.at(i);
        }
        mSurfaceTensorFieldCopies.clear();

        // pointTensorField
        for (uint i = 0; i < mPointTensorFieldCopies.size(); i++)
        {
            delete mPointTensorFieldCopies.at(i);
        }
        mPointTensorFieldCopies.clear();

        // volSymmTensor
        for (uint i = 0; i < mVolSymmTensorFieldCopies.size(); i++)
        {
            delete mVolSymmTensorFieldCopies.at(i);
        }
        mVolSymmTensorFieldCopies.clear();

        // NOTE: Add here delete for other types, if needed

        mCheckpointing = false;

        delete mMeshPoints;
        delete mMeshOldPoints;
    }

    // Delete the CHT module
    if (nullptr != mCHT)
    {
        DEBUG(adapterInfo("Destroying the CHT module..."));
        delete mCHT;
        mCHT = nullptr;
    }

    // Delete the FSI module
    if (nullptr != mFSI)
    {
        DEBUG(adapterInfo("Destroying the FSI module..."));
        delete mFSI;
        mFSI = nullptr;
    }

    // Delete the FF module
    if (nullptr != mFF)
    {
        DEBUG(adapterInfo("Destroying the FF module..."));
        delete mFF;
        mFF = nullptr;
    }

    // Delete the Generic module
    if (nullptr != mGeneric)
    {
        DEBUG(adapterInfo("Destroying the Generic module..."));
        delete mGeneric;
        mGeneric = nullptr;
    }

    // NOTE: Delete your new module here

    return;
}

preciceAdapter::Adapter::~Adapter()
try
{
    Teardown();

    TIMING_MODE(
        // Continuing the output started in the destructor of preciceAdapterFunctionObject
        Info << "Time exclusively in the adapter: " << (time_in_config_read_ + time_in_mesh_setup + time_in_checkpointing_setup + time_in_write + time_in_read + time_in_checkpointing_write + time_in_checkpointing_read).str() << nl;
        Info << "  (S) reading CoSimIODict:       " << time_in_config_read_.str() << nl;
        Info << "  (S) constructing preCICE:      " << time_in_co_sim_io_construct.str() << nl;
        Info << "  (S) setting up the interfaces: " << time_in_mesh_setup.str() << nl;
        Info << "  (S) setting up checkpointing:  " << time_in_checkpointing_setup.str() << nl;
        Info << "  (I) writing data:              " << time_in_write.str() << nl;
        Info << "  (I) reading data:              " << time_in_read.str() << nl;
        Info << "  (I) writing checkpoints:       " << time_in_checkpointing_write.str() << nl;
        Info << "  (I) reading checkpoints:       " << time_in_checkpointing_read.str() << nl;
        Info << "  (I) writing OpenFOAM results:  " << time_in_write_results.str() << " (at the end of converged time windows)" << nl << nl;
        Info << "Time exclusively in preCICE:     " << (time_in_initialize + time_in_advance + time_in_finalize).str() << nl;
        Info << "  (S) initialize():              " << time_in_initialize.str() << nl;
        Info << "  (I) advance():                 " << time_in_advance.str() << nl;
        Info << "  (I) finalize():                " << time_in_finalize.str() << nl;
        Info << "  These times include time waiting for other participants." << nl;
        Info << "  See also precice-profiling on the website https://precice.org/tooling-performance-analysis.html." << nl;
        Info << "-------------------------------------------------------------------------------------" << nl;)

    return;
}
catch (const CoSimIOError& e)
{
    std::exit(EXIT_FAILURE);
}
