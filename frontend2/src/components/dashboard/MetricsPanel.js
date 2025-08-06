function MetricsPanel({ metrics, data }) {
  const renderMetricValue = (metric) => {
    const value = data?.[metric.dataKey];
    
    switch (metric.type) {
      case 'status':
        return (
          <span className={`px-3 py-1 rounded-full text-sm font-semibold
            ${value ? 'bg-red-100 text-red-600' : 'bg-green-100 text-green-600'}`}
          >
            {value ? 'Detected' : 'Clear'}
          </span>
        );
      
      case 'percentage':
        return `${(value * 100).toFixed(1)}%`;
      
      case 'timestamp':
        return value ? new Date(value).toLocaleString() : '-';
      
      case 'number':
        return metric.precision !== undefined
          ? Number(value).toFixed(metric.precision)
          : value ?? 0;
      
      default:
        return value ?? '-';
    }
  };

  return (
    <div className="grid gap-4">
      {metrics.map((metric) => (
        <div
          key={metric.id}
          className="rounded-2xl bg-white p-6 shadow-md flex flex-col justify-between"
        >
          <div>
            <h2 className="font-semibold text-gray-700 mb-1">{metric.label}</h2>
            <div className="mt-4 flex items-center justify-between">
              <span className="text-3xl font-bold text-blue-600">
                {renderMetricValue(metric)}
              </span>
            </div>
          </div>
        </div>
      ))}
    </div>
  );
}

export default MetricsPanel; 