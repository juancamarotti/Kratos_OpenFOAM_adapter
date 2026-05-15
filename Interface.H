#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <vector>
#include "fvCFD.H"
#include "CouplingDataUser.H"
#include <precice/precice.hpp>

#include "pointPatchField.H"

// CoSimIO header file
#include "co_sim_io.hpp"
#include "co_sim_io_mpi.hpp"

namespace preciceAdapter
{

//- field options
struct FieldConfig
{
    std::string name;
    std::string solver_name;
    std::string operation;
    bool flip_normal;
};

class Interface
{
protected:
    //- preCICE solver interface
    precice::Participant& mPrecice;

    //- Array of CoSimIO::Model parts(to store mesh info) for all coupling interfaces
    std::unique_ptr<CoSimIO::ModelPart> mpModelPart;

    //- CoSimIO info object
    CoSimIO::Info mInfo;

    //- Member variables required for CoSimIO
    std::string mConnectionName;

    //- Mesh name used in the preCICE configuration
    std::string mMeshName;

    //- Point locations type (faceCenters vs faceNodes)
    LocationType mLocationType {LocationType::none};

    //- Mesh ID assigned by preCICE to the interface
    int mMeshID;

    //- Names of the OpenFOAM patches that form the interface
    std::vector<std::string> patchNames_;

    //- OpenFOAM patches that form the interface
    std::vector<int> patchIDs_;

    //- Names of the OpenFOAM cell sets to be coupled (for volume coupling)
    std::vector<std::string> cellSetNames_;

    //- Number of data points (cell centers) on the interface
    int numDataLocations_ = 0;

    //- Vertex IDs assigned by preCICE
    std::vector<int> vertexIDs_;

    //- Buffer for the coupling data
    std::vector<double> dataBuffer_;

    //- Vector of CouplingDataReaders
    std::vector<CouplingDataUser*> couplingDataReaders_;

    //- Vector of CouplingDataWriters
    std::vector<CouplingDataUser*> couplingDataWriters_;

    //Switch for faceTriangulation (nearest projection)
    bool meshConnectivity_;

    //- Reset the displacement during interface definition
    bool restartFromDeformed_;

    // Simulation dimension
    unsigned int dim_;

    //- Extracts the locations of the face centers or face nodes
    //  and exposes them to preCICE with setMeshVertices
    void configureMesh(const Foam::fvMesh& mesh,
                       const std::string& namePointDisplacement,
                       const std::string& nameCellDisplacement);

public:
    //- Constructor
    Interface(
        precice::Participant& precice,
        const Foam::fvMesh& mesh,
        std::string meshName,
        std::string locationsType,
        std::vector<std::string> patchNames,
        std::vector<std::string> cellSetNames,
        bool meshConnectivity,
        bool restartFromDeformed,
        const std::string& namePointDisplacement,
        const std::string& nameCellDisplacement,
        std::string ConnectionName);

    //- Add a CouplingDataUser to read data from the interface
    void addCouplingDataReader(
        const FieldConfig& fieldConfig,
        CouplingDataUser* couplingDataReader);


    //- Add a CouplingDataUser to write data on the interface
    void addCouplingDataWriter(
        const FieldConfig& fieldConfig,
        CouplingDataUser* couplingDataWriter);

    //- Allocate an appropriate buffer for scalar or vector data.
    //  If at least one couplingDataUser has vector data, then
    //  define a buffer for 3D data. Otherwise, for 1D data.
    void createBuffer();

    //- Call read() on each registered couplingDataReader to read the coupling
    //  data from the buffer and apply the boundary conditions
    void readCouplingData(double relativeReadTime);

    //- Call write() on each registered couplingDataWriter to extract the boundary
    //  data and write them into the buffer
    void writeCouplingData();

    //- Destructor
    ~Interface();
};

}

#endif
