## C++ HireFS(HIRE File System)

## Code directory structure
common:
  |-- include   
  |-- src       
3rd-libraries:
  |-- curl     
  |-- openssl
  |-- secp256k1
hirefs:
  |-- build          
  |-- hireLogManager  
  |-- hireManager     
  |-- hireVolume        
  |-- websocket_server
filetrans
  |--dist      
  |--utg     
  |--docs      
  |--src      
  |--web       
docker
  |--files
    |--etc
    |--opt
    |--usr
  |--Dockerfile

## Hardware Requirements
- Minimum:
```
CPU with 4+ cores (x86_64)
2GB RAM
10TB free storage space
```

## Building the source
### 1. Build hirefs
- build hireversion.h
```cpp
#if !defined(__HIRE_VERSION_H)
#define __HIRE_VERSION_H
#define HIRE_PRODUCT_VERSION "v0.166"
#endif
```

-  x86_64 : debian ubuntu centos  gcc C++11
```shell
 cd hirefs
 cat hireversion.h
 mkdir build
 cd build
 cmake ..
 make

 #move to docker
 mv hirefs docker/usr/bin/utgfsd
```

### 2. Build filetrans
```shell
  cd filetrans
  cd src
  make

  #move to docker
  mv dist/utg.tar.gz docker/opt/utgfs/
  cd docker/opt/utgfs
  tar zxvf utg.tar.gz
  rm utg.tar.gz
```

### 3. Build docker
```shell
  cd docker
  rm -f files.tar.gz
  
  # docker/files
  cd files
  tar cfzv files.tar.gz *

  #build and push to dockerhub
  #cd ../
  #DIST_VERSION=`get_version`
  #docker login -u username -p passwd
  #docker build -t username/utg-storage:v$DIST_VERSION .
  #docker push username/utg-storage:v$DIST_VERSION
```
