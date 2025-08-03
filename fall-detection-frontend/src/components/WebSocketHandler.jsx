import { useEffect, useState } from "react";

export default function WebSocketHandler({ onData }) {
  const [status, setStatus] = useState("Connecting...");

  useEffect(() => {
    const ws = new WebSocket("ws://18.191.154.31:8000/ws");

    ws.onopen = () => setStatus("Websocket Connected");

    ws.onmessage = (event) => {
      try {
        const parsed = JSON.parse(event.data);
        onData(parsed);
      } catch (err) {
        console.error("Invalid JSON from WebSocket:", err);
      }
    };

    ws.onclose = () => setStatus("Websocket Disconnected");

    ws.onerror = (err) => {
      console.error("WebSocket error:", err);
      setStatus("⚠️ Error");
    };

    return () => ws.close();
  }, [onData]);

  return <div className="invisible">{status}</div>;
}

