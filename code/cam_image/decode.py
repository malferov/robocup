#!/bin/python3

import numpy as np
from PIL import Image
import base64

with open("image.raw", "r") as f:
    b64_data = f.read()

raw = base64.b64decode(b64_data)

#WIDTH = 96
#HEIGHT = 96
WIDTH = 160
HEIGHT = 120

rgb565 = np.frombuffer(raw, dtype='>u2').reshape((HEIGHT, WIDTH))

r = ((rgb565 >> 11) & 0x1F) << 3
g = ((rgb565 >> 5) & 0x3F) << 2
b = (rgb565 & 0x1F) << 3

rgb888 = np.dstack((r, g, b)).astype(np.uint8)

Image.fromarray(rgb888).show()
