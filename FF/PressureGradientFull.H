#ifndef FF_FULL_PRESSURE_GRADIENT_H
#define FF_FULL_PRESSURE_GRADIENT_H

#include "CouplingDataUser.H"

#include "fvCFD.H"

namespace CoSimIOAdapter
{
namespace FF
{

//- Class that writes and reads pressure gradient
class PressureGradientFull : public CouplingDataUser
{

private:
    //- Pressure field
    Foam::volScalarField* p_;

    // Gradient field
    Foam::volVectorField gradP_;

public:
    //- Constructor
    PressureGradientFull(
        const Foam::fvMesh& mesh,
        const std::string nameP);

    //- Write the pressure gradient values into the buffer
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim) override;

    //- Read the pressure gradient values from the buffer
    void Read(double* buffer, const unsigned int dim) override;

    bool IsLocationTypeSupported(const bool meshConnectivity) const final;

    //- Get the name of the current data field
    std::string GetDataName() const final;
};

}
}

// NOTE: In order to couple another variable, you need to add another
// subclass of the CouplingDataUser. Take this file and PressureGradientFull.C
// as an example and look for more notes in the Adapter.C and Adapter.H.
// Essentially, the new class needs to implement a constructor (which also
// sets the mDataType) and implement the Write() and Read() methods.
// Then, you need to create objects of this class in the Adapter.C,
// whenever requested from the configuration file (see notes).

#endif
