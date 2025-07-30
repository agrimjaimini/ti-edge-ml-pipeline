from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse
from pydantic import BaseModel
from typing import List
from model.model import create_model
from model.inference import predict
import json
import asyncio
import os
import sys
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from database import model_db
from utils import get_models_dir
app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

clients: List[WebSocket] = []

class RadarPayload(BaseModel):
    model_name: str
    x_pos:    List[float]
    y_pos:    List[float]
    z_pos:    List[float]
    snr:      List[float]
    noise:    List[float]

    def to_sensor_data(self) -> List[List[float]]:
        """Convert radar data to sensor data format."""
        return [[self.x_pos[i], self.y_pos[i], self.z_pos[i]]
                for i in range(len(self.x_pos))]

class CreateModelPayload(BaseModel):
    name: str
    num_classes: int
    data_dir: str
    epochs: int
    batch_size: int
    learning_rate: float
    weight_decay: float

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

@app.post("/inference")
async def inference_hook(payload: RadarPayload):
    """Process radar data and broadcast results to WebSocket clients."""
    sensor_data = payload.to_sensor_data()

    # Get model info from database to get num_classes
    model_info = model_db.get_model(payload.model_name)
    if not model_info:
        return JSONResponse(
            status_code=404,
            content={
                "status": "error",
                "message": f"Model '{payload.model_name}' not found in database"
            }
        )
    
    result = predict(sensor_data, payload.model_name, model_info['num_classes'])

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

@app.post("/create_model")
async def create_model_endpoint(payload: CreateModelPayload):
    """Create a new model."""
    try:
        # Use absolute path for models directory
        models_dir = get_models_dir()
        os.makedirs(models_dir, exist_ok=True)


        loop = asyncio.get_event_loop()
        await loop.run_in_executor(
                None, 
                create_model,
                payload.name,
                payload.num_classes,
                payload.data_dir,
                payload.epochs,
                payload.batch_size,
                payload.learning_rate,
                payload.weight_decay
            )

        model_path = os.path.join(models_dir, f"{payload.name}.pth")
        if not os.path.exists(model_path):
            return JSONResponse(
                status_code=500,
                content={
                    "status": "error",
                    "message": "Model training completed but file not found"
                }
            )
        success = model_db.add_model(
                name=payload.name,
                file_path=model_path,
                num_classes=payload.num_classes,
                data_dir=payload.data_dir,
                epochs=payload.epochs,
                batch_size=payload.batch_size,
                learning_rate=payload.learning_rate,
                weight_decay=payload.weight_decay
        )
            
        if success:
            return JSONResponse(content={
                "status": "success",
                "message": f"Model '{payload.name}' created and registered successfully",
                "model_info": {
                    "name": payload.name,
                    "file_path": model_path,
                    "num_classes": payload.num_classes,
                    "epochs": payload.epochs,
                    "batch_size": payload.batch_size
                }
            })
        else:
            return JSONResponse(
                status_code=500,
                content={
                    "status": "error",
                    "message": "Model created but failed to register in database"
                }
            )
                
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={
                "status": "error",
                "message": f"Failed to create model: {str(e)}"
            }
        )

@app.get("/models/{model_name}")
async def get_model(model_name: str):
    """Get specific model information."""
    try:
        model = model_db.get_model(model_name)
        if model:
            return JSONResponse(content={
                "status": "success",
                "model": model
            })
        else:
            return JSONResponse(
                status_code=404,
                content={
                    "status": "error",
                    "message": f"Model '{model_name}' not found"
                }
            )
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={
                "status": "error",
                "message": f"Failed to retrieve model: {str(e)}"
            }
        )
    
@app.get("/")
async def home():
    """Return simple home page."""
    return "hello"




if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8080, reload=True)