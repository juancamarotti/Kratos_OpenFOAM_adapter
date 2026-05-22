#include "Temperature.H"
#include "primitivePatchInterpolation.H"


using namespace Foam;

preciceAdapter::CHT::Temperature::Temperature(
    const Foam::fvMesh& mesh,
    const std::string nameT)
: T_(
    const_cast<volScalarField*>(
        &mesh.lookupObject<volScalarField>(nameT))),
  mesh_(mesh)
{
    mDataType = scalar;
}

std::size_t preciceAdapter::CHT::Temperature::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (const auto& cell : T_->internalField())
            {
                buffer[bufferIndex++] = cell;
            }
        }
        else
        {
            for (const auto& cellSetName : mCellSetNames)
            {
                cellSet overlapRegion(T_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    // Copy temperature into the buffer
                    buffer[bufferIndex++] = T_->internalField()[currentCell];
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        const scalarField& TPatch(
            T_->boundaryField()[patchID]);

        //If we use the mesh connectivity, we interpolate from the centres to the nodes
        if (meshConnectivity)
        {
            //Create an Interpolation object at the boundary Field
            primitivePatchInterpolation patchInterpolator(mesh_.boundaryMesh()[patchID]);

            //Interpolate from centers to nodes
            scalarField TPoints(
                patchInterpolator.faceToPointInterpolate(TPatch));

            forAll(TPoints, i)
            {
                // Copy the temperature into the buffer
                buffer[bufferIndex++] =
                    TPoints[i];
            }
        }
        else
        {
            forAll(TPatch, i)
            {
                // Copy the temperature into the buffer
                buffer[bufferIndex++] =
                    TPatch[i];
            }
        }
    }
    return bufferIndex;
}

void preciceAdapter::CHT::Temperature::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (auto& cell : T_->ref())
            {
                cell = buffer[bufferIndex++];
            }
        }
        else
        {
            for (const auto& cellSetName : mCellSetNames)
            {
                cellSet overlapRegion(T_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    // Copy temperature into the buffer
                    T_->ref()[currentCell] = buffer[bufferIndex++];
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // For every cell of the patch
        forAll(T_->boundaryField()[patchID], i)
        {
            // Set the temperature as the buffer value
            T_->boundaryFieldRef()[patchID][i] =
                buffer[bufferIndex++];
        }
    }
}

bool preciceAdapter::CHT::Temperature::IsLocationTypeSupported(const bool meshConnectivity) const
{
    if (meshConnectivity)
    {
        return (this->mLocationType == LocationType::FaceNodes);
    }
    else
    {
        return (this->mLocationType == LocationType::FaceCenters || this->mLocationType == LocationType::VolumeCenters);
    }
}

std::string preciceAdapter::CHT::Temperature::GetDataName() const
{
    return "Temperature";
}
