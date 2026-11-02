# Safe Buffer

## Description

Safe buffer uses a data structure to manage a buffer and if the functions are used then the buffer cannot be overflowed.

### How to use

If the safebuffer structure is instantiated on the stack then the user needs to supply a buffer. For example:

```
uint8_t buffer[10];
safebuffer_t sb;
safebuffer_init(&sb, buffer, sizeof(buffer));
```

When you need to use an existing safebuffer for something new, then call:
```
safebuffer_reset(&sb);
```

You can also instantiate a safebuffer on the heap:
```
safebuffer_t *sb = safebuffer_malloc(10);
```

In this case you must call:
```
safebuffer_free(sb);
```
Don't use k_free(sb);

The add functions return 0 if all the data was added and -ENOMEM if it did not fit. The data that fits is still added.
When data is lost, `overflow` is set and stays set until `safebuffer_reset` is called. `full` is set when no space is
left.

`safebuffer_snprintf` writes a terminating zero after the message and `write_index` points to it, so the next data
added overwrites it. A message of n characters therefore needs n + 1 bytes of space. If the message is cut short,
-ENOMEM is returned and the terminating zero is in the last byte of the buffer.

### Reading

Data is read from `read_index` up to `write_index`. The read functions return -ENOSPC if not enough data is left, then
nothing is read.
```
uint8_t type;
uint8_t payload[16];

safebuffer_read_char(&sb, &type);                       // Read one byte
safebuffer_peek_char(&sb, &type);                       // Look at the next byte without reading it
safebuffer_skip(&sb, 2);                                // Skip 2 bytes
safebuffer_read_data(&sb, payload, sizeof(payload));    // Read 16 bytes, or nothing if fewer are left
safebuffer_get_read_left_count(&sb);                    // Number of bytes not read yet
```

`safebuffer_compact` moves the data that has not been read yet to the start of the buffer and sets `read_index` to 0.
This makes space for more data, for example in a parser that processes part of the data and then receives more.

### Reserving space

`safebuffer_reserve` reserves bytes at the end of the data and returns a pointer to them, so they can be filled in
after the rest of the data was added. For example a header with the length of the payload:
```
uint8_t *header = safebuffer_reserve(&sb, 2);

if (header) {
    safebuffer_add_data(&sb, payload, payload_len);
    header[0] = payload_len >> 8;
    header[1] = payload_len & 0xFF;
}
```
NULL is returned if there is not enough space, then nothing is reserved and `overflow` is set.

### Other functions

`safebuffer_data_len` returns the number of bytes written to the buffer and `safebuffer_is_empty` returns true if
nothing was written. Use these instead of reading `write_index` directly.

### Read only buffers

A buffer initialised with `safebuffer_init_const` is read only. The add functions return -EACCES and
`safebuffer_reset` only resets `read_index`.

The functions are not thread safe. If a safebuffer is used by more than one thread, the caller must protect it.

### Changes in V2.1.0

safebuffer is now a Zephyr module, see [Import into Zephyr](#import-into-zephyr). Enable it with
`CONFIG_SAFEBUFFER=y` and remove the `add_subdirectory` lines from your CMakeLists.txt. There are no changes to the code.

### Changes in V2.0.0

V2.0.0 is not backwards compatible with V1.x:
- The fields `p_gp`, `value`, `datatype` and `p_processed`, and the function `safebuffer_set_processed`, were removed.
  rs_protocol V1.0.1 and older use `p_gp`, use a version of rs_protocol that does not.
- `safebuffer_add_data` takes a `const void *`, `safebuffer_add_string` takes a `const char *`.
- `safebuffer_snprintf` returns -ENOMEM if the message was cut short, also when only the last character did not fit.
  It no longer clears `overflow`.
- `safebuffer_snprintf` has a printf format attribute, the compiler now warns about format strings that don't match
  their arguments.

New functions: `safebuffer_read_data`, `safebuffer_peek_char`, `safebuffer_skip`, `safebuffer_get_read_left_count`,
`safebuffer_reserve`, `safebuffer_is_empty`, `safebuffer_data_len` and `safebuffer_compact`.

## Import into Zephyr

safebuffer is a Zephyr module (see zephyr/module.yml), so Zephyr finds it by itself once it is in the west workspace or
listed in `ZEPHYR_EXTRA_MODULES`. No changes to CMakeLists.txt or Kconfig are needed, enable it in prj.conf:
```
CONFIG_SAFEBUFFER=y
```

### Workspace application

If your application has its own west.yml (a workspace application, for example one started from Nordic's
ncs-example-application), add the library to that west.yml. Don't edit the west.yml of the SDK (nrf/west.yml), the
change is lost when the SDK is updated and is not part of your project.

Under remotes, add (once for all Robospace libraries):
```
    - name: robospace
      url-base: https://github.com/RobospaceNZ
```

Under projects, add:
```
    - name: safebuffer
      remote: robospace
      revision: V2.1.0
      path: modules/lib/safebuffer
```

Then run `west update` in the workspace folder.

### Freestanding application

If your application uses an SDK installed under C:\ncs, add the library to your application, for example as a git
submodule:
```
git submodule add https://github.com/RobospaceNZ/safebuffer libs/safebuffer
```

Add the following to your CMakeLists.txt file, before `find_package(Zephyr ...)`:
```
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/libs/safebuffer)
```
