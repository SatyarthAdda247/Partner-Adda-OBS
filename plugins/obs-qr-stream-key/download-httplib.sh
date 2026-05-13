#!/bin/bash

# Download cpp-httplib header
echo "Downloading cpp-httplib..."
curl -L -o httplib.h https://raw.githubusercontent.com/yhirose/cpp-httplib/master/httplib.h

if [ $? -eq 0 ]; then
    echo "✓ Successfully downloaded httplib.h"
else
    echo "✗ Failed to download httplib.h"
    exit 1
fi
