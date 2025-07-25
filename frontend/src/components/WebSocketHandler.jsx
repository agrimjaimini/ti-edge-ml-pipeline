import { useEffect, useState } from "react";

export default function WebSocketHandler({ onData }) {
  const [status, setStatus] = useState("Connecting…");

  useEffect(() => {
    const ws = new WebSocket("ws://18.227.26.179:8080/ws");

    ws.onopen = () => setStatus("Connected");

    ws.onmessage = (event) => {
      const data = JSON.parse(event.data);
      onData(data);
    };

    ws.onclose = () => setStatus("Disconnected");

    ws.onerror = () => setStatus("⚠️ Error");

    return () => ws.close();
  }, [onData]);

  return <div className="text-lg mb-4">{status}</div>;
}

// import { useEffect, useState } from "react";

// export default function WebSocketHandler({ onData }) {
//   const [status, setStatus] = useState("Connecting...");

//   useEffect(() => {
//     const ws = new WebSocket("ws://3.141.30.137:8080/ws");
    

//     ws.onopen = () => setStatus("Websocket Connected");

//     ws.onmessage = (event) => {
//       try {
//         const parsed = JSON.parse(event.data);
//         onData(parsed);
//       } catch (err) {
//         console.error("Invalid JSON from WebSocket:", err);
//       }
//     };

//     ws.onclose = () => setStatus("Websocket Disconnected");

//     ws.onerror = (err) => {
//       console.error("WebSocket error:", err);
//       setStatus("⚠️ Error");
//     };

//     return () => ws.close();
//   }, [onData]);

//   return <div className="invisible">{status}</div>;
// }

