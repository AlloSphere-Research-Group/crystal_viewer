# Crystal Viewer

Crystal structures represent a wide variety of materials. These structures such as body-centered cubic(bcc) and face-centered cubic(fcc) exist in diverse geometric arrangements, and even more so if quasicrystal(aperiodic crystals) structures are included. Understanding the transformation pathways between these structures such as the Bain transformation(fcc to bcc) is crucial to researching the various mechanisms of structural phase transformations and dynamic instabilities. However, the number of known pathways are limited, and the structural incommensurabilities present a computational challenge. This research is aimed at reconstructing the various 3D crystal structures from a common simpler crystalline lattice existing in 4D/5D or higher dimensions, from which we can easily explore transformation pathways that exist as rotations and other transformations in higher dimensions. By exploring a common parent structure at higher dimensions, we aim to explore the relations between the various topologies of crystal structures as well as quasicrystals, which are known to be mathematically derivable from projections of a higher dimensional periodic lattice.

## Installation
Crystal Viewer currently requires:
 * git
 * cmake version 3.0 or higher
 * c++ compiler

## How to compile / run
From a terminal you can run:

    ./configure.sh
    ./run.sh

on windows systems without bash:
    ./configure.bat
    ./run.bat

This will configure and compile the project, and run the binary if compilation is successful.

Alternatively, you can open the CMakeLists.txt proeject in an IDE like VS Code, Visual Studio or Qt Creator and have the IDE manage the configuration and execution of cmake.

You can also generate other IDE projects through cmake.

## How to remove the build tree
If you need to delete the build:

    ./distclean.sh

on windows systems without bash:
    ./distclean.bat

should recursively clean all the build directories of the project including those of allolib and its submodules.

## Acknowledgements
TINC is funded by NSF grant # OAC 2004693: "Elements: Cyber-infrastructure for Interactive Computation and Display of Materials Datasets."