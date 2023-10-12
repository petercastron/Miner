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

## Hardware Requirements
- Minimum:
```
CPU with 4+ cores (x86_64)
2GB RAM
10TB free storage space
```

## Building the source
-  x86_64 : debian ubuntu centos  gcc C++11
```shell
 cd hirefs
 mkdir build
 cd build
 cmake ..
 make
```
