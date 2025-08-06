import { useEffect, useRef, useCallback } from 'react';
import { useAppContext } from '../../context/AppContext';

function WebSocketHandler({ useCase, onData }) {
  const { setIsConnected, setTrainingProgress } = useAppContext();
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
        
        // Handle training progress messages
        if (data.event === 'training_started') {
          setTrainingProgress(prev => ({
            ...prev,
            isTraining: true,
            modelName: data.model_name,
            totalEpochs: data.total_epochs,
            currentEpoch: 0,
            status: 'training',
            metrics: []
          }));
        } else if (data.event === 'training_progress') {
          setTrainingProgress(prev => {
            const newMetrics = [...prev.metrics];
            
            // Update or add the current epoch's metrics
            const epochIndex = data.epoch - 1;
            if (epochIndex < newMetrics.length) {
              newMetrics[epochIndex] = {
                ...newMetrics[epochIndex],
                ...data.metrics
              };
            } else {
              newMetrics.push(data.metrics);
            }
            
            return {
              ...prev,
              currentEpoch: data.epoch,
              metrics: newMetrics
            };
          });
        } else if (data.event === 'training_completed') {
          setTrainingProgress(prev => ({
            ...prev,
            isTraining: false,
            status: 'completed'
          }));
        } else if (data.event === 'training_error') {
          setTrainingProgress(prev => ({
            ...prev,
            isTraining: false,
            status: 'error'
          }));
        }
        
        // Pass all data to the component's onData handler
        if (stableOnData) {
          stableOnData(data);
        }
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
  }, [useCase, stableOnData, setIsConnected, setTrainingProgress]);

  return null;
}

export default WebSocketHandler; 