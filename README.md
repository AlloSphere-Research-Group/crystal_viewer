# Crystal Viewer

Crystal structures represent a wide variety of materials. These structures such as body-centered cubic(bcc) and face-centered cubic(fcc) exist in diverse geometric arrangements, and even more so if quasicrystal(aperiodic crystals) structures are included. Understanding the transformation pathways between these structures such as the Bain transformation(fcc to bcc) is crucial to researching the various mechanisms of structural phase transformations and dynamic instabilities. However, the number of known pathways are limited, and the structural incommensurabilities present a computational challenge. This research is aimed at reconstructing the various 3D crystal structures from a common simpler crystalline lattice existing in 4D/5D or higher dimensions, from which we can easily explore transformation pathways that exist as rotations and other transformations in higher dimensions. By exploring a common parent structure at higher dimensions, we aim to explore the relations between the various topologies of crystal structures as well as quasicrystals, which are known to be mathematically derivable from projections of a higher dimensional periodic lattice.

# Installation
Crystal Viewer currently requires:
 * bash shell
 * git
 * cmake version 3.0 or higher

## How to compile / run
The src/ folder contains the initial main.cpp starter code.

On a bash shell you can run:

    ./configure.sh
    ./run.sh

This will configure and compile the project, and run the binary if compilation is successful.

Alternatively, you can open the CMakeLists.txt proeject in an IDE like VS Code, Visual Studio or Qt Creator and have the IDE manage the configuration and execution of cmake.

You can also generate other IDE projects through cmake.

## How to perform a distclean
If you need to delete the build,

    ./distclean.sh

should recursively clean all the build directories of the project including those of allolib and its submodules.

## Copyright
Copyright 2023 AlloSphere Research Group

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Author: Kon Hyong Kim