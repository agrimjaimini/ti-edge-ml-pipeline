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
    snr:      List[float]    # you can keep these if you ever want to visualize or extend
    noise:    List[float]
    # you can ignore Frame Count, Time Stamp, Num Points if you like:
    # frame_count:   int = Field(..., alias="Frame Count: ")
    # time_stamp:    int = Field(..., alias="Time Stamp: ")
    # num_points:    int = Field(..., alias="Num Points: ")

    def to_sensor_data(self) -> List[List[float]]:
        # zip x,y,z into a list of [x,y,z] points
        return [[self.x_pos[i], self.y_pos[i], self.z_pos[i]]
                for i in range(len(self.x_pos))]

@app.websocket("/ws/occupancy")
async def occupancy_ws(websocket: WebSocket):
    await websocket.accept()
    clients.append(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        clients.remove(websocket)

@app.post("/occupancy")
async def occupancy_hook(payload: RadarPayload):
    # 1) turn the radar JSON into the simple [[x,y,z],…] shape your model expects
    sensor_data = payload.to_sensor_data()

    # 2) run inference
    result = predict_occupancy(sensor_data)

    # 3) broadcast to all WS clients
    text = json.dumps(result)
    dead = []
    for ws in clients:
        try:
            await ws.send_text(text)
        except:
            dead.append(ws)
    for ws in dead:
        clients.remove(ws)

    return JSONResponse(content={"status": "ok", "sent_to": len(clients)})

@app.get("/")
async def home():
    return "hello"

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8080, reload=True)