# Fast Homomorphic Image Processing
These days neural networks and fully homomorphic encryption are a meme. For example, Microsoft demonstrated with Cryptonets of a neural network generating predictions fully homomorphically on the MNIST dataset. However, it would be useful to have a way to preprocess images homomorphically. Consider the use case where an edge device sends a homomorphically encrypted image to a server that runs a prediction algorithm with two neural networks that take in different sized features, as is common. It would be prohibitive to make the edge device homomorphically encrypt two copies of the images, since that would be prohibitively expensive. Therefore, having proprocessing and feature extraction computed homomorphically will provide much more flexibility for homomorphic neural nets.  

### Homomorphic Image Resizing
There are two common types of interpolation used when images are scaled: [bilinear interpolation](https://en.wikipedia.org/wiki/Bilinear_interpolation) and [bicubic interpolation](https://en.wikipedia.org/wiki/Bicubic_interpolation). Bilinear interpolation requires a 2 by 2 square around the point to be interpolated, and involves linear interpolation in one direction and then in the other in a two dimensional space. Bicubic interpolation is similar except cubic rather than linear interpolation is used, and it requires a 4 by 4 square toaround a point to be interpolated. Bicubic interpolation tends to result in sharper images than does bilinear interpolation. 


### Homomorphic Bicubic Image Resizing

### Homomorphic JPEG-2 Encoding
Image compression cuts the frames of the video into blocks, which is then compressed using frequency analysis. To compress video homomorphically, we implement the [discrete cosine transform](https://en.wikipedia.org/wiki/Discrete_cosine_transform) 
and multiply by a [quantization](https://en.wikipedia.org/wiki/Quantization_(image_processing)) factor to throw away the higher frequencies of our image. 

![JPEG workflow](docs/jpgworkflow.png)



## Installation Instructions

We use Microsoft's [SEAL](https://www.microsoft.com/en-us/research/publication/simple-encrypted-arithmetic-library-seal-v2-2/) library for most of the heavy lifting for the FHE side. Since it has no public repository, version 2.3 of the library has been added as a git submodule from an unofficial source.


We used ```g++-7``` as our default compiler for everything. If you want to change this change the ```CXX=``` portions of the makefile and the install script. The install script ```install.sh``` should update the git submodule containing SEAL and build it automatically. Then, you should be able to make our example programs in ```homo/```.

### Dependencies
You will also need opencv installed as a library. for Mac OSX, install opencv via brew:
```
brew install opencv
```
On Debian, install using apt
```
sudo apt install opencv -y
```

## Acknowledgements and External Sources

We used a few external libraries and source code, here are links to their respective pages!
* [SEAL](https://www.microsoft.com/en-us/research/publication/simple-encrypted-arithmetic-library-seal-v2-2/), a homomorphic computation library from Microsoft Research
* [Jon Olick](https://www.jonolick.com/code.html)'s JPEG encoder
* [Lecture Notes](http://www.intensecrypto.org/public/index.html) by Boaz Barak that got us started
* [ffmpeg](https://www.ffmpeg.org/), video transcoding and audio processing library
* [C++ Command Line Options](https://github.com/jarro2783/cxxopts), for quality of life
* [Image Resizing](https://blog.demofox.org/2015/08/15/resizing-images-with-bicubic-interpolation/) by demofox
