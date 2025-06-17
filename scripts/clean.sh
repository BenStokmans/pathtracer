#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}Cleaning build artifacts...${NC}"

if [ -d "build" ]; then
    rm -rf build
    echo -e "${GREEN}Build directory removed${NC}"
else
    echo -e "${BLUE}Build directory doesn't exist${NC}"
fi

if [ -f "CMakeCache.txt" ]; then
    rm -f CMakeCache.txt
    echo -e "${GREEN}Removed CMakeCache.txt${NC}"
fi

if [ -d "CMakeFiles" ]; then
    rm -rf CMakeFiles
    echo -e "${GREEN}Removed CMakeFiles directory${NC}"
fi

echo -e "${GREEN}Clean complete!${NC}"
