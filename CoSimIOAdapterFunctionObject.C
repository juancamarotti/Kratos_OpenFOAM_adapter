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

\*---------------------------------------------------------------------------*/

#include "CoSimIOAdapterFunctionObject.H"

// OpenFOAM header files
#include "Time.H"
#include "fvMesh.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace functionObjects
{
defineTypeNameAndDebug(CoSimIOAdapterFunctionObject, 0);
addToRunTimeSelectionTable(functionObject, CoSimIOAdapterFunctionObject, dictionary);
}
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::functionObjects::CoSimIOAdapterFunctionObject::CoSimIOAdapterFunctionObject(
    const word& rName,
    const Time& rRunTime,
    const dictionary& rDict)
: fvMeshFunctionObject(rName, rRunTime, rDict),
  mAdapter(rRunTime, mesh_)
{

#if (defined OPENFOAM && (OPENFOAM >= 1712)) || (defined OPENFOAM_PLUS && (OPENFOAM_PLUS >= 1712))
    // Patch for issue #27: warning "MPI was already finalized" while
    // running in serial. This only affects openfoam.com, while initNull()
    // does not exist in openfoam.org.
    UPstream::initNull();
#endif

    read(rDict);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::functionObjects::CoSimIOAdapterFunctionObject::~CoSimIOAdapterFunctionObject()
{
#ifdef ADAPTER_ENABLE_TIMINGS
    Info << "-------------------- CoSimIO adapter timers (primary rank) --------------------------" << nl;
    Info << "Total time in adapter + CoSimIO: " << time_in_all.str() << " (format: day-hh:mm:ss.ms)" << nl;
    Info << "  For setting up (S):            " << time_in_setup.str() << " (read() function)" << nl;
    Info << "  For all iterations (I):        " << time_in_execute.str() << " (execute() and adjustTimeStep() functions)" << nl << nl;
#endif
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::functionObjects::CoSimIOAdapterFunctionObject::read(const dictionary& rDict)
{
#ifdef ADAPTER_ENABLE_TIMINGS
    // Save the current wall clock time stamp to the clock
    clockValue clock;
    clock.update();
#endif

    mAdapter.configure();
    std::cout << "THE CONFIGURE OF THE ADAPTER WAS SUCCESSFUL" << std::endl;
    std::exit(0);

#ifdef ADAPTER_ENABLE_TIMINGS
    // Accumulate the time in this section into a global timer.
    // Same in all function object methods.
    time_in_all += clock.elapsed();
    time_in_setup = clock.elapsed();
#endif

    return true;
}


bool Foam::functionObjects::CoSimIOAdapterFunctionObject::execute()
{
#ifdef ADAPTER_ENABLE_TIMINGS
    clockValue clock;
    clock.update();
#endif

    mAdapter.execute();

#ifdef ADAPTER_ENABLE_TIMINGS
    time_in_all += clock.elapsed();
    time_in_execute += clock.elapsed();
#endif

    return true;
}


bool Foam::functionObjects::CoSimIOAdapterFunctionObject::end()
{
#ifdef ADAPTER_ENABLE_TIMINGS
    clockValue clock;
    clock.update();
#endif

    mAdapter.end();

#ifdef ADAPTER_ENABLE_TIMINGS
    time_in_all += clock.elapsed();
    time_in_execute += clock.elapsed();
#endif

    return true;
}


bool Foam::functionObjects::CoSimIOAdapterFunctionObject::write()
{
    return true;
}

bool Foam::functionObjects::CoSimIOAdapterFunctionObject::adjustTimeStep()
{
#ifdef ADAPTER_ENABLE_TIMINGS
    clockValue clock;
    clock.update();
#endif

    mAdapter.adjustTimeStep();

#ifdef ADAPTER_ENABLE_TIMINGS
    time_in_all += clock.elapsed();
    time_in_execute += clock.elapsed();
#endif

    return true;
}

// ************************************************************************* //
