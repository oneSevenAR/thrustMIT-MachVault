import sys
import torch
from torchvision import transforms
from PIL import Image
import pandas as pd
from model import DogNet
import torch.nn.functional as F

# === PATHS ===
image_path = sys.argv[1] if len(sys.argv) > 1 else "/home/dev-sharma/Desktop/joe.jpg"
model_path = "dog_classifier.pth"
labels_csv_path = "dog-breed-identification/labels.csv"

# === GET CLASS NAMES ===
df = pd.read_csv(labels_csv_path)
class_names = sorted(df['breed'].unique())
num_classes = len(class_names)

# === TRANSFORMS (match training) ===
transform = transforms.Compose([
    transforms.Resize((224, 224)),
    transforms.ToTensor()
])

# === LOAD IMAGE ===
img = Image.open(image_path).convert("RGB")
img_tensor = transform(img).unsqueeze(0)  # shape: [1, 3, 224, 224]

# === LOAD MODEL ===
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = DogNet(num_classes=num_classes)
model.load_state_dict(torch.load(model_path, map_location=device))
model = model.to(device)
model.eval()

# === PREDICT ===
with torch.no_grad():
    img_tensor = img_tensor.to(device)
    outputs = model(img_tensor)
    probs = F.softmax(outputs, dim=1)
    pred_index = torch.argmax(probs, dim=1).item()
    confidence = probs[0][pred_index].item() * 100
    pred_breed = class_names[pred_index]

print(f"Predicted breed: {pred_breed}")
print(f"Confidence: {confidence:.2f}%")
