#ifndef FF_PHI_H
#define FF_PHI_H

#include "CouplingDataUser.H"

#include "fvCFD.H"

namespace CoSimIOAdapter
{
namespace FF
{

//- Class that writes and reads the flux phi
class Phi : public CouplingDataUser
{

private:
    //- Phi field
    Foam::surfaceScalarField* phi_;

public:
    //- Constructor
    Phi(
        const Foam::fvMesh& mesh,
        const std::string namePhi);

    //- Write the Phi values into the buffer
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim) final;

    //- Read the Phi values from the buffer
    void Read(double* buffer, const unsigned int dim) final;

    bool IsLocationTypeSupported(const bool meshConnectivity) const final;

    //- Get the name of the current data field
    std::string GetDataName() const final;
};

}
}

#endif
