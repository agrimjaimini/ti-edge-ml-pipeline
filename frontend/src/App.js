import { useState } from "react";
import WebSocketHandler from "./components/WebSocketHandler";
import ProbabilityBarChart from "./components/ProbabilityBarChart";
import RadarPointPlot from "./components/RadarPointPlot";
import "./index.css";

function App() {
  const [data, setData] = useState(null);
  const [isConnected, setIsConnected] = useState(false);
  const [allCounts, setAllCounts] = useState([]);
  const [dailyAverage, setDailyAverage] = useState(0);
  const [maxOccupancy, setMaxOccupancy] = useState(0);
  const [smoothedProbabilities, setSmoothedProbabilities] = useState(null);
  const [probabilityHistory, setProbabilityHistory] = useState([]);
  
  // EMA parameters
  const ALPHA = 0.05;  // More smoothing
  const WINDOW_SIZE = 15;  // Larger window

  const calculateEMA = (newProbs) => {
    if (!newProbs) return null;

    // Add new probabilities to history
    setProbabilityHistory(prev => {
      const updated = [...prev, newProbs];
      // Keep only last WINDOW_SIZE frames
      return updated.slice(-WINDOW_SIZE);
    });

    // If we don't have enough history, return current probabilities
    if (probabilityHistory.length < 2) {
      return newProbs;
    }

    // Calculate EMA for each probability
    const prevEMA = smoothedProbabilities || newProbs;
    return newProbs.map((prob, idx) => {
      return ALPHA * prob + (1 - ALPHA) * prevEMA[idx];
    });
  };

  return (
    <div className="min-h-screen bg-gradient-to-b from-gray-50 to-white p-8 font-sans">
      <header className="flex items-center justify-between mb-8">
        <div>
          <h1 className="text-3xl font-extrabold">Occupancy Dashboard</h1>
          <p className="text-gray-500 text-sm">Real-time occupancy monitoring system</p>
        </div>
        <button className="px-4 py-1 text-sm rounded-full bg-blue-100 text-blue-600 font-semibold shadow">
          Live Data
        </button>
      </header>

      <WebSocketHandler
        onData={(incoming) => {
          setData(incoming);
          setIsConnected(true);
          const count = incoming.occupancy.predicted_count;
          setAllCounts((prev) => {
            const updated = [...prev, count];
            setDailyAverage(updated.reduce((a, b) => a + b, 0) / updated.length);
            setMaxOccupancy(Math.max(...updated));
            return updated;
          });
          
          // Calculate smoothed probabilities
          const newSmoothed = calculateEMA(incoming.occupancy.probabilities);
          setSmoothedProbabilities(newSmoothed);
        }}
      />

      <div className="grid grid-cols-3 gap-6 mb-10">
        <div className="rounded-2xl bg-white p-6 shadow-md flex flex-col justify-between">
          <h2 className="font-semibold text-gray-700 mb-1">People Detected</h2>
          <p className="text-sm text-gray-400">Current room occupancy</p>
          <div className="mt-4 flex items-center justify-between">
            <span className="text-5xl font-bold text-blue-600">{data?.occupancy.predicted_count ?? 0}</span>
            <span className="px-3 py-1 rounded-full bg-blue-100 text-blue-600 text-sm font-semibold">Live</span>
          </div>
        </div>

        <div className="rounded-2xl bg-white p-6 shadow-md flex flex-col justify-between">
  <h2 className="font-semibold text-gray-700 mb-2">System Status</h2>
  <p className="text-sm text-gray-400 mb-4">Sensor and system health</p>
  
  <div className="space-y-3 text-sm text-gray-500">
    <div className={`flex items-center justify-between p-2 rounded-lg ${isConnected ? 'bg-green-50' : 'bg-red-50'}`}>
      <span className="font-medium text-gray-600">WebSocket Connection</span>
      <span
        className={`px-3 py-1 rounded-full text-xs font-semibold ${
          isConnected ? "bg-green-100 text-green-600" : "bg-red-100 text-red-500"
        }`}
      >
        {isConnected ? "Active" : "Disconnected"}
      </span>
    </div>

    <div className="flex items-center justify-between">
      <span className="font-medium text-gray-600">Last Update</span>
      <span className="text-gray-600">Just now</span>
    </div>
  </div>
</div>


        <div className="rounded-2xl bg-white p-6 shadow-md flex flex-col justify-between">
          <h2 className="font-semibold text-gray-700 mb-1">Data Summary</h2>
          <p className="text-sm text-gray-400">Today's occupancy trends</p>
          <div className="mt-4 space-y-1 text-sm text-gray-500">
            <p className="flex justify-between">
              <span>Daily Average</span>
              <span className="font-semibold">{dailyAverage.toFixed(1)}</span>
            </p>
            <p className="flex justify-between">
              <span>Max Occupancy</span>
              <span className="font-semibold">{maxOccupancy}</span>
            </p>
            <p className="flex justify-between">
              <span>Confidence</span>
              <span className="font-semibold text-green-600">
                {data?.occupancy?.probabilities
                  ? `${(Math.max(...data.occupancy.probabilities) * 100).toFixed(0)}%`
                  : "-"}
              </span>
            </p>
          </div>
        </div>
      </div>

      <div className="grid grid-cols-2 gap-6">
        <div className="rounded-2xl bg-white p-6 shadow-md">
          <h2 className="font-semibold text-gray-700 mb-1"></h2>
          <p className="text-sm text-gray-400 mb-4">Live analysis of detected objects</p>
          <ProbabilityBarChart 
            probabilities={data?.occupancy?.probabilities || []} 
            smoothedProbabilities={smoothedProbabilities || []}
          />
        </div>

        <div className="rounded-2xl bg-white p-6 shadow-md">
          <h2 className="font-semibold text-gray-700 mb-1"></h2>
          <p className="text-sm text-gray-400 mb-4">3D visualization of detected points</p>
          <RadarPointPlot
            x={data?.point_data?.x_pos || []}
            y={data?.point_data?.y_pos || []}
            z={data?.point_data?.z_pos || []}
          />
        </div>
      </div>
    </div>
  );
}

export default App;
