#!/bin/bash

# Loop through all .bson files in the current directory
for file in *.bson; do
    # Check if the file exists (in case no .bson files are found)
    [ -e "$file" ] || continue

    # Get the base filename without extension
    base_name="${file%.bson}"

    # Dump to .json
    bsondump "$file" > "${base_name}.json"
    echo "[INFO] Converted $file to ${base_name}.json"
done
