#!/bin/bash

# Set the port number.
PORT=${1:-7500}

# Change to area directory.
cd ../area || { echo "Directory ../area not found!"; exit 1; }

# Set limits
ulimit -c unlimited
rm -f shutdown.txt

while true; do
    # Find the next available log index
    INDEX=1000
    while [ -e "../log/$INDEX.log" ]; do
        ((INDEX++))
    done
    LOGFILE="../log/$INDEX.log"

    # Record starting time
    date > "$LOGFILE"
    date > boot.txt

    # Check if already running using ss (or netstat if preferred)
    if ss -tuln | grep -q ":$PORT "; then
        echo "Port $PORT is already in use."
        exit 0
    fi

    # Run AFKMud
    ../src/afkmud "$PORT" >> "$LOGFILE" 2>&1

    # Crash handling and GDB analysis
    if [ -f "core" ]; then
        mv core ../src
        cd ../src
        date > "../crash/$INDEX.crash"
        gdb -batch -x commands afkmud core >> "../crash/$INDEX.crash"
        cd ../area
    fi

    # Check for clean shutdown
    if [ -f "shutdown.txt" ]; then
        rm -f shutdown.txt
        exit 0
    fi

    # Wait before restarting
    sleep 5
done
