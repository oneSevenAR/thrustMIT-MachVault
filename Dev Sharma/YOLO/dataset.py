# dataset.py

import os
import pandas as pd
from PIL import Image
from torch.utils.data import Dataset
from torchvision import transforms

class DogBreedDataset(Dataset):
    def __init__(self, csv_file, img_dir, transform=None):
        self.data = pd.read_csv(csv_file)
        self.img_dir = img_dir
        self.transform = transform

        # get list of unique breeds and make class mappings
        self.breeds = sorted(self.data['breed'].unique())
        self.class_to_idx = {breed: idx for idx, breed in enumerate(self.breeds)}
        self.idx_to_class = {idx: breed for breed, idx in self.class_to_idx.items()}

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        row = self.data.iloc[idx]
        img_path = os.path.join(self.img_dir, row['id'] + '.jpg')
        image = Image.open(img_path).convert("RGB")

        label = self.class_to_idx[row['breed']]

        if self.transform:
            image = self.transform(image)

        return image, label
