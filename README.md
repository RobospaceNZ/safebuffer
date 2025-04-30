# Safe Buffer

## Description

Safe buffer uses a data structure to manage a buffer and if the functions are used then the buffer cannot be overflowed.

## Import into Zephyr

This code can be cloned as part of the user's code, but can also be cloned into your Zephyr library. The advantage of this is, if it is cloned into Zephyr once, it will be available to all your Zephyr projects.

For the examples below, we are assuming Nordic NCS 2.9.0. Please adjust to your version.

If you are using Nordic's NCS, open west.yml located at:<br>
C:\ncs\v2.9.0\nrf\west.yml

Under remotes, add:
```
    - name: safebuffer
      url-base: https://github.com/RobospaceNZ
```

Under projects, add:
```
    - name: safebuffer
      remote: safebuffer
      revision: V1.0.1
      path: modules/lib/safebuffer
```

Open command prompt in C:\ncs\v2.9.0. Remember to update b620d30767 if you use a different NCS (See C:\ncs\toolchains\toolchains.json for the version codes). Run the following:
```
SET PATH=C:\ncs\toolchains\b620d30767\opt\bin;C:\ncs\toolchains\b620d30767\opt\bin\Scripts;%PATH%
west update
```

The safe buffer code will be located at:<br>
C:\ncs\v2.9.0\modules\lib\safebuffer

Add the following to your CMakeLists.txt file:
```
set(SAFEBUFFER_SRC ${ZEPHYR_BASE}/../modules/lib/safebuffer)
add_subdirectory(${SAFEBUFFER_SRC} ${CMAKE_BINARY_DIR}/safebuffer_build)
include_directories(${SAFEBUFFER_SRC}/include)
```
