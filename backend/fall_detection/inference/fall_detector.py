import torch
import numpy as np
from collections import deque
from ..model.pointnet_lstm import PointNetLSTM

class FallDetector:
    def __init__(self, model_path, sequence_length=30, num_points=128, device=None):
        self.sequence_length = sequence_length
        self.num_points = num_points
        self.device = device or torch.device('cuda' if torch.cuda.is_available() else 'cpu')
        
        # Initialize model
        self.model = PointNetLSTM(num_points=num_points)
        state_dict = torch.load(model_path, map_location=self.device)
        self.model.load_state_dict(state_dict)
        self.model.to(self.device)
        self.model.eval()
        
        # Initialize frame buffer as a deque with maxlen
        # Using append for oldest→newest order
        self.frame_buffer = deque(maxlen=sequence_length)
        
        # Initialize EMA
        self.ema_alpha = 0.7  # Higher alpha means more weight on recent values
        self.ema_value = None  # Will be initialized on first prediction
        
    def normalize_points(self, points):
        """Normalize point cloud to fixed number of points"""
        N = len(points)
        if N >= self.num_points:
            choice = np.random.choice(N, self.num_points, replace=False)
            return points[choice]
        else:
            pad = np.zeros((self.num_points - N, 3), dtype=points.dtype)
            return np.vstack([points, pad])
            
    def update_ema(self, new_value):
        """Update exponential moving average"""
        if self.ema_value is None:
            self.ema_value = new_value
        else:
            self.ema_value = self.ema_alpha * new_value + (1 - self.ema_alpha) * self.ema_value
        return self.ema_value
            
    def process_frame(self, frame_data):
        """Process a single frame and return fall detection result
        Returns None until buffer has enough frames for first inference"""
        try:
            # Extract and normalize point cloud
            points = np.stack([
                frame_data['x_pos'],
                frame_data['y_pos'],
                frame_data['z_pos']
            ], axis=1)
            
            normalized_points = self.normalize_points(points)
            # Add newest frame to the end, maintaining oldest→newest order
            self.frame_buffer.append(normalized_points)
            
            # Perform inference if we have enough frames
            if len(self.frame_buffer) >= self.sequence_length:
                return self.detect_fall()
            
            return None
        except Exception as e:
            print(f"Error processing frame: {e}")
            return None
        
    def detect_fall(self):
        """Perform fall detection on current sequence"""
        try:
            with torch.no_grad():
                # Convert deque to numpy array
                # Frames are in oldest→newest order
                sequence = np.stack(list(self.frame_buffer))
                sequence = torch.from_numpy(sequence).float()
                sequence = sequence.unsqueeze(0)  # Add batch dimension
                sequence = sequence.to(self.device)
                
                # Get prediction
                outputs = self.model(sequence)
                probabilities = torch.softmax(outputs, dim=1)
                fall_prob = probabilities[0, 1].item()
                
                # Update EMA
                smoothed_prob = self.update_ema(fall_prob)
                
                return {
                    "is_fall": smoothed_prob > 0.7,  # Threshold applied to smoothed probability
                    "fall_probability": smoothed_prob,  # Return smoothed probability
                    "raw_probability": fall_prob,  # Also return raw probability for debugging
                    "sequence_complete": True
                }
        except Exception as e:
            print(f"Error during fall detection: {e}")
            return None
            
    def reset(self):
        """Clear the frame buffer and reset EMA"""
        self.frame_buffer.clear()
        self.ema_value = None  # Reset EMA when clearing buffer 