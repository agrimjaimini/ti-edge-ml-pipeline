from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from typing import List
from model.inference import predict_occupancy
from fall_detection.inference.fall_detector import FallDetector
import json
import os

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# Initialize fall detector
model_paths = [
    os.path.join(os.path.dirname(__file__), 'fall_detection/model/pointnet_lstm_fall.pth'),
    os.path.join(os.path.dirname(__file__), 'pointnet_lstm_fall.pth'),
    'pointnet_lstm_fall.pth'
]

# Try to find model weights
model_path = None
for path in model_paths:
    if os.path.exists(path):
        model_path = path
        break

if model_path is None:
    raise FileNotFoundError("Could not find model weights file")

fall_detector = FallDetector(
    sequence_length=30,
    num_points=128,
    model_path=model_path  # Changed from weights_path to model_path
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

async def broadcast_to_clients(message: str):
    """Send message to all connected clients and clean up dead connections."""
    dead = []
    for ws in clients[:]:  # Create a copy of the list to iterate over
        try:
            await ws.send_text(message)
        except Exception as e:
            print(f"Failed to send to client: {e}")
            if ws in clients:  # Check if client is still in list
                clients.remove(ws)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    """Handle WebSocket connections and process real-time data."""
    await websocket.accept()
    clients.append(websocket)
    try:
        while True:
            # Receive JSON data
            data = await websocket.receive_json()
            
            # Process frame through fall detector
            fall_result = fall_detector.process_frame({
                "x_pos": data["x_pos"],
                "y_pos": data["y_pos"],
                "z_pos": data["z_pos"],
                "snr": data.get("snr", []),
                "noise": data.get("noise", [])
            })

            # Create response message
            message = {
                "point_data": {
                    "x_pos": data["x_pos"],
                    "y_pos": data["y_pos"],
                    "z_pos": data["z_pos"],
                    "snr": data.get("snr", []),
                    "noise": data.get("noise", [])
                }
            }
            
            # Add fall detection result if available
            if fall_result:
                message["fall_detection"] = fall_result
            
            # Send back to client
            await websocket.send_json(message)
            
    except WebSocketDisconnect:
        if websocket in clients:
            clients.remove(websocket)
    except Exception as e:
        print(f"Error processing frame: {e}")
        if websocket in clients:
            clients.remove(websocket)

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

@app.post("/fall")
async def fall_hook(payload: RadarPayload):
    """Process fall detection for HTTP requests."""
    fall_result = fall_detector.process_frame({
        "x_pos": payload.x_pos,
        "y_pos": payload.y_pos,
        "z_pos": payload.z_pos,
        "snr": payload.snr,
        "noise": payload.noise
    })

    if fall_result:
        message = json.dumps({
            "fall_detection": fall_result,
            "point_data": {
                "x_pos": payload.x_pos,
                "y_pos": payload.y_pos,
                "z_pos": payload.z_pos,
                "snr": payload.snr,
                "noise": payload.noise
            }
        })
        await broadcast_to_clients(message)

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
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)