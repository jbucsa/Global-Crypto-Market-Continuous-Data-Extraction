function parseTimestamp(raw) {
    const cleaned = raw.replace(" UTC", "").replace(/\.(\d{3})\d*/, ".$1") + "Z";
    return new Date(cleaned);
  }
  
  export function filterData(data, exchangeFilter, currencyFilter, timeFilter) {
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
  }
  