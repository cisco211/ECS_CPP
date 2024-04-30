# ECS = Entity Component System
Very simple, but fully self contained ECS for C++14 (or higher).
Right now, this is just an experiment/study to better understand ECS.
The code might change a lot later, so use it at your own risk (no support).

Reference: [A SIMPLE ENTITY COMPONENT SYSTEM (ECS) [C++]](https://austinmorlan.com/posts/entity_component_system/) by `Austin Morlan`.

## Setup
Right now you have to care about compiling/linking by your own,
but look how i have done it in `VSCode` with `MS Build Tools` on windows:

1. Run `VSCode`.
2. Install extension: `C/C++` (ms-vscode.cpptools).
3. Install extension: `C/C++ Runner` (franneck94.c-cpp-runner).
4. Inside `./.vscode/`, copy `settings-sample.json` into `settings.json`.
5. Now you change the following settings:
   * `C_Cpp_Runner.cStandard` = `c11`
   * `C_Cpp_Runner.cppStandard` = `c++14`
   * `C_Cpp_Runner.msvcBatchPath` = *Path to `vcvarsall.bat`*
   * `C_Cpp_Runner.useMsvc` = `true`
6. In `VSCode`, the status bar at the bottom now has new buttons:
   * `Wrench/Screwdriver` = Switch between build configurations
     (Debug, Release, ...)
   * `Gear` = Run compiler.
   * `Play` = Run compiled program.
   * `Bug` = Run compiled program with debugger.
   * `Trash` = Clear build directory (clean).
7. Done.

## Run
 Just compile it, then run it.
