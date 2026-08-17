#include "ImplicitMomentum.H"
#include "coupledVelocityFvPatchField.H"

using namespace Foam;

CoSimIOAdapter::FF::ImplicitMomentum::ImplicitMomentum(
    const Foam::fvMesh& mesh,
    const std::string nameImplicitMomentum)
{
    if (mesh.foundObject<volScalarField>(nameImplicitMomentum))
    {
        adapterInfo("Loaded existing implicit momentum object " + nameImplicitMomentum, "debug");
        ImplicitMomentum_ = const_cast<volScalarField*>(
            &mesh.lookupObject<volScalarField>(nameImplicitMomentum));
    }
    else
    {
        adapterInfo("Creating a new implicit momentum object " + nameImplicitMomentum, "debug");
        ImplicitMomentum_ = new volScalarField(
            IOobject(
                nameImplicitMomentum,
                mesh.time().timeName(),
                mesh,
                IOobject::MUST_READ,
                IOobject::AUTO_WRITE),
            mesh);
    }
    mDataType = scalar;
}

std::size_t CoSimIOAdapter::FF::ImplicitMomentum::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (const auto& cell : ImplicitMomentum_->internalField())
            {
                buffer[bufferIndex++] = cell;
            }
        }
        else
        {
            for (const auto& cellSetName : mCellSetNames)
            {
                cellSet overlapRegion(ImplicitMomentum_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    buffer[bufferIndex++] = ImplicitMomentum_->internalField()[currentCell];
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        scalarField ImplicitMomentumPatch = ImplicitMomentum_->boundaryField()[patchID];

        // For every cell of the patch
        forAll(ImplicitMomentum_->boundaryFieldRef()[patchID], i)
        {
            // Copy the velocity into the buffer
            buffer[bufferIndex++] =
                ImplicitMomentumPatch[i];
        }
    }
    return bufferIndex;
}

void CoSimIOAdapter::FF::ImplicitMomentum::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (auto& cell : ImplicitMomentum_->ref())
            {
                cell = buffer[bufferIndex++];
            }
        }
        else
        {
            for (const auto& cellSetName : mCellSetNames)
            {
                cellSet overlapRegion(ImplicitMomentum_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    ImplicitMomentum_->ref()[currentCell] = buffer[bufferIndex++];
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // Get the velocity value boundary patch
        scalarField* valuePatchPtr = &ImplicitMomentum_->boundaryFieldRef()[patchID];
        scalarField& valuePatch = *valuePatchPtr;

        // For every cell of the patch
        forAll(ImplicitMomentum_->boundaryFieldRef()[patchID], i)
        {
            // Set the velocity as the buffer value
            valuePatch[i] =
                buffer[bufferIndex++];
        }
    }
}

bool CoSimIOAdapter::FF::ImplicitMomentum::IsLocationTypeSupported(const bool meshConnectivity) const
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

std::string CoSimIOAdapter::FF::ImplicitMomentum::GetDataName() const
{
    return "ImplicitMomentum";
}
