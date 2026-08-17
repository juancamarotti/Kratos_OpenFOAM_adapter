#include "Velocity.H"
#include "coupledVelocityFvPatchField.H"

using namespace Foam;

CoSimIOAdapter::FF::Velocity::Velocity(
    const Foam::fvMesh& mesh,
    const std::string nameU,
    const std::string namePhi,
    bool fluxCorrection)
: phi_(const_cast<surfaceScalarField*>(
    &mesh.lookupObject<surfaceScalarField>(namePhi))),
  fluxCorrection_(fluxCorrection)
{
    if (mesh.foundObject<volVectorField>(nameU))
    {
        adapterInfo("Using existing velocity object " + nameU, "debug");
        U_ = const_cast<volVectorField*>(
            &mesh.lookupObject<volVectorField>(nameU));
    }
    else
    {
        adapterInfo("Creating a new velocity object " + nameU, "debug");
        U_ = new volVectorField(
            IOobject(
                nameU,
                mesh.time().timeName(),
                mesh,
                IOobject::MUST_READ,
                IOobject::AUTO_WRITE),
            mesh);
    }
    mDataType = vector;
}

std::size_t CoSimIOAdapter::FF::Velocity::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    int bufferIndex = 0;

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (const auto& cell : U_->internalField())
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
                cellSet overlapRegion(U_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    // x-dimension
                    buffer[bufferIndex++] = U_->internalField()[currentCell].x();

                    // y-dimension
                    buffer[bufferIndex++] = U_->internalField()[currentCell].y();

                    if (dim == 3)
                    {
                        // z-dimension
                        buffer[bufferIndex++] = U_->internalField()[currentCell].z();
                    }
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        vectorField UPatch = U_->boundaryField()[patchID];

        // Correct the velocity by the boundary face flux
        if (fluxCorrection_)
        {
            scalarField phip = phi_->boundaryFieldRef()[patchID];
            const vectorField n = U_->boundaryField()[patchID].patch().nf().cref();
            const scalarField& magS = U_->boundaryFieldRef()[patchID].patch().magSf();
            UPatch = UPatch - n * (n & U_->boundaryField()[patchID]) + n * phip / magS;
        }

        // For every cell of the patch
        forAll(U_->boundaryFieldRef()[patchID], i)
        {
            // Copy the velocity into the buffer
            // x-dimension
            buffer[bufferIndex++] =
                UPatch[i].x();

            // y-dimension
            buffer[bufferIndex++] =
                UPatch[i].y();

            if (dim == 3)
            {
                // z-dimension
                buffer[bufferIndex++] =
                    UPatch[i].z();
            }
        }
    }
    return bufferIndex;
}

void CoSimIOAdapter::FF::Velocity::Read(double* buffer, const unsigned int dim)
{
    int bufferIndex = 0;

    if (this->mLocationType == LocationType::VolumeCenters)
    {
        if (mCellSetNames.empty())
        {
            for (auto& cell : U_->ref())
            {
                // x-dimension
                cell.x() = buffer[bufferIndex++];

                // y-dimension
                cell.y() = buffer[bufferIndex++];

                if (dim == 3)
                {
                    // z-dimension
                    cell.z() = buffer[bufferIndex++];
                }
            }
        }
        else
        {
            for (const auto& cellSetName : mCellSetNames)
            {
                cellSet overlapRegion(U_->mesh(), cellSetName);
                const labelList& cells = overlapRegion.toc();

                for (const auto& currentCell : cells)
                {
                    // x-dimension
                    U_->ref()[currentCell].x() = buffer[bufferIndex++];

                    // y-dimension
                    U_->ref()[currentCell].y() = buffer[bufferIndex++];

                    if (dim == 3)
                    {
                        // z-dimension
                        U_->ref()[currentCell].z() = buffer[bufferIndex++];
                    }
                }
            }
        }
    }

    // For every boundary patch of the interface
    for (uint j = 0; j < mPatchIDs.size(); j++)
    {
        int patchID = mPatchIDs.at(j);

        // Get the velocity value boundary patch
        vectorField* valuePatchPtr = &U_->boundaryFieldRef()[patchID];
        if (isA<coupledVelocityFvPatchField>(U_->boundaryFieldRef()[patchID]))
        {
            valuePatchPtr = &refCast<coupledVelocityFvPatchField>(
                                 U_->boundaryFieldRef()[patchID])
                                 .refValue();
        }
        vectorField& valuePatch = *valuePatchPtr;

        // For every cell of the patch
        forAll(U_->boundaryFieldRef()[patchID], i)
        {
            // Set the velocity as the buffer value
            // x-dimension
            valuePatch[i].x() =
                buffer[bufferIndex++];

            // y-dimension
            valuePatch[i].y() =
                buffer[bufferIndex++];

            if (dim == 3)
            {
                // z-dimension
                valuePatch[i].z() =
                    buffer[bufferIndex++];
            }
        }
    }
}

bool CoSimIOAdapter::FF::Velocity::IsLocationTypeSupported(const bool meshConnectivity) const
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

std::string CoSimIOAdapter::FF::Velocity::GetDataName() const
{
    return "Velocity";
}
