#!/bin/bash

DIRECTORY=.

# Patterns of files to delete
PATTERNS=("*.ko" "*.cmd" "*.o" "*.mod.c" "*.mod" "*.mod.o" "Module.symvers" "modules.order")

# Loop through each pattern and delete matching files
for pattern in "${PATTERNS[@]}"; do
    find "$DIRECTORY" -type f -name "$pattern" -exec rm -f {} +
done

echo "Deletion complete."
