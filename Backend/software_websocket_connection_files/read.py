import bson

def read_bson_from_file(filename):
    try:
        # Open the BSON file in binary read mode
        with open(filename, "rb") as file:
            # Read the entire BSON file into a variable
            bson_data = file.read()
            
            # Deserialize the BSON data into Python dictionaries
            documents = bson.decode_all(bson_data)  # Use decode_all for multiple documents
            
            # Print the extracted documents
            print("Documents read from BSON file:")
            for document in documents:
                print(document)
                
                # Access specific fields if you want
                print("Exchange:", document.get("exchange", "N/A"))
                print("Price:", document.get("price", "N/A"))
                print("Currency:", document.get("currency", "N/A"))
    
    except Exception as e:
        print(f"[ERROR] Failed to read BSON file {filename}: {e}")



import bson

def check_bson_format(filename):
    try:
        # Open the BSON file in binary read mode
        with open(filename, "rb") as file:
            # Read the entire BSON file into a variable
            bson_data = file.read()

            # Attempt to decode the BSON data into Python dictionaries
            documents = bson.decode_all(bson_data)  # Use decode_all for multiple documents

            # If we successfully decoded the BSON, print a success message
            print(f"BSON file '{filename}' is properly formatted.")
            return True

    except Exception as e:
        print(f"[ERROR] Failed to read or decode BSON file {filename}: {e}")
        return False

# Test with your BSON file
# filename = "Coinbase_ticker_20250430.bson"
# if check_bson_format(filename):
#     print("BSON file is valid!")
# else:
#     print("BSON file is invalid or malformed.")


# # Example usage
read_bson_from_file("Huobi_ticker_20250430.bson")
