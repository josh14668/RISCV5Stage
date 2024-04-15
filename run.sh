#!/bin/bash

# Compile test.cpp
g++ -w -o fsCore fsCore.cpp

# Check if compilation was successful
if [ $? -eq 0 ]; then
    # Run the compiled executable
    ./fsCore "input/testcase1"
else
    echo "Compilation failed."
fi
