import { useState } from "react";
import WebSocketHandler from "./components/WebSocketHandler";
import ProbabilityBarChart from "./components/ProbabilityBarChart";
import RadarPointPlot from "./components/RadarPointPlot";
import "./index.css";

function App() {
  const [data, setData] = useState(null);
  const [isConnected, setIsConnected] = useState(false);
  const [fallEvents, setFallEvents] = useState([]);
  const [dailyFalls, setDailyFalls] = useState(0);
  const [lastFallTime, setLastFallTime] = useState(null);

  return (
    <div className="min-h-screen bg-gradient-to-b from-gray-50 to-white p-8 font-sans">
      <header className="flex items-center justify-between mb-8">
        <div>
          <h1 className="text-3xl font-extrabold">Fall Detection Dashboard</h1>
          <p className="text-gray-500 text-sm">Real-time fall monitoring system</p>
        </div>
        <button className="px-4 py-1 text-sm rounded-full bg-blue-100 text-blue-600 font-semibold shadow">
          Live Data
        </button>
      </header>

      <WebSocketHandler
        onData={(incoming) => {
          console.log("Received data:", incoming); // Debug log
          setData(incoming);
          setIsConnected(true);
          
          // Check if we have fall detection data
          if (incoming.fall_detection) {
            const fallData = incoming.fall_detection;
            if (fallData.is_fall) {
              setFallEvents(prev => [...prev, {
                time: new Date(),
                probability: fallData.fall_probability
              }]);
              setLastFallTime(new Date());
              setDailyFalls(prev => prev + 1);
            }
          }
        }}
      />

      <div className="grid grid-cols-3 gap-6 mb-10">
        <div className="rounded-2xl bg-white p-6 shadow-md flex flex-col justify-between">
          <h2 className="font-semibold text-gray-700 mb-1">Fall Status</h2>
          <p className="text-sm text-gray-400">Current fall detection status</p>
          <div className="mt-4 flex items-center justify-between">
            {data ? (
              <span className={`text-2xl font-bold ${data.fall_detection?.is_fall ? 'text-red-600' : 'text-green-600'}`}>
                {data.fall_detection?.is_fall ? 'FALL DETECTED!' : 'No Falls Detected'}
              </span>
            ) : (
              <span className="text-2xl font-bold text-gray-400">Waiting for data...</span>
            )}
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
          <h2 className="font-semibold text-gray-700 mb-1">Fall Statistics</h2>
          <p className="text-sm text-gray-400">Today's fall detection summary</p>
          <div className="mt-4 space-y-1 text-sm text-gray-500">
            <p className="flex justify-between">
              <span>Falls Today</span>
              <span className="font-semibold">{dailyFalls}</span>
            </p>
            <p className="flex justify-between">
              <span>Last Fall</span>
              <span className="font-semibold">{lastFallTime ? lastFallTime.toLocaleTimeString() : 'N/A'}</span>
            </p>
            <p className="flex justify-between">
              <span>Fall Probability</span>
              <span className="font-semibold text-blue-600">
                {data?.fall_detection?.fall_probability
                  ? `${(data.fall_detection.fall_probability * 100).toFixed(0)}%`
                  : "-"}
              </span>
            </p>
          </div>
        </div>
      </div>

      <div className="grid grid-cols-2 gap-6">
        <div className="rounded-2xl bg-white p-6 shadow-md">
          <h2 className="font-semibold text-gray-700 mb-1">Fall Detection Analysis</h2>
          <p className="text-sm text-gray-400 mb-4">Live analysis of fall detection</p>
          <ProbabilityBarChart 
            probabilities={data?.fall_detection ? [
              data.fall_detection.fall_probability,
              1 - data.fall_detection.fall_probability
            ] : undefined} 
          />
        </div>

        <div className="rounded-2xl bg-white p-6 shadow-md">
          <h2 className="font-semibold text-gray-700 mb-1">Point Cloud Visualization</h2>
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
