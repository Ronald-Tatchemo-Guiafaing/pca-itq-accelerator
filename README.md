# PCA-ITQ accelerator

A camera takes one photo. The photo is reduced, then turned into a short code of 64 bits. Two different photos should give two different codes. That code says which handwritten digit the photo is.

The training stays on the PC. The Zynq only does the projection.

- preprocess_core.cpp reads a 112 by 112 image and writes 28 by 28 values.
- pca_core.cpp reads those 784 values and writes 64 bits.

The camera is connected after this projection gives the same 64 bits as the PC.
