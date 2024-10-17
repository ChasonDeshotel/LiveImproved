#!/bin/bash

[ -z "$WINDOWS_USER" ] && { echo "WINDOWS_USER is not set"; exit 1; }
[ -z "$WINDOWS_IP" ] && { echo "WINDOWS_IP is not set"; exit 1; }
[ -z "$WINDOWS_SOURCE_DIR" ] && { echo "WINDOWS_SOURCE_DIR is not set"; exit 1; }

WINDOWS_OUTPUT_DIR="$WINDOWS_SOURCE_DIR/build/win"
FLAG_FILE="compilation_complete.flag"
MAX_WAIT_TIME=300  # Maximum wait time in seconds

# Sync the source code to the Windows machine (if using Git)
#git push

# Run the compilation script on the Windows machine
echo "Starting remote CMake and Ninja build process..."
ssh $WINDOWS_USER@$WINDOWS_IP "$WINDOWS_SOURCE_DIR\dev\win\compile.bat"

# Wait for the build to complete
##echo "Waiting for build to complete..."
##start_time=$(date +%s)
##while true; do
##    if ssh $WINDOWS_USER@$WINDOWS_IP "if exist $WINDOWS_OUTPUT_DIR\\$FLAG_FILE (exit 0) else (exit 1)"; then
##        echo "Build completed."
##        break
##    fi
##
##    current_time=$(date +%s)
##    elapsed_time=$((current_time - start_time))
##    if [ $elapsed_time -ge $MAX_WAIT_TIME ]; then
##        echo "Timeout: Build did not complete within $MAX_WAIT_TIME seconds."
##        exit 1
##    fi
##
##    echo "Still waiting... (${elapsed_time} seconds elapsed)"
##    sleep 5
##done

# Remove the flag file
#echo "Removing completion flag file..."
#ssh $WINDOWS_USER@$WINDOWS_IP "rm $WINDOWS_OUTPUT_DIR/$FLAG_FILE"
#
## Retrieve the CMake-generated compile_commands.json file
#echo "Retrieving CMake-generated compile_commands.json..."
#scp $WINDOWS_USER@$WINDOWS_IP:$WINDOWS_OUTPUT_DIR/compile_commands.json ./build
#
## Retrieve the build log
#echo "Retrieving compile_log.txt..."
#scp $WINDOWS_USER@$WINDOWS_IP:$WINDOWS_OUTPUT_DIR/compile_log.txt ./build

#echo "Files retrieved from Windows machine."
#
## Display the contents of the log file
#echo "Contents of compile_log.txt:"
#cat ./build/compile_log.txt

echo "Remote build process complete."
