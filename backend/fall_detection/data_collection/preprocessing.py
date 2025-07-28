import os
import json
import numpy as np
from torch.utils.data import Dataset, DataLoader
from sklearn.model_selection import train_test_split

def load_sequences(data_dir):
    """
    Load fall and no-fall sequences from JSON files
    """
    sequences = []
    labels = []
    
    # Load fall sequences
    fall_path = os.path.join(data_dir, "fall_sequences.json")
    if os.path.exists(fall_path):
        with open(fall_path, 'r') as f:
            data = json.load(f)
            for seq in data["sequences"]:
                # Extract point clouds from frames
                sequence_clouds = []
                for frame in seq["frames"]:
                    xyz = np.stack([
                        frame['x_pos'],
                        frame['y_pos'],
                        frame['z_pos']
                    ], axis=1)  # shape (Ni, 3)
                    sequence_clouds.append(xyz)
                sequences.append(sequence_clouds)
                labels.append(1)  # 1 for fall
    
    # Load no-fall sequences
    no_fall_path = os.path.join(data_dir, "no_fall_sequences.json")
    if os.path.exists(no_fall_path):
        with open(no_fall_path, 'r') as f:
            data = json.load(f)
            for seq in data["sequences"]:
                sequence_clouds = []
                for frame in seq["frames"]:
                    xyz = np.stack([
                        frame['x_pos'],
                        frame['y_pos'],
                        frame['z_pos']
                    ], axis=1)
                sequence_clouds.append(xyz)
                sequences.append(sequence_clouds)
                labels.append(0)  # 0 for no_fall
    
    print(f"Loaded {len(sequences)} sequences")
    print(f"Falls: {labels.count(1)}, No Falls: {labels.count(0)}")
    return sequences, labels

class FallDetectionDataset(Dataset):
    def __init__(self, sequences, labels, num_points=128):
        self.sequences = sequences  # List of sequences, each with 20 frames
        self.labels = labels
        self.num_points = num_points
        
    def __len__(self):
        return len(self.sequences)
    
    def normalize_sequence(self, sequence):
        """Normalize each frame in sequence to fixed number of points"""
        normalized = []
        for cloud in sequence:
            N = cloud.shape[0]
            if N >= self.num_points:
                choice = np.random.choice(N, self.num_points, replace=False)
                normalized.append(cloud[choice, :])
            else:
                pad = np.zeros((self.num_points - N, 3), dtype=cloud.dtype)
                normalized.append(np.vstack([cloud, pad]))
        return np.stack(normalized)  # (20, num_points, 3)
    
    def __getitem__(self, idx):
        sequence = self.sequences[idx]  # List of (Ni, 3) arrays
        label = self.labels[idx]
        
        # Normalize point clouds
        sequence = self.normalize_sequence(sequence)
        
        return sequence.astype(np.float32), np.int64(label)

def get_dataloaders(data_dir,
                    batch_size=32,
                    num_points=128,
                    test_split=0.2,
                    random_seed=42):
    """Create train and validation dataloaders"""
    
    # Load sequences and labels
    sequences, labels = load_sequences(data_dir)
    
    # Split data
    X_train, X_val, y_train, y_val = train_test_split(
        sequences, labels,
        test_size=test_split,
        random_state=random_seed,
        stratify=labels
    )
    
    # Create datasets
    train_ds = FallDetectionDataset(X_train, y_train, num_points=num_points)
    val_ds = FallDetectionDataset(X_val, y_val, num_points=num_points)
    
    # Create dataloaders
    train_loader = DataLoader(train_ds,
                            batch_size=batch_size,
                            shuffle=True,
                            num_workers=4)
    val_loader = DataLoader(val_ds,
                           batch_size=batch_size,
                           shuffle=False,
                           num_workers=4)
    
    print("\nDataset split:")
    print(f"Training: {len(X_train)} sequences")
    print(f"Validation: {len(X_val)} sequences")
    
    return train_loader, val_loader 