#include "Stress.H"

using namespace Foam;

preciceAdapter::FSI::Stress::Stress(
    const Foam::fvMesh& mesh,
    const std::string solverType)
: ForceBase(mesh, solverType)
{
    Stress_ = new volVectorField(
        IOobject(
            "Stress",
            mesh_.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE),
        mesh,
        dimensionedVector(
            "pdim",
            dimensionSet(1, -1, -2, 0, 0, 0, 0),
            Foam::vector::zero));
}

std::size_t preciceAdapter::FSI::Stress::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    return this->writeToBuffer(buffer, *Stress_, dim);
}

void preciceAdapter::FSI::Stress::Read(double* buffer, const unsigned int dim)
{
    this->readFromBuffer(buffer);
}

bool preciceAdapter::FSI::Stress::IsLocationTypeSupported(const bool meshConnectivity) const
{
    if (meshConnectivity)
    {
        return false;
    }
    else
    {
        return (this->mLocationType == LocationType::FaceCenters);
    }
}

std::string preciceAdapter::FSI::Stress::GetDataName() const
{
    return "Stress";
}

Foam::tmp<Foam::vectorField> preciceAdapter::FSI::Stress::getFaceVectors(const unsigned int patchID) const
{
    // face normal vectors
    return mesh_.boundary()[patchID].nf();
}

preciceAdapter::FSI::Stress::~Stress()
{
    delete Stress_;
}
