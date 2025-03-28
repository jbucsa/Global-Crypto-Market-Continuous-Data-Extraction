import React, { useEffect, useState } from "react";

function App() {
  const [tickerData, setTickerData] = useState([]);
  const [tradeData, setTradeData] = useState([]);
  const [lastUpdated, setLastUpdated] = useState(null);

  const fetchData = () => {
    console.log("Fetching ticker and trade data...");
    setLastUpdated(new Date().toLocaleString());

    fetch("https://4zzomsk66b.execute-api.us-east-2.amazonaws.com/data")
      .then((res) => {
        if (!res.ok) throw new Error(`HTTP error! status: ${res.status}`);
        return res.json();
      })
      .then((json) => {
        console.log("Fetched data:", json);
        setTickerData(json.ticker || []);
        setTradeData(json.trades || []);
      })
      .catch((err) => console.error("Fetch error:", err));
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

  useEffect(() => {
    fetchData();
    // const interval = setInterval(fetchData, 10000);
    // return () => clearInterval(interval);
  }, []);

  const sectionStyle = {
    marginTop: "2rem",
    padding: "1rem",
    backgroundColor: "#ffffff",
    border: "1px solid #ddd",
    borderRadius: "8px",
    boxShadow: "0 2px 5px rgba(0,0,0,0.05)",
  };

  const itemStyle = {
    padding: "1rem",
    marginBottom: "1rem",
    background: "#f9f9f9",
    borderRadius: "6px",
    border: "1px solid #e0e0e0",
  };

  const buttonStyle = {
    padding: "0.5rem 1rem",
    marginBottom: "1rem",
    marginRight: "1rem",
    border: "none",
    borderRadius: "4px",
    backgroundColor: "#007BFF",
    color: "white",
    cursor: "pointer",
  };

  return (
    <div style={{ padding: "2rem", fontFamily: "Arial, sans-serif", maxWidth: "900px", margin: "auto" }}>
      <h1 style={{ textAlign: "center" }}>Crypto Dashboard</h1>

      {lastUpdated && (
        <p style={{ textAlign: "center", color: "#555" }}>
          <strong>Last Updated:</strong> {lastUpdated}
        </p>
      )}

      <div style={sectionStyle}>
        <h2>Ticker Data</h2>
        <button style={buttonStyle} onClick={() => downloadJSON(tickerData, "ticker_data.json")}>
          Download Ticker Data
        </button>
        {tickerData.length > 0 ? (
          tickerData.map((entry, index) => (
            <div key={`ticker-${index}`} style={itemStyle}>
              <strong>{entry.currency}</strong> | {entry.exchange} | ${entry.price}
              <br />
              <small style={{ color: "#666" }}>{entry.timestamp}</small>
            </div>
          ))
        ) : (
          <p>Loading ticker data...</p>
        )}
      </div>

      <div style={sectionStyle}>
        <h2>Trade Data</h2>
        <button style={buttonStyle} onClick={() => downloadJSON(tradeData, "trade_data.json")}>
          Download Trade Data
        </button>
        {tradeData.length > 0 ? (
          tradeData.map((entry, index) => (
            <div key={`trade-${index}`} style={itemStyle}>
              <strong>{entry.currency}</strong> | {entry.exchange} | ${entry.price} | size: {entry.size}
              <br />
              <small style={{ color: "#666" }}>{entry.timestamp}</small>
            </div>
          ))
        ) : (
          <p>Loading trade data...</p>
        )}
      </div>
    </div>
  );
}

export default App;
