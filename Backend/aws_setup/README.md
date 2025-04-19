## CURRENTLY UNUSED

1. **S3 Bucket** – for storing the uploaded JSON files
2. **IAM User** – with S3 access
3. **AWS CLI** – installed and configured on your machine

---

## SETUP STEPS

### **1. Create an S3 Bucket**

- Go to: [https://s3.console.aws.amazon.com/s3/](https://s3.console.aws.amazon.com/s3/)
- Click **“Create bucket”**
- Name it: `crypto-json-storage` (or use your preferred name)
- Leave **Block all public access** checked (recommended)
- Click **Create bucket**

---

### **2. Create an IAM User for CLI Upload**

- Go to: [https://console.aws.amazon.com/iam/](https://console.aws.amazon.com/iam/)
- **Users > Add users**
  - Username: `jsonUploader`
  - Access type: **Programmatic access**
- Permissions: Attach policy → **AmazonS3FullAccess**
- Complete the setup and **save the Access Key & Secret Key**

---

### **3. Configure AWS CLI on Your Machine**

**Install AWS CLI:**

You have two options to install AWS CLI:

**Option 1: Use the pre-downloaded zip (e.g., `awscliv2.zip`)**

```bash
unzip awscliv2.zip
sudo ./aws/install
```

**Option 2: Download directly from AWS and install**

```bash
curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
unzip awscliv2.zip && sudo ./aws/install
```

**Set it up:**

```bash
aws configure
```

Enter the following:

- AWS Access Key ID
- AWS Secret Access Key
- Default region name: `us-east-2` *(or your preferred region)*
- Default output format: `json`

---

### **4. Upload JSON Files Manually**

To upload a single JSON file (e.g., `mydata.json`):

```bash
aws s3 cp ~/projects/mydata.json s3://crypto-json-storage/mydata.json
```

---

### **5. Automate Upload with Bash Script**

You can use the included script to continuously upload files at 5-second intervals:

**Script: `upload_to_s3.sh`**

```bash
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
```

**Make it executable and run:**

```bash
chmod +x upload_to_s3.sh
./upload_to_s3.sh
```