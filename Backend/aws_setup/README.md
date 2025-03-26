1. **S3 Bucket** – for storing the uploaded JSON file  
2. **IAM User** – with S3 access  
3. **AWS CLI** – installed and configured on your machine  
---

## SETUP STEPS:

### **1. Create an S3 Bucket**
- Go to: https://s3.console.aws.amazon.com/s3/
- Click **“Create bucket”**
- Name it: `my-json-storage` (or your own name)
- Leave **Block all public access** checked (recommended)
- Click **Create bucket**

---

### **2. Create an IAM User for CLI Upload**
- Go to: https://console.aws.amazon.com/iam/
- **Users > Add users**
  - Username: `jsonUploader`
  - Access: Programmatic access
- Permissions: Attach policy → **AmazonS3FullAccess**
- Click through and **save the Access Key & Secret Key**

---

### **3. Configure AWS CLI on Your Machine**
Install AWS CLI:
```bash
# Linux
curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o "awscliv2.zip"
unzip awscliv2.zip && sudo ./aws/install
```

Set it up:
```bash
aws configure
```
Enter:
- AWS Access Key ID
- AWS Secret Access Key
- Region: `us-east-2` *(or your chosen region)*
- Output format: `json`

---

### **4. Upload JSON File to S3**
Let’s say your file is at: `~/projects/mydata.json`

Run:
```bash
aws s3 cp ~/projects/mydata.json s3://my-json-storage/mydata.json
```
