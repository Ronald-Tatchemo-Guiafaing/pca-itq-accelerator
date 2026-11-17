# PCA-ITQ accelerator

A camera takes one photo of a car. The photo is reduced, then turned into two short codes of 64 bits. One code is the registration. The other code is the country mark on the blue band (F for France, D for Germany, and the other national letters). The same registration can exist in two countries, so both codes are required.

The training stays on the PC. The Zynq only does the projection. pca_core.cpp adds a bias so the board can apply the same centred projection as the PC.

- preprocess_core.cpp reads a 112 by 112 image and writes 28 by 28 values.
- pca_core.cpp reads those 784 values and writes 64 bits.
The same registration in two countries must stay two different results, because the country mark is read on its own.

The camera is connected after this projection gives the same 64 bits as the PC.
