from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from model.inference import predict_fall_probability
import json

app = FastAPI()


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

clients = []
#clients here can open a connection
@app.websocket("/ws/fall")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    clients.append(websocket)
    try:
        while True:
            data = await websocket.receive_text()
            parsed = json.loads(data)
            sensor_data = parsed.get("sensor_data", [])

            probability = predict_fall_probability(sensor_data)
            result = {
                "probability": probability,
                "fall_detected": probability > 0.8
            }

            await websocket.send_text(json.dumps(result))
    except WebSocketDisconnect:
        clients.remove(websocket)