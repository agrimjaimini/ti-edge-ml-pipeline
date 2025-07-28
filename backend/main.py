from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from typing import List
from model.inference import predict_occupancy
import json
from fall_detection.inference.fall_detector import FallDetector

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

fall_detector = FallDetector(
    model_path="pointnet_lstm_fall_final.pth",
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
    clients.append(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        clients.remove(websocket)

async def broadcast_to_clients(message: str):
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
async def home():
    return "hello"

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8080, reload=True)