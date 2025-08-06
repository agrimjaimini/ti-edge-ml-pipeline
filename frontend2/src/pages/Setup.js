import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import DatasetUpload from '../components/setup/DatasetUpload';
import ModelTraining from '../components/setup/ModelTraining';
import UseCaseConfig from '../components/setup/UseCaseConfig';
import { useAppContext } from '../context/AppContext';

function Setup() {
  const [step, setStep] = useState('upload'); // upload, train, configure
  const { setUseCase, setModel } = useAppContext();
  const navigate = useNavigate();

  const handleUpload = async (dataset) => {
    // Handle dataset upload
    setStep('train');
  };

  const handleTrainingComplete = (trainedModel) => {
    setModel(trainedModel);
    setStep('configure');
  };

  const handleSetupComplete = (selectedUseCase) => {
    setUseCase(selectedUseCase);
    // Navigate to dashboard using React Router
    navigate('/dashboard');
  };

  return (
    <div className="min-h-screen bg-gradient-to-b from-gray-50 to-white p-8 font-sans">
      <header className="text-center mb-12">
        <h1 className="text-4xl font-bold mb-4">TI Edge ML Pipeline Setup</h1>
        <p className="text-gray-600">Configure your machine learning pipeline</p>
      </header>

      <div className="max-w-2xl mx-auto">
        {step === 'upload' && (
          <DatasetUpload onUpload={handleUpload} />
        )}
        {step === 'train' && (
          <ModelTraining onComplete={handleTrainingComplete} />
        )}
        {step === 'configure' && (
          <UseCaseConfig onComplete={handleSetupComplete} />
        )}

        {/* Progress indicator */}
        <div className="mt-8 flex justify-center gap-2">
          <div className={`h-2 w-16 rounded ${step === 'upload' ? 'bg-blue-600' : 'bg-gray-200'}`} />
          <div className={`h-2 w-16 rounded ${step === 'train' ? 'bg-blue-600' : 'bg-gray-200'}`} />
          <div className={`h-2 w-16 rounded ${step === 'configure' ? 'bg-blue-600' : 'bg-gray-200'}`} />
        </div>
      </div>
    </div>
  );
}

export default Setup; 