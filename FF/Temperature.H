#ifndef FF_TEMPERATURE_H
#define FF_TEMPERATURE_H

#include "CouplingDataUser.H"

#include "fvCFD.H"

namespace CoSimIOAdapter
{
namespace FF
{

//- Class that writes and reads temperature
class Temperature : public CouplingDataUser
{

private:
    //- Temperature field
    Foam::volScalarField* T_;

public:
    //- Constructor
    Temperature(
        const Foam::fvMesh& mesh,
        const std::string nameT);

    //- Write the temperature values into the buffer
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim) override;

    //- Read the temperature values from the buffer
    void Read(double* buffer, const unsigned int dim) override;

    bool IsLocationTypeSupported(const bool meshConnectivity) const override;

    //- Get the name of the current data field
    std::string GetDataName() const override;
};

}
}

#endif
