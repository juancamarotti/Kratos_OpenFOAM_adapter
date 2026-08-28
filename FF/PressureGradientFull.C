#include "PressureGradientFull.H"

using namespace Foam;

CoSimIOAdapter::FF::PressureGradientFull::PressureGradientFull(
    const Foam::fvMesh& mesh,
    const std::string nameP)
: p_(
    const_cast<volScalarField*>(
        &mesh.lookupObject<volScalarField>(nameP))),
  gradP_(IOobject(
             "gradP",
             mesh.time().timeName(),
             mesh,
             IOobject::NO_READ,
             IOobject::NO_WRITE),
         fvc::grad(*p_))
{
    mDataType = vector;
}

std::size_t CoSimIOAdapter::FF::PressureGradientFull::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;
    gradP_ = fvc::grad(*p_);

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (const auto& cell : gradP_.internalField())
            {
                // x-dimension
                buffer[bufferIndex++] = cell.x();

                // y-dimension
                buffer[bufferIndex++] = cell.y();

                if (dim == 3)
                {
                    // z-dimension
                    buffer[bufferIndex++] = cell.z();
                }
            }
        }
        else
        {
            for (const auto& cellSetName : mCellSetNames)
            {
                cellSet overlapRegion(p_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    // x-dimension
                    buffer[bufferIndex++] = gradP_.internalField()[currentCell].x();

                    // y-dimension
                    buffer[bufferIndex++] = gradP_.internalField()[currentCell].y();

                    if (dim == 3)
                    {
                        // z-dimension
                        buffer[bufferIndex++] = gradP_.internalField()[currentCell].z();
                    }
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // For every cell of the patch
        forAll(gradP_.boundaryFieldRef()[patchID], i)
        {
            // Copy the velocity into the buffer
            // x-dimension
            buffer[bufferIndex++] =
                gradP_.boundaryFieldRef()[patchID][i].x();

            // y-dimension
            buffer[bufferIndex++] =
                gradP_.boundaryFieldRef()[patchID][i].y();

            if (dim == 3)
            {
                // z-dimension
                buffer[bufferIndex++] =
                    gradP_.boundaryFieldRef()[patchID][i].z();
            }
        }
    }
    return bufferIndex;
}

void CoSimIOAdapter::FF::PressureGradientFull::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // Get the pressure gradient boundary patch
        scalarField& gradientPatch =
            refCast<fixedGradientFvPatchScalarField>(
                p_->boundaryFieldRef()[patchID])
                .gradient();

        // For every cell of the patch
        forAll(gradientPatch, i)
        {
            // Set the pressure gradient as the buffer value
            gradientPatch[i] =
                buffer[bufferIndex++];
        }
    }
}

bool CoSimIOAdapter::FF::PressureGradientFull::IsLocationTypeSupported(const bool meshConnectivity) const
{
    return (this->mLocationType == LocationType::FaceCenters || this->mLocationType == LocationType::VolumeCenters);
}

std::string CoSimIOAdapter::FF::PressureGradientFull::GetDataName() const
{
    return "PressureGradientFull";
}
