import { useEffect } from 'react';
import { useAppContext } from '../context/AppContext';
import WebSocketHandler from '../components/common/WebSocketHandler';
import MetricsPanel from '../components/dashboard/MetricsPanel';
import VisualizationPanel from '../components/dashboard/VisualizationPanel';
import StatusPanel from '../components/dashboard/StatusPanel';
import { useCases } from '../config/useCases';

function Dashboard() {
  const { useCase, model, data, setData } = useAppContext();
  const config = useCases[useCase];

  useEffect(() => {
    // Redirect to setup if no use case is selected
    if (!useCase || !model) {
      window.location.href = '/';
    }
  }, [useCase, model]);

  if (!config) return null;

  return (
    <div className="min-h-screen bg-gradient-to-b from-gray-50 to-white p-8 font-sans">
      <header className="flex items-center justify-between mb-8">
        <div>
          <h1 className="text-3xl font-extrabold">{config.name}</h1>
          <p className="text-gray-500 text-sm">{config.description}</p>
        </div>
        <div className="flex items-center gap-4">
          <span className="text-sm text-gray-600">Model: {model?.name}</span>
          <button className="px-4 py-1 text-sm rounded-full bg-blue-100 text-blue-600 font-semibold shadow">
            Live Data
          </button>
        </div>
      </header>

      <WebSocketHandler
        useCase={useCase}
        onData={(incoming) => {
          setData(incoming);
        }}
      />

      <div className="grid grid-cols-3 gap-6 mb-10">
        <MetricsPanel metrics={config.metrics} data={data} />
        <StatusPanel />
        <div className="rounded-2xl bg-white p-6 shadow-md">
          <h2 className="font-semibold text-gray-700 mb-1">Model Performance</h2>
          <div className="text-sm text-gray-500">
            {/* Add model performance metrics */}
          </div>
        </div>
      </div>

      <VisualizationPanel
        types={config.visualizations}
        data={data}
      />
    </div>
  );
}

export default Dashboard; 