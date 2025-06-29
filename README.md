# TI Edge ML Hybrid Pipeline

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
