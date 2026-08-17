#include "Alpha.H"

using namespace Foam;

CoSimIOAdapter::FF::Alpha::Alpha(
    const Foam::fvMesh& mesh,
    const std::string nameAlpha)
: Alpha_(
    const_cast<volScalarField*>(
        &mesh.lookupObject<volScalarField>(nameAlpha)))
{
    mDataType = scalar;
}

std::size_t CoSimIOAdapter::FF::Alpha::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (const auto& cell : Alpha_->internalField())
            {
                buffer[bufferIndex++] = cell;
            }
        }
        else
        {
            for (const auto& cellSetName : mCellSetNames)
            {
                cellSet overlapRegion(Alpha_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    // Copy the alpha valus into the buffer
                    buffer[bufferIndex++] = Alpha_->internalField()[currentCell];
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // For every cell of the patch
        forAll(Alpha_->boundaryFieldRef()[patchID], i)
        {
            // Copy the Alpha into the buffer
            buffer[bufferIndex++] =
                Alpha_->boundaryFieldRef()[patchID][i];
        }
    }
    return bufferIndex;
}

void CoSimIOAdapter::FF::Alpha::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (auto& cell : Alpha_->ref())
            {
                cell = buffer[bufferIndex++];
            }
        }
        else
        {
            for (const auto& cellSetName : mCellSetNames)
            {
                cellSet overlapRegion(Alpha_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    // Copy the pressure into the buffer
                    Alpha_->ref()[currentCell] = buffer[bufferIndex++];
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);
        // For every cell of the patch
        forAll(Alpha_->boundaryFieldRef()[patchID], i)
        {
            Alpha_->boundaryFieldRef()[patchID][i] = buffer[bufferIndex++];
        }
    }
}

bool CoSimIOAdapter::FF::Alpha::IsLocationTypeSupported(const bool meshConnectivity) const
{
    if (meshConnectivity)
    {
        return false;
    }
    else
    {
        return (this->mLocationType == LocationType::FaceCenters || this->mLocationType == LocationType::VolumeCenters);
    }
}

std::string CoSimIOAdapter::FF::Alpha::GetDataName() const
{
    return "Alpha";
}
