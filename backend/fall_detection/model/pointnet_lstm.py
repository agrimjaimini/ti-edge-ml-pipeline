import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.tensorboard import SummaryWriter
from ...model.model import TNet  # Reuse TNet from original model
from ..data_collection.preprocessing import get_dataloaders

class PointNetLSTM(nn.Module):
    def __init__(self, num_points=128, hidden_size=512):
        super().__init__()
        
        # PointNet components
        self.input_transform = TNet(k=3)
        self.feature_transform = TNet(k=64)
        
        self.conv1 = nn.Conv1d(3, 64, 1)
        self.conv2 = nn.Conv1d(64, 128, 1)
        self.conv3 = nn.Conv1d(128, 1024, 1)
        
        self.bn1 = nn.BatchNorm1d(64)
        self.bn2 = nn.BatchNorm1d(128)
        self.bn3 = nn.BatchNorm1d(1024)
        
        # LSTM for temporal processing
        self.lstm = nn.LSTM(
            input_size=1024,  # PointNet feature size
            hidden_size=hidden_size,
            num_layers=2,
            batch_first=True,
            dropout=0.3
        )
        
        # Fall detection head
        self.fc1 = nn.Linear(hidden_size, 256)
        self.fc2 = nn.Linear(256, 128)
        self.fc3 = nn.Linear(128, 2)  # Binary classification
        
        self.bn4 = nn.BatchNorm1d(256)
        self.bn5 = nn.BatchNorm1d(128)
        
        self.dropout = nn.Dropout(0.3)
        
    def pointnet_forward(self, x):
        # x shape: (batch, sequence_length, num_points, 3)
        batch_size, seq_len, num_points, _ = x.size()
        
        # Reshape for PointNet processing
        x = x.view(batch_size * seq_len, num_points, 3)
        x = x.transpose(2, 1)  # (batch*seq, 3, num_points)
        
        # TNet transformations
        trans = self.input_transform(x)
        x = torch.bmm(trans, x)
        
        # PointNet layers
        x = torch.relu(self.bn1(self.conv1(x)))
        
        trans_feat = self.feature_transform(x)
        x = torch.bmm(trans_feat, x)
        
        x = torch.relu(self.bn2(self.conv2(x)))
        x = self.bn3(self.conv3(x))
        
        # Max pooling
        x = torch.max(x, 2)[0]  # (batch*seq, 1024)
        
        # Reshape back to sequences
        x = x.view(batch_size, seq_len, -1)  # (batch, seq, 1024)
        
        return x
        
    def forward(self, x):
        # Extract point cloud features
        x = self.pointnet_forward(x)  # (batch, seq, 1024)
        
        # Process temporal sequence
        lstm_out, _ = self.lstm(x)  # (batch, seq, hidden)
        
        # Use last timestep for classification
        x = lstm_out[:, -1, :]  # (batch, hidden)
        
        # Classification layers
        x = torch.relu(self.bn4(self.fc1(x)))
        x = self.dropout(x)
        x = torch.relu(self.bn5(self.fc2(x)))
        x = self.dropout(x)
        x = self.fc3(x)
        
        return x  # Returns logits [no_fall, fall]

def train_step(model, loader, optimizer, criterion, device):
    model.train()
    total_loss = 0
    
    for sequences, labels in loader:
        sequences, labels = sequences.to(device), labels.to(device)
        
        optimizer.zero_grad()
        outputs = model(sequences)
        loss = criterion(outputs, labels)
        
        loss.backward()
        optimizer.step()
        
        total_loss += loss.item() * sequences.size(0)
        
    return total_loss / len(loader.dataset)

def evaluate(model, loader, criterion, device):
    model.eval()
    total_loss = 0
    correct = 0
    total = 0
    
    with torch.no_grad():
        for sequences, labels in loader:
            sequences, labels = sequences.to(device), labels.to(device)
            outputs = model(sequences)
            
            loss = criterion(outputs, labels)
            total_loss += loss.item() * sequences.size(0)
            
            _, predicted = outputs.max(1)
            correct += predicted.eq(labels).sum().item()
            total += labels.size(0)
            
    accuracy = correct / total
    avg_loss = total_loss / len(loader.dataset)
    
    return avg_loss, accuracy 

if __name__ == '__main__':
    # Set device
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"Using device: {device}")

    # Get dataloaders
    train_loader, val_loader = get_dataloaders('data/sequences', 
                                             batch_size=32,
                                             num_points=128)

    # Initialize model
    model = PointNetLSTM(num_points=128).to(device)
    optimizer = optim.Adam(model.parameters(), lr=1e-3)
    criterion = nn.CrossEntropyLoss()

    # TensorBoard writer
    writer = SummaryWriter(log_dir='runs/pointnet_lstm_fall')

    # Training loop
    epochs = 50  # Adjust as needed
    best_val_acc = 0.0
    
    print("Starting training...")
    for epoch in range(1, epochs + 1):
        train_loss = train_step(model, train_loader, optimizer, criterion, device)
        val_loss, val_acc = evaluate(model, val_loader, criterion, device)

        # Log metrics
        print(f'Epoch {epoch:02d}: Train Loss: {train_loss:.4f} | Val Loss: {val_loss:.4f} | Val Acc: {val_acc:.4f}')
        writer.add_scalar('Loss/Train', train_loss, epoch)
        writer.add_scalar('Loss/Val', val_loss, epoch)
        writer.add_scalar('Accuracy/Val', val_acc, epoch)

        # Save best model
        if val_acc > best_val_acc:
            best_val_acc = val_acc
            print(f"New best validation accuracy: {val_acc:.4f}")
            torch.save(model.state_dict(), 'pointnet_lstm_fall.pth')
            print("Saved model checkpoint")

    print("Training finished!")
    print(f"Best validation accuracy: {best_val_acc:.4f}")

    # Save final model
    torch.save(model.state_dict(), 'pointnet_lstm_fall_final.pth')
    print("Saved final model") 