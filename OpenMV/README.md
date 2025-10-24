# How to Run the OpenMV Framebuffer Script

## Step 1  
Open the `pyopenmv_fb.py` file from the OpenMV repository.  
Locate the following line:  
```python
sensor.set_pixformat(sensor.GRAYSCALE)
```  
Adjust it based on your camera configuration.  
(For the RT1062 board, **use GRAYSCALE**.)

## Step 2  
Download `pyopenmv.py` as well.  
Place **both files in the same directory.**  
Ensure your Python environment has all required libraries installed (e.g., `pygame`), or the script may fail to execute.

## Step 3  
Run the script using:  
```bash
python pyopenmv_fb.py --port /dev/ttyACM0
```  

