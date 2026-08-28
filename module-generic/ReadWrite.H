#ifndef MODULE_ReadWrite_H
#define MODULE_ReadWrite_H

#include "Interface.H"
#include "CouplingDataUser.H"

#include "fvCFD.H"
#include "cellSet.H"

namespace CoSimIOAdapter
{
namespace Generic
{

class ScalarFieldCoupler : public CouplingDataUser // One CouplingDataUser for each field
{
protected:
    Foam::volScalarField* scalarField_;
    const Foam::fvMesh& mesh_;
    const CoSimIOAdapter::FieldConfig& fieldConfig_;

public:
    ScalarFieldCoupler(const Foam::fvMesh& mesh, const CoSimIOAdapter::FieldConfig& fieldConfig);

    void Initialize();
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim);
    void Read(double* buffer, const unsigned int dim);
    bool IsLocationTypeSupported(const bool meshConnectivity) const;
    std::string GetDataName() const;

    virtual ~ScalarFieldCoupler() {};
};

class VectorFieldCoupler : public CouplingDataUser
{
protected:
    Foam::volVectorField* vectorField_;
    const Foam::fvMesh& mesh_;
    const CoSimIOAdapter::FieldConfig& fieldConfig_;

public:
    VectorFieldCoupler(const Foam::fvMesh& mesh, const CoSimIOAdapter::FieldConfig& fieldConfig);

    void Initialize();
    std::size_t Write(double* buffer, bool meshConnectivity, const unsigned int dim);
    void Read(double* buffer, const unsigned int dim);
    bool IsLocationTypeSupported(const bool meshConnectivity) const;
    std::string GetDataName() const;

    virtual ~VectorFieldCoupler() {};
};

}
}

#endif
