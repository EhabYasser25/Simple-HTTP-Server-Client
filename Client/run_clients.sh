#!/bin/bash

# Server address and port
SERVER_ADDRESS="127.0.0.1"
SERVER_PORT="80"

# Number of clients to run
NUM_CLIENTS=40

# Run 10 instances of the client in the background
for ((i=1; i<=NUM_CLIENTS; i++)); do
    ./my_client "$SERVER_ADDRESS" "$SERVER_PORT" &  # Replace with your actual client command
done

# Wait for all background processes to finish
wait

echo "All 60 clients have finished."
