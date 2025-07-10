# model.py

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.tensorboard import SummaryWriter
from model.data_collection.preprocessing import get_dataloaders
class TNet(nn.Module):
    def __init__(self, k=3):
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
        init = torch.eye(self.k, requires_grad=True).repeat(batchsize,1,1).to(x.device)
        mat = self.fc3(x).view(-1, self.k, self.k) + init
        return mat

class PointNetClassifier(nn.Module):
    def __init__(self, num_classes=4):
        super().__init__()
        self.input_tnet = TNet(k=3)
        self.conv1 = nn.Conv1d(3, 64, 1)
        self.bn1   = nn.BatchNorm1d(64)

        self.feature_tnet = TNet(k=64)
        self.conv2 = nn.Conv1d(64, 128, 1)
        self.conv3 = nn.Conv1d(128, 1024, 1)
        self.bn2   = nn.BatchNorm1d(128)
        self.bn3   = nn.BatchNorm1d(1024)

        self.fc1 = nn.Linear(1024, 512)
        self.bn4 = nn.BatchNorm1d(512)
        self.drop1 = nn.Dropout(p=0.3)
        self.fc2 = nn.Linear(512, 256)
        self.bn5 = nn.BatchNorm1d(256)
        self.drop2 = nn.Dropout(p=0.3)
        self.fc3 = nn.Linear(256, num_classes)

    def forward(self, x):
        # x: (B, N, 3)
        x = x.transpose(2,1)  # (B, 3, N)
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

def train(model, loader, optimizer, criterion, device):
    model.train()
    running_loss = 0.0
    for points, labels in loader:
        points, labels = points.to(device), labels.to(device)
        optimizer.zero_grad()
        outputs = model(points)
        loss = criterion(outputs, labels)
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

if __name__ == '__main__':
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    train_loader, val_loader = get_dataloaders('data/json', batch_size=32, num_points=128)

    model = PointNetClassifier(num_classes=4).to(device)
    optimizer = optim.Adam(model.parameters(), lr=1e-3)
    criterion = nn.CrossEntropyLoss()

    writer = SummaryWriter(log_dir='runs/pointnet_radar')

    epochs = 10
    for epoch in range(1, epochs + 1):
        train_loss = train(model, train_loader, optimizer, criterion, device)
        val_loss, val_acc = evaluate(model, val_loader, criterion, device)

        print(f'Epoch {epoch:02d} | Train Loss: {train_loss:.4f} | Val Loss: {val_loss:.4f} | Val Acc: {val_acc:.4f}')
        writer.add_scalar('Loss/Train', train_loss, epoch)
        writer.add_scalar('Loss/Val',   val_loss,   epoch)
        writer.add_scalar('Acc/Val',    val_acc,    epoch)

    # Save final model
    torch.save(model.state_dict(), 'pointnet_occupancy.pth')