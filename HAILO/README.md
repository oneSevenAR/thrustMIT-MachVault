# Converting a PyTorch (.pt) Model to Hailo Executable Format (.hef)

## Introduction

This documentation outlines the process for training a YOLOv8 model using PyTorch, exporting it to ONNX format, and converting the ONNX model to Hailo Executable Format (.hef) for deployment on Hailo hardware. The steps are designed for users working with custom datasets and assume familiarity with Python virtual environments and Docker. Follow the sections sequentially to ensure a successful conversion.



## Prerequisites

- **OS**: Ubuntu 24.04 LTS
- **System Environment**:
  - **Ultralytics**: Version 8.3.180 (latest, compatible with PyTorch 2.5.1). Install via:
    ```
    pip install ultralytics==8.3.180
    ```
    - Source: [PyPI - Ultralytics](https://pypi.org/project/ultralytics/8.3.180/)
  - **Python**: Version 3.11.11 (supported, Ultralytics requires 3.8–3.12). Download from:
    - [Python 3.11.11](https://www.python.org/downloads/release/python-31111/)[](https://stackoverflow.com/questions/57238344/i-have-a-gpu-and-cuda-installed-in-windows-10-but-pytorchs-torch-cuda-is-availa)
  - **PyTorch**: Version 2.5.1 (latest stable, compatible with CUDA 12.6.2). Install via:
    ```
    pip install torch==2.5.1 torchvision==0.20.0 torchaudio==2.5.0 --index-url https://download.pytorch.org/whl/cu126
    ```
    - Source: [PyTorch Get Started](https://pytorch.org/get-started/locally/)[](https://telin.ugent.be/telin-docs/windows/pytorch/)
  - **CUDA**: Version 12.6.2 (supported by PyTorch 2.5.1). Download from:
    - [NVIDIA CUDA Toolkit 12.6.2](https://developer.nvidia.com/cuda-12-6-2-download-archive)[](https://www.digitalocean.com/community/tutorials/install-cuda-cudnn-for-gpu)
    - **Note**: Ensure the NVIDIA driver version (575.64.03) is compatible (minimum ~560). Update if needed from:
      - [NVIDIA Driver Downloads](https://www.nvidia.com/download/index.aspx)
  - **NVIDIA Container Toolkit**: Required for running the Hailo Docker container. Install via:
    - [NVIDIA Container Toolkit Installation Guide](https://docs.nvidia.com/datacenter/cloud-native/container-toolkit/latest/install-guide.html)[](https://learn.microsoft.com/en-us/windows/ai/directml/gpu-cuda-in-wsl)[](https://docs.nvidia.com/cuda/wsl-user-guide/index.html)
    - Follow the guide for your OS (e.g., Ubuntu for WSL2 or Linux).
- **Dataset**: A prepared dataset in YOLO format, including a `data.yaml` file specifying paths to training/validation images and labels.
- **Hailo AI Software Suite**: Access to the Hailo Docker container (via `hailo_ai_sw_suite_docker_run.sh`). Obtain from:
  - [Hailo Developer Zone](https://hailo.ai/developer-zone/) (requires registration) or contact [Hailo Support](https://hailo.ai/company-overview/contact-us/).
  - Clone the Hailo Model Zoo:
    ```
    git clone https://github.com/hailo-ai/hailo_model_zoo.git
    cd hailo_model_zoo
    pip install -e .
    ```
    - Source: [Hailo Model Zoo GitHub](https://github.com/hailo-ai/hailo_model_zoo)[](https://www.digitalocean.com/community/tutorials/install-cuda-cudnn-for-gpu)
- **Hardware**: Compatible with Hailo-8 architecture (specified as `hailo8` in commands).
- **File Paths**: A shared directory (e.g., `/local/shared_with_docker`) for mounting files into the Docker container.
- **Model Zoo Files**: Access to `yolov8n.yaml` from the Hailo Model Zoo (`/hailo_model_zoo/hailo_model_zoo/cfg/networks`).

**Note**: Replace placeholders like paths and class counts with your specific values. Ensure all operations are performed in the appropriate virtual environment or Docker container. The specified environment is compatible for Ultralytics workflows, but the Hailo steps run in a Docker container, isolating dependencies from the host CUDA and driver.

## Step 1: Training the Model

Train a YOLOv8 model on your custom dataset using the Ultralytics library.

1. Load the pre-trained model (use `yolov8n.pt` for a smaller, faster model):
   ```python
   from ultralytics import YOLO

   # Load model
   model = YOLO("yolov8n.pt")
   ```

2. Train the model on your dataset:
   ```python
   # Train on your dataset
   model.train(data="/path/to/your/data.yaml", epochs=100, imgsz=640, batch=16)
   ```

   - Replace `"/path/to/your/data.yaml"` with the actual path to your dataset's YAML file, which defines the training configuration (e.g., paths to images, labels, and number of classes).

3. After training completes, locate the trained model weights in the `runs/train/weights` directory. The `best.pt` file represents the optimal model checkpoint.

## Step 2: Converting to ONNX

Export the trained PyTorch model (.pt) to ONNX format for compatibility with Hailo tools.

1. Navigate to the directory containing the `best.pt` file (e.g., `runs/train/weights`).

2. Run the export command in the same Python virtual environment:
   ```
   yolo export model=/path/to/trained/best.pt imgsz=640 format=onnx opset=11
   ```

   - Replace `"/path/to/trained/best.pt"` with the actual path to your `best.pt` file.
   - The `best.onnx` file will be saved in the same folder as `best.pt`.

## Step 3: Converting ONNX to HEF

Compile the ONNX model to .hef format using the Hailo Dataflow Compiler within a Docker container.

1. Start the Hailo Docker container:
   ```
   ./hailo_ai_sw_suite_docker_run.sh --override --hailort-enable-service
   ```

   - The `--override` flag deletes any existing container and starts a new one.
   - The `--hailort-enable-service` flag enables necessary Hailo services.

2. Prepare files for the container:
   - Copy the training images folder to `/local/shared_with_docker/train/images` (for calibration).
   - Copy `yolov8n.yaml` from `/hailo_model_zoo/hailo_model_zoo/cfg/networks` to `/local/shared_with_docker`.
   - Copy `best.onnx` to `/local/shared_with_docker`.

3. Inside the Docker container, run the compilation command:
   ```
   hailomz compile --ckpt /local/shared_with_docker/best.onnx --calib-path /local/shared_with_docker/train/images --yaml /local/shared_with_docker/yolov8n.yaml --classes 15 --hw-arch hailo8
   ```

   - Adjust `--classes 15` to match the number of classes in your dataset.
   - Ensure the `--calib-path` points to the folder containing training images for calibration.

4. Wait for the process to complete. Upon success, the .hef file will be generated in the working directory.

## Important Notes

- Always use the training images for calibration to maintain model accuracy during quantization.
- Verify file paths and permissions within the Docker container to avoid access errors.
- If issues arise (e.g., parsing failures), inspect the YAML configuration or update the Hailo tools.
- For custom models or different YOLO variants, adapt the YAML file accordingly.

## References

- [Ultralytics Documentation](https://docs.ultralytics.com/)
- [Hailo Model Zoo GitHub](https://github.com/hailo-ai/hailo_model_zoo)
- [Hailo Community Forum](https://community.hailo.ai/)