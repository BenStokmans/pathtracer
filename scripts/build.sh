#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}Building PathTracer...${NC}"

if [ ! -d "build" ]; then
    echo -e "${YELLOW}Creating build directory...${NC}"
    mkdir build
fi

cd build

echo -e "${YELLOW}Configuring with CMake...${NC}"
cmake ..

echo -e "${YELLOW}Compiling...${NC}"
cmake --build .

echo -e "${GREEN}Build complete!${NC}"
echo -e "${BLUE}Executable: build/pathtracer${NC}"
echo -e "${BLUE}Run with: ./scripts/run.sh${NC}"
