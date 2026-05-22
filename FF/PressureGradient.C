#include "PressureGradient.H"

using namespace Foam;

preciceAdapter::FF::PressureGradient::PressureGradient(
    const Foam::fvMesh& mesh,
    const std::string nameP)
: p_(
    const_cast<volScalarField*>(
        &mesh.lookupObject<volScalarField>(nameP)))
{
    mDataType = scalar;
}

std::size_t preciceAdapter::FF::PressureGradient::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // Get the pressure gradient boundary patch
        const scalarField gradientPatch((p_->boundaryFieldRef()[patchID])
                                            .snGrad());

        // For every cell of the patch
        forAll(gradientPatch, i)
        {
            // Copy the pressure gradient into the buffer
            buffer[bufferIndex++] =
                -gradientPatch[i];
        }
    }
    return bufferIndex;
}

void preciceAdapter::FF::PressureGradient::Read(double* buffer, const unsigned int dim)
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

bool preciceAdapter::FF::PressureGradient::IsLocationTypeSupported(const bool meshConnectivity) const
{
    return (this->mLocationType == LocationType::FaceCenters);
}

std::string preciceAdapter::FF::PressureGradient::GetDataName() const
{
    return "PressureGradient";
}
