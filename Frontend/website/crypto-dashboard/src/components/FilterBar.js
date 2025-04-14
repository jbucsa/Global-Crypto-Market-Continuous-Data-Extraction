import React from "react";

function getUniqueValues(data, key) {
  return [...new Set(data.map((item) => item[key]))].sort();
}

const Dropdown = ({ label, value, onChange, options, styles }) => (
  <label style={{ margin: "0.5rem" }}>
    {label}
    <select value={value} onChange={onChange} style={styles.select}>
      {options.map(({ label, value }) => (
        <option key={value ?? label} value={value}>
          {label}
        </option>
      ))}
    </select>
  </label>
);

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
  styles,
}) {
  const exchangeOptions = [
    { label: "All", value: "" },
    ...getUniqueValues(allData, "exchange").map((val) => ({ label: val, value: val })),
  ];

  const currencyOptions = [
    { label: "All", value: "" },
    ...getUniqueValues(allData, "currency").map((val) => ({ label: val, value: val })),
  ];

  const timeOptions = [
    { label: "All", value: "all" },
    { label: "1", value: 1 },
    { label: "5", value: 5 },
    { label: "10", value: 10 },
  ];

  return (
    <div style={{ marginBottom: "1.5rem", textAlign: "center" }}>
      <div style={{ ...styles.controlRow, justifyContent: "center", flexWrap: "wrap" }}>
        <Dropdown
          label="View:"
          value={view}
          onChange={(e) => setView(e.target.value)}
          options={[
            { label: "All", value: "all" },
            { label: "Ticker", value: "ticker" },
            { label: "Trades", value: "trades" },
          ]}
          styles={styles}
        />

        <Dropdown
          label="Exchange:"
          value={exchangeFilter}
          onChange={(e) => setExchangeFilter(e.target.value)}
          options={exchangeOptions}
          styles={styles}
        />

        <Dropdown
          label="Currency:"
          value={currencyFilter}
          onChange={(e) => setCurrencyFilter(e.target.value)}
          options={currencyOptions}
          styles={styles}
        />

        <Dropdown
          label="Time (min):"
          value={timeFilter}
          onChange={(e) =>
            setTimeFilter(e.target.value === "all" ? "all" : parseInt(e.target.value))
          }
          options={timeOptions}
          styles={styles}
        />
      </div>
    </div>
  );
}

export default FilterBar;
