import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.tensorboard import SummaryWriter
import os
import sys

# Add backend directory to path
backend_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if backend_dir not in sys.path:
    sys.path.append(backend_dir)

# Direct imports from backend
from model.model import TNet
from fall_detection.data_collection.preprocessing import get_dataloaders

class PointNetLSTM(nn.Module):
    def __init__(self, num_points=128, hidden_size=256):  # Reduced from 512 to 256
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
            num_layers=1,     # Single layer
            batch_first=True,
            dropout=0         # Remove dropout since we only have 1 layer
        )
        
        # Fall detection head
        self.fc1 = nn.Linear(hidden_size, 256)
        self.fc2 = nn.Linear(256, 128)
        self.fc3 = nn.Linear(128, 2)  # Binary classification
        
        self.bn4 = nn.BatchNorm1d(256)
        self.bn5 = nn.BatchNorm1d(128)
        
        self.dropout = nn.Dropout(0.5)  # Keep dropout in fully connected layers
        
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
    correct = 0
    total = 0
    
    # Per-class tracking
    fall_correct = 0
    fall_total = 0
    no_fall_correct = 0
    no_fall_total = 0
    
    batch_loss = 0
    batch_correct = 0
    batch_total = 0
    
    for batch_idx, (sequences, labels) in enumerate(loader):
        sequences, labels = sequences.to(device), labels.to(device)
        
        optimizer.zero_grad()
        outputs = model(sequences)
        loss = criterion(outputs, labels)
        
        loss.backward()
        optimizer.step()
        
        # Calculate batch-specific metrics
        batch_loss = loss.item()
        _, predicted = outputs.max(1)
        batch_correct = predicted.eq(labels).sum().item()
        batch_total = labels.size(0)
        
        # Update running totals
        correct += batch_correct
        total += batch_total
        total_loss += batch_loss * batch_total
        
        # Per-class accuracy
        fall_mask = labels == 1
        no_fall_mask = labels == 0
        
        fall_correct += predicted[fall_mask].eq(labels[fall_mask]).sum().item()
        fall_total += fall_mask.sum().item()
        
        no_fall_correct += predicted[no_fall_mask].eq(labels[no_fall_mask]).sum().item()
        no_fall_total += no_fall_mask.sum().item()
        
        # Print progress every 10 batches with batch-specific accuracy
        if (batch_idx + 1) % 10 == 0:
            batch_acc = 100. * batch_correct / batch_total
            running_acc = 100. * correct / total
            fall_acc = 100. * fall_correct / fall_total if fall_total > 0 else 0
            no_fall_acc = 100. * no_fall_correct / no_fall_total if no_fall_total > 0 else 0
            
            print(f'Batch [{batch_idx + 1}/{len(loader)}] | '
                  f'Loss: {batch_loss:.4f} | '
                  f'Batch Acc: {batch_acc:.1f}% | '
                  f'Running Acc: {running_acc:.1f}% | '
                  f'Fall Acc: {fall_acc:.1f}% | '
                  f'No-Fall Acc: {no_fall_acc:.1f}%')
    
    return total_loss / total

def evaluate(model, loader, criterion, device):
    model.eval()
    total_loss = 0
    correct = 0
    total = 0
    
    # For detailed metrics
    fall_correct = 0
    fall_total = 0
    no_fall_correct = 0
    no_fall_total = 0
    
    with torch.no_grad():
        for sequences, labels in loader:
            sequences, labels = sequences.to(device), labels.to(device)
            outputs = model(sequences)
            
            loss = criterion(outputs, labels)
            total_loss += loss.item() * labels.size(0)
            
            # Get predictions
            _, predicted = outputs.max(1)
            
            # Overall accuracy
            correct += predicted.eq(labels).sum().item()
            total += labels.size(0)
            
            # Per-class accuracy
            fall_mask = labels == 1
            no_fall_mask = labels == 0
            
            fall_correct += predicted[fall_mask].eq(labels[fall_mask]).sum().item()
            fall_total += fall_mask.sum().item()
            
            no_fall_correct += predicted[no_fall_mask].eq(labels[no_fall_mask]).sum().item()
            no_fall_total += no_fall_mask.sum().item()
    
    # Calculate metrics
    accuracy = correct / total if total > 0 else 0
    fall_acc = fall_correct / fall_total if fall_total > 0 else 0
    no_fall_acc = no_fall_correct / no_fall_total if no_fall_total > 0 else 0
    avg_loss = total_loss / total if total > 0 else 0
    
    metrics = {
        'loss': avg_loss,
        'accuracy': accuracy,
        'fall_accuracy': fall_acc,
        'no_fall_accuracy': no_fall_acc,
        'fall_total': fall_total,
        'no_fall_total': no_fall_total
    }
    
    return metrics

if __name__ == '__main__':
    # Set device
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"Using device: {device}")

    # Get dataloaders with correct path
    data_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), 
                           'data_collection/data/sequences')
    
    # Load sequences to calculate class weights
    sequences, labels = get_dataloaders(data_dir, return_raw=True)
    n_falls = labels.count(1)
    n_no_falls = labels.count(0)
    total = n_falls + n_no_falls
    
    # Increase fall weight significantly to combat bias
    fall_weight = 1.3 * total / (2 * n_falls)      # Reduced from 2.0 to 1.0 to decrease sensitivity
    no_fall_weight = total / (2 * n_no_falls)
    print(f"\nClass weights - Fall: {fall_weight:.3f}, No-Fall: {no_fall_weight:.3f}")
    
    class_weights = torch.tensor([no_fall_weight, fall_weight]).to(device)
    
    # Create dataloaders with adjusted parameters
    train_loader, val_loader = get_dataloaders(
        data_dir,
        batch_size=8,  # Reduced from 16
        num_points=128,
        test_split=0.1  # Use more data for training
    )

    # Initialize model
    model = PointNetLSTM(num_points=128, hidden_size=256).to(device)
    optimizer = optim.Adam(model.parameters(), lr=1e-3, weight_decay=0.01)  # Added weight decay
    criterion = nn.CrossEntropyLoss(weight=class_weights)
    
    # Add learning rate scheduler
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(
        optimizer,
        mode='max',
        factor=0.5,
        patience=3
    )

    # TensorBoard writer
    writer = SummaryWriter(log_dir='runs/pointnet_lstm_fall')

    # Training loop
    epochs = 5
    best_val_acc = 0.0
    
    print("\nStarting training...")
    print("Press Ctrl+C to stop training early\n")
    
    try:
        for epoch in range(1, epochs + 1):
            print(f'\nEpoch {epoch}/{epochs}')
            print('-' * 20)
            
            train_loss = train_step(model, train_loader, optimizer, criterion, device)
            val_metrics = evaluate(model, val_loader, criterion, device)

            # Log metrics
            print(f'\nEpoch Summary:')
            print(f'Train Loss: {train_loss:.4f}')
            print(f'Val Loss: {val_metrics["loss"]:.4f}')
            print(f'Val Overall Acc: {val_metrics["accuracy"]:.2%}')
            print(f'Val Fall Detection Acc: {val_metrics["fall_accuracy"]:.2%} ({val_metrics["fall_total"]} samples)')
            print(f'Val No-Fall Detection Acc: {val_metrics["no_fall_accuracy"]:.2%} ({val_metrics["no_fall_total"]} samples)')
            
            # Update learning rate based on fall detection accuracy
            scheduler.step(val_metrics["fall_accuracy"])
            
            writer.add_scalar('Loss/Train', train_loss, epoch)
            writer.add_scalar('Loss/Val', val_metrics["loss"], epoch)
            writer.add_scalar('Accuracy/Val', val_metrics["accuracy"], epoch)
            writer.add_scalar('Accuracy/Val_Fall', val_metrics["fall_accuracy"], epoch)
            writer.add_scalar('Accuracy/Val_NoFall', val_metrics["no_fall_accuracy"], epoch)

            # Save best model based on overall accuracy
            if val_metrics["accuracy"] > best_val_acc:
                best_val_acc = val_metrics["accuracy"]
                print(f"\nNew best validation accuracy: {val_metrics['accuracy']:.2%}")
                torch.save(model.state_dict(), 'pointnet_lstm_fall_best.pth')
                print("Saved model checkpoint")
            
            print("\nPress Ctrl+C to stop training")
            
    except KeyboardInterrupt:
        print("\nTraining interrupted by user")
    
    print(f"\nBest validation accuracy: {best_val_acc:.2%}")

    # Save final model
    torch.save(model.state_dict(), 'pointnet_lstm_fall_final.pth')
    print("Saved final model") 