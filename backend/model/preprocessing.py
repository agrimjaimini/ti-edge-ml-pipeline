import os
import json
import numpy as np
from glob import glob
from torch.utils.data import Dataset, DataLoader
from sklearn.model_selection import train_test_split
import sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from utils import get_data_subdir

def load_all_frames(data_dir):
    """
    Load every frame from every JSON and return
    two lists: clouds (N_i x 5 arrays with x,y,z,snr,noise) and labels.
    
    Args:
        data_dir (str): Directory containing JSON files
    """
    clouds = []
    labels = []
    # Use absolute path for data directory
    data_path = get_data_subdir(data_dir)
    files = sorted(glob(os.path.join(data_path, '*.json')))
    for path in files:
        try:
            with open(path, 'r') as f:
                frames = json.load(f)
            for frame in frames:
                # Check if SNR and noise data are available
                if 'snr' in frame and 'noise' in frame and len(frame['snr']) > 0 and len(frame['noise']) > 0:
                    # Create 5D features: [x, y, z, snr, noise]
                    xyz_snr_noise = np.stack([
                        frame['x_pos'],
                        frame['y_pos'],
                        frame['z_pos'],
                        frame['snr'],
                        frame['noise']
                    ], axis=1)  # shape (Ni, 5)
                else:
                    # Fallback to 3D if SNR/noise not available, pad with zeros
                    xyz = np.stack([
                        frame['x_pos'],
                        frame['y_pos'],
                        frame['z_pos']
                    ], axis=1)  # shape (Ni, 3)
                    # Pad with zeros for SNR and noise
                    zeros = np.zeros((xyz.shape[0], 2))
                    xyz_snr_noise = np.hstack([xyz, zeros])  # shape (Ni, 5)
                
                clouds.append(xyz_snr_noise)
                labels.append(frame['people_count'])  # Use people_count as label
        except (json.JSONDecodeError, KeyError, FileNotFoundError) as e:
            print(f"Error loading file {path}: {e}")
            continue
    return clouds, labels

class RadarDataset(Dataset):
    def __init__(self, clouds, labels, num_points=128, transform=None):
        self.clouds = clouds
        self.labels = labels
        self.num_points = num_points
        self.transform = transform

    def __len__(self):
        return len(self.clouds)

    def __getitem__(self, idx):
        cloud = self.clouds[idx]  # (Ni, 5) - now includes x,y,z,snr,noise
        label = self.labels[idx]

        N = cloud.shape[0]
        if N >= self.num_points:
            choice = np.random.choice(N, self.num_points, replace=False)
            cloud = cloud[choice, :]
        else:
            pad = np.zeros((self.num_points - N, 5), dtype=cloud.dtype)
            cloud = np.vstack([cloud, pad])

        if self.transform:
            cloud = self.transform(cloud)

        return cloud.astype(np.float32), np.int64(label)

def get_dataloaders(data_dir,
                    batch_size=32,
                    num_points=128,
                    test_split=0.2,
                    random_seed=42):
    # 1) load every frame & label
    clouds, labels = load_all_frames(data_dir)
    
    # Check if we have any data
    if not clouds or not labels:
        raise ValueError(f"No valid data found in {data_dir}")

    # 2) stratified split so each class appears in both sets
    X_train, X_val, y_train, y_val = train_test_split(
        clouds, labels,
        test_size=test_split,
        random_state=random_seed,
        stratify=labels
    )

    # 3) wrap them in Datasets + DataLoaders
    train_ds = RadarDataset(X_train, y_train, num_points=num_points)
    val_ds   = RadarDataset(X_val,   y_val,   num_points=num_points)

    train_loader = DataLoader(train_ds,
                              batch_size=batch_size,
                              shuffle=True,
                              num_workers=4)
    val_loader   = DataLoader(val_ds,
                              batch_size=batch_size,
                              shuffle=False,
                              num_workers=4)

    # optional: print class distributions
    from collections import Counter
    print("Train label counts:", Counter(y_train))
    print("Val   label counts:", Counter(y_val))
    print(f"Input features: {clouds[0].shape[1]}D (x,y,z,snr,noise)")

    return train_loader, val_loader

if __name__ == '__main__':
    tr, va = get_dataloaders('json', batch_size=16, num_points=128)
    print(f"{len(tr.dataset)} train frames, {len(va.dataset)} val frames")