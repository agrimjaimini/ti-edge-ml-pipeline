import os
import json
import numpy as np
import random
from torch.utils.data import Dataset, DataLoader

def train_test_split(sequences, labels, test_size=0.2, random_seed=42):
    """
    Split sequences into train and validation sets.
    Ensures entire sequences stay together and maintains class balance.
    """
    if random_seed:
        random.seed(random_seed)
    
    # Split by class first to maintain balance
    fall_indices = [i for i, label in enumerate(labels) if label == 1]
    no_fall_indices = [i for i, label in enumerate(labels) if label == 0]
    
    # Calculate split sizes for each class
    n_fall_val = int(len(fall_indices) * test_size)
    n_no_fall_val = int(len(no_fall_indices) * test_size)
    
    # Shuffle indices
    random.shuffle(fall_indices)
    random.shuffle(no_fall_indices)
    
    # Split indices
    val_indices = fall_indices[:n_fall_val] + no_fall_indices[:n_no_fall_val]
    train_indices = fall_indices[n_fall_val:] + no_fall_indices[n_no_fall_val:]
    
    # Create train/val splits
    train_sequences = [sequences[i] for i in train_indices]
    val_sequences = [sequences[i] for i in val_indices]
    train_labels = [labels[i] for i in train_indices]
    val_labels = [labels[i] for i in val_indices]
    
    # Print split statistics
    print("\nSplit Statistics:")
    print(f"Train - Falls: {train_labels.count(1)}, No Falls: {train_labels.count(0)}")
    print(f"Val - Falls: {val_labels.count(1)}, No Falls: {val_labels.count(0)}")
    
    return train_sequences, val_sequences, train_labels, val_labels

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
    def __init__(self, sequences, labels, num_points=128, sequence_length=30):
        self.sequences = sequences  # List of sequences, each with variable frames
        self.labels = labels
        self.num_points = num_points
        self.sequence_length = sequence_length
        
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
        
        # Ensure sequence length is fixed
        if len(normalized) > self.sequence_length:
            # Take the last sequence_length frames
            normalized = normalized[-self.sequence_length:]
        elif len(normalized) < self.sequence_length:
            # Pad with copies of the last frame
            last_frame = normalized[-1] if normalized else np.zeros((self.num_points, 3))
            while len(normalized) < self.sequence_length:
                normalized.append(last_frame.copy())
        
        return np.stack(normalized)  # (sequence_length, num_points, 3)
    
    def __getitem__(self, idx):
        sequence = self.sequences[idx]  # List of (Ni, 3) arrays
        label = self.labels[idx]
        
        # Normalize point clouds and sequence length
        sequence = self.normalize_sequence(sequence)
        
        return sequence.astype(np.float32), np.int64(label)

def get_dataloaders(data_dir,
                    batch_size=32,
                    num_points=128,
                    sequence_length=30,
                    test_split=0.2,
                    random_seed=42,
                    return_raw=False):
    """Create train and validation dataloaders"""
    
    # Load sequences and labels
    sequences, labels = load_sequences(data_dir)
    
    # Return raw data if requested (for class weights calculation)
    if return_raw:
        return sequences, labels
    
    # Split data while maintaining class balance
    X_train, X_val, y_train, y_val = train_test_split(
        sequences, labels,
        test_size=test_split,
        random_seed=random_seed
    )
    
    # Create datasets
    train_ds = FallDetectionDataset(X_train, y_train, 
                                   num_points=num_points,
                                   sequence_length=sequence_length)
    val_ds = FallDetectionDataset(X_val, y_val, 
                                 num_points=num_points,
                                 sequence_length=sequence_length)
    
    # Create dataloaders with proper shuffling
    train_loader = DataLoader(train_ds,
                            batch_size=batch_size,
                            shuffle=True,  # Shuffle training data
                            num_workers=4,
                            drop_last=True)  # Drop incomplete batches
    val_loader = DataLoader(val_ds,
                           batch_size=batch_size,
                           shuffle=False,  # Don't shuffle validation
                           num_workers=4,
                           drop_last=False)  # Keep all validation samples
    
    print("\nDataset split:")
    print(f"Training: {len(X_train)} sequences")
    print(f"Validation: {len(X_val)} sequences")
    
    return train_loader, val_loader 