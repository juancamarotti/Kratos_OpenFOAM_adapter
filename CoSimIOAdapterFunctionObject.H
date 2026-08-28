/*---------------------------------------------------------------------------*\
CoSimIO-adapter for OpenFOAM

Based on the PreCICE adapter for OpenFOAM. See also the README.md.
-------------------------------------------------------------------------------

License
    This adapter is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This adapter is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with the adapter.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::functionObjects::CoSimIOAdapterFunctionObject

Description
    This function object prepares an OpenFOAM solver for CoSimIO (https://kratosmultiphysics.github.io/CoSimIO/).
    See the README.md for details.

    NOTE: In case the functionObjects interface is different for your OpenFOAM
    version, you may create a new function object with the same name and type,
    using the template for functionObjects that you can usually find in the
    $FOAM_ETC/codeTemplates/ directory. This is only a wrapper to the
    adapter's methods (see cosimioAdapterFunctionObject.C).

    Example of function object specification:
    \verbatim
    CoSimIO_Adapter
    {
        type CoSimIOAdapterFunctionObject;
        libs ("libCoSimIOAdapterFunctionObject.so");
    }
    \endverbatim

Usage
    \table
        Property     | Description                             | Required | Default value
        type         | type name: CoSimIOAdapterFunctionObject | yes      |
    \endtable

SourceFiles
    CoSimIOAdapterFunctionObject.C

\*---------------------------------------------------------------------------*/

#ifndef COSIMIOADAPTERFUNCTIONOBJECT_H
#define COSIMIOADAPTERFUNCTIONOBJECT_H

// OpenFOAM header files
#include "fvMeshFunctionObject.H"
#include "clockValue.H"

// Main adapter header file
#include "Adapter.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{

/*---------------------------------------------------------------------------*\
                Class CoSimIOAdapterFunctionObject Declaration
\*---------------------------------------------------------------------------*/

class CoSimIOAdapterFunctionObject
: public fvMeshFunctionObject
{
    // Private data

#ifdef adapterENABLE_TIMINGS
    //- Cumulative time spent in the complete adapter function object (including preCICE calls)
    clockValue time_in_all;
    clockValue time_in_setup;
    clockValue time_in_execute;
#endif

    //- Adapter's main class instance
    CoSimIOAdapter::Adapter mAdapter;

    // Private Member Functions

    //- No copy construct
    CoSimIOAdapterFunctionObject(const CoSimIOAdapterFunctionObject&) = delete;

    //- No copy assignment
    void operator=(const CoSimIOAdapterFunctionObject&) = delete;


public:
    //- Runtime type information
    TypeName("CoSimIOAdapterFunctionObject");


    // Constructors

    //- Construct from Time and dictionary
    CoSimIOAdapterFunctionObject(
        const word& rName,
        const Time& rRunTime,
        const dictionary& rDict);


    //- Destructor
    ~CoSimIOAdapterFunctionObject() final;


    // Member Functions

    //- Read the CoSimIOAdapterFunctionObject data
    bool read(const dictionary&) final;

    //- Execute in the beginning of each time-loop, after the first one
    bool execute() final;

    //- Execute at the final time-loop, after execute()
    bool end() final;

    //- Write the CoSimIOAdapterFunctionObject
    bool write() final;

    //- Called at the end of Time::adjustDeltaT() if adjustTime is true
    bool adjustTimeStep() final;

    /*
        // NOTE: If you add a new module that needs to execute methods
        // whenever the mesh is updated or its points moved,
        // you may put your calls inside the following methods.

        // Update for changes of mesh
        void updateMesh(const mapPolyMesh& mpm) final;

        // Update for changes of mesh
        void movePoints(const polyMesh& mesh) final;
    */
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace functionObjects
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
