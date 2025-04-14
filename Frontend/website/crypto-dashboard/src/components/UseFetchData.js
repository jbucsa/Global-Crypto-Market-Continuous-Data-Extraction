import { useEffect, useState } from "react";

function parseNDJSON(text) {
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
}

function useFetchData() {
  const [tickerData, setTickerData] = useState([]);
  const [tradeData, setTradeData] = useState([]);
  const [lastUpdated, setLastUpdated] = useState(null);
  const [error, setError] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    const fetchData = async () => {
      setLoading(true);
      setLastUpdated(new Date().toLocaleString());
      setError(null);

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

        setTickerData(parseNDJSON(tickerText));
        setTradeData(parseNDJSON(tradesText));

        console.log("Fetched and parsed NDJSON data.");
      } catch (err) {
        console.error("Fetch error:", err);
        setError(err.message || "Unknown fetch error");
      } finally {
        setLoading(false);
      }
    };

    fetchData();

    // To enable polling every 10s, uncomment below:
    // const interval = setInterval(fetchData, 10000);
    // return () => clearInterval(interval);
  }, []);

  return {
    tickerData,
    tradeData,
    lastUpdated,
    loading,
    error,
  };
}

export default useFetchData;
