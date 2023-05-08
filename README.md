# Crystal Viewer

Crystal structures represent a wide variety of materials. These structures such as body-centered cubic(bcc) and face-centered cubic(fcc) exist in diverse geometric arrangements, and even more so if quasicrystal(aperiodic crystals) structures are included. Understanding the transformation pathways between these structures such as the Bain transformation(fcc to bcc) is crucial to researching the various mechanisms of structural phase transformations and dynamic instabilities. However, the number of known pathways are limited, and the structural incommensurabilities present a computational challenge. This research is aimed at reconstructing the various 3D crystal structures from a common simpler crystalline lattice existing in 4D/5D or higher dimensions, from which we can easily explore transformation pathways that exist as rotations and other transformations in higher dimensions. By exploring a common parent structure at higher dimensions, we aim to explore the relations between the various topologies of crystal structures as well as quasicrystals, which are known to be mathematically derivable from projections of a higher dimensional periodic lattice.

# Installation
Crystal Viewer currently requires:
 * bash shell
 * git
 * cmake version 3.0 or higher

## Using `alloinit`
The [`alloinit`](utils/alloinit.md) one-step project initializer can be used to
initialize a new alloinit project as follows:

```sh
curl -L https://github.com/Allosphere-Research-Group/allotemplate/raw/master/utils/alloinit \
    | bash -s proj  # Download `alloinit` and initialize an `allotemplate` project in `proj/`.
cd proj             # A copy of `alloinit` is now in `proj/utils`.
./run.sh            # Run your new project!
```

## Manually creating a new project based on allotemplate
On a bash shell:

    git clone https://github.com/AlloSphere-Research-Group/allotemplate.git <project folder name>
    cd <project folder name>
    ./init.sh

This will prepare the project as a fresh git repository and will add allolib and al_ext as submodules.

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