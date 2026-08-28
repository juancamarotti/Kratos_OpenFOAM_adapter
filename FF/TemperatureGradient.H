#ifndef FF_TEMPERATURE_GRADIENT_H
#define FF_TEMPERATURE_GRADIENT_H

#include "CouplingDataUser.H"

#include "fvCFD.H"

namespace CoSimIOAdapter
{
namespace FF
{

//- Class that writes and reads Temperature gradient
class TemperatureGradient : public CouplingDataUser
{

private:
    //- Temperature field
    Foam::volScalarField* T_;

public:
    //- Constructor
    TemperatureGradient(
        const Foam::fvMesh& mesh,
        const std::string nameT);

    //- Write the Temperature gradient values into the buffer
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim) final;

    //- Read the Temperature gradient values from the buffer
    void Read(double* buffer, const unsigned int dim) final;

    bool IsLocationTypeSupported(const bool meshConnectivity) const final;

    //- Get the name of the current data field
    std::string GetDataName() const final;
};

}
}

#endif
