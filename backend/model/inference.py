# model/inference.py
import os, numpy as np, torch
from .model import PointNetClassifier
import sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from database import model_db

# (A) Where the weights live
HERE       = os.path.dirname(__file__)
REPO_ROOT  = os.path.abspath(os.path.join(HERE, "..", ".."))
MODEL_DIR = DEFAULT_MODELS_DIR = os.path.join(REPO_ROOT, "models")
DEVICE     = torch.device("cuda" if torch.cuda.is_available() else "cpu")

_model: PointNetClassifier = None
_current_model_path: str = None

def _load_model(model_name: str, num_classes: int) -> PointNetClassifier:
    global _model, _current_model_path
    
    model_info = model_db.get_model(model_name)
    if not model_info:
        raise ValueError(f"Model '{model_name}' not found in database")
    
    model_path = model_info['file_path']
    # Check if we need to load a new model (different path or no model loaded)
    if _model is None or _current_model_path != model_path:
        m = PointNetClassifier(num_classes=num_classes)
        
        # Ensure the model path exists
        if not os.path.exists(model_path):
            raise FileNotFoundError(f"Model file not found: {model_path}")
            
        state = torch.load(model_path, map_location=DEVICE)
        m.load_state_dict(state)
        m.to(DEVICE).eval()
        _model = m
        _current_model_path = model_path
    
    return _model

def _sample_or_pad(pts: np.ndarray, num_points: int = 128) -> np.ndarray:
    N = pts.shape[0]
    if N >= num_points:
        idx = np.random.choice(N, num_points, replace=False)
        return pts[idx]
    else:
        pad_idx = np.random.choice(N, num_points - N, replace=True)
        return np.vstack([pts, pts[pad_idx]])

def predict(sensor_data: list[list[float]], model_name: str, num_classes: int) -> dict:
    """
    sensor_data: list of [x,y,z] points for one frame
    model_path: string path to the model file to load
    num_classes: number of classes the model was trained on
    num_points: number of points to sample/pad to
    returns: {
      'predicted_count': int,
      'probabilities': [p0, p1, p2, p3, p4]  # sum to 1
    }
    """
    model = _load_model(model_name, num_classes)

    pts = np.array(sensor_data, dtype=np.float32)      # (M,3)
    pts_fixed = _sample_or_pad(pts, num_points=128)    # (128,3)
    x = torch.from_numpy(pts_fixed)                    # → (128,3)
    x = x.unsqueeze(0).to(DEVICE)                      # → (1,128,3)

    with torch.no_grad():
        logits = model(x)                              # (1,5)
        probs  = torch.softmax(logits, dim=1)[0].cpu().numpy()

    pred_count = int(probs.argmax())
    return {
        "predicted_count": pred_count,
        "probabilities":    probs.tolist()
    }