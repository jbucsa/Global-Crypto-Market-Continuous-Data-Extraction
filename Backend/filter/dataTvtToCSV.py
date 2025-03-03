import re
import csv

def process_input_to_csv(input_file, output_file):
    # Mapping for product replacements
    product_mapping = {
        'tBTCUSD': 'BTC-USD',
        'BTCUSDT': 'BTC-USD',
        'ADAUSDT': 'ADA-USD',
        'ETHUSDT': 'ETH-USD'
    }
    
    # Read and parse the input file
    data = []
    with open(input_file, 'r') as f:
        for line in f:
            stripped_line = line.strip()
            if not stripped_line:
                continue  # Skip blank lines
            # Use regex to extract components
            match = re.match(r'^\[(.*?)\]\[(.*?)\]\[(.*?)\]\s*Price:\s*(\d+\.?\d*)$', stripped_line)
            if not match:
                print(f"Skipping invalid line: {stripped_line}")
                continue
            time_val = match.group(1)
            exchange_val = match.group(2)
            product_val = match.group(3)
            # Apply product replacements
            product_val = product_mapping.get(product_val, product_val)
            price_val = match.group(4)
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
process_input_to_csv("Global-Crypto-Market-Continuous-Data-Extraction/Backend/software_websocket_connection_files/arbitrage_data.txt", "Global-Crypto-Market-Continuous-Data-Extraction/Backend/filter/output.csv")