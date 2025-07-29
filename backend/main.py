from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse, FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
from typing import List
import json
import os
from fall_detection.inference.fall_detector import FallDetector

app = FastAPI()

# Mount static files
app.mount("/static", StaticFiles(directory="static"), name="static")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

fall_detector = FallDetector(
    model_path="backend/pointnet_lstm_fall_final.pth",
    sequence_length=20,
    num_points=128
)

clients: List[WebSocket] = []

class RadarPayload(BaseModel):
    x_pos:    List[float]
    y_pos:    List[float]
    z_pos:    List[float]
    snr:      List[float]
    noise:    List[float]

    def to_sensor_data(self) -> List[List[float]]:
        return [[self.x_pos[i], self.y_pos[i], self.z_pos[i]]
                for i in range(len(self.x_pos))]

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    if websocket not in clients:  # Check if already in list
        clients.append(websocket)
    try:
        while True:
            # Receive JSON data
            data = await websocket.receive_json()
            
            # Process frame through fall detector
            fall_result = fall_detector.process_frame({
                "x_pos": data["x_pos"],
                "y_pos": data["y_pos"],
                "z_pos": data["z_pos"]
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
        if websocket in clients:  # Check before removing
            clients.remove(websocket)
    except Exception as e:
        print(f"Error processing frame: {e}")
        if websocket in clients:  # Check before removing
            clients.remove(websocket)

async def broadcast_to_clients(message: str):
    disconnected = []
    for ws in clients[:]:  # Create a copy of the list to iterate
        try:
            await ws.send_text(message)
        except Exception as e:
            print(f"Error broadcasting to client: {e}")
            disconnected.append(ws)
    
    # Clean up disconnected clients
    for ws in disconnected:
        if ws in clients:  # Check before removing
            clients.remove(ws)

@app.post("/occupancy")
async def occupancy_hook(payload: RadarPayload):
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
    fall_result = fall_detector.process_frame({
        "x_pos": payload.x_pos,
        "y_pos": payload.y_pos,
        "z_pos": payload.z_pos
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
async def get_index():
    return FileResponse('static/index.html')

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)