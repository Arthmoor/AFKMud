#! /bin/bash
# Set the port number.

if [ -n $1 ]
 then port=$1
    echo "I am now listening on port $port as you specified."
else
 port=9500
    echo "You did not specify a port to run on. I have set it to $port. To specify a port number, port 9500 for example, please use nohup ./startup 9500"
fi

echo listening on port $port

# Change to area directory.
cd ../area

# Initial shutdown.txt file check and removal.
if [ -e shutdown.txt ]  # Start Clean.
 then
  rm -f shutdown.txt
  echo "Initial shutdown file removal machine"
 else
  echo "No shutdown file found. Continuing..."
fi

# Check if port specified is available.
echo checking port $port
mandible=$(netstat -an | grep :$port | grep -c LISTEN)

    if [ $mandible -gt 0 ]
      then
    echo Port $port is already in use.
        exit 0
      else
    echo "Port check success! $port is not already in use"
    fi

while [ 1 ]
do
    # Run AFKMud.
    ../src/afkmud "$port"

    # Restart, giving old connections a chance to die.
    if [ -e shutdown.txt ]
     then
    exit 0
    fi
sleep 5
done