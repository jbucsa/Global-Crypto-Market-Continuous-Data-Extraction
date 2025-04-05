import React, { useEffect, useState } from "react";

function App() {
  const [tickerData, setTickerData] = useState([]);
  const [tradeData, setTradeData] = useState([]);
  const [lastUpdated, setLastUpdated] = useState(null);
  const [view, setView] = useState("both");
  const [exchangeFilter, setExchangeFilter] = useState("");
  const [currencyFilter, setCurrencyFilter] = useState("");
  const [timeFilter, setTimeFilter] = useState("all");

  const fetchData = async () => {
    console.log("Fetching ticker and trade data...");
    setLastUpdated(new Date().toLocaleString());
  
    const parseNDJSON = (text) => {
      return text
        .trim()
        .split("\n")
        .map((line) => {
          try {
            return JSON.parse(line);
          } catch (err) {
            console.error("Bad JSON line:", line);
            return null;
          }
        })
        .filter(Boolean);
    };
  
    try {
      const [tickerRes, tradesRes] = await Promise.all([
        fetch("https://global-crypto-data-ext.duckdns.org/data/ticker_output_data.json"),
        fetch("https://global-crypto-data-ext.duckdns.org/data/trades_output_data.json"),
      ]);
  
      if (!tickerRes.ok || !tradesRes.ok) {
        throw new Error("Failed to fetch one or both NDJSON files.");
      }
  
      const [tickerText, tradesText] = await Promise.all([
        tickerRes.text(),
        tradesRes.text(),
      ]);
  
      const tickerJson = parseNDJSON(tickerText);
      const tradesJson = parseNDJSON(tradesText);
  
      setTickerData(tickerJson);
      setTradeData(tradesJson);
  
      console.log("Fetched and parsed NDJSON data.");
    } catch (err) {
      console.error("Fetch error:", err);
    }
  };  

  const downloadJSON = (data, filename) => {
    const blob = new Blob([JSON.stringify(data, null, 2)], {
      type: "application/json",
    });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
  };

  const parseTimestamp = (raw) => {
    const cleaned = raw.replace(" UTC", "").replace(/\.(\d{3})\d*/, ".$1") + "Z";
    return new Date(cleaned);
  };  

  const filterData = (data) => {
    const now = new Date();
    return data.filter((item) => {
      const time = parseTimestamp(item.timestamp);
      const minutesDiff = (now - time) / 60000;
  
      return (
        (!exchangeFilter || item.exchange === exchangeFilter) &&
        (!currencyFilter || item.currency === currencyFilter) &&
        (timeFilter === "all" || minutesDiff <= timeFilter)
      );
    });
  };  

  const getUniqueValues = (data, key) => {
    return [...new Set(data.map((item) => item[key]))].sort();
  };

  useEffect(() => {
    fetchData();
  }, []);

  const sectionStyle = {
    marginTop: "2rem",
    padding: "1rem",
    backgroundColor: "#ffffff",
    border: "1px solid #ddd",
    borderRadius: "12px",
    boxShadow: "0 4px 12px rgba(0,0,0,0.08)",
  };

  const itemStyle = {
    padding: "1rem",
    marginBottom: "1rem",
    background: "#f9fafc",
    borderRadius: "8px",
    border: "1px solid #d1d5db",
  };

  const buttonStyle = {
    padding: "0.6rem 1.2rem",
    margin: "0.5rem",
    border: "none",
    borderRadius: "8px",
    backgroundColor: "#2563eb",
    color: "white",
    fontWeight: "bold",
    cursor: "pointer",
    fontSize: "0.95rem",
  };

  const selectStyle = {
    padding: "0.4rem 0.6rem",
    marginLeft: "0.5rem",
    borderRadius: "6px",
    border: "1px solid #ccc",
    background: "#fff",
    fontSize: "0.95rem",
  };

  const allData = [...tickerData, ...tradeData];

  return (
    <div style={{ padding: "2rem", fontFamily: "Segoe UI, sans-serif", maxWidth: "900px", margin: "auto" }}>
      <h1 style={{ textAlign: "center" }}>Crypto Dashboard</h1>

      {lastUpdated && (
        <p style={{ textAlign: "center", color: "#555" }}>
          <strong>Last Updated:</strong> {lastUpdated}
        </p>
      )}

      <div style={{ marginBottom: "1.5rem", textAlign: "center" }}>
        <label>
          View:
          <select value={view} onChange={(e) => setView(e.target.value)} style={selectStyle}>
            <option value="both">Both</option>
            <option value="ticker">Ticker</option>
            <option value="trades">Trades</option>
          </select>
        </label>
        <label style={{ marginLeft: "1rem" }}>
          Exchange:
          <select value={exchangeFilter} onChange={(e) => setExchangeFilter(e.target.value)} style={selectStyle}>
            <option value="">All</option>
            {getUniqueValues(allData, "exchange").map((exchange) => (
              <option key={exchange} value={exchange}>{exchange}</option>
            ))}
          </select>
        </label>
        <label style={{ marginLeft: "1rem" }}>
          Currency:
          <select value={currencyFilter} onChange={(e) => setCurrencyFilter(e.target.value)} style={selectStyle}>
            <option value="">All</option>
            {getUniqueValues(allData, "currency").map((currency) => (
              <option key={currency} value={currency}>{currency}</option>
            ))}
          </select>
        </label>
        <label style={{ marginLeft: "1rem" }}>
          Time (min):
          <select value={timeFilter} onChange={(e) => { const val = e.target.value; setTimeFilter(val === "all" ? "all" : parseInt(val)); }} style={selectStyle}>
          <option value="all">All</option>
          <option value={1}>1</option>
          <option value={5}>5</option>
          <option value={10}>10</option>
          </select>
        </label>
      </div>

      {(view === "both" || view === "ticker") && (
        <div style={sectionStyle}>
          <h2>Ticker Data</h2>
          <button style={buttonStyle} onClick={() => downloadJSON(tickerData, "ticker_data.json")}>
            Download Ticker Data
          </button>
          {filterData(tickerData).length > 0 ? (
            filterData(tickerData).map((entry, index) => (
              <div key={`ticker-${index}`} style={itemStyle}>
                <strong>{entry.currency}</strong> | {entry.exchange} | ${entry.price}
                <br />
                <small style={{ color: "#666" }}>{entry.timestamp}</small>
              </div>
            ))
          ) : (
            <p>No matching ticker data.</p>
          )}
        </div>
      )}

      {(view === "both" || view === "trades") && (
        <div style={sectionStyle}>
          <h2>Trade Data</h2>
          <button style={buttonStyle} onClick={() => downloadJSON(tradeData, "trade_data.json")}>
            Download Trade Data
          </button>
          {filterData(tradeData).length > 0 ? (
            filterData(tradeData).map((entry, index) => (
              <div key={`trade-${index}`} style={itemStyle}>
                <strong>{entry.currency}</strong> | {entry.exchange} | ${entry.price} | size: {entry.size}
                <br />
                <small style={{ color: "#666" }}>{entry.timestamp}</small>
              </div>
            ))
          ) : (
            <p>No matching trade data.</p>
          )}
        </div>
      )}
    </div>
  );
}

export default App;