#ifndef INTERFACE_H
#define INTERFACE_H

#include <string>
#include <vector>
#include "fvCFD.H"
#include "CouplingDataUser.H"

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
    //- Array of CoSimIO::Model parts(to store mesh info) for all coupling interfaces
    std::unique_ptr<CoSimIO::ModelPart> mpModelPart;

    //- CoSimIO info object
    CoSimIO::Info mInfo;

    //- Member variables required for CoSimIO
    std::string mConnectionName;

    //- Mesh name used in the CoSimIO configuration
    std::string mMeshName;

    //- Point locations type (FaceCenters vs FaceNodes)
    LocationType mLocationType {LocationType::none};

    // //- Mesh ID assigned by CoSimIO to the interface
    // int mMeshID;

    //- Names of the OpenFOAM patches that form the interface
    std::vector<std::string> mPatchNames;

    //- OpenFOAM patches that form the interface
    std::vector<int> mPatchIDs;

    //- Names of the OpenFOAM cell sets to be coupled (for volume coupling)
    std::vector<std::string> mCellSetNames;

    //- Number of data points (cell centers) on the interface
    int mNumDataLocations = 0;

    //- Vertex IDs
    std::vector<int> mVertexIDs;

    //- Buffer for the coupling data
    std::vector<double> mDataBuffer;

    //- Vector of CouplingDataReaders
    std::vector<CouplingDataUser*> mCouplingDataReaders;

    //- Vector of CouplingDataWriters
    std::vector<CouplingDataUser*> mCouplingDataWriters;

    //Switch for faceTriangulation (nearest projection)
    bool mMeshConnectivity;

    //- Reset the displacement during interface definition
    bool mRestartFromDeformed;

    // Simulation dimension
    unsigned int mDim;

    //- Extracts the locations of the face centers or face nodes
    //  and exports them to CoSimIO 
    void ConfigureMesh(const Foam::fvMesh& mesh,
                       const std::string& namePointDisplacement,
                       const std::string& nameCellDisplacement);

public:
    //- Constructor
    Interface(
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
    void AddCouplingDataReader(
        const FieldConfig& fieldConfig,
        CouplingDataUser* couplingDataReader);


    //- Add a CouplingDataUser to write data on the interface
    void AddCouplingDataWriter(
        const FieldConfig& fieldConfig,
        CouplingDataUser* couplingDataWriter);

    //- Allocate an appropriate buffer for scalar or vector data.
    //  If at least one couplingDataUser has vector data, then
    //  define a buffer for 3D data. Otherwise, for 1D data.
    void CreateBuffer();

    //- Call read() on each registered couplingDataReader to read the coupling
    //  data from the buffer and apply the boundary conditions
    void ReadCouplingData(double relativeReadTime);

    //- Call write() on each registered couplingDataWriter to extract the boundary
    //  data and write them into the buffer
    void WriteCouplingData();

    //- Destructor
    ~Interface();
};

}

#endif
