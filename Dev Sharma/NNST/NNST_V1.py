import tensorflow as tf
import numpy as np
import cv2
import os
import time
from tensorflow.keras.applications import VGG19
from tensorflow.keras.models import Model

# Paths
content_path = "C:\\Users\\devsh\\Desktop\\content\\man.jpg"  # Content image
style_folder = "C:\\Users\\devsh\\Downloads\\archive\\Images\\Training"  # Style images folder
output_path = "C:\\Users\\devsh\\Desktop\\finaloutput"

def load_and_process_image(image_path):
    img = cv2.imread(image_path)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    img = cv2.resize(img, (512, 512))
    img = np.expand_dims(img, axis=0).astype(np.float32)
    return img

# Load content image
content_image = load_and_process_image(content_path)

# Load all style images
style_images = [os.path.join(style_folder, img) for img in os.listdir(style_folder) if img.endswith(('jpg', 'png'))]
num_styles = len(style_images)
print(f"✅ Loaded {num_styles} style images")

# Define feature extraction model using VGG19
vgg = VGG19(weights='imagenet', include_top=False)
style_layers = ['block1_conv1', 'block2_conv1', 'block3_conv1', 'block4_conv1', 'block5_conv1']
content_layer = 'block4_conv2'
model_outputs = [vgg.get_layer(name).output for name in style_layers + [content_layer]]
feature_extractor = Model(inputs=vgg.input, outputs=model_outputs)

def compute_loss(style_features, content_features, generated_features):
    # Content loss
    content_loss = tf.reduce_mean(tf.square(generated_features[-1] - content_features[-1]))
    
    # Style loss
    style_loss = 0
    for sf, gf in zip(style_features[:-1], generated_features[:-1]):
        style_loss += tf.reduce_mean(tf.square(gf - sf))
    
    return content_loss + 0.1 * style_loss

def train_style_transfer(epochs=10):
    generated_image = tf.Variable(content_image, dtype=tf.float32)
    optimizer = tf.keras.optimizers.Adam(learning_rate=0.02)
    
    for epoch in range(epochs):
        start_time = time.time()
        style_image = load_and_process_image(np.random.choice(style_images))
        
        with tf.GradientTape() as tape:
            style_features = feature_extractor(style_image)
            content_features = feature_extractor(content_image)
            generated_features = feature_extractor(generated_image)
            loss = compute_loss(style_features, content_features, generated_features)
        
        gradients = tape.gradient(loss, generated_image)
        optimizer.apply_gradients([(gradients, generated_image)])
        
        progress = (epoch + 1) / epochs * 100
        print(f"Epoch {epoch+1}/{epochs} - {progress:.2f}% Complete - Loss: {loss.numpy():.4f} - Time: {time.time() - start_time:.2f}s")
    
    # Save the final output
    final_output = np.clip(generated_image.numpy()[0], 0, 255).astype(np.uint8)
    output_file = os.path.join(output_path, "stylized_output.jpg")
    cv2.imwrite(output_file, cv2.cvtColor(final_output, cv2.COLOR_RGB2BGR))
    print(f"✅ Style transfer complete! Saved at {output_file}")

# Train
train_style_transfer(epochs=10)
