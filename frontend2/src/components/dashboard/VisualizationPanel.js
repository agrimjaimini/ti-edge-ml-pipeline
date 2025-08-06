import { useAppContext } from '../../context/AppContext';

function VisualizationPanel({ types = [], data }) {
  const { useCase } = useAppContext();

  const renderVisualization = (visualization) => {
    // Handle both string types and configuration objects
    const type = typeof visualization === 'string' ? visualization : visualization.type;
    
    switch (type) {
      case 'pointCloud':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📡</div>
              <p className="text-gray-600">{visualization.title || '3D Point Cloud'}</p>
              <p className="text-sm text-gray-500">{visualization.description || 'Real-time visualization of radar points'}</p>
              <p className="text-sm text-gray-500">Points: {data?.points?.length || 0}</p>
            </div>
          </div>
        );
      
      case 'probability':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📊</div>
              <p className="text-gray-600">{visualization.title || 'Fall Probability'}</p>
              <p className="text-sm text-gray-500">{visualization.description || 'Real-time fall detection probability'}</p>
              <p className="text-sm text-gray-500">Probability: {data?.probabilities ? 'Available' : 'N/A'}</p>
            </div>
          </div>
        );
      
      case 'occupancyChart':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📈</div>
              <p className="text-gray-600">{visualization.title || 'Occupancy Trend'}</p>
              <p className="text-sm text-gray-500">{visualization.description || 'Real-time occupancy levels'}</p>
              <p className="text-sm text-gray-500">History: {data?.occupancy_history ? 'Available' : 'N/A'}</p>
            </div>
          </div>
        );
      
      case 'chart':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📈</div>
              <p className="text-gray-600">{visualization.title || 'Line Chart'}</p>
              <p className="text-sm text-gray-500">{visualization.description || 'Time-series data visualization'}</p>
              <p className="text-sm text-gray-500">Data points: {data ? Object.keys(data).length : 0}</p>
            </div>
          </div>
        );
      
      case 'barChart':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📊</div>
              <p className="text-gray-600">{visualization.title || 'Bar Chart'}</p>
              <p className="text-sm text-gray-500">{visualization.description || 'Categorical data comparison'}</p>
              <p className="text-sm text-gray-500">Categories: {data ? Object.keys(data).length : 0}</p>
            </div>
          </div>
        );
      
      case 'heatmap':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">🔥</div>
              <p className="text-gray-600">{visualization.title || 'Heatmap'}</p>
              <p className="text-sm text-gray-500">{visualization.description || '2D density visualization'}</p>
              <p className="text-sm text-gray-500">Activity level: {data?.activity || 'N/A'}</p>
            </div>
          </div>
        );
      
      case 'gauge':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">🎯</div>
              <p className="text-gray-600">{visualization.title || 'Gauge'}</p>
              <p className="text-sm text-gray-500">{visualization.description || 'Circular progress indicator'}</p>
              <p className="text-sm text-gray-500">Value: {data?.gauge_value || 'N/A'}</p>
            </div>
          </div>
        );
      
      case 'radar':
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📡</div>
              <p className="text-gray-600">{visualization.title || 'Radar Plot'}</p>
              <p className="text-sm text-gray-500">{visualization.description || 'Multi-dimensional data plot'}</p>
              <p className="text-sm text-gray-500">Dimensions: {data?.radar_data ? Object.keys(data.radar_data).length : 0}</p>
            </div>
          </div>
        );
      
      default:
        return (
          <div className="bg-gray-100 rounded-lg p-4 h-64 flex items-center justify-center">
            <div className="text-center">
              <div className="text-2xl mb-2">📈</div>
              <p className="text-gray-600">{visualization.title || 'Data Visualization'}</p>
              <p className="text-sm text-gray-500">{visualization.description || `Type: ${type}`}</p>
            </div>
          </div>
        );
    }
  };

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <h2 className="text-xl font-semibold text-gray-800">Visualizations</h2>
        <span className="text-sm text-gray-500">Use Case: {useCase?.name || 'Custom'}</span>
      </div>
      
      {types.length > 0 ? (
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {types.map((visualization, index) => (
            <div key={index} className="bg-white rounded-lg shadow-md overflow-hidden">
              <div className="p-4 border-b border-gray-100">
                <h3 className="font-medium text-gray-900">{visualization.title}</h3>
                <p className="text-sm text-gray-500 mt-1">{visualization.description}</p>
              </div>
              {renderVisualization(visualization)}
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