#include "Stress.H"

using namespace Foam;

CoSimIOAdapter::FSI::Stress::Stress(
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

std::size_t CoSimIOAdapter::FSI::Stress::Write(double* buffer, bool meshConnectivity, const unsigned int dim)
{
    return this->writeToBuffer(buffer, *Stress_, dim);
}

void CoSimIOAdapter::FSI::Stress::Read(double* buffer, const unsigned int dim)
{
    this->readFromBuffer(buffer);
}

bool CoSimIOAdapter::FSI::Stress::IsLocationTypeSupported(const bool meshConnectivity) const
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

std::string CoSimIOAdapter::FSI::Stress::GetDataName() const
{
    return "Stress";
}

Foam::tmp<Foam::vectorField> CoSimIOAdapter::FSI::Stress::getFaceVectors(const unsigned int patchID) const
{
    // face normal vectors
    return mesh_.boundary()[patchID].nf();
}

CoSimIOAdapter::FSI::Stress::~Stress()
{
    delete Stress_;
}
