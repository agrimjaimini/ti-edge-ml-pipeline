import torch
import numpy as np
from collections import deque
from ..model.pointnet_lstm import PointNetLSTM

class FallDetector:
    def __init__(self, model_path, sequence_length=20, num_points=128, device=None):
        self.sequence_length = sequence_length
        self.num_points = num_points
        self.device = device or torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        
        # Initialize model
        self.model = PointNetLSTM(num_points=num_points)
        state_dict = torch.load(model_path, map_location=self.device)
        self.model.load_state_dict(state_dict)
        self.model.to(self.device)
        self.model.eval()
        
        # Initialize frame buffer
        self.frame_buffer = deque(maxlen=sequence_length)
        
    def normalize_points(self, points):
        """Normalize point cloud to fixed number of points"""
        N = len(points)
        if N >= self.num_points:
            choice = np.random.choice(N, self.num_points, replace=False)
            return points[choice]
        else:
            pad = np.zeros((self.num_points - N, 3), dtype=points.dtype)
            return np.vstack([points, pad])
            
    def process_frame(self, frame_data):
        """Process a single frame and return fall detection if sequence is complete"""
        # Extract and normalize point cloud
        points = np.stack([
            frame_data['x_pos'],
            frame_data['y_pos'],
            frame_data['z_pos']
        ], axis=1)
        
        normalized_points = self.normalize_points(points)
        self.frame_buffer.append(normalized_points)
        
        # If buffer is full, perform inference
        if len(self.frame_buffer) == self.sequence_length:
            return self.detect_fall()
        
        return None
        
    def detect_fall(self):
        """Perform fall detection on current sequence"""
        with torch.no_grad():
            # Prepare sequence
            sequence = np.stack(list(self.frame_buffer))
            sequence = torch.from_numpy(sequence).float()
            sequence = sequence.unsqueeze(0)  # Add batch dimension
            sequence = sequence.to(self.device)
            
            # Get prediction
            outputs = self.model(sequence)
            probabilities = torch.softmax(outputs, dim=1)
            fall_prob = probabilities[0, 1].item()
            
            return {
                "is_fall": fall_prob > 0.5,
                "fall_probability": fall_prob,
                "sequence_complete": True
            }
            
    def reset(self):
        """Clear the frame buffer"""
        self.frame_buffer.clear() 