import React from "react";
import DataGraph from "./DataGraph";

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

const ButtonGroup = ({ current, setTab, styles }) => {
  const buttons = [
    { key: "list", label: "List View" },
    { key: "graph", label: "Graph View" },
  ];

  return (
    <div style={{ ...styles.controlRow, justifyContent: "center" }}>
      {buttons.map(({ key, label }) => (
        <button
          key={key}
          onClick={() => setTab(key)}
          style={{
            ...styles.button,
            backgroundColor: current === key ? "#444" : "#ccc",
            color: current === key ? "#fff" : "#000",
            width: "auto",
            minWidth: "100px",
            padding: "0.4rem 0.75rem",
          }}
        >
          {label}
        </button>
      ))}
    </div>
  );
};

const DataSection = ({
  title,
  data,
  tab,
  setTab,
  fileName,
  isTrade,
  styles,
  darkMode,
}) => (
  <div style={styles.section}>
    <h2>{title}</h2>
    <ButtonGroup current={tab} setTab={setTab} styles={styles} />
    <button
      style={{ ...styles.button, marginBottom: "1rem" }}
      onClick={() => downloadJSON(data, fileName)}
    >
      Download {title}
    </button>

    {tab === "list" ? (
      data.length > 0 ? (
        data.map((entry, index) => (
          <div key={`${title}-${index}`} style={styles.item}>
            <strong>{entry.currency}</strong> | {entry.exchange} | ${entry.price}
            {isTrade && <> | size: {entry.size}</>}
            <br />
            <small style={{ color: "#666" }}>{entry.timestamp}</small>
          </div>
        ))
      ) : (
        <p>No matching {title.toLowerCase()}.</p>
      )
    ) : (
      <DataGraph
        data={data}
        title={`${title} Chart`}
        styles={styles}
        darkMode={darkMode}
      />
    )}
  </div>
);

function TickerTradeList({
  view,
  tickerData,
  tradeData,
  styles,
  darkMode,
  tabs,
  setTabs, // this is updateTabs(key, value)
}) {
  return (
    <>
      {(view === "all" || view === "ticker") && (
        <DataSection
          title="Ticker Data"
          data={tickerData}
          tab={tabs.ticker}
          setTab={(val) => setTabs("ticker", val)}
          fileName="ticker_data.json"
          isTrade={false}
          styles={styles}
          darkMode={darkMode}
        />
      )}

      {(view === "all" || view === "trades") && (
        <DataSection
          title="Trade Data"
          data={tradeData}
          tab={tabs.trades}
          setTab={(val) => setTabs("trades", val)}
          fileName="trade_data.json"
          isTrade={true}
          styles={styles}
          darkMode={darkMode}
        />
      )}
    </>
  );
}

export default TickerTradeList;
