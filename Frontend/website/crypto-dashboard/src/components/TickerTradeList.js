import React from "react";

function downloadJSON(data, filename) {
  const blob = new Blob([JSON.stringify(data, null, 2)], {
    type: "application/json",
  });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}

function TickerTradeList({ view, tickerData, tradeData, styles }) {
  return (
    <>
      {(view === "all" || view === "ticker") && (
        <div style={styles.section}>
          <h2>Ticker Data</h2>
          <button style={styles.button} onClick={() => downloadJSON(tickerData, "ticker_data.json")}>
            Download Ticker Data
          </button>
          {tickerData.length > 0 ? (
            tickerData.map((entry, index) => (
              <div key={`ticker-${index}`} style={styles.item}>
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

      {(view === "all" || view === "trades") && (
        <div style={styles.section}>
          <h2>Trade Data</h2>
          <button style={styles.button} onClick={() => downloadJSON(tradeData, "trade_data.json")}>
            Download Trade Data
          </button>
          {tradeData.length > 0 ? (
            tradeData.map((entry, index) => (
              <div key={`trade-${index}`} style={styles.item}>
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
    </>
  );
}

export default TickerTradeList;
