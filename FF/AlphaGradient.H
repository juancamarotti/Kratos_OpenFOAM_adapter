#ifndef FF_ALPHA_GRADIENT_H
#define FF_ALPHA_GRADIENT_H

#include "CouplingDataUser.H"

#include "fvCFD.H"

namespace preciceAdapter
{
namespace FF
{

//- Class that writes and reads Alpha gradient
class AlphaGradient : public CouplingDataUser
{

private:
    //- Alpha field
    Foam::volScalarField* Alpha_;

public:
    //- Constructor
    AlphaGradient(
        const Foam::fvMesh& mesh,
        const std::string nameAlpha);

    //- Write the Alpha gradient values into the buffer
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim) final;

    //- Read the Alpha gradient values from the buffer
    void Read(double* buffer, const unsigned int dim) final;

    bool IsLocationTypeSupported(const bool meshConnectivity) const final;

    //- Get the name of the current data field
    std::string GetDataName() const final;
};

}
}

#endif
