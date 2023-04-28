# Xi (Ξ) Engine
This is a small game engine project written to aid us in our [Ludum Dare](ldjam.com) attempts!
For the most part, it is written completely from scratch using only the available operating system apis

## Building

### Windows
For this you will need the msvc toolchain for `cl.exe`. Once your environment is setup it is as simple as
executing the `win32.bat` in the root of the repository. This will build `xid.dll` and `xid.lib` to link
against.

### Linux
Linux support is very preliminary and currently relies on [SDL2](libsdl.org/). To build on Linux you will need
the `gcc` toolchain installed and it is as simple as executing `linux.sh` in the root of the repository. This
will build `libxid.so`, however you do not need to link against this as long as it is within the same folder as
your executable. It will be dynamically loaded at runtime.

## Features

Some of the engine features are listed below:
- Window handling
- Input handling for keyboard and mouse
- Audio mixing for music and sound effect playback (with layered music!)
-  Threaded asset packing and loading into custom asset file format
	- PNG images
	- Frame-based animations
	- Wav audio files
- OpenGL-based rendering
- Virtual memory arena based allocation system
- Counted string utilities
- File system and File I/O utilities
- Maths utilties for vectors and matrices
- Live code reloading to update game while it is running

## TODO
Features to add:
- Custom render targets with custom post processing shaders
- Asset caching with change repacking
- Port to a 'better' graphics api
- Native Linux support, wayland and x11.
- Controller input
- Other assets, meshes, fonts etc.
- ... a lot more
