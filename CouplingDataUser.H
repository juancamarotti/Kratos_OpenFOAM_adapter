#ifndef COUPLINGDATAUSER_H
#define COUPLINGDATAUSER_H

#include "Utilities.H"
#include <vector>
#include <string>

namespace preciceAdapter
{
// A small enum to deal with the different locations of the
// coupling interface
enum class LocationType
{
    none,
    FaceCenters,
    FaceNodes,
    VolumeCenters
};

class CouplingDataUser
{

protected:
    enum DataType
    {
        scalar,
        vector
    };

    //- Type of the coupling data (scalar or vector)
    DataType mDataType = scalar;

    //- OpenFOAM patches that form the interface
    std::vector<int> mPatchIDs;

    //- Names of the OpenFOAM cell sets to be coupled (for volume coupling)
    std::vector<std::string> mCellSetNames;

    //- data name
    std::string mDataName;

    //- location type of the interface
    LocationType mLocationType = LocationType::none;

    //- flip normal option
    bool mFlipNormal = false;

public:
    //- Constructor
    CouplingDataUser();

    //- Returns true if the data are scalar
    bool HasScalarData();

    //- Returns true if the data are vector
    bool HasVectorData();

    //- Set the data name
    void SetDataName(std::string DataName);

    //- Get the data name
    const std::string& DataName();

    //- Set flip normal option
    void SetFlipNormal(bool flipNormal);

    //- Apply flip normal to the data buffer (multiply by -1)
    void ApplyFlipNormal(double* pData, std::size_t Size);

    //- Set the patch IDs that form the interface
    void SetPatchIDs(std::vector<int> patchIDs);

    //- Set the cellSetNames that form the overlapping cells of the interface
    void SetCellSetNames(std::vector<std::string> cellSetNames);

    //- Set the locations type of the interface
    void SetLocationsType(LocationType locationsType);

    // Check if the dataset supports this interface nodes location
    void CheckDataLocation(const bool meshConnectivity) const;

    //- option to Initialize data in derived data classes
    virtual void Initialize();

    //- Write the coupling data to the buffer
    // Returns the number of entries that were filles in the buffer (nComp * vertices)
    virtual std::size_t Write(double* dataBuffer, bool meshConnectivity, const unsigned int dim) = 0;

    //- Read the coupling data from the buffer
    virtual void Read(double* dataBuffer, const unsigned int dim) = 0;

    //- Given the meshConnectivity, return if the underlying loactionType of the
    //- interface nodes is supported by the data set
    virtual bool IsLocationTypeSupported(const bool meshConnectivity) const = 0;

    //- Get the name of the current data field
    virtual std::string GetDataName() const = 0;

    //- Destructor
    virtual ~CouplingDataUser() {}
};

}

#endif
