#include "CouplingDataUser.H"

CoSimIOAdapter::CouplingDataUser::CouplingDataUser()
{
}

bool CoSimIOAdapter::CouplingDataUser::HasScalarData()
{
    return mDataType == scalar;
}

bool CoSimIOAdapter::CouplingDataUser::HasVectorData()
{
    return mDataType == vector;
}

void CoSimIOAdapter::CouplingDataUser::SetDataName(std::string dataName)
{
    mDataName = std::move(dataName);
}

const std::string& CoSimIOAdapter::CouplingDataUser::DataName()
{
    return mDataName;
}

void CoSimIOAdapter::CouplingDataUser::SetFlipNormal(bool flipNormal)
{
    mFlipNormal = flipNormal;
}

void CoSimIOAdapter::CouplingDataUser::ApplyFlipNormal(
    double* pData,
    std::size_t Size)
{
    if (mFlipNormal)
    {
        for (std::size_t i = 0; i < Size; ++i)
        {
            pData[i] *= -1.0;
        }
    }
}

void CoSimIOAdapter::CouplingDataUser::SetPatchIDs(std::vector<int> patchIDs)
{
    mPatchIDs = patchIDs;
}

void CoSimIOAdapter::CouplingDataUser::SetCellSetNames(std::vector<std::string> cellSetNames)
{
    mCellSetNames = cellSetNames;
}

void CoSimIOAdapter::CouplingDataUser::SetLocationsType(LocationType locationsType)
{
    mLocationType = locationsType;
}

void CoSimIOAdapter::CouplingDataUser::CheckDataLocation(const bool meshConnectivity) const
{
    if (this->IsLocationTypeSupported(meshConnectivity) == false)
    {
        std::string location("none");
        if (mLocationType == LocationType::FaceCenters)
            location = "FaceCenters";
        else if (mLocationType == LocationType::FaceNodes)
            location = "FaceNodes";
        else if (mLocationType == LocationType::VolumeCenters)
            location = "VolumeCenters";

        if (meshConnectivity)
        {
            adapterInfo("The data \"" + GetDataName() + "\""
                            + " does not currently support mesh connectivity (e.g., nearest-projection mapping) for location type "
                            + "\"" + location + "\".",
                        "error");
        }
        else
        {
            adapterInfo("The data \"" + GetDataName() + "\" does not support location type \""
                            + location + "\". Please select a different location type.",
                        "error");
        }
    }
}

// Dummy implementation which can be overwritten in derived classes if required
void CoSimIOAdapter::CouplingDataUser::Initialize()
{
    return;
}
