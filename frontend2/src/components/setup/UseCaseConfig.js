import { useState } from 'react';

function UseCaseConfig({ onComplete }) {
  const [useCaseName, setUseCaseName] = useState('');
  const [useCaseDescription, setUseCaseDescription] = useState('');
  const [selectedMetrics, setSelectedMetrics] = useState([]);
  const [selectedVisualizations, setSelectedVisualizations] = useState([]);
  const [step, setStep] = useState('name'); // name, metrics, visualizations, review
  const [error, setError] = useState(null);

  // Available metric types
  const metricTypes = [
    { id: 'status', label: 'Status Indicator', description: 'Boolean status (Detected/Clear)' },
    { id: 'percentage', label: 'Percentage', description: 'Percentage value (0-100%)' },
    { id: 'number', label: 'Number', description: 'Numeric value with optional precision' },
    { id: 'timestamp', label: 'Timestamp', description: 'Date and time value' },
    { id: 'count', label: 'Count', description: 'Integer count value' }
  ];

  // Available visualization types
  const visualizationTypes = [
    { id: 'pointCloud', label: '3D Point Cloud', description: 'Real-time 3D point visualization' },
    { id: 'chart', label: 'Line Chart', description: 'Time-series data visualization' },
    { id: 'barChart', label: 'Bar Chart', description: 'Categorical data comparison' },
    { id: 'heatmap', label: 'Heatmap', description: '2D density visualization' },
    { id: 'gauge', label: 'Gauge', description: 'Circular progress indicator' },
    { id: 'radar', label: 'Radar Plot', description: 'Multi-dimensional data plot' }
  ];

  const handleNext = () => {
    if (step === 'name' && (!useCaseName.trim() || !useCaseDescription.trim())) {
      setError('Please provide both a name and description for your use case');
      return;
    }
    setError(null);
    setStep(getNextStep(step));
  };

  const handleBack = () => {
    setStep(getPreviousStep(step));
    setError(null);
  };

  const getNextStep = (currentStep) => {
    const steps = ['name', 'metrics', 'visualizations', 'review'];
    const currentIndex = steps.indexOf(currentStep);
    return steps[currentIndex + 1] || currentStep;
  };

  const getPreviousStep = (currentStep) => {
    const steps = ['name', 'metrics', 'visualizations', 'review'];
    const currentIndex = steps.indexOf(currentStep);
    return steps[currentIndex - 1] || currentStep;
  };

  const handleComplete = () => {
    if (selectedMetrics.length === 0) {
      setError('Please select at least one metric');
      return;
    }
    
    const customUseCase = {
      name: useCaseName,
      description: useCaseDescription,
      metrics: selectedMetrics,
      visualizations: selectedVisualizations,
      dataFormat: {
        required: selectedMetrics.map(m => m.dataKey).filter(Boolean)
      }
    };
    
    onComplete(customUseCase);
  };

  const addMetric = () => {
    const newMetric = {
      id: `metric_${selectedMetrics.length + 1}`,
      label: '',
      dataKey: '',
      type: 'number',
      precision: undefined
    };
    setSelectedMetrics([...selectedMetrics, newMetric]);
  };

  const updateMetric = (index, field, value) => {
    const updatedMetrics = [...selectedMetrics];
    updatedMetrics[index] = { ...updatedMetrics[index], [field]: value };
    setSelectedMetrics(updatedMetrics);
  };

  const removeMetric = (index) => {
    setSelectedMetrics(selectedMetrics.filter((_, i) => i !== index));
  };

  const toggleVisualization = (vizType) => {
    if (selectedVisualizations.find(v => v.type === vizType)) {
      setSelectedVisualizations(selectedVisualizations.filter(v => v.type !== vizType));
    } else {
      const vizConfig = visualizationTypes.find(v => v.id === vizType);
      setSelectedVisualizations([...selectedVisualizations, {
        type: vizType,
        title: vizConfig.label,
        description: vizConfig.description
      }]);
    }
  };

  const renderStep = () => {
    switch (step) {
      case 'name':
        return (
          <div className="space-y-6">
            <div className="text-center">
              <h2 className="text-2xl font-bold mb-2">Create Your Use Case</h2>
              <p className="text-gray-600">Give your use case a name and description</p>
            </div>
            
            <div className="space-y-4">
              <div>
                <label className="block text-sm font-medium text-gray-700 mb-2">
                  Use Case Name
                </label>
                <input
                  type="text"
                  value={useCaseName}
                  onChange={(e) => setUseCaseName(e.target.value)}
                  placeholder="e.g., Fall Detection, Occupancy Monitoring, Gesture Recognition"
                  className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500"
                />
              </div>
              
              <div>
                <label className="block text-sm font-medium text-gray-700 mb-2">
                  Description
                </label>
                <textarea
                  value={useCaseDescription}
                  onChange={(e) => setUseCaseDescription(e.target.value)}
                  placeholder="Describe what this use case will monitor or detect..."
                  rows={3}
                  className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500"
                />
              </div>
            </div>
          </div>
        );

      case 'metrics':
        return (
          <div className="space-y-6">
            <div className="text-center">
              <h2 className="text-2xl font-bold mb-2">Configure Metrics</h2>
              <p className="text-gray-600">Select the key metrics you want to track</p>
            </div>
            
            <div className="space-y-4">
              {selectedMetrics.map((metric, index) => (
                <div key={index} className="p-4 border border-gray-200 rounded-lg">
                  <div className="flex justify-between items-start mb-3">
                    <h4 className="font-medium">Metric {index + 1}</h4>
                    <button
                      type="button"
                      onClick={() => removeMetric(index)}
                      className="text-red-500 hover:text-red-700 text-sm"
                    >
                      Remove
                    </button>
                  </div>
                  
                  <div className="grid grid-cols-2 gap-4">
                    <div>
                      <label className="block text-sm font-medium text-gray-700 mb-1">
                        Label
                      </label>
                      <input
                        type="text"
                        value={metric.label}
                        onChange={(e) => updateMetric(index, 'label', e.target.value)}
                        placeholder="e.g., Detection Status"
                        className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500"
                      />
                    </div>
                    
                    <div>
                      <label className="block text-sm font-medium text-gray-700 mb-1">
                        Data Key
                      </label>
                      <input
                        type="text"
                        value={metric.dataKey}
                        onChange={(e) => updateMetric(index, 'dataKey', e.target.value)}
                        placeholder="e.g., is_detected"
                        className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500"
                      />
                    </div>
                    
                    <div>
                      <label className="block text-sm font-medium text-gray-700 mb-1">
                        Type
                      </label>
                      <select
                        value={metric.type}
                        onChange={(e) => updateMetric(index, 'type', e.target.value)}
                        className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500"
                      >
                        {metricTypes.map(type => (
                          <option key={type.id} value={type.id}>
                            {type.label}
                          </option>
                        ))}
                      </select>
                    </div>
                    
                    {metric.type === 'number' && (
                      <div>
                        <label className="block text-sm font-medium text-gray-700 mb-1">
                          Decimal Places
                        </label>
                        <input
                          type="number"
                          min="0"
                          max="5"
                          value={metric.precision || ''}
                          onChange={(e) => updateMetric(index, 'precision', e.target.value ? parseInt(e.target.value) : undefined)}
                          placeholder="0"
                          className="w-full px-3 py-2 border border-gray-300 rounded-md focus:outline-none focus:ring-2 focus:ring-blue-500"
                        />
                      </div>
                    )}
                  </div>
                </div>
              ))}
              
              <button
                type="button"
                onClick={addMetric}
                className="w-full py-3 border-2 border-dashed border-gray-300 rounded-lg text-gray-500 hover:border-blue-300 hover:text-blue-500 transition-colors"
              >
                + Add Metric
              </button>
            </div>
          </div>
        );

      case 'visualizations':
        return (
          <div className="space-y-6">
            <div className="text-center">
              <h2 className="text-2xl font-bold mb-2">Choose Visualizations</h2>
              <p className="text-gray-600">Select how you want to visualize your data</p>
            </div>
            
            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              {visualizationTypes.map(vizType => (
                <div
                  key={vizType.id}
                  className={`
                    p-4 rounded-lg border-2 cursor-pointer transition-colors
                    ${selectedVisualizations.find(v => v.type === vizType.id)
                      ? 'border-blue-500 bg-blue-50'
                      : 'border-gray-200 hover:border-blue-200'
                    }
                  `}
                  onClick={() => toggleVisualization(vizType.id)}
                >
                  <div className="flex items-start">
                    <div className="flex-1">
                      <h3 className="font-medium text-gray-900">{vizType.label}</h3>
                      <p className="mt-1 text-sm text-gray-500">{vizType.description}</p>
                    </div>
                    <div className="ml-3 flex h-5 items-center">
                      <input
                        type="checkbox"
                        checked={selectedVisualizations.find(v => v.type === vizType.id) !== undefined}
                        onChange={() => toggleVisualization(vizType.id)}
                        className="h-4 w-4 border-gray-300 text-blue-600 focus:ring-blue-500"
                      />
                    </div>
                  </div>
                </div>
              ))}
            </div>
          </div>
        );

      case 'review':
        return (
          <div className="space-y-6">
            <div className="text-center">
              <h2 className="text-2xl font-bold mb-2">Review Your Configuration</h2>
              <p className="text-gray-600">Review your use case before creating the dashboard</p>
            </div>
            
            <div className="bg-gray-50 rounded-lg p-6 space-y-4">
              <div>
                <h3 className="font-medium text-gray-900 mb-2">Use Case Details</h3>
                <p className="text-sm text-gray-600"><strong>Name:</strong> {useCaseName}</p>
                <p className="text-sm text-gray-600"><strong>Description:</strong> {useCaseDescription}</p>
              </div>
              
              <div>
                <h3 className="font-medium text-gray-900 mb-2">Metrics ({selectedMetrics.length})</h3>
                <ul className="text-sm text-gray-600 space-y-1">
                  {selectedMetrics.map((metric, index) => (
                    <li key={index}>• {metric.label} ({metric.type})</li>
                  ))}
                </ul>
              </div>
              
              <div>
                <h3 className="font-medium text-gray-900 mb-2">Visualizations ({selectedVisualizations.length})</h3>
                <ul className="text-sm text-gray-600 space-y-1">
                  {selectedVisualizations.map((viz, index) => (
                    <li key={index}>• {viz.title}</li>
                  ))}
                </ul>
              </div>
            </div>
          </div>
        );

      default:
        return null;
    }
  };

  return (
    <div className="space-y-6">
      {renderStep()}

      {error && (
        <div className="bg-red-50 text-red-600 p-4 rounded-lg text-sm">
          {error}
        </div>
      )}

      <div className="flex justify-between">
        {step !== 'name' && (
          <button
            type="button"
            onClick={handleBack}
            className="px-6 py-2 border border-gray-300 text-gray-700 rounded-md hover:bg-gray-50"
          >
            Back
          </button>
        )}
        
        <div className="ml-auto">
          {step === 'review' ? (
            <button
              type="button"
              onClick={handleComplete}
              className="px-6 py-2 bg-blue-600 hover:bg-blue-700 text-white font-medium rounded-md"
            >
              Create Dashboard
            </button>
          ) : (
            <button
              type="button"
              onClick={handleNext}
              className="px-6 py-2 bg-blue-600 hover:bg-blue-700 text-white font-medium rounded-md"
            >
              Next
            </button>
          )}
        </div>
      </div>

      {/* Progress indicator */}
      <div className="mt-8 flex justify-center gap-2">
        {['name', 'metrics', 'visualizations', 'review'].map((stepName, index) => (
          <div
            key={stepName}
            className={`h-2 w-16 rounded transition-colors ${
              step === stepName ? 'bg-blue-600' : 
              ['name', 'metrics', 'visualizations', 'review'].indexOf(step) > index ? 'bg-blue-300' : 'bg-gray-200'
            }`}
          />
        ))}
      </div>
    </div>
  );
}

export default UseCaseConfig; 