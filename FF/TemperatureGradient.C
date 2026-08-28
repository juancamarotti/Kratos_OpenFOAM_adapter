#include "TemperatureGradient.H"
#include "mixedFvPatchFields.H"

using namespace Foam;

CoSimIOAdapter::FF::TemperatureGradient::TemperatureGradient(
    const Foam::fvMesh& mesh,
    const std::string nameT)
: T_(
    const_cast<volScalarField*>(
        &mesh.lookupObject<volScalarField>(nameT)))
{
    mDataType = scalar;
}

std::size_t CoSimIOAdapter::FF::TemperatureGradient::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // Get the Temperature gradient boundary patch
        const scalarField gradientPatch((T_->boundaryFieldRef()[patchID])
                                            .snGrad());

        // For every cell of the patch
        forAll(gradientPatch, i)
        {
            // Copy the Temperature gradient into the buffer
            buffer[bufferIndex++] =
                gradientPatch[i];
        }
    }
    return bufferIndex;
}

void CoSimIOAdapter::FF::TemperatureGradient::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // Get the Temperature gradient boundary patch
        scalarField& gradientPatch =
            refCast<fixedGradientFvPatchScalarField>(
                T_->boundaryFieldRef()[patchID])
                .gradient();

        // For every cell of the patch
        forAll(gradientPatch, i)
        {
            // Set the Temperature gradient as the buffer value
            gradientPatch[i] =
                -buffer[bufferIndex++];
        }
    }
}

bool CoSimIOAdapter::FF::TemperatureGradient::IsLocationTypeSupported(const bool meshConnectivity) const
{
    return (this->mLocationType == LocationType::FaceCenters);
}

std::string CoSimIOAdapter::FF::TemperatureGradient::GetDataName() const
{
    return "TemperatureGradient";
}
