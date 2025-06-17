#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}Building PathTracer in Debug mode...${NC}"

if [ ! -d "build" ]; then
    echo -e "${YELLOW}Creating build directory...${NC}"
    mkdir build
fi

cd build

echo -e "${YELLOW}Configuring with CMake (Debug)...${NC}"
cmake -DCMAKE_BUILD_TYPE=Debug ..

echo -e "${YELLOW}Compiling...${NC}"
cmake --build .

echo -e "${GREEN}Debug build complete!${NC}"
echo -e "${BLUE}Starting debugger...${NC}"
echo -e "${YELLOW}Type 'run' to start the application in the debugger${NC}"

lldb ./pathtracer
