import { createContext, useContext, useState } from 'react';

const AppContext = createContext();

export function AppProvider({ children }) {
  const [useCase, setUseCase] = useState(null);
  const [model, setModel] = useState(null);
  const [data, setData] = useState(null);
  const [isConnected, setIsConnected] = useState(false);
  
  // Training progress state
  const [trainingProgress, setTrainingProgress] = useState({
    isTraining: false,
    currentEpoch: 0,
    totalEpochs: 0,
    metrics: [],
    modelName: '',
    status: 'idle' // 'idle', 'training', 'completed', 'error'
  });

  const value = {
    useCase,
    setUseCase,
    model,
    setModel,
    data,
    setData,
    isConnected,
    setIsConnected,
    trainingProgress,
    setTrainingProgress
  };

  return (
    <AppContext.Provider value={value}>
      {children}
    </AppContext.Provider>
  );
}

export function useAppContext() {
  const context = useContext(AppContext);
  if (!context) {
    throw new Error('useAppContext must be used within an AppProvider');
  }
  return context;
} 