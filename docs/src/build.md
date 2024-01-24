# Building
This project has all dependencies either directly in the source tree or as git submodules. Make sure you clone with submodules. `--recurse-submodules`.

Uses [Premake](https://premake.github.io/). Have this installed.

`premake5 vs2022` to generate a [Visual Studio](https://visualstudio.microsoft.com/) solution for Windows.

`premake5 gmake2` to generate makefiles.