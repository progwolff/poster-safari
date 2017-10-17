# Poster Safari -- Server backend

## Install dependencies

Dependencies:

* tesseract
* libleptonia
* jsoncpp
* easyloggingpp
* sparsehash
* cURLpp
* opencv (self-compile version after tesseract insallation)

### Ubuntu / Debian

```
sudo apt-get install -y tesseract-ocr-dev tesseract-ocr libleptonica-dev libjsoncpp-dev
wget https://github.com/muflihun/easyloggingpp/releases/download/v9.94.2/easyloggingpp_v9.94.2.tar.gz
tar xf easyloggingpp_v9.94.2.tar.gz 
sudo cp easylogging++.cc /usr/include/  
sudo cp easylogging++.h /usr/include/

git clone https://github.com/opencv/opencv --depth 1
git clone https://github.com/opencv/opencv_contrib --depth 1
mkdir build && cd build 
cmake -D WITH_OPENCL=ON -D WITH_OPENGL=ON -D WITH_TBB=ON  -D WITH_XINE=ON -D BUILD_WITH_DEBUG_INFO=OFF -D BUILD_TESTS=OFF -D BUILD_PERF_TESTS=OFF  -D BUILD_EXAMPLES=ON -D INSTALL_C_EXAMPLES=ON -D INSTALL_PYTHON_EXAMPLES=ON -DCMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr -D CMAKE_SKIP_RPATH=ON -DOPENCV_EXTRA_MODULES_PATH="../opencv_contrib/modules" ../opencv/ 
make 
sudo make install

git clone https://github.com/open-source-parsers/jsoncpp.git --depth 1
mkdir buildjson
cd buildjson
cmake -DCMAKE_BUILD_TYPE=debug -DBUILD_SHARED_LIBS=1 -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_INCLUDEDIR=include/jsoncpp ../jsoncpp
make
sudo make install
```

### Arch Linux

1. Install official Packages: ```pacman -S jsoncpp tesseract sparsehash```
2. Install from AUR: e.g.:``` packer -S easyloggingpp libcurlpp```
3. Install opencv after tesseract to compile against it: e.g.:```packer -S opencv-git```
4. Install training data for German and English language : ```pacman -S tesseract-data-{deu,eng}```

## Make server backend

```
$ mkdir build && cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr ..
$ make
# make install
```
