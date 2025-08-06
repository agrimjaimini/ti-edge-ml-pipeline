import { useEffect, useRef, useCallback } from 'react';
import { useAppContext } from '../../context/AppContext';

function WebSocketHandler({ useCase, onData }) {
  const { setIsConnected } = useAppContext();
  const wsRef = useRef(null);
  const isConnectingRef = useRef(false);

  // Stabilize the onData callback
  const stableOnData = useCallback(onData, []);

  useEffect(() => {
    // Prevent multiple connections
    if (wsRef.current || isConnectingRef.current) {
      return;
    }

    isConnectingRef.current = true;
    const ws = new WebSocket('ws://localhost:8000/ws');
    wsRef.current = ws;

    ws.onopen = () => {
      setIsConnected(true);
      isConnectingRef.current = false;
      // Send use case configuration
      ws.send(JSON.stringify({ useCase }));
    };

    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        stableOnData(data);
      } catch (err) {
        console.error('Invalid JSON from WebSocket:', err);
      }
    };

    ws.onclose = () => {
      setIsConnected(false);
      wsRef.current = null;
      isConnectingRef.current = false;
    };

    ws.onerror = (err) => {
      console.error('WebSocket error:', err);
      setIsConnected(false);
      wsRef.current = null;
      isConnectingRef.current = false;
    };

    return () => {
      if (wsRef.current) {
        wsRef.current.close();
        wsRef.current = null;
      }
      isConnectingRef.current = false;
    };
  }, [useCase, stableOnData, setIsConnected]);

  return null;
}

export default WebSocketHandler; 