import { useAppContext } from '../../context/AppContext';

function StatusPanel() {
  const { isConnected, model } = useAppContext();

  return (
    <div className="rounded-2xl bg-white p-6 shadow-md">
      <h2 className="font-semibold text-gray-700 mb-4">System Status</h2>
      
      <div className="space-y-3">
        <div className="flex items-center justify-between">
          <span className="text-sm text-gray-600">Connection</span>
          <div className="flex items-center gap-2">
            <div className={`w-2 h-2 rounded-full ${isConnected ? 'bg-green-500' : 'bg-red-500'}`}></div>
            <span className={`text-sm font-medium ${isConnected ? 'text-green-600' : 'text-red-600'}`}>
              {isConnected ? 'Connected' : 'Disconnected'}
            </span>
          </div>
        </div>
        
        <div className="flex items-center justify-between">
          <span className="text-sm text-gray-600">Model Status</span>
          <span className="text-sm font-medium text-blue-600">
            {model ? 'Active' : 'Not Loaded'}
          </span>
        </div>
        
        <div className="flex items-center justify-between">
          <span className="text-sm text-gray-600">Data Stream</span>
          <span className="text-sm font-medium text-green-600">
            {isConnected ? 'Live' : 'Offline'}
          </span>
        </div>
      </div>
    </div>
  );
}

export default StatusPanel; 