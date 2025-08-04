from fastapi import FastAPI, WebSocket, WebSocketDisconnect, UploadFile, File
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
import shutil
from pathlib import Path
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from database import model_db
from utils import get_models_dir, get_data_dir
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
        # Ensure all lists have the same length
        min_length = min(len(self.x_pos), len(self.y_pos), len(self.z_pos))
        if min_length == 0:
            return []
        
        return [[self.x_pos[i], self.y_pos[i], self.z_pos[i]]
                for i in range(min_length)]

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
    
    result = predict(sensor_data, payload.model_name, model_info['num_classes'], payload.snr, payload.noise)

    combined_message = json.dumps({
        "event": "inference",
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
        models_dir = get_models_dir()
        os.makedirs(models_dir, exist_ok=True)


        loop = asyncio.get_event_loop()
        try:
            def progress_callback(data):
                # Run the broadcast in the event loop properly
                try:
                    future = asyncio.run_coroutine_threadsafe(
                        broadcast_to_clients(json.dumps(data)), 
                        loop
                    )
                    # Don't wait for the result to avoid blocking
                except Exception as e:
                    print(f"Error broadcasting progress: {e}")
            
            await loop.run_in_executor(
                    None, 
                    create_model,
                    payload.name,
                    payload.num_classes,
                    payload.data_dir,
                    payload.epochs,
                    payload.batch_size,
                    payload.learning_rate,
                    payload.weight_decay,
                    progress_callback
                )
        except Exception as e:
            return JSONResponse(
                status_code=500,
                content={
                    "status": "error",
                    "message": f"Model training failed: {str(e)}"
                }
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

@app.post("/start_training")
async def start_training_endpoint(payload: CreateModelPayload):
    """Start model training with real-time progress updates."""
    try:
        # Send training start notification
        start_message = json.dumps({
            "event": "training_started",
            "model_name": payload.name,
            "total_epochs": payload.epochs,
            "batch_size": payload.batch_size,
            "learning_rate": payload.learning_rate
        })
        await broadcast_to_clients(start_message)

        # Use absolute path for models directory
        models_dir = get_models_dir()
        os.makedirs(models_dir, exist_ok=True)

        loop = asyncio.get_event_loop()
        try:
            # Create a progress callback function that broadcasts to WebSocket clients
            def progress_callback(data):
                # Run the broadcast in the event loop
                asyncio.create_task(broadcast_to_clients(json.dumps(data)))
            
            await loop.run_in_executor(
                    None, 
                    create_model,
                    payload.name,
                    payload.num_classes,
                    payload.data_dir,
                    payload.epochs,
                    payload.batch_size,
                    payload.learning_rate,
                    payload.weight_decay,
                    progress_callback
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
                    "message": f"Model '{payload.name}' trained and registered successfully",
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
                        "message": "Model trained but failed to register in database"
                    }
                )
                    
        except Exception as e:
            return JSONResponse(
                status_code=500,
                content={
                    "status": "error",
                    "message": f"Model training failed: {str(e)}"
                }
            )
                
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={
                "status": "error",
                "message": f"Failed to start training: {str(e)}"
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
    
@app.post("/upload_data")
async def upload_data(file: UploadFile = File(...)):
    """Upload a JSON file to the data directory and store metadata in database."""
    try:
        # Validate file type - only allow JSON files
        if not file.filename.lower().endswith('.json'):
            return JSONResponse(
                status_code=400,
                content={
                    "status": "error",
                    "message": "Only JSON files (.json) are allowed"
                }
            )
        
        # Validate content type if provided
        if file.content_type and file.content_type not in ['application/json', 'text/json']:
            return JSONResponse(
                status_code=400,
                content={
                    "status": "error",
                    "message": "Invalid content type. Only JSON files are allowed"
                }
            )
        
        # Validate JSON content
        try:
            content = await file.read()
            # Try to parse as JSON to validate
            json.loads(content.decode('utf-8'))
            # Reset file pointer for saving
            await file.seek(0)
        except json.JSONDecodeError:
            return JSONResponse(
                status_code=400,
                content={
                    "status": "error",
                    "message": "Invalid JSON content. Please upload a valid JSON file"
                }
            )
        except UnicodeDecodeError:
            return JSONResponse(
                status_code=400,
                content={
                    "status": "error",
                    "message": "File must be UTF-8 encoded"
                }
            )
        
        # Get the data directory path
        data_dir = get_data_dir()
        
        # Create the full file path
        file_path = os.path.join(data_dir, file.filename)
        
        # Check if file already exists
        if os.path.exists(file_path):
            return JSONResponse(
                status_code=409,
                content={
                    "status": "error",
                    "message": f"File '{file.filename}' already exists"
                }
            )
        
        # Save the uploaded file
        with open(file_path, "wb") as buffer:
            shutil.copyfileobj(file.file, buffer)
        
        # Get file size
        file_size = os.path.getsize(file_path)
        
        # Store file metadata in database
        success = model_db.add_file(
            filename=file.filename,
            file_path=file_path,
            file_size_bytes=file_size,
            content_type="application/json"
        )
        
        if not success:
            # If database storage fails, remove the uploaded file
            os.remove(file_path)
            return JSONResponse(
                status_code=500,
                content={
                    "status": "error",
                    "message": "File uploaded but failed to store metadata in database"
                }
            )
        
        return JSONResponse(content={
            "status": "success",
            "message": f"JSON file '{file.filename}' uploaded successfully",
            "file_info": {
                "filename": file.filename,
                "size_bytes": file_size,
                "path": file_path,
                "content_type": "application/json"
            }
        })
        
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={
                "status": "error",
                "message": f"Failed to upload file: {str(e)}"
            }
        )

@app.get("/data_files")
async def list_data_files():
    """List all files from the database."""
    try:
        files = model_db.get_all_files()
        
        return JSONResponse(content={
            "status": "success",
            "files": files,
            "total_files": len(files)
        })
        
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={
                "status": "error",
                "message": f"Failed to list files: {str(e)}"
            }
        )

@app.get("/data_files/{filename}")
async def get_file_info(filename: str):
    """Get information about a specific file from the database."""
    try:
        file_info = model_db.get_file(filename)
        
        if file_info:
            return JSONResponse(content={
                "status": "success",
                "file": file_info
            })
        else:
            return JSONResponse(
                status_code=404,
                content={
                    "status": "error",
                    "message": f"File '{filename}' not found in database"
                }
            )
        
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={
                "status": "error",
                "message": f"Failed to retrieve file info: {str(e)}"
            }
        )

@app.delete("/data_files/{filename}")
async def delete_file(filename: str):
    """Delete a file from both the filesystem and database."""
    try:
        # Get file info from database
        file_info = model_db.get_file(filename)
        
        if not file_info:
            return JSONResponse(
                status_code=404,
                content={
                    "status": "error",
                    "message": f"File '{filename}' not found in database"
                }
            )
        
        # Delete from filesystem
        file_path = file_info['file_path']
        if os.path.exists(file_path):
            os.remove(file_path)
        
        # Delete from database
        success = model_db.delete_file(filename)
        
        if success:
            return JSONResponse(content={
                "status": "success",
                "message": f"File '{filename}' deleted successfully"
            })
        else:
            return JSONResponse(
                status_code=500,
                content={
                    "status": "error",
                    "message": "File deleted from filesystem but failed to remove from database"
                }
            )
        
    except Exception as e:
        return JSONResponse(
            status_code=500,
            content={
                "status": "error",
                "message": f"Failed to delete file: {str(e)}"
            }
        )

@app.get("/")
async def home():
    """Return simple home page."""
    return "hello"




if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)