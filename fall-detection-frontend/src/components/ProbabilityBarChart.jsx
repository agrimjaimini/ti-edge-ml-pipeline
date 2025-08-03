import { BarChart, Bar, XAxis, YAxis, Tooltip, CartesianGrid, Legend } from "recharts";

export default function ProbabilityBarChart({ probabilities }) {
  // If we don't have valid probabilities, show empty data
  if (!probabilities || !probabilities.length) {
    return (
      <div className="p-4 bg-white rounded shadow">
        <h2 className="text-xl font-semibold mb-2">Fall Detection Probability</h2>
        <div className="h-[400px] flex items-center justify-center text-gray-500">
          Waiting for data...
        </div>
      </div>
    );
  }

  const data = [
    {
      name: "Fall",
      probability: probabilities[0]
    },
    {
      name: "No Fall",
      probability: probabilities[1]
    }
  ];

  return (
    <div className="p-4 bg-white rounded shadow">
      <h2 className="text-xl font-semibold mb-2">Fall Detection Probability</h2>
      <BarChart width={500} height={400} data={data}>
        <CartesianGrid strokeDasharray="3 3" />
        <XAxis dataKey="name" />
        <YAxis domain={[0, 1]} tickFormatter={(value) => `${(value * 100)}%`} />
        <Tooltip 
          formatter={(value) => `${(value * 100).toFixed(1)}%`}
          labelStyle={{ color: '#666' }}
        />
        <Legend />
        <Bar dataKey="probability" fill="#3182ce" name="Probability" />
      </BarChart>
    </div>
  );
}
