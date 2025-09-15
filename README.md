# Godot 3D Force Field

Computer Shader based euclidian CFD solver, implemented as a gdextension for Godot 4.4, written in C++ and GLSL.


<img src="https://jonasklein.dev/godot-force-field/example_1.png" width="480">
<img src="https://jonasklein.dev/godot-force-field/example_2.png" width="480">

[Video (45MB)](https://jonasklein.dev/godot-force-field/video_example.mov)

## Building and Running

The repository includes an example godot project visualizing the grid and allowing for some
simple manipulation. Note that the project uses cmake and not scoons. After building, run `make install` to
copy all relevant files to the project folder.

## Notes

The extension currently only works properly with either the DirectX12 or Metal backend. It seems there is a bug
in the Vulkan backend that does not allow for using the 3D texture the simulation result is written to in a godot
shader.

## Resources
This is based on a javascript 2D implementation by Matthias Müller
which he explains in a very concise way in this fantastic [video](https://www.youtube.com/watch?v=iKAVRgIrUOU).