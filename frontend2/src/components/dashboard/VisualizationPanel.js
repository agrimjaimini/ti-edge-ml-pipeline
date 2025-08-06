import { useAppContext } from '../../context/AppContext';

function VisualizationPanel({ types = [], data }) {
  const { useCase } = useAppContext();

  const renderVisualization = (type) => {
    switch (type) {
      case 'radar':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📡</div>
              <p className="text-gray-600">Radar Visualization</p>
              <p className="text-sm text-gray-500">Points: {data?.points?.length || 0}</p>
            </div>
          </div>
        );
      
      case 'chart':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📊</div>
              <p className="text-gray-600">Performance Chart</p>
              <p className="text-sm text-gray-500">Data points: {data ? Object.keys(data).length : 0}</p>
            </div>
          </div>
        );
      
      case 'heatmap':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">🔥</div>
              <p className="text-gray-600">Activity Heatmap</p>
              <p className="text-sm text-gray-500">Activity level: {data?.activity || 'N/A'}</p>
            </div>
          </div>
        );
      
      default:
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📈</div>
              <p className="text-gray-600">Data Visualization</p>
              <p className="text-sm text-gray-500">Type: {type}</p>
            </div>
          </div>
        );
    }
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h2 className="text-xl font-semibold text-gray-800">Visualizations</h2>
        <span className="text-sm text-gray-500">Use Case: {useCase}</span>
      </div>
      
      {types.length > 0 ? (
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {types.map((type, index) => (
            <div key={index}>
              {renderVisualization(type)}
            </div>
          ))}
        </div>
      ) : (
        <div className="bg-gray-50 rounded-lg p-8 text-center">
          <div className="text-3xl mb-4">📊</div>
          <p className="text-gray-600">No visualizations configured</p>
          <p className="text-sm text-gray-500 mt-2">Configure visualizations in your use case settings</p>
        </div>
      )}
    </div>
  );
}

export default VisualizationPanel; 