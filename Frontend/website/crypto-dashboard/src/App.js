// App.js
import React, { useState } from "react";
import useFetchData from "./components/UseFetchData";
import FilterBar from "./components/FilterBar";
import TickerTradeList from "./components/TickerTradeList";
import { filterData } from "./components/FilterJSON";
import { styles } from "./components/Styles";
import { useEffect } from "react";

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

  const [view, setView] = useState(() => localStorage.getItem("view") || "all");
  const [exchangeFilter, setExchangeFilter] = useState(() => localStorage.getItem("exchangeFilter") || "");
  const [currencyFilter, setCurrencyFilter] = useState(() => localStorage.getItem("currencyFilter") || "");
  const [timeFilter, setTimeFilter] = useState(() => localStorage.getItem("timeFilter") || "all");

  const handleSetView = (val) => {
    setView(val);
    localStorage.setItem("view", val);
  };

  const handleSetExchangeFilter = (val) => {
    setExchangeFilter(val);
    localStorage.setItem("exchangeFilter", val);
  };

  const handleSetCurrencyFilter = (val) => {
    setCurrencyFilter(val);
    localStorage.setItem("currencyFilter", val);
  };

  const handleSetTimeFilter = (val) => {
    setTimeFilter(val);
    localStorage.setItem("timeFilter", val);
  };

  const allData = [...tickerData, ...tradeData];
  const currentStyles = styles(darkMode);

  return (
    <div style={currentStyles.container}>
      <div style={{ display: "flex", justifyContent: "flex-end", marginBottom: "1rem" }}>
        <label style={currentStyles.toggleWrapper}>
          <input type="checkbox" checked={darkMode} onChange={toggleDarkMode} style={{ display: "none" }} />
          <div style={currentStyles.toggleTrack}>
            <div style={currentStyles.toggleThumb(darkMode)}>
              {darkMode ? "🌙" : "☀️"}
            </div>
          </div>
        </label>
      </div>
      <h1 style={{ textAlign: "center" }}>Crypto Dashboard</h1>
      {lastUpdated && (
        <p style={{ textAlign: "center", color: darkMode ? "#bbb" : "#555" }}>
          <strong>Last Updated:</strong> {lastUpdated}
        </p>
      )}
      <FilterBar
        view={view}
        setView={handleSetView}
        exchangeFilter={exchangeFilter}
        setExchangeFilter={handleSetExchangeFilter}
        currencyFilter={currencyFilter}
        setCurrencyFilter={handleSetCurrencyFilter}
        timeFilter={timeFilter}
        setTimeFilter={handleSetTimeFilter}
        allData={allData}
        darkMode={darkMode}
        styles={currentStyles}
      />
      <TickerTradeList
        view={view}
        tickerData={filterData(tickerData, exchangeFilter, currencyFilter, timeFilter)}
        tradeData={filterData(tradeData, exchangeFilter, currencyFilter, timeFilter)}
        darkMode={darkMode}
        styles={currentStyles}
      />
    </div>
  );
}

export default App;
