# preprocess.py

import os
import json
import numpy as np
from glob import glob
from torch.utils.data import Dataset, DataLoader
from sklearn.model_selection import train_test_split

def load_all_frames(data_dir):
    """
    Load every frame from every JSON and return
    two lists: clouds (N_i x 3 arrays) and labels.
    """
    clouds = []
    labels = []
    files = sorted(glob(os.path.join(data_dir, '*.json')))
    for path in files:
        # filename like "0people.json" → label 0
        label = int(os.path.basename(path).split('people')[0])
        with open(path, 'r') as f:
            frames = json.load(f)
        for frame in frames:
            xyz = np.stack([
                frame['x_pos'],
                frame['y_pos'],
                frame['z_pos']
            ], axis=1)  # shape (Ni, 3)
            clouds.append(xyz)
            labels.append(label)
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
        cloud = self.clouds[idx]  # (Ni, 3)
        label = self.labels[idx]

        N = cloud.shape[0]
        if N >= self.num_points:
            choice = np.random.choice(N, self.num_points, replace=False)
            cloud = cloud[choice, :]
        else:
            pad = np.zeros((self.num_points - N, 3), dtype=cloud.dtype)
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

    return train_loader, val_loader

if __name__ == '__main__':
    tr, va = get_dataloaders('data/json', batch_size=16, num_points=128)
    print(f"{len(tr.dataset)} train frames, {len(va.dataset)} val frames")