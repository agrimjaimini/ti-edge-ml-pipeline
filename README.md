# TI Edge-to-Cloud ML Pipeline

A real-time hybrid machine learning pipeline powered by the Texas Instruments (TI) IWRL6432AOPEVM mmWave sensor.
Designed for efficient edge-to-cloud inference and event-driven processing. Currently implemented with a fall 
detection use-case. 

Features
	•	Real-time mmWave sensor data ingestion
	•	Edge-triggered cloud pipeline via ESP32 and AWS Lambda
	•	Deployed fall detection model using sequential point cloud data
	•	WebSocket-based communication to backend inference service
	•	Alerts and frontend updates triggered when a fall is detected

## Contributors
- Agrim Jaimini — Cornell University
- Saahil Mehta - University of Texas at Austin
- Prathik Narsetty - University of Texas at Austin
- Anish Hariharan - Texas A&M University

## Project Structure

```
.
├── backend/
│   ├── main.py
│   └── model/
│       ├── data_collection/
│       ├── inference.py
│       └── model.py
├── data/
│   ├── json/
│   └── logs/
├── frontend/
│   └── index.html
├── runs/
├── requirements.txt
├── Dockerfile
├── docker-compose.yml
├── .gitignore
├── README.md
└── venv/
```

## Setup

1. **Create a virtual environment (recommended: .venv):**
   ```sh
   python3 -m venv .venv
   ```
2. **Activate the virtual environment:**
   - On macOS/Linux:
     ```sh
     source .venv/bin/activate
     ```
   - On Windows:
     ```sh
     .venv\Scripts\activate
     ```
3. **Install dependencies:**
   ```sh
   pip install -r requirements.txt
   ```

4. **Run the backend or training as needed.**

All generated files, logs, and virtual environments are ignored by git as per the .gitignore file.
