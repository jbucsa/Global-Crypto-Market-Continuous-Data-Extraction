#!/bin/bash

while true
do
    echo "Uploading files at $(date)..."

    # Upload ticker data
    aws s3 cp ../software_websocket_connection_files/ticker_output_data.json s3://crypto-json-storage/

    # Upload trade data
    aws s3 cp ../software_websocket_connection_files/trades_output_data.json s3://crypto-json-storage/

    echo "Upload complete. Waiting 5 seconds..."
    echo ""
    sleep 5
done

# run 'chmod +x upload_to_s3.sh' to make executable
# run './upload_to_s3.sh' to execute