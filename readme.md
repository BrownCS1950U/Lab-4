# Object / Mesh Viewer

The following repository contains the implementation of an mesh animation viewer for any type of file that supports animations. The project has been implemented in C++ using OpenGL and GLSL.

## Usage

To compile, create a directory called `build` and create Makefile using the cmake build system

    mkdir build && cd build
    cmake ..
    make

Once the project is compiled, simply run the project with an animation mesh file. An example is in the main directory with the name `iclone.glb`.

You can run the viewer with the example file with the following command:

`./viewer ../iclone.glb`

Make sure you give the correct location for the executable and the animation mesh file.
To find more animated meshes, head over to mixamo.com for free customizable animated meshes and characters.