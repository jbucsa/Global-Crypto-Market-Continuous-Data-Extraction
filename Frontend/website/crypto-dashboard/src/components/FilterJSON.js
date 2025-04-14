function parseTimestamp(raw) {
  if (!raw) return new Date(NaN);
  const cleaned = raw
    .replace(" UTC", "")
    .replace(/\.(\d{3})\d*/, ".$1") + "Z";
  return new Date(cleaned);
}

export function filterData(data, exchangeFilter, currencyFilter, timeFilter) {
  const now = new Date();

  return data.filter((item) => {
    const time = parseTimestamp(item.timestamp);
    if (isNaN(time)) return false;

    const minutesAgo = (now - time) / 60000;

    const exchangeMatch = !exchangeFilter || item.exchange === exchangeFilter;
    const currencyMatch = !currencyFilter || item.currency === currencyFilter;
    const timeMatch = timeFilter === "all" || minutesAgo <= timeFilter;

    return exchangeMatch && currencyMatch && timeMatch;
  });
}
