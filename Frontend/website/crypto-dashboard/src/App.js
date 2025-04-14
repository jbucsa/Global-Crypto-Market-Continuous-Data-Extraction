import React, { useState, useEffect } from "react";
import useFetchData from "./components/UseFetchData";
import FilterBar from "./components/FilterBar";
import TickerTradeList from "./components/TickerTradeList";
import { filterData } from "./components/FilterJSON";
import { styles } from "./components/Styles";

function App() {
  const { tickerData, tradeData, lastUpdated } = useFetchData();

  const [darkMode, setDarkMode] = useState(() => {
    return localStorage.getItem("darkMode") === "true";
  });

  useEffect(() => {
    document.body.style.backgroundColor = darkMode ? "#121212" : "#ffffff";
    document.body.style.color = darkMode ? "#f1f1f1" : "#000000";
  }, [darkMode]);

  const toggleDarkMode = () => {
    const next = !darkMode;
    setDarkMode(next);
    localStorage.setItem("darkMode", next);
  };

  const setWithStorage = (setter, key) => (value) => {
    setter(value);
    localStorage.setItem(key, value);
  };

  const [view, setView] = useState(() => localStorage.getItem("view") || "all");
  const [exchangeFilter, setExchangeFilter] = useState(() => localStorage.getItem("exchangeFilter") || "");
  const [currencyFilter, setCurrencyFilter] = useState(() => localStorage.getItem("currencyFilter") || "");
  const [timeFilter, setTimeFilter] = useState(() => localStorage.getItem("timeFilter") || "all");

  const [tabs, setTabs] = useState(() => {
    const saved = localStorage.getItem("tabs");
    return saved ? JSON.parse(saved) : { ticker: "list", trades: "list" };
  });

  const updateTabs = (key, value) => {
    setTabs((prev) => {
      const updated = { ...prev, [key]: value };
      localStorage.setItem("tabs", JSON.stringify(updated));
      return updated;
    });
  };

  const allData = [...tickerData, ...tradeData];
  const currentStyles = styles(darkMode);

  return (
    <div style={currentStyles.container}>
      {/* Dark Mode Toggle */}
      <div style={{ display: "flex", justifyContent: "flex-end", marginBottom: "1rem" }}>
        <label style={currentStyles.toggleWrapper}>
          <input
            type="checkbox"
            checked={darkMode}
            onChange={toggleDarkMode}
            style={{ display: "none" }}
          />
          <div style={currentStyles.toggleTrack}>
            <div style={currentStyles.toggleThumb(darkMode)}>
              {darkMode ? "🌙" : "☀️"}
            </div>
          </div>
        </label>
      </div>

      {/* Header */}
      <h1 style={{ textAlign: "center" }}>Crypto Dashboard</h1>
      {lastUpdated && (
        <p style={{ textAlign: "center", color: darkMode ? "#bbb" : "#555" }}>
          <strong>Last Updated:</strong> {lastUpdated}
        </p>
      )}

      {/* Filters */}
      <FilterBar
        view={view}
        setView={setWithStorage(setView, "view")}
        exchangeFilter={exchangeFilter}
        setExchangeFilter={setWithStorage(setExchangeFilter, "exchangeFilter")}
        currencyFilter={currencyFilter}
        setCurrencyFilter={setWithStorage(setCurrencyFilter, "currencyFilter")}
        timeFilter={timeFilter}
        setTimeFilter={setWithStorage(setTimeFilter, "timeFilter")}
        allData={allData}
        darkMode={darkMode}
        styles={currentStyles}
      />

      {/* Data Display */}
      <TickerTradeList
        view={view}
        tickerData={filterData(tickerData, exchangeFilter, currencyFilter, timeFilter)}
        tradeData={filterData(tradeData, exchangeFilter, currencyFilter, timeFilter)}
        darkMode={darkMode}
        styles={currentStyles}
        tabs={tabs}
        setTabs={updateTabs}
      />
    </div>
  );
}

export default App;
