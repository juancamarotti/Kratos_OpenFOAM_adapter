#include "CouplingDataUser.H"

preciceAdapter::CouplingDataUser::CouplingDataUser()
{
}

bool preciceAdapter::CouplingDataUser::HasScalarData()
{
    return mDataType == scalar;
}

bool preciceAdapter::CouplingDataUser::HasVectorData()
{
    return mDataType == vector;
}

void preciceAdapter::CouplingDataUser::SetDataName(std::string dataName)
{
    mDataName = std::move(dataName);
}

const std::string& preciceAdapter::CouplingDataUser::DataName()
{
    return mDataName;
}

void preciceAdapter::CouplingDataUser::SetFlipNormal(bool flipNormal)
{
    mFlipNormal = flipNormal;
}

void preciceAdapter::CouplingDataUser::ApplyFlipNormal(precice::span<double> dataBuffer)
{
    if (mFlipNormal)
    {
        for (double& val : dataBuffer)
        {
            val *= -1.0;
        }
    }
}

void preciceAdapter::CouplingDataUser::SetPatchIDs(std::vector<int> patchIDs)
{
    mPatchIDs = patchIDs;
}

void preciceAdapter::CouplingDataUser::SetCellSetNames(std::vector<std::string> cellSetNames)
{
    mCellSetNames = cellSetNames;
}

void preciceAdapter::CouplingDataUser::SetLocationsType(LocationType locationsType)
{
    mLocationType = locationsType;
}

void preciceAdapter::CouplingDataUser::CheckDataLocation(const bool meshConnectivity) const
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
void preciceAdapter::CouplingDataUser::Initialize()
{
    return;
}
