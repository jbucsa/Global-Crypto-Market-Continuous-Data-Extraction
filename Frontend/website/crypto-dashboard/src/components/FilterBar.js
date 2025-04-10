import React from "react";

function getUniqueValues(data, key) {
  return [...new Set(data.map((item) => item[key]))].sort();
}

function FilterBar({
    view,
    setView,
    exchangeFilter,
    setExchangeFilter,
    currencyFilter,
    setCurrencyFilter,
    timeFilter,
    setTimeFilter,
    allData,
    styles
  }) {
  
  return (
    <div style={{ marginBottom: "1.5rem", textAlign: "center" }}>
      <label>
        View:
        <select value={view} onChange={(e) => setView(e.target.value)} style={styles.select}>
          <option value="all">All</option>
          <option value="ticker">Ticker</option>
          <option value="trades">Trades</option>
        </select>
      </label>
      <label style={{ marginLeft: "1rem" }}>
        Exchange:
        <select value={exchangeFilter} onChange={(e) => setExchangeFilter(e.target.value)} style={styles.select}>
          <option value="">All</option>
          {getUniqueValues(allData, "exchange").map((exchange) => (
            <option key={exchange} value={exchange}>{exchange}</option>
          ))}
        </select>
      </label>
      <label style={{ marginLeft: "1rem" }}>
        Currency:
        <select value={currencyFilter} onChange={(e) => setCurrencyFilter(e.target.value)} style={styles.select}>
          <option value="">All</option>
          {getUniqueValues(allData, "currency").map((currency) => (
            <option key={currency} value={currency}>{currency}</option>
          ))}
        </select>
      </label>
      <label style={{ marginLeft: "1rem" }}>
        Time (min):
        <select
          value={timeFilter}
          onChange={(e) => {
            const val = e.target.value;
            setTimeFilter(val === "all" ? "all" : parseInt(val));
          }}
          style={styles.select}
        >
          <option value="all">All</option>
          <option value={1}>1</option>
          <option value={5}>5</option>
          <option value={10}>10</option>
        </select>
      </label>
    </div>
  );
}

export default FilterBar;
