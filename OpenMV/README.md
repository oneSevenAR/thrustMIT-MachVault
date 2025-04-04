# how to run this janky openmv fb crap

## step 1  
open up `pyopenmv_fb.py` from the openmv repo  
change this line  
```python  
sensor.set_pixformat(sensor.GRAYSCALE)  
```  
or whatever fits ur damn camera  
(for RT1062 it's GRAYSCALE dont cry abt it)  

## step 2  
grab `pyopenmv.py` too  
dump both files in the same damn folder  
make sure u aint dumb and ur virtual env got all the libs like `pygame` or else itll shit itself  

## step 3  
run it like this  
```bash  
python pyopenmv_fb.py --port /dev/ttyACM0  
```  

done now go do smth useful for once

