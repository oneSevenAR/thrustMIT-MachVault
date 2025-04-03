import torch
import torch.nn as nn
import torch.optim as optim
from torchvision import transforms, models
from PIL import Image
import matplotlib.pyplot as plt

# ============ Load Images ============
def load_image(image_path, max_size=512):
    image = Image.open(image_path).convert("RGB")
    transform = transforms.Compose([
        transforms.Resize((max_size, max_size)),
        transforms.ToTensor()
    ])
    image = transform(image).unsqueeze(0)  # Add batch dimension
    return image.to(device)

# ============ Display & Save Image ============
def imshow(tensor, title=None, save_path=None):
    image = tensor.clone().detach().squeeze(0).cpu()
    image = torch.clamp(image, 0, 1)  # Ensure values are in [0,1] range
    image = image.numpy().transpose(1, 2, 0)  # Convert to HWC format

    plt.imshow(image)
    if title:
        plt.title(title)
    plt.axis('off')

    if save_path:
        plt.imsave(save_path, image)

    plt.show()

# ============ Gram Matrix for Style ============
def gram_matrix(tensor):
    _, c, h, w = tensor.size()
    tensor = tensor.view(c, h * w)  # Flatten to (channels, pixels)
    gram = torch.mm(tensor, tensor.t())  # Compute Gram matrix
    return gram / (c * h * w)  # Normalize

# ============ Feature Extraction ============
class VGGFeatures(nn.Module):
    def __init__(self, model):
        super(VGGFeatures, self).__init__()
        self.model = model.features[:21]  # Extract only first few layers
        for param in self.model.parameters():
            param.requires_grad = False  # Freeze weights
    
    def forward(self, x):
        features = {}
        for idx, layer in enumerate(self.model):
            x = layer(x)
            if idx in {0, 5, 10, 19}:  # Chosen layers for style/content
                features[idx] = x
        return features

# ============ Compute Loss ============
def compute_loss(content_features, style_features, target_features, style_weight=1e6, content_weight=1):
    content_loss = torch.nn.functional.mse_loss(target_features[10], content_features[10])

    style_loss = 0
    for layer in style_features:
        gram_style = gram_matrix(style_features[layer])
        gram_target = gram_matrix(target_features[layer])
        style_loss += torch.nn.functional.mse_loss(gram_target, gram_style)
    
    return content_weight * content_loss + style_weight * style_loss

# ============ Style Transfer Function ============
def style_transfer(content_img, style_img, num_steps=100, style_weight=1e6, content_weight=1):
    target = content_img.clone().requires_grad_(True).to(device)
    optimizer = optim.LBFGS([target])

    for step in range(num_steps):
        def closure():
            optimizer.zero_grad()
            target_features = model(target)
            loss = compute_loss(content_features, style_features, target_features, style_weight, content_weight)
            loss.backward()
            return loss
        
        optimizer.step(closure)

        if step % 5 == 0:
            print(f"Step {step}/{num_steps} - Loss: {closure().item()}")

    return target

# ============ Load Model ============
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
vgg = models.vgg19(pretrained=True).to(device).eval()
model = VGGFeatures(vgg).to(device)

# ============ Load Images ============
content_img = load_image(r"C:\Users\devsh\Desktop\Ship.png")
style_img = load_image(r"C:\Users\devsh\Downloads\starrynight.jpeg")

# ============ Extract Features ============
content_features = model(content_img)
style_features = model(style_img)

# ============ Run Style Transfer ============
output = style_transfer(content_img, style_img)

# ============ Save & Show Result ============
output_path = r"C:\Users\devsh\Desktop\styled_image.png"
imshow(output, "Styled Image", save_path=output_path)

print(f"Styled image saved at: {output_path}")
