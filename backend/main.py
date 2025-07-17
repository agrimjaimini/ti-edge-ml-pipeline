from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from typing import List
from model.inference import predict_occupancy
import json

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

clients: List[WebSocket] = []

class RadarPayload(BaseModel):
    x_pos:    List[float]
    y_pos:    List[float]
    z_pos:    List[float]
    snr:      List[float]
    noise:    List[float]

    def to_sensor_data(self) -> List[List[float]]:
        """Convert radar data to sensor data format."""
        return [[self.x_pos[i], self.y_pos[i], self.z_pos[i]]
                for i in range(len(self.x_pos))]

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """Handle WebSocket connections."""
    await websocket.accept()
    clients.append(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        clients.remove(websocket)

async def broadcast_to_clients(message: str):
    """Send message to all connected clients and clean up dead connections."""
    dead = []
    for ws in clients:
        try:
            await ws.send_text(message)
        except:
            dead.append(ws)
    for ws in dead:
        clients.remove(ws)

@app.post("/occupancy")
async def occupancy_hook(payload: RadarPayload):
    """Process radar data and broadcast results to WebSocket clients."""
    sensor_data = payload.to_sensor_data()

    result = predict_occupancy(sensor_data)

    combined_message = json.dumps({
        "occupancy": result,
        "point_data": {
            "x_pos": payload.x_pos,
            "y_pos": payload.y_pos,
            "z_pos": payload.z_pos,
            "snr": payload.snr,
            "noise": payload.noise
        }
    })
    await broadcast_to_clients(combined_message)

    return JSONResponse(content={
        "status": "ok", 
        "clients": len(clients)
    })

@app.get("/")
async def home():
    """Return simple home page."""
    return "hello"

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8080, reload=True)