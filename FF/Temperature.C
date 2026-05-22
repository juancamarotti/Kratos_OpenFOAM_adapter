#include "Temperature.H"

using namespace Foam;

preciceAdapter::FF::Temperature::Temperature(
    const Foam::fvMesh& mesh,
    const std::string nameT)
: T_(
    const_cast<volScalarField*>(
        &mesh.lookupObject<volScalarField>(nameT)))
{
    mDataType = scalar;
}

std::size_t preciceAdapter::FF::Temperature::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);
        scalarField gradientPatch((T_->boundaryFieldRef()[patchID])
                                      .snGrad());

        // For every cell of the patch
        forAll(T_->boundaryFieldRef()[patchID], i)
        {
            // Copy the pressure into the buffer
            buffer[bufferIndex++] =
                T_->boundaryFieldRef()[patchID][i];
        }
    }
    return bufferIndex;
}

void preciceAdapter::FF::Temperature::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);
        // For every cell of the patch
        forAll(T_->boundaryFieldRef()[patchID], i)
        {
            T_->boundaryFieldRef()[patchID][i] = buffer[bufferIndex++];
        }
    }
}

bool preciceAdapter::FF::Temperature::IsLocationTypeSupported(const bool meshConnectivity) const
{
    return (this->mLocationType == LocationType::FaceCenters);
}

std::string preciceAdapter::FF::Temperature::GetDataName() const
{
    return "Temperature";
}
