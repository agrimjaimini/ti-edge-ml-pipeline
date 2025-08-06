import { useState } from 'react';
import axios from 'axios';

function ModelTraining({ onComplete }) {
  const [formData, setFormData] = useState({
    name: '',
    num_classes: 2,
    data_dir: '',  // Add the missing data_dir field
    epochs: 6,
    batch_size: 8,
    learning_rate: 0.001,
    weight_decay: 0.01
  });

  const [training, setTraining] = useState(false);
  const [error, setError] = useState(null);

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError(null);
    setTraining(true);

    try {
      const response = await axios.post('http://localhost:8000/create_model', formData);
      
      if (response.data.status === 'success') {
        onComplete(response.data.model_info);
      } else {
        setError(response.data.message);
      }
    } catch (err) {
      setError(err.response?.data?.message || 'Failed to start training');
    } finally {
      setTraining(false);
    }
  };

  const handleChange = (e) => {
    const { name, value } = e.target;
    setFormData(prev => ({
      ...prev,
      [name]: (name === 'name' || name === 'data_dir') ? value : Number(value)
    }));
  };

  return (
    <div className="space-y-6">
      <div className="text-center">
        <h2 className="text-2xl font-bold mb-2">Configure Model Training</h2>
        <p className="text-gray-600">Set up your model training parameters</p>
      </div>

      <form onSubmit={handleSubmit} className="space-y-4">
        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Model Name
          </label>
          <input
            type="text"
            name="name"
            value={formData.name}
            onChange={handleChange}
            className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-blue-500 focus:border-blue-500"
            required
          />
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Number of Classes
          </label>
          <input
            type="number"
            name="num_classes"
            value={formData.num_classes}
            onChange={handleChange}
            min="2"
            className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-blue-500 focus:border-blue-500"
            required
          />
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Data Directory
          </label>
          <input
            type="text"
            name="data_dir"
            value={formData.data_dir}
            onChange={handleChange}
            placeholder="Leave empty for default data directory"
            className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-blue-500 focus:border-blue-500"
          />
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Number of Epochs
          </label>
          <input
            type="number"
            name="epochs"
            value={formData.epochs}
            onChange={handleChange}
            min="1"
            className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-blue-500 focus:border-blue-500"
            required
          />
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Batch Size
          </label>
          <input
            type="number"
            name="batch_size"
            value={formData.batch_size}
            onChange={handleChange}
            min="1"
            className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-blue-500 focus:border-blue-500"
            required
          />
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Learning Rate
          </label>
          <input
            type="number"
            name="learning_rate"
            value={formData.learning_rate}
            onChange={handleChange}
            step="0.0001"
            min="0.0001"
            className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-blue-500 focus:border-blue-500"
            required
          />
        </div>

        <div>
          <label className="block text-sm font-medium text-gray-700 mb-1">
            Weight Decay
          </label>
          <input
            type="number"
            name="weight_decay"
            value={formData.weight_decay}
            onChange={handleChange}
            step="0.001"
            min="0"
            className="w-full px-3 py-2 border border-gray-300 rounded-md shadow-sm focus:ring-blue-500 focus:border-blue-500"
            required
          />
        </div>

        {error && (
          <div className="bg-red-50 text-red-600 p-4 rounded-lg text-sm">
            {error}
          </div>
        )}

        <button
          type="submit"
          disabled={training}
          className={`
            w-full py-2 px-4 rounded-md text-white font-medium
            ${training
              ? 'bg-blue-400 cursor-not-allowed'
              : 'bg-blue-600 hover:bg-blue-700'
            }
          `}
        >
          {training ? 'Training...' : 'Start Training'}
        </button>
      </form>
    </div>
  );
}

export default ModelTraining; 