# train_classifier.py

import torch
import torch.nn as nn
from torch.utils.data import DataLoader
from torchvision import transforms
from dataset import DogBreedDataset
from model import DogNet
from sklearn.model_selection import train_test_split
import pandas as pd

# === CONFIG ===
BATCH_SIZE = 32
EPOCHS = 17
IMG_SIZE = 224
CSV_PATH = 'dog-breed-identification/labels.csv'
IMG_DIR = 'dog-breed-identification/train'

# === SETUP ===
device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

# === TRANSFORMS ===
transform = transforms.Compose([
    transforms.Resize((IMG_SIZE, IMG_SIZE)),
    transforms.ToTensor()
])

# === DATA ===
df = pd.read_csv(CSV_PATH)
train_df, val_df = train_test_split(df, test_size=0.2, stratify=df['breed'])

train_df.to_csv("train_split.csv", index=False)
val_df.to_csv("val_split.csv", index=False)

train_set = DogBreedDataset("train_split.csv", IMG_DIR, transform=transform)
val_set = DogBreedDataset("val_split.csv", IMG_DIR, transform=transform)

train_loader = DataLoader(train_set, batch_size=BATCH_SIZE, shuffle=True)
val_loader = DataLoader(val_set, batch_size=BATCH_SIZE)

# === MODEL ===
num_classes = len(train_set.breeds)
model = DogNet(num_classes).to(device)

criterion = nn.CrossEntropyLoss()
optimizer = torch.optim.Adam(model.parameters(), lr=1e-4)

# === TRAIN LOOP ===
for epoch in range(EPOCHS):
    model.train()
    total_loss = 0
    correct = 0
    total = 0

    for imgs, labels in train_loader:
        imgs, labels = imgs.to(device), labels.to(device)

        preds = model(imgs)
        loss = criterion(preds, labels)

        optimizer.zero_grad()
        loss.backward()
        optimizer.step()

        total_loss += loss.item()
        correct += (preds.argmax(1) == labels).sum().item()
        total += labels.size(0)

    acc = 100 * correct / total
    print(f"[Epoch {epoch+1}] Loss: {total_loss:.4f} | Accuracy: {acc:.2f}%")

# === SAVE MODEL ===
torch.save(model.state_dict(), 'dog_classifier.pth')
