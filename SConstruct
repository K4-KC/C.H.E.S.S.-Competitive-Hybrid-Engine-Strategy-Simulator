#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=["src/"])

# Automatically finds all .cpp files in src/ directory
sources = Glob("src/*.cpp")

# Changed library name from "NeuralNet" to "chess_ai" to reflect combined module
library = env.SharedLibrary(
    "keychan-chess/bin/kCC_modules{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
    source=sources,
)

Default(library)
