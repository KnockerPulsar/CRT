Build command for debugging opencl applications in GDB:
```
    cmake ../ -DENABLE_ICD=OFF -DCMAKE_BUILD_TYPE=Debug
    make -j6
    sudo make install   # Installs to /usr/local/lib
```

Environment variables for debugging in GDB:
```
export POCL_LEAVE_KERNEL_COMPILER_TEMP_FILES=1 
export POCL_MAX_PTHREAD_COUNT=1
```

