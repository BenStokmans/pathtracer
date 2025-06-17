#!/bin/bash

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [ ! -f "build/pathtracer" ]; then
    echo -e "${YELLOW}Executable not found. Building first...${NC}"
    ./scripts/build.sh
fi

echo -e "${GREEN}Running PathTracer...${NC}"
cd build
./pathtracer
