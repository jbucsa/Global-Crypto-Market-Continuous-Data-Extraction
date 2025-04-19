# DuckDNS Update Script

This script updates your DuckDNS subdomain with your current public IP address. It's useful for keeping DNS records in sync when using a dynamic IP (common for residential internet connections).

---

## What This Does

Many home internet connections do not have a fixed IP address. DuckDNS is a free dynamic DNS service that lets you point a domain (e.g., `your-subdomain.duckdns.org`) to your current IP. This script regularly updates DuckDNS with your IP address so your domain always resolves correctly.

---

## What You Need Before Using

- A registered DuckDNS account: [https://www.duckdns.org](https://www.duckdns.org)
- A DuckDNS subdomain (e.g., `global-crypto-data-ext`)
- Your unique DuckDNS token (found on your DuckDNS dashboard)
- `curl` installed on your system
- Optional: access to `crontab` for scheduling updates automatically

---

## Setup Instructions

### **1. Create a Directory for DuckDNS Files**
Create a local directory to keep your update script and logs:

```bash
mkdir -p ~/duckdns
cd ~/duckdns
```

### **2. Create the Update Script**
Create a script file named `duck.sh`:

```bash
echo url="https://www.duckdns.org/update?domains=global-crypto-data-ext&token=959b6331-fc42-4c34-ba30-f6627abd1255&ip=" | curl -k -o ~/duckdns/duck.log -K -
```

Make the script executable:

```bash
chmod +x duck.sh
```

This script uses `curl` to send an update request to DuckDNS. The `-k` flag allows connections to SSL sites without certificates. Output is saved to `~/duckdns/duck.log`.

---

### **3. Schedule Automatic Updates with Cron**

To run this script every 5 minutes:

1. Open your crontab:
```bash
crontab -e
```

2. Add this line to the file:
```bash
*/5 * * * * ~/duckdns/duck.sh >/dev/null 2>&1
```

This line ensures the script runs every 5 minutes silently (no terminal output).

---

## Notes
- Replace the `token` and `domains` parameters in the script with your own values from [https://www.duckdns.org](https://www.duckdns.org).
- Make sure `curl` is installed on your system: you can check with `which curl`.
- The `duck.log` file will record the update status for troubleshooting.

You're now set up for continuous IP updates to your DuckDNS domain.