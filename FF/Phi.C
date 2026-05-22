#include "Phi.H"

using namespace Foam;

preciceAdapter::FF::Phi::Phi(
    const Foam::fvMesh& mesh,
    const std::string namePhi)
: phi_(
    const_cast<surfaceScalarField*>(
        &mesh.lookupObject<surfaceScalarField>(namePhi)))
{
    mDataType = scalar;
}

std::size_t preciceAdapter::FF::Phi::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // For every cell of the patch
        forAll(phi_->boundaryFieldRef()[patchID], i)
        {
            // Copy the Phi into the buffer
            buffer[bufferIndex++] =
                phi_->boundaryFieldRef()[patchID][i];
        }
    }
    return bufferIndex;
}

void preciceAdapter::FF::Phi::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        forAll(phi_->boundaryFieldRef()[patchID], i)
        {
            phi_->boundaryFieldRef()[patchID][i] = -buffer[bufferIndex++];
        }
    }
}

bool preciceAdapter::FF::Phi::IsLocationTypeSupported(const bool meshConnectivity) const
{
    return (this->mLocationType == LocationType::FaceCenters);
}

std::string preciceAdapter::FF::Phi::GetDataName() const
{
    return "Phi";
}
