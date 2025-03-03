import re
import csv
from datetime import datetime

def process_input_to_csv(input_file, output_file):
    # Mapping for product replacements
    product_mapping = {
        'tBTCUSD': 'BTC-USD',
        'BTCUSDT': 'BTC-USD',
        'ADAUSDT': 'ADA-USD',
        'ETHUSDT': 'ETH-USD'
    }
    price_counters = {
        'ADA-USD': None,
        'BTC-USD': None,
        'ETH-USD': None
    }
    
    # Read and parse lines, extract timestamps for sorting
    lines_with_time = []
    with open(input_file, 'r') as f:
        for line in f:
            stripped_line = line.strip()
            if not stripped_line:
                continue  # Skip blank lines
            
            # Extract timestamp
            timestamp_match = re.match(r'^\[(.*?)\]', stripped_line)
            if not timestamp_match:
                print(f"Skipping line without valid timestamp: {stripped_line}")
                continue
            timestamp_str = timestamp_match.group(1)
            
            # Convert timestamp to datetime for sorting (handling 'Z' as UTC)
            try:
                if timestamp_str.endswith('Z'):
                    timestamp_str = timestamp_str[:-1] + '+00:00'
                dt = datetime.fromisoformat(timestamp_str)
            except ValueError as e:
                print(f"Skipping line with invalid timestamp {timestamp_str}: {e}")
                continue
            lines_with_time.append((dt, stripped_line))
    
    # Sort lines by timestamp
    lines_with_time.sort(key=lambda x: x[0])
    
    # Process each line in sorted order
    data = []
    for dt, line in lines_with_time:
        # Parse the line components
        match = re.match(r'^\[(.*?)\]\[(.*?)\]\[(.*?)\]\s*Price:\s*(\d+\.?\d*)$', line)
        if not match:
            print(f"Skipping invalid line: {line}")
            continue
        time_val = match.group(1)
        exchange_val = match.group(2)
        product_val = match.group(3)
        price_val = match.group(4)
        
        # Apply product replacements
        product_val = product_mapping.get(product_val, product_val)
        
        # Update counters or resolve unknown product
        if product_val in price_counters:
            # Update the counter for known product
            price_counters[product_val] = float(price_val)
        elif product_val == 'unknown':
            current_price = float(price_val)
            closest_product = None
            min_diff = float('inf')
            # Find closest product counter
            for product in price_counters:
                counter_price = price_counters[product]
                if counter_price is None:
                    continue
                diff = abs(current_price - counter_price)
                if diff < min_diff:
                    min_diff = diff
                    closest_product = product
            if closest_product is not None:
                product_val = closest_product
        
        # Append the processed data
        data.append((time_val, exchange_val, product_val, price_val))
    
    # Prepare rows with index starting at 1
    rows = []
    for index, (time, exchange, product, price) in enumerate(data, start=1):
        rows.append([index, time, exchange, product, price])
    
    # Write to CSV
    with open(output_file, 'w', newline='') as f_out:
        writer = csv.writer(f_out)
        writer.writerow(['index', 'time', 'exchange', 'product', 'price'])
        writer.writerows(rows)



# Example usage (modify filenames as needed)
process_input_to_csv("Global-Crypto-Market-Continuous-Data-Extraction/Backend/software_websocket_connection_files/arbitrage_data.txt", "Global-Crypto-Market-Continuous-Data-Extraction/Backend/filter/outputPython.csv")