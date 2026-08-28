#ifndef CHT_TEMPERATURE_H
#define CHT_TEMPERATURE_H

#include "CouplingDataUser.H"

#include "fvCFD.H"
#include "cellSet.H"

namespace CoSimIOAdapter
{
namespace CHT
{

//- Class that writes and reads temperature.
class Temperature : public CouplingDataUser
{

private:
    //- Temperature field
    Foam::volScalarField* T_;
    const Foam::fvMesh& mesh_;

public:
    //- Constructor
    Temperature(
        const Foam::fvMesh& mesh,
        const std::string nameT);

    //- Write the temperature values into the buffer
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim) final;

    //- Read the temperature values from the buffer
    void Read(double* buffer, const unsigned int dim) final;

    bool IsLocationTypeSupported(const bool meshConnectivity) const final;

    //- Get the name of the current data field
    std::string GetDataName() const final;
};

}
}

// NOTE: In order to couple another variable, you need to add another
// subclass of the CouplingDataUser. Take this file and Temperature.C
// as an example and look for more notes in the Adapter.C and Adapter.H.
// Essentially, the new class needs to implement a constructor (which also
// sets the mDataType) and implement the write() and read() methods.
// Then, you need to create objects of this class in the Adapter.C,
// whenever requested from the configuration file (see notes).

#endif
