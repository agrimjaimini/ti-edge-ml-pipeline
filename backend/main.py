from fastapi import FastAPI, WebSocket, WebSocketDisconnect, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from model.inference import predict_occupancy
import json

app = FastAPI()

# Allow CORS for all origins
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

# Track connected clients
clients: list[WebSocket] = []

class SensorPayload(BaseModel):
    sensor_data: list[list[float]]

@app.websocket("/ws/occupancy")
async def occupancy_ws(websocket: WebSocket):
    await websocket.accept()
    clients.append(websocket)
    try:
        while True:
            # just keep the connection alive, discard any incoming message
            await websocket.receive_text()
    except WebSocketDisconnect:
        clients.remove(websocket)

@app.post("/occupancy")
async def occupancy_hook(payload: SensorPayload):
    result = predict_occupancy(payload.sensor_data)
    text = json.dumps(result)

    dead_clients = []
    for client in clients:
        try:
            await client.send_text(text)
        except Exception:
            dead_clients.append(client)

    for client in dead_clients:
        clients.remove(client)

    return JSONResponse(content={"status": "ok", "sent_to": len(clients)})

@app.get("/")
async def home():
    return "hello"

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8080, reload=True)