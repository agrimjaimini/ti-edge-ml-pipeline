import json
import logging
import os
import urllib3
import websocket
import ssl

logger = logging.getLogger()
logger.setLevel(logging.INFO)

# Set these in your Lambda's environment variables:
WEBSOCKET_URL = os.environ.get("WEBSOCKET_URL", "ws://localhost:8000/ws")  # Change this to your actual WebSocket URL when deployed
HTTP_FALLBACK_URL = os.environ.get("HTTP_FALLBACK_URL", "http://localhost:8000/fall")  # HTTP fallback endpoint

http = urllib3.PoolManager()

def forward_to_websocket(data):
    """Attempt to forward data via WebSocket"""
    try:
        # Create a WebSocket connection
        ws = websocket.create_connection(
            WEBSOCKET_URL,
            sslopt={"cert_reqs": ssl.CERT_NONE}  # Only for testing, use proper SSL in production
        )
        
        # Send the data
        ws.send(json.dumps(data))
        
        # Get response
        result = ws.recv()
        logger.info("WebSocket response: %s", result)
        
        # Close connection
        ws.close()
        return True
        
    except Exception as e:
        logger.error("WebSocket error: %s", e)
        return False

def forward_to_http(data):
    """Fallback to HTTP if WebSocket fails"""
    try:
        encoded = json.dumps(data).encode("utf-8")
        resp = http.request(
            "POST",
            HTTP_FALLBACK_URL,
            body=encoded,
            headers={"Content-Type": "application/json"},
            timeout=5.0
        )
        logger.info("HTTP Fallback response status: %s", resp.status)
        return True
    except Exception as e:
        logger.error("HTTP Fallback error: %s", e)
        return False

def lambda_handler(event, context):
    # Log incoming data
    logger.info("Received radar data: %s", json.dumps(event))
    
    # Extract point cloud data
    try:
        point_data = {
            "x_pos": event.get("x_pos", []),
            "y_pos": event.get("y_pos", []),
            "z_pos": event.get("z_pos", []),
            "snr": event.get("snr", []),
            "noise": event.get("noise", [])
        }
        
        # Validate data
        if not (point_data["x_pos"] and point_data["y_pos"] and point_data["z_pos"]):
            raise ValueError("Missing required point cloud coordinates")
            
        # Try WebSocket first, fallback to HTTP
        if not forward_to_websocket(point_data):
            logger.info("WebSocket failed, trying HTTP fallback...")
            if not forward_to_http(point_data):
                raise Exception("Both WebSocket and HTTP forwarding failed")
        
        return {
            "statusCode": 200,
            "body": json.dumps({
                "message": "Data forwarded successfully",
                "timestamp": event.get("timestamp", "")
            })
        }
        
    except Exception as e:
        logger.error("Error processing data: %s", e)
        return {
            "statusCode": 500,
            "body": json.dumps({
                "error": str(e)
            })
        }

# For local testing
if __name__ == "__main__":
    # Test data
    test_event = {
        "x_pos": [1.0, 2.0, 3.0],
        "y_pos": [0.1, 0.2, 0.3],
        "z_pos": [0.5, 0.6, 0.7],
        "snr": [10, 11, 12],
        "noise": [1, 1, 1],
        "timestamp": "2024-02-20T12:00:00Z"
    }
    
    print("Testing with sample data...")
    result = lambda_handler(test_event, None)
    print("Result:", result) 