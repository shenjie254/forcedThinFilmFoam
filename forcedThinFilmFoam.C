/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2016 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Application
    simpleFoam

Description
    Steady-state solver for incompressible, turbulent flow, using the SIMPLE
    algorithm.

\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
#include "simpleControl.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


int main(int argc, char *argv[])
{
    #include "postProcess.H"

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "createControl.H"
    #include "createFields.H"

    // Determine the growth and diffusion field BEFORE incrementing time.
    #include "update_growth_field.H"
    growth.write();
    #include "update_D_field.H"
    D_active.write();

    while (runTime.run())
    {
        runTime++;

        Info<< "Time = " << runTime.timeName() << nl << endl;

        // Get the laplacian of h for the surface tension
        lap_h = fvc::laplacian(h);

        if(poisson_velocity){ // Solve for C, update vbottom and phi
            solve( fvm::laplacian(CPoisson) == repulsion*h );
            //CPoisson.write();
            vBottom = fvc::grad(CPoisson);
            phi = fvc::interpolate(vBottom) & mesh.Sf();
        }

        while (simple.correctNonOrthogonal())
        {
            fvScalarMatrix hEqn
            (
                fvm::ddt(h)
              + fvm::div(phi, h)
              - fvm::laplacian(D_active, h)
              - ((rho*g)/(3*mu))*fvm::laplacian(pow(h,3),h)
              + (sigma/(3*mu))*fvc::laplacian(pow(h,3), lap_h)
              - fvm::SuSp(growth_factor, h)
            );
            hEqn.solve();
        }

        // The growth and D fields are written based on the h field that is written.
        #include "update_growth_field.H"
        #include "update_D_field.H"

        runTime.write();

        Info<< "ExecutionTime = " << runTime.elapsedCpuTime() << " s"
            << "  ClockTime = " << runTime.elapsedClockTime() << " s"
            << nl << endl;
    }

    Info<< "End\n" << endl;
    
    return 0;
}


// ************************************************************************* //
