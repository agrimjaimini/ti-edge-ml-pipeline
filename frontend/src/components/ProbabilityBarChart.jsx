import { BarChart, Bar, XAxis, YAxis, Tooltip, CartesianGrid, Legend } from "recharts";

export default function ProbabilityBarChart({ probabilities, smoothedProbabilities }) {
  const data = probabilities.map((p, index) => ({
    name: `Class ${index + 1}`,
    current: p,
    smoothed: smoothedProbabilities[index] || p,
  }));

  return (
    <div className="p-4 bg-white rounded shadow">
      <h2 className="text-xl font-semibold mb-2">Classification Probabilities</h2>
      <BarChart width={500} height={400} data={data}>
        <CartesianGrid strokeDasharray="3 3" />
        <XAxis dataKey="name" />
        <YAxis domain={[0, 1]} />
        <Tooltip 
          formatter={(value, name) => [
            `${(value * 100).toFixed(1)}%`, 
            name === 'current' ? 'Current' : 'Smoothed'
          ]}
        />
        <Legend />
        <Bar dataKey="current" fill="#3182ce" name="Current" />
        <Bar dataKey="smoothed" fill="#9f7aea" name="Smoothed (EMA)" />
      </BarChart>
    </div>
  );
}
