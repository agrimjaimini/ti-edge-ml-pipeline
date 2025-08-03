# model.py

import torch
import torch.nn as nn
import torch.optim as optim
from .preprocessing import get_dataloaders
import os
import sys
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from utils import get_model_path

class TNet(nn.Module):
    def __init__(self, k=5):  # Changed from k=3 to k=5 for 5D input
        super().__init__()
        self.k = k
        self.conv1 = nn.Conv1d(k, 64, 1)
        self.conv2 = nn.Conv1d(64, 128, 1)
        self.conv3 = nn.Conv1d(128, 1024, 1)
        self.fc1   = nn.Linear(1024, 512)
        self.fc2   = nn.Linear(512, 256)
        self.fc3   = nn.Linear(256, k*k)

        self.bn1 = nn.BatchNorm1d(64)
        self.bn2 = nn.BatchNorm1d(128)
        self.bn3 = nn.BatchNorm1d(1024)
        self.bn4 = nn.BatchNorm1d(512)
        self.bn5 = nn.BatchNorm1d(256)

    def forward(self, x):
        batchsize = x.size(0)
        x = self.bn1(self.conv1(x))
        x = self.bn2(self.conv2(x))
        x = self.bn3(self.conv3(x))
        x = torch.max(x, 2)[0]
        x = torch.relu(self.bn4(self.fc1(x)))
        x = torch.relu(self.bn5(self.fc2(x)))
        init = torch.eye(self.k, requires_grad=False).repeat(batchsize,1,1).to(x.device)
        mat = self.fc3(x).view(-1, self.k, self.k) + init
        return mat

class PointNetClassifier(nn.Module):
    def __init__(self, num_classes):
        super().__init__()
        self.input_tnet = TNet(k=5)  # Changed from k=3 to k=5 for 5D input
        self.conv1 = nn.Conv1d(5, 64, 1)  # Changed from 3 to 5 input channels
        self.bn1   = nn.BatchNorm1d(64)

        self.feature_tnet = TNet(k=64)
        self.conv2 = nn.Conv1d(64, 128, 1)
        self.conv3 = nn.Conv1d(128, 1024, 1)
        self.bn2   = nn.BatchNorm1d(128)
        self.bn3   = nn.BatchNorm1d(1024)

        self.fc1 = nn.Linear(1024, 512)
        self.bn4 = nn.BatchNorm1d(512)
        self.drop1 = nn.Dropout(p=0.5)
        self.fc2 = nn.Linear(512, 256)
        self.bn5 = nn.BatchNorm1d(256)
        self.drop2 = nn.Dropout(p=0.5)
        self.fc3 = nn.Linear(256, num_classes)

    def forward(self, x):
        # x: (B, N, 5) - now includes x,y,z,snr,noise
        x = x.transpose(2,1)  # (B, 5, N)
        trans = self.input_tnet(x)
        x = torch.bmm(trans, x)
        x = torch.relu(self.bn1(self.conv1(x)))

        trans_feat = self.feature_tnet(x)
        x = torch.bmm(trans_feat, x)
        x = torch.relu(self.bn2(self.conv2(x)))
        x = self.bn3(self.conv3(x))  # (B, 1024, N)
        x = torch.max(x, 2)[0]       # (B, 1024)

        x = torch.relu(self.bn4(self.fc1(x)))
        x = self.drop1(x)
        x = torch.relu(self.bn5(self.fc2(x)))
        x = self.drop2(x)
        x = self.fc3(x)
        return x

def train(model, loader, optimizer, criterion, device, reg_weight=0.001):
    model.train()
    running_loss = 0.0
    for points, labels in loader:
        points, labels = points.to(device), labels.to(device)
        optimizer.zero_grad()
        outputs = model(points)
        loss = criterion(outputs, labels)
        reg_loss = 0.0
        if hasattr(model, 'input_tnet') and hasattr(model, 'feature_tnet'):
            x = points.transpose(2,1)
            trans = model.input_tnet(x)
            I5 = torch.eye(5).to(trans.device).unsqueeze(0)  # Changed from I3 to I5
            diff5 = torch.bmm(trans, trans.transpose(1,2)) - I5
            reg_loss += torch.mean(torch.norm(diff5, dim=(1,2))**2)
            x = torch.bmm(trans, x)
            # Use the model's conv1 and bn1 layers for feature transformation
            x = torch.relu(model.bn1(model.conv1(x)))
            trans_feat = model.feature_tnet(x)
            I64 = torch.eye(64).to(trans_feat.device).unsqueeze(0)
            diff64 = torch.bmm(trans_feat, trans_feat.transpose(1,2)) - I64
            reg_loss += torch.mean(torch.norm(diff64, dim=(1,2))**2)
        loss = loss + reg_weight * reg_loss
        loss.backward()
        optimizer.step()
        running_loss += loss.item() * points.size(0)
    return running_loss / len(loader.dataset)

def evaluate(model, loader, criterion, device):
    model.eval()
    correct = total = 0
    loss_sum = 0.0
    with torch.no_grad():
        for points, labels in loader:
            points, labels = points.to(device), labels.to(device)
            outputs = model(points)
            loss = criterion(outputs, labels)
            loss_sum += loss.item() * points.size(0)
            preds = outputs.argmax(dim=1)
            correct += (preds == labels).sum().item()
            total += labels.size(0)
    return loss_sum / len(loader.dataset), correct / total

def create_model(name: str, num_classes: int, data_dir: str, epochs: int, batch_size: int, learning_rate: float, weight_decay: float, progress_callback=None):

    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    train_loader, val_loader = get_dataloaders(data_dir, batch_size, num_points=128)
    model = PointNetClassifier(num_classes).to(device)
    optimizer = optim.Adam(model.parameters(), lr=learning_rate, weight_decay=weight_decay)
    criterion = nn.CrossEntropyLoss()
    scheduler = torch.optim.lr_scheduler.StepLR(optimizer, step_size=20, gamma=0.5)

    for epoch in range(1, epochs + 1):
        train_loss = train(model, train_loader, optimizer, criterion, device)
        val_loss, val_acc = evaluate(model, val_loader, criterion, device)
        scheduler.step()

        print(f'Epoch {epoch:02d} | Train Loss: {train_loss:.4f} | Val Loss: {val_loss:.4f} | Val Acc: {val_acc:.4f}')
        
        if progress_callback:
            progress_data = {
                "event": "training_progress",
                "model_name": name,
                "epoch": epoch,
                "total_epochs": epochs,
                "train_loss": float(train_loss),
                "val_loss": float(val_loss),
                "val_accuracy": float(val_acc),
                "progress_percent": (epoch / epochs) * 100
            }
            progress_callback(progress_data)

    model_path = get_model_path(name)
    os.makedirs(os.path.dirname(model_path), exist_ok=True)
    torch.save(model.state_dict(), model_path)
    
    # Send completion notification
    if progress_callback:
        completion_data = {
            "event": "training_complete",
            "model_name": name,
            "final_train_loss": float(train_loss),
            "final_val_loss": float(val_loss),
            "final_val_accuracy": float(val_acc),
            "model_path": model_path
        }
        progress_callback(completion_data)
    

