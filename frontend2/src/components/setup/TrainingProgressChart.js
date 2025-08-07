import { useEffect, useState } from 'react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import { useAppContext } from '../../context/AppContext';

function TrainingProgressChart() {
  const { trainingProgress } = useAppContext();
  const [chartData, setChartData] = useState([]);

  useEffect(() => {
    if (trainingProgress.metrics.length > 0) {
      // Transform metrics array into chart data format
      const data = trainingProgress.metrics.map((metric, index) => ({
        epoch: index + 1,
        trainLoss: metric.train_loss || 0,
        valLoss: metric.val_loss || 0,
        trainAcc: metric.train_accuracy ? metric.train_accuracy * 100 : 0,
        valAcc: metric.val_accuracy ? metric.val_accuracy * 100 : 0
      }));
      setChartData(data);
    }
  }, [trainingProgress.metrics]);

  if (!trainingProgress.isTraining && trainingProgress.status === 'idle') {
    return null;
  }

  return (
    <div className="bg-white rounded-lg shadow-md p-6 mt-6">
      <div className="flex items-center justify-between mb-4">
        <h3 className="text-lg font-semibold text-gray-800">Training Progress</h3>
        <div className="flex items-center gap-4">
          <span className="text-sm text-gray-600">
            Model: {trainingProgress.modelName}
          </span>
          <div className={`px-3 py-1 rounded-full text-xs font-semibold ${
            trainingProgress.status === 'training' 
              ? 'bg-blue-100 text-blue-600' 
              : trainingProgress.status === 'completed'
              ? 'bg-green-100 text-green-600'
              : 'bg-red-100 text-red-600'
          }`}>
            {trainingProgress.status === 'training' ? 'Training...' : 
             trainingProgress.status === 'completed' ? 'Completed' : 'Error'}
          </div>
        </div>
      </div>

      {/* Training Progress Line Chart */}
      <div className="h-80">
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={chartData} margin={{ top: 20, right: 30, left: 40, bottom: 60 }}>
            <CartesianGrid strokeDasharray="3 3" stroke="#e5e7eb" />
            <XAxis 
              dataKey="epoch" 
              type="number"
              domain={['dataMin', 'dataMax']}
              label={{ value: 'Epochs', position: 'insideBottom', offset: -10 }}
              stroke="#6b7280"
            />
            <YAxis 
              type="number"
              domain={['auto', 'auto']}
              label={{ value: 'Loss Value', angle: -90, position: 'insideLeft' }}
              stroke="#6b7280"
            />
            <Tooltip 
              formatter={(value, name) => [
                value.toFixed(4), 
                name === 'trainLoss' ? 'Training Loss' : 'Validation Loss'
              ]}
              labelFormatter={(epoch) => `Epoch ${epoch}`}
              contentStyle={{
                backgroundColor: '#f9fafb',
                border: '1px solid #e5e7eb',
                borderRadius: '8px'
              }}
            />
            <Legend 
              wrapperStyle={{ paddingTop: '20px' }}
            />
            <Line 
              type="monotone" 
              dataKey="trainLoss" 
              stroke="#3b82f6" 
              strokeWidth={3}
              name="Training Loss"
              dot={{ r: 4, fill: '#3b82f6', strokeWidth: 0 }}
              activeDot={{ r: 6, fill: '#1d4ed8', strokeWidth: 2, stroke: '#ffffff' }}
              connectNulls={true}
            />
            <Line 
              type="monotone" 
              dataKey="valLoss" 
              stroke="#ef4444" 
              strokeWidth={3}
              name="Validation Loss"
              dot={{ r: 4, fill: '#ef4444', strokeWidth: 0 }}
              activeDot={{ r: 6, fill: '#dc2626', strokeWidth: 2, stroke: '#ffffff' }}
              connectNulls={true}
            />
          </LineChart>
        </ResponsiveContainer>
      </div>

      {/* Progress Bar */}
      <div className="mt-6">
        <div className="flex justify-between text-sm text-gray-600 mb-2">
          <span>Epoch Progress</span>
          <span>{trainingProgress.currentEpoch} / {trainingProgress.totalEpochs}</span>
        </div>
        <div className="w-full bg-gray-200 rounded-full h-2">
          <div 
            className="bg-blue-600 h-2 rounded-full transition-all duration-300"
            style={{ 
              width: trainingProgress.totalEpochs > 0 
                ? `${(trainingProgress.currentEpoch / trainingProgress.totalEpochs) * 100}%` 
                : '0%' 
            }}
          />
        </div>
      </div>

      {/* Current Metrics Display */}
      {chartData.length > 0 && (
        <div className="mt-6 grid grid-cols-1 md:grid-cols-3 gap-4">
          <div className="bg-blue-50 border border-blue-200 rounded-lg p-4">
            <div className="text-sm text-blue-600 font-medium">Training Loss</div>
            <div className="text-2xl font-bold text-blue-700">
              {chartData[chartData.length - 1]?.trainLoss.toFixed(4) || 'N/A'}
            </div>
          </div>
          <div className="bg-red-50 border border-red-200 rounded-lg p-4">
            <div className="text-sm text-red-600 font-medium">Validation Loss</div>
            <div className="text-2xl font-bold text-red-700">
              {chartData[chartData.length - 1]?.valLoss.toFixed(4) || 'N/A'}
            </div>
          </div>
          <div className="bg-green-50 border border-green-200 rounded-lg p-4">
            <div className="text-sm text-green-600 font-medium">Validation Accuracy</div>
            <div className="text-2xl font-bold text-green-700">
              {chartData[chartData.length - 1]?.valAcc.toFixed(2) || 'N/A'}%
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default TrainingProgressChart;