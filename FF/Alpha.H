#ifndef FF_ALPHA_H
#define FF_ALPHA_H

#include "CouplingDataUser.H"

#include "fvCFD.H"
#include "cellSet.H"

namespace preciceAdapter
{
namespace FF
{

//- Class that writes and reads Alpha
class Alpha : public CouplingDataUser
{

private:
    //- Alpha field
    Foam::volScalarField* Alpha_;

public:
    //- Constructor
    Alpha(
        const Foam::fvMesh& mesh,
        const std::string nameAlpha);

    //- Write the Alpha values into the buffer
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim) final;

    //- Read the Alpha values from the buffer
    void Read(double* buffer, const unsigned int dim) final;

    bool IsLocationTypeSupported(const bool meshConnectivity) const final;

    //- Get the name of the current data field
    std::string GetDataName() const final;
};

}
}

#endif
