# model/inference.py
import os, numpy as np, torch
from .model import PointNetClassifier

# (A) Where the weights live
HERE       = os.path.dirname(__file__)
REPO_ROOT  = os.path.abspath(os.path.join(HERE, "..", ".."))
MODEL_PATH = os.path.join(REPO_ROOT, "pointnet_occupancy.pth")
DEVICE     = torch.device("cuda" if torch.cuda.is_available() else "cpu")

_model: PointNetClassifier = None

def _load_model(num_classes=4) -> PointNetClassifier:
    global _model
    if _model is None:
        m = PointNetClassifier(num_classes=num_classes)
        state = torch.load(MODEL_PATH, map_location=DEVICE)
        m.load_state_dict(state)
        m.to(DEVICE).eval()
        _model = m
    return _model

def _sample_or_pad(pts: np.ndarray, num_points: int = 128) -> np.ndarray:
    N = pts.shape[0]
    if N >= num_points:
        idx = np.random.choice(N, num_points, replace=False)
        return pts[idx]
    else:
        pad_idx = np.random.choice(N, num_points - N, replace=True)
        return np.vstack([pts, pts[pad_idx]])

def predict_occupancy(sensor_data: list[list[float]]) -> dict:
    """
    sensor_data: list of [x,y,z] points for one frame
    returns: {
      'predicted_count': int,
      'probabilities': [p0, p1, p2, p3]  # sum to 1
    }
    """
    model = _load_model(num_classes=4)

    pts = np.array(sensor_data, dtype=np.float32)      # (M,3)
    pts_fixed = _sample_or_pad(pts, num_points=128)    # (128,3)
    x = torch.from_numpy(pts_fixed)                    # → (128,3)
    x = x.unsqueeze(0).to(DEVICE)                      # → (1,128,3)

    with torch.no_grad():
        logits = model(x)                              # (1,4)
        probs  = torch.softmax(logits, dim=1)[0].cpu().numpy()

    pred_count = int(probs.argmax())
    return {
        "predicted_count": pred_count,
        "probabilities":    probs.tolist()
    }