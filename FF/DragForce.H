#ifndef FF_DRAG_FORCE_H
#define FF_DRAG_FORCE_H

#include "CouplingDataUser.H"

#include "fvCFD.H"
#include "cellSet.H"

namespace CoSimIOAdapter
{
namespace FF
{

//- Class that writes and reads velocity
class DragForce : public CouplingDataUser
{

private:
    //- DragForce field
    Foam::volVectorField* F_d_;

public:
    //- Constructor
    DragForce(
        const Foam::fvMesh& mesh,
        const std::string nameFd);

    //- Write the velocity values into the buffer
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim) final;

    //- Read the velocity values from the buffer
    void Read(double* buffer, const unsigned int dim) final;

    bool IsLocationTypeSupported(const bool meshConnectivity) const final;

    //- Get the name of the current data field
    std::string GetDataName() const final;
};

}
}

// NOTE: In order to couple another variable, you need to add another
// subclass of the CouplingDataUser. Take this file and DragForce.C
// as an example and look for more notes in the Adapter.C and Adapter.H.
// Essentially, the new class needs to implement a constructor (which also
// sets the mDataType) and implement the Write() and Read() methods.
// Then, you need to create objects of this class in the Adapter.C,
// whenever requested from the configuration file (see notes).

#endif
