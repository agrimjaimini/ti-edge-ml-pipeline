from flask import Flask, request, jsonify
from flask_cors import CORS
from flask_sock import Sock
from model.inference import predict_occupancy
import json

app = Flask(__name__)
CORS(app)              # allow_origins="*"

sock = Sock(app)       # add WebSocket support

clients: list = []

@sock.route('/ws/occupancy')
def occupancy_ws(ws):
    """Dashboards connect here to receive live updates."""
    clients.append(ws)
    try:
        while True:
            # just keep the connection open; ignore any incoming text
            ws.receive()
    except Exception:
        clients.remove(ws)

@app.route('/occupancy', methods=['POST'])
def occupancy_hook():
    """
    Called by your Lambda. Runs the model, then broadcasts to all WS clients.
    """
    payload = request.get_json()
    # 1) Run inference
    result = predict_occupancy(payload['sensor_data'])
    # 2) Broadcast
    text = json.dumps(result)
    dead = []
    for ws in clients:
        try:
            ws.send(text)
        except Exception:
            dead.append(ws)
    for ws in dead:
        clients.remove(ws)

    return jsonify(status="ok", sent_to=len(clients))

@app.route('/', methods=['GET'])
def home():
    return "hello"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8080)