import numpy as np

def predict_fall_probability(data: list) -> float:
    probability = np.clip(np.random.normal(loc=0.5, scale=0.2), 0, 1)
    return float(probability)