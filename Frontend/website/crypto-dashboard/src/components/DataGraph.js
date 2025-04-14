import React from "react";
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  Tooltip,
  CartesianGrid,
  ResponsiveContainer,
} from "recharts";

function DataGraph({ data, dataKey = "price", title }) {
  if (!Array.isArray(data) || data.length === 0) {
    return <p>No data available for chart.</p>;
  }

  return (
    <div style={{ marginTop: "1rem" }}>
      <h2>{title}</h2>
      <ResponsiveContainer width="100%" height={300}>
        <LineChart data={data}>
          <CartesianGrid strokeDasharray="3 3" />
          <XAxis
            dataKey="timestamp"
            tickFormatter={(tick) => new Date(tick).toLocaleTimeString()}
          />
          <YAxis domain={["auto", "auto"]} />
          <Tooltip labelFormatter={(label) => new Date(label).toLocaleString()} />
          <Line
            type="monotone"
            dataKey={dataKey}
            stroke="#8884d8"
            dot={false}
            name="Price"
          />
        </LineChart>
      </ResponsiveContainer>
    </div>
  );
}

export default DataGraph;
