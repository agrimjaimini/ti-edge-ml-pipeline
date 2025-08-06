import { useState } from 'react';
import { useCases } from '../../config/useCases';

function UseCaseConfig({ onComplete }) {
  const [selectedUseCase, setSelectedUseCase] = useState('');
  const [error, setError] = useState(null);

  const handleSubmit = (e) => {
    e.preventDefault();
    if (!selectedUseCase) {
      setError('Please select a use case');
      return;
    }
    onComplete(selectedUseCase);
  };

  return (
    <div className="space-y-6">
      <div className="text-center">
        <h2 className="text-2xl font-bold mb-2">Select Use Case</h2>
        <p className="text-gray-600">Choose how you want to use your model</p>
      </div>

      <form onSubmit={handleSubmit} className="space-y-6">
        <div className="grid grid-cols-1 gap-4">
          {Object.entries(useCases).map(([key, config]) => (
            <div
              key={key}
              className={`
                p-4 rounded-lg border-2 cursor-pointer transition-colors
                ${selectedUseCase === key
                  ? 'border-blue-500 bg-blue-50'
                  : 'border-gray-200 hover:border-blue-200'
                }
              `}
              onClick={() => setSelectedUseCase(key)}
            >
              <div className="flex items-start">
                <div className="flex-1">
                  <h3 className="font-medium text-gray-900">{config.name}</h3>
                  <p className="mt-1 text-sm text-gray-500">{config.description}</p>
                </div>
                <div className="ml-3 flex h-5 items-center">
                  <input
                    type="radio"
                    name="use-case"
                    value={key}
                    checked={selectedUseCase === key}
                    onChange={(e) => setSelectedUseCase(e.target.value)}
                    className="h-4 w-4 border-gray-300 text-blue-600 focus:ring-blue-500"
                  />
                </div>
              </div>

              {selectedUseCase === key && (
                <div className="mt-4 text-sm text-gray-600">
                  <h4 className="font-medium mb-2">Features:</h4>
                  <ul className="list-disc list-inside space-y-1">
                    <li>Real-time data visualization</li>
                    <li>{config.metrics.length} key metrics tracked</li>
                    <li>{config.visualizations.length} visualization types</li>
                  </ul>
                </div>
              )}
            </div>
          ))}
        </div>

        {error && (
          <div className="bg-red-50 text-red-600 p-4 rounded-lg text-sm">
            {error}
          </div>
        )}

        <button
          type="submit"
          className="w-full py-2 px-4 bg-blue-600 hover:bg-blue-700 text-white font-medium rounded-md"
        >
          Continue to Dashboard
        </button>
      </form>
    </div>
  );
}

export default UseCaseConfig; 