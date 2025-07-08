import { useEffect, useState } from "react";

const FallMonitor = () => {
  const [probability, setProbability] = useState(null);
  const [fallDetected, setFallDetected] = useState(false);
  const [status, setStatus] = useState("Disconnected");

  useEffect(() => {
    const ws = new WebSocket("ws://localhost:8000/ws/fall");

    ws.onopen = () => {
      setStatus("Connected ✅");
      // Simulate sensor data every 3s
      const interval = setInterval(() => {
        const dummy = {
          sensor_data: Array.from({ length: 5 }, () => (Math.random() - 0.5) * 2),
        };
        ws.send(JSON.stringify(dummy));
      }, 3000);
      return () => clearInterval(interval);
    };

    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      setProbability(data.probability);
      setFallDetected(data.fall_detected);
    };

    ws.onclose = () => {
      setStatus("Disconnected ❌");
    };

    return () => {
      ws.close();
    };
  }, []);

  return (
    <div style={{ padding: "20px", fontFamily: "sans-serif" }}>
      <h1>Fall Detection Dashboard</h1>
      <p>Status: {status}</p>
      <h2>
        Probability:{" "}
        {probability !== null ? (probability * 100).toFixed(2) + "%" : "--"}
      </h2>
      {fallDetected && (
        <div style={{ color: "red", fontWeight: "bold", fontSize: "20px" }}>
          🚨 FALL DETECTED!
        </div>
      )}
    </div>
  );
};

export default FallMonitor;