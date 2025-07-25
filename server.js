const WebSocket = require('ws');
const wss = new WebSocket.Server({ port: 8000 });

wss.on('connection', (ws) => {
  console.log('Client connected');
  setInterval(() => {
    ws.send(
      JSON.stringify({
        occupancy: {
          predicted_count: Math.floor(Math.random() * 4),
          probabilities: [Math.random(), Math.random(), Math.random(), Math.random()],
        },
        point_data: {
          x_pos: Array.from({ length: 20 }, () => Math.random() * 10),
          y_pos: Array.from({ length: 20 }, () => Math.random() * 10),
          z_pos: Array.from({ length: 20 }, () => Math.random() * 10),
          snr: [],
          noise: [],
        },
      })
    );
  }, 2000);
});

console.log('Mock WebSocket server running on ws://localhost:8000');
