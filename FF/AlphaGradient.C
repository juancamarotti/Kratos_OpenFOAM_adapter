#include "AlphaGradient.H"
#include "mixedFvPatchFields.H"

using namespace Foam;

preciceAdapter::FF::AlphaGradient::AlphaGradient(
    const Foam::fvMesh& mesh,
    const std::string nameAlpha)
: Alpha_(
    const_cast<volScalarField*>(
        &mesh.lookupObject<volScalarField>(nameAlpha)))
{
    mDataType = scalar;
}

std::size_t preciceAdapter::FF::AlphaGradient::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // Get the Alpha gradient boundary patch
        const scalarField gradientPatch((Alpha_->boundaryFieldRef()[patchID])
                                            .snGrad());

        // For every cell of the patch
        forAll(gradientPatch, i)
        {
            // Copy the Alpha gradient into the buffer
            buffer[bufferIndex++] =
                -gradientPatch[i];
        }
    }
    return bufferIndex;
}

void preciceAdapter::FF::AlphaGradient::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // Get the Alpha gradient boundary patch
        scalarField& gradientPatch =
            refCast<fixedGradientFvPatchScalarField>(
                Alpha_->boundaryFieldRef()[patchID])
                .gradient();

        // For every cell of the patch
        forAll(gradientPatch, i)
        {
            // Set the Alpha gradient as the buffer value
            gradientPatch[i] =
                buffer[bufferIndex++];
        }
    }
}

bool preciceAdapter::FF::AlphaGradient::IsLocationTypeSupported(const bool meshConnectivity) const
{
    return (this->mLocationType == LocationType::FaceCenters);
}

std::string preciceAdapter::FF::AlphaGradient::GetDataName() const
{
    return "AlphaGradient";
}
