import { BarChart, Bar, XAxis, YAxis, Tooltip, CartesianGrid, Legend } from "recharts";

export default function ProbabilityBarChart({ probabilities, smoothedProbabilities }) {
  const data = probabilities.map((p, index) => ({
    name: `${index} People`,
    raw: p,
    smoothed: smoothedProbabilities?.[index] || p
  }));

  return (
    <div className="p-4 bg-white rounded shadow">
      <h2 className="text-xl font-semibold mb-2">Classification Probabilities</h2>
      <BarChart width={500} height={400} data={data}>
        <CartesianGrid strokeDasharray="3 3" />
        <XAxis dataKey="name" />
        <YAxis domain={[0, 1]} tickFormatter={(value) => `${(value * 100)}%`} />
        <Tooltip 
          formatter={(value) => `${(value * 100).toFixed(1)}%`}
          labelStyle={{ color: '#666' }}
        />
        <Legend />
        <Bar dataKey="raw" fill="#3182ce" name="Raw Probability" />
        <Bar dataKey="smoothed" fill="#34d399" name="Smoothed Probability" />
      </BarChart>
    </div>
  );
}
