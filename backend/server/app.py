from flask import Flask
from flask_socketio import SocketIO, emit
import torch
import numpy as np
from model import load_model

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")

model = load_model()

@socketio.on('fall_data')
def handle_fall_data(json_data):
    """
    Expected input format:
    {
    }
    """
    #Input data into LSTM here, and send probability as a Websocket event to frontend

@app.route("/")
def index():
    return "Fall Detection WebSocket Backend Running"

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)