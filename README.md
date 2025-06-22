# Safe Buffer

## Description

Safe buffer uses a data structure to manage a buffer and if the functions are used then the buffer cannot be overflowed.

### How to use

If the safebuffer structure is instantiated on the stack then the user needs to supply a buffer. For example:

uint8_t buffer[10];
safebuffer_t sb;
safebuffer_init(&sb, buffer, sizeof(buffer));

When you need to use an existing safebuffer for something new, then call:
safebuffer_reset(&sb);

You can also instantiate a safebuffer on the heap:
safebuffer_t *sb = safebuffer_malloc(10);

In this case you must call:
safebuffer_free(sb);
Don't use k_free(sb);

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
      revision: V1.0.2
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
```
