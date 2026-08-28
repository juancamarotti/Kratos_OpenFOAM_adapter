#include "Generic.H"

#include "Utilities.H"

using namespace Foam;

CoSimIOAdapter::Generic::GenericInterface::GenericInterface(
    const Foam::fvMesh& mesh)
: mesh_(mesh) {}

bool CoSimIOAdapter::Generic::GenericInterface::configure(const IOdictionary& adapterConfig)
{
    DEBUG(adapterInfo("Configuring the Generic module..."));

    // Scan OpenFOAM object registry once for all
    // available volScalarFields and volVectorFields
    for (const auto& solver_name : mesh_.sortedNames<volScalarField>())
    {
        availableVolScalarFields += solver_name + " ";
    }
    DEBUG(adapterInfo("    Available volScalarFields: " + availableVolScalarFields));

    for (const auto& solver_name : mesh_.sortedNames<volVectorField>())
    {
        availableVolVectorFields += solver_name + " ";
    }
    DEBUG(adapterInfo("    Available volVectorFields: " + availableVolVectorFields));

    // Read the Generic-module specific options from the adapter's configuration file
    if (!readConfig(adapterConfig))
    {
        return false;
    }

    return true;
}

bool CoSimIOAdapter::Generic::GenericInterface::readConfig(const IOdictionary& adapterConfig)
{
    // Empty for now. No other specific configuration options for the Generic module.
    return true;
}

bool CoSimIOAdapter::Generic::GenericInterface::addWriters(const CoSimIOAdapter::FieldConfig& fieldConfig, Interface* interface)
{
    bool found = false;

    // Force to use the new schema with the Generic module
    if (fieldConfig.solver_name != "Undefined (legacy mode)")
    {
        // Determine type of field
        if (mesh_.foundObject<volScalarField>(fieldConfig.solver_name))
        {
            found = true;
            interface->AddCouplingDataWriter(
                fieldConfig,
                new ScalarFieldCoupler(mesh_, fieldConfig));
        }
        else if (mesh_.foundObject<volVectorField>(fieldConfig.solver_name))
        {
            found = true;
            interface->AddCouplingDataWriter(
                fieldConfig,
                new VectorFieldCoupler(mesh_, fieldConfig));
        }
        else
        {
            found = false;
            std::string msg = "Generic module: Data \"" + fieldConfig.name + "\", solver name: \"" + fieldConfig.solver_name + "\" not found!\n";
            msg += "Available fields: " + availableVolScalarFields + availableVolVectorFields;
            adapterInfo(msg, "warning");
        }
    }

    if (found)
    {
        DEBUG(adapterInfo("Added writer: " + fieldConfig.name));
    }
    return found;
}

bool CoSimIOAdapter::Generic::GenericInterface::addReaders(const CoSimIOAdapter::FieldConfig& fieldConfig, Interface* interface)
{
    bool found = false;

    // Force to use the new schema with the Generic modul
    if (fieldConfig.solver_name != "Undefined (legacy mode)")
    {
        // Determine type of field
        if (mesh_.foundObject<volScalarField>(fieldConfig.solver_name))
        {
            found = true;
            interface->AddCouplingDataReader(
                fieldConfig,
                new ScalarFieldCoupler(mesh_, fieldConfig));
        }
        else if (mesh_.foundObject<volVectorField>(fieldConfig.solver_name))
        {
            found = true;
            interface->AddCouplingDataReader(
                fieldConfig,
                new VectorFieldCoupler(mesh_, fieldConfig));
        }
        else
        {
            found = false;
            std::string msg = "Generic module: Data \"" + fieldConfig.name + "\" (solver name: \"" + fieldConfig.solver_name + "\") not found!\n";
            msg += "Available fields: " + availableVolScalarFields + availableVolVectorFields;
            adapterInfo(msg, "warning");
        }
    }

    if (found)
    {
        DEBUG(adapterInfo("Added reader: " + fieldConfig.name));
    }
    return found;
}
