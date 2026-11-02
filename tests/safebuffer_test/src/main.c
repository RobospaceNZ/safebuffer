#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include "safebuffer.h"

ZTEST(safebuffer, test_safebuffer_init)
{
    safebuffer_t sb;
    uint8_t buffer[10];
    safebuffer_init(&sb, buffer, sizeof(buffer));

    zassert_equal(sb.len, 10, "Buffer size should be initialized to 10");
    zassert_equal(sb.write_index, 0, "Write index should be initialized to 0");
    zassert_equal(sb.read_index, 0, "Read index should be initialized to 0");
    zassert_false(sb.full, "Buffer should not be full after initialization");
    zassert_false(sb.overflow, "Buffer should not have overflow after initialization");
}

ZTEST(safebuffer, test_safebuffer_add_data)
{
    safebuffer_t *sb;
    sb = safebuffer_malloc(7);
    zassert_not_null(sb, "Failed to allocate safebuffer");
    zassert_equal(safebuffer_add_char(sb, 'A'), 0, "Add char should succeed");
    zassert_equal(sb->write_index, 1, "Write index should be 1 after adding a char");
    zassert_false(sb->full, "Buffer should not be full after adding a char");
    zassert_false(sb->overflow, "Buffer should not have overflow after adding a char");

    uint8_t data[] = {1, 2, 3, 4, 5};
    zassert_equal(safebuffer_add_data(sb, data, sizeof(data)), 0, "Add data should succeed");
    zassert_equal(sb->write_index, 6, "Write index should be 6 after adding data");
    zassert_false(sb->full, "Buffer should not be full after adding data");
    zassert_false(sb->overflow, "Buffer should not have overflow after adding data");

    zassert_equal(safebuffer_add_char(sb, 'A'), 0, "Add char should succeed");
    zassert_equal(sb->write_index, 7, "Write index should be 7 after adding another char");
    zassert_true(sb->full, "Buffer should be full after adding another char");
    zassert_false(sb->overflow, "Buffer should not have overflow after adding another char");
    zassert_equal(safebuffer_add_char(sb, 'B'), -ENOMEM, "Add char should fail due to overflow");
    zassert_true(sb->full, "Buffer should still be full after failed add char");
    zassert_true(sb->overflow, "Buffer should have overflow after failed add char");
    zassert_equal(safebuffer_free(sb), 0, "Freeing safebuffer should succeed");
}

ZTEST(safebuffer, test_safebuffer_add_safebuffer)
{
    safebuffer_t *sb1, *sb2;
    sb1 = safebuffer_malloc(10);
    zassert_not_null(sb1, "Failed to allocate safebuffer 1");
    sb2 = safebuffer_malloc(5);
    zassert_not_null(sb2, "Failed to allocate safebuffer 2");

    zassert_equal(safebuffer_add_char(sb1, 'A'), 0, "Add char to sb1 should succeed");
    zassert_equal(safebuffer_add_char(sb2, 'B'), 0, "Add char to sb2 should succeed");

    zassert_equal(safebuffer_add_safebuffer(sb1, sb2), 0, "Add sb2 to sb1 should succeed");
    zassert_equal(sb1->write_index, 2, "Write index of sb1 should be 2 after adding sb2");
    zassert_false(sb1->full, "Buffer sb1 should not be full after adding sb2");
    zassert_false(sb1->overflow, "Buffer sb1 should not have overflow after adding sb2");

    zassert_equal(safebuffer_get_data_left_count(sb1), 8, "Data left in sb1 should be 8 after adding sb2");
    zassert_equal(safebuffer_get_data_left_count(sb2), 4, "Data left in sb2 should be 4");

    zassert_equal(safebuffer_free(sb1), 0, "Freeing safebuffer 1 should succeed");
    zassert_equal(safebuffer_free(sb2), 0, "Freeing safebuffer 2 should succeed");
}

ZTEST(safebuffer, test_safebuffer_add_string)
{
    safebuffer_t *sb;
    sb = safebuffer_malloc(20);
    zassert_not_null(sb, "Failed to allocate safebuffer");

    char *test_str = "Hello";
    zassert_equal(safebuffer_add_string(sb, test_str), 0, "Add string should succeed");
    zassert_equal(sb->write_index, 5, "Write index should be 5 after adding string");
    zassert_false(sb->full, "Buffer should not be full after adding string");
    zassert_false(sb->overflow, "Buffer should not have overflow after adding string");

    char *long_str = "This is a long string that will not fit";
    zassert_equal(safebuffer_add_string(sb, long_str), -ENOMEM, "Add long string should fail due to overflow");
    zassert_true(sb->full, "Buffer should be full after failed add string");
    zassert_true(sb->overflow, "Buffer should have overflow after failed add string");

    zassert_equal(safebuffer_free(sb), 0, "Freeing safebuffer should succeed");
}

static void print_debug_data(safebuffer_t *sb, const char *message)
{
    printk("%s. Buffer contents: ", message);
    for (uint32_t i = 0; i < sb->len; i++) {
        printk("0x%02X ", sb->buf[i]);
    }
    printk("\nWrite index: %d, Read index: %d, Full: %d, Overflow: %d\n",
           sb->write_index, sb->read_index, sb->full, sb->overflow);
}

ZTEST(safebuffer, test_safebuffer_printf)
{
    safebuffer_t sb;
    uint8_t buffer[10];
    safebuffer_init(&sb, buffer, sizeof(buffer) - 1);
    memset(buffer, 0xAA, sizeof(buffer)); // Fill buffer with dummy data

    zassert_equal(safebuffer_snprintf(&sb, "12345678"), 0, "snprintf should succeed");
    print_debug_data(&sb, "After short message");
    zassert_equal(sb.write_index, 8, "Write index should be 8 after snprintf");
    zassert_false(sb.full, "Buffer should not be full after snprintf");
    zassert_false(sb.overflow, "Buffer should not have overflow after snprintf");

    safebuffer_reset(&sb);
    zassert_equal(sb.write_index, 0, "Write index should be reset to 0 after reset");
    zassert_equal(sb.read_index, 0, "Read index should be reset to 0 after reset");
    zassert_false(sb.full, "Buffer should not be full after reset");
    zassert_false(sb.overflow, "Buffer should not have overflow after reset");

    zassert_equal(safebuffer_snprintf(&sb, "1234567890123456789"), -ENOMEM, "snprintf should fail due to overflow");
    print_debug_data(&sb, "After overflow message");
    zassert_equal(sb.write_index, 9, "Write index should be 9 after snprintf overflow");
    zassert_true(sb.full, "Buffer should be full after failed snprintf");
    zassert_true(sb.overflow, "Buffer should have overflow after failed snprintf");
    zassert_equal(buffer[8], 0, "Terminating zero should be at index 8 after overflow");
    zassert_equal(buffer[9], 0xAA, "Buffer should not have been modified after overflow");

    safebuffer_reset(&sb);
    memset(buffer, 0xAA, sizeof(buffer)); // Fill buffer with dummy data
    // 9 characters need 10 bytes with the terminating zero, the buffer has 9 so the last character is lost
    zassert_equal(safebuffer_snprintf(&sb, "123456789"), -ENOMEM, "snprintf should fail when the terminating zero does not fit");
    print_debug_data(&sb, "Message one byte too long");
    zassert_equal(sb.write_index, 9, "Write index should be 9 after snprintf overflow");
    zassert_true(sb.full, "Buffer should be full after snprintf overflow");
    zassert_true(sb.overflow, "Buffer should have overflow when the last character did not fit");
    zassert_equal(buffer[7], '8', "The characters that fit should be written");
    zassert_equal(buffer[8], 0, "Buffer should have terminating zero in the last byte");
    zassert_equal(buffer[9], 0xAA, "Buffer should not have been modified past its end");
}

ZTEST(safebuffer, test_safebuffer_printf_append)
{
    safebuffer_t sb;
    uint8_t buffer[11];
    safebuffer_init(&sb, buffer, sizeof(buffer) - 1);
    memset(buffer, 0xAA, sizeof(buffer));

    zassert_equal(safebuffer_add_char(&sb, 'A'), 0, "Add char should succeed");
    zassert_equal(safebuffer_snprintf(&sb, "%d", 1234), 0, "snprintf after existing data should succeed");
    zassert_equal(sb.write_index, 5, "Write index should point to the terminating zero");
    zassert_equal(memcmp(buffer, "A1234", 6), 0, "Message should be added after the existing data with a terminating zero");
    zassert_equal(safebuffer_snprintf(&sb, "%s", "BCDE"), 0, "Second snprintf should overwrite the terminating zero");
    zassert_equal(sb.write_index, 9, "Exactly one byte left for the terminating zero");
    zassert_false(sb.full, "Buffer is not full, the terminating zero can be overwritten");
    zassert_false(sb.overflow, "Nothing was lost");
    zassert_equal(memcmp(buffer, "A1234BCDE", 10), 0, "Both messages should be in the buffer");
    zassert_equal(buffer[10], 0xAA, "Buffer should not have been modified past its end");
}

ZTEST(safebuffer, test_safebuffer_printf_full_buffer)
{
    safebuffer_t sb;
    uint8_t buffer[5];
    safebuffer_init(&sb, buffer, sizeof(buffer) - 1);
    memset(buffer, 0xAA, sizeof(buffer));

    zassert_equal(safebuffer_add_data(&sb, "1234", 4), 0, "Filling the buffer should succeed");
    zassert_true(sb.full, "Buffer should be full");
    // This used to write past the end of the buffer
    zassert_equal(safebuffer_snprintf(&sb, "%s", "more"), -ENOMEM, "snprintf on a full buffer should fail");
    zassert_equal(sb.write_index, 4, "Write index should not change");
    zassert_true(sb.overflow, "Overflow should be set");
    zassert_equal(buffer[4], 0xAA, "Buffer should not have been modified past its end");

    safebuffer_reset(&sb);
    zassert_equal(safebuffer_add_data(&sb, "1234", 4), 0, "Filling the buffer should succeed");
    zassert_equal(safebuffer_snprintf(&sb, "%s", ""), 0, "An empty message fits in a full buffer");
    zassert_false(sb.overflow, "Nothing was lost");
}

ZTEST(safebuffer, test_safebuffer_overflow_is_sticky)
{
    safebuffer_t sb;
    uint8_t buffer[4];
    safebuffer_init(&sb, buffer, sizeof(buffer));

    zassert_equal(safebuffer_add_data(&sb, "12345", 5), -ENOMEM, "Too much data should fail");
    zassert_true(sb.overflow, "Overflow should be set");
    zassert_equal(sb.write_index, 4, "The data that fits should be added");
    sb.write_index = 0;     // Users change write_index directly
    zassert_equal(safebuffer_add_char(&sb, 'A'), 0, "Add char should succeed");
    zassert_equal(safebuffer_snprintf(&sb, "B"), 0, "snprintf should succeed");
    zassert_true(sb.overflow, "A successful write must not clear an earlier overflow");
    safebuffer_reset(&sb);
    zassert_false(sb.overflow, "Reset clears overflow");
}

ZTEST(safebuffer, test_safebuffer_write_index_past_end)
{
    safebuffer_t sb;
    uint8_t buffer[6];
    safebuffer_init(&sb, buffer, sizeof(buffer) - 2);
    memset(buffer, 0xAA, sizeof(buffer));

    sb.write_index = sb.len + 1;    // Set past the end by the user
    zassert_equal(safebuffer_get_data_left_count(&sb), 0, "No space left when write index is past the end");
    zassert_equal(safebuffer_add_char(&sb, 'A'), -ENOMEM, "Add char must not write past the end");
    zassert_equal(safebuffer_add_data(&sb, "AB", 2), -ENOMEM, "Add data must not write past the end");
    zassert_equal(safebuffer_add_string(&sb, "AB"), -ENOMEM, "Add string must not write past the end");
    zassert_equal(safebuffer_snprintf(&sb, "AB"), -ENOMEM, "snprintf must not write past the end");
    zassert_equal(buffer[4], 0xAA, "Buffer should not have been modified past its end");
    zassert_equal(buffer[5], 0xAA, "Buffer should not have been modified past its end");
}

ZTEST(safebuffer, test_safebuffer_invalid_arguments)
{
    safebuffer_t sb;
    uint8_t buffer[4];
    safebuffer_init(&sb, buffer, sizeof(buffer));

    zassert_equal(safebuffer_add_data(NULL, "A", 1), -EFAULT, "NULL safebuffer should fail");
    zassert_equal(safebuffer_add_data(&sb, NULL, 1), -EINVAL, "NULL data should fail");
    zassert_equal(safebuffer_add_data(&sb, NULL, 0), 0, "Adding no data is allowed");
    zassert_equal(safebuffer_add_string(&sb, NULL), -EINVAL, "NULL string should fail");
    zassert_equal(safebuffer_add_safebuffer(&sb, NULL), -EFAULT, "NULL input safebuffer should fail");
    zassert_equal(sb.write_index, 0, "Nothing should have been added");
    zassert_is_null(safebuffer_malloc(0), "Allocating a zero length safebuffer should fail");
    zassert_equal(safebuffer_free(NULL), -EFAULT, "Freeing NULL should fail");
}

ZTEST(safebuffer, test_safebuffer_read)
{
    safebuffer_t sb;
    uint8_t buffer[4];
    uint8_t c;
    safebuffer_init(&sb, buffer, sizeof(buffer));

    zassert_equal(safebuffer_read_char(&sb, &c), -ENOSPC, "Reading an empty buffer should fail");
    safebuffer_add_data(&sb, "AB", 2);
    zassert_equal(safebuffer_read_char(&sb, &c), 0, "Read should succeed");
    zassert_equal(c, 'A', "First character should be read first");
    zassert_equal(safebuffer_read_char(&sb, &c), 0, "Read should succeed");
    zassert_equal(c, 'B', "Second character should be read");
    zassert_equal(safebuffer_read_char(&sb, &c), -ENOSPC, "Reading past the data should fail");
}

ZTEST(safebuffer, test_safebuffer_init_const)
{
    static const uint8_t data[] = {'X', 'Y'};
    safebuffer_t sb;
    uint8_t c;

    zassert_equal(safebuffer_init_const(&sb, data, sizeof(data)), 0, "Init const should succeed");
    zassert_true(sb.read_only, "Buffer should be read only");
    zassert_equal(sb.write_index, 2, "All the data should be available");
    zassert_equal(safebuffer_read_char(&sb, &c), 0, "Read should succeed");
    zassert_equal(c, 'X', "First character should be read");
    zassert_equal(safebuffer_reset(&sb), 0, "Reset should succeed");
    zassert_equal(sb.write_index, 2, "Reset must not remove the data of a read only buffer");
    zassert_equal(sb.read_index, 0, "Reset should start reading from the beginning");
    zassert_equal(safebuffer_add_char(&sb, 'A'), -EACCES, "Adding to a read only buffer should fail");
    zassert_equal(safebuffer_add_data(&sb, "A", 1), -EACCES, "Adding to a read only buffer should fail");
    zassert_equal(safebuffer_add_string(&sb, "A"), -EACCES, "Adding to a read only buffer should fail");
    zassert_equal(safebuffer_snprintf(&sb, "A"), -EACCES, "Adding to a read only buffer should fail");
}

ZTEST(safebuffer, test_safebuffer_read_data)
{
    safebuffer_t sb;
    uint8_t buffer[8];
    uint8_t out[4];
    safebuffer_init(&sb, buffer, sizeof(buffer));
    safebuffer_add_data(&sb, "ABCDE", 5);

    zassert_equal(safebuffer_get_read_left_count(&sb), 5, "All the data should be left to read");
    zassert_equal(safebuffer_read_data(&sb, out, 3), 0, "Reading 3 bytes should succeed");
    zassert_equal(memcmp(out, "ABC", 3), 0, "The first 3 bytes should be read");
    zassert_equal(sb.read_index, 3, "Read index should move past the data that was read");
    zassert_equal(safebuffer_get_read_left_count(&sb), 2, "2 bytes should be left to read");

    memset(out, 0, sizeof(out));
    zassert_equal(safebuffer_read_data(&sb, out, 3), -ENOSPC, "Reading more than is left should fail");
    zassert_equal(sb.read_index, 3, "Nothing should be read if not enough data is left");
    zassert_equal(out[0], 0, "The output should not be changed if not enough data is left");
    zassert_equal(safebuffer_read_data(&sb, out, 2), 0, "Reading exactly what is left should succeed");
    zassert_equal(memcmp(out, "DE", 2), 0, "The last 2 bytes should be read");
    zassert_equal(safebuffer_get_read_left_count(&sb), 0, "Nothing should be left to read");
    zassert_equal(safebuffer_read_data(&sb, out, 0), 0, "Reading nothing is allowed");
    zassert_equal(safebuffer_read_data(&sb, NULL, 1), -EINVAL, "NULL output should fail");
}

ZTEST(safebuffer, test_safebuffer_peek_and_skip)
{
    safebuffer_t sb;
    uint8_t buffer[8];
    uint8_t c;
    safebuffer_init(&sb, buffer, sizeof(buffer));

    zassert_equal(safebuffer_peek_char(&sb, &c), -ENOSPC, "Peeking an empty buffer should fail");
    zassert_equal(safebuffer_skip(&sb, 1), -ENOSPC, "Skipping in an empty buffer should fail");
    safebuffer_add_data(&sb, "ABCD", 4);

    zassert_equal(safebuffer_peek_char(&sb, &c), 0, "Peek should succeed");
    zassert_equal(c, 'A', "Peek should return the next character");
    zassert_equal(sb.read_index, 0, "Peek must not change the read index");
    zassert_equal(safebuffer_skip(&sb, 2), 0, "Skipping 2 bytes should succeed");
    zassert_equal(safebuffer_peek_char(&sb, &c), 0, "Peek should succeed");
    zassert_equal(c, 'C', "Peek should return the character after the skipped bytes");
    zassert_equal(safebuffer_skip(&sb, 3), -ENOSPC, "Skipping more than is left should fail");
    zassert_equal(sb.read_index, 2, "Nothing should be skipped if not enough data is left");
    zassert_equal(safebuffer_skip(&sb, 2), 0, "Skipping exactly what is left should succeed");
    zassert_equal(safebuffer_peek_char(&sb, &c), -ENOSPC, "Peek past the data should fail");
}

ZTEST(safebuffer, test_safebuffer_read_write_index_past_end)
{
    safebuffer_t sb;
    uint8_t buffer[6];
    uint8_t out[6];
    safebuffer_init(&sb, buffer, sizeof(buffer) - 2);
    memset(buffer, 0xAA, sizeof(buffer));

    sb.write_index = sb.len + 2;    // Set past the end by the user
    zassert_equal(safebuffer_data_len(&sb), 4, "Data length is limited to the buffer length");
    zassert_equal(safebuffer_get_read_left_count(&sb), 4, "Only the buffer can be read");
    zassert_equal(safebuffer_read_data(&sb, out, 5), -ENOSPC, "Reading past the end of the buffer should fail");
    zassert_equal(safebuffer_read_data(&sb, out, 4), 0, "Reading the whole buffer should succeed");
    zassert_equal(safebuffer_read_char(&sb, out), -ENOSPC, "Read char must not read past the end of the buffer");
}

ZTEST(safebuffer, test_safebuffer_reserve)
{
    safebuffer_t sb;
    uint8_t buffer[9];
    uint8_t *header;
    safebuffer_init(&sb, buffer, sizeof(buffer) - 1);
    memset(buffer, 0xAA, sizeof(buffer));

    // Reserve a header, add the payload and then fill in the header
    header = safebuffer_reserve(&sb, 2);
    zassert_not_null(header, "Reserving 2 bytes should succeed");
    zassert_equal(header, buffer, "The reserved bytes should be at the start of the buffer");
    zassert_equal(sb.write_index, 2, "Write index should move past the reserved bytes");
    zassert_equal(safebuffer_add_data(&sb, "XYZ", 3), 0, "Adding the payload should succeed");
    header[0] = 'L';
    header[1] = '3';
    zassert_equal(memcmp(buffer, "L3XYZ", 5), 0, "The header should be before the payload");

    header = safebuffer_reserve(&sb, 3);
    zassert_not_null(header, "Reserving the rest of the buffer should succeed");
    zassert_equal(header, &buffer[5], "The reserved bytes should be after the existing data");
    zassert_true(sb.full, "Buffer should be full after reserving the rest");
    zassert_false(sb.overflow, "Nothing was lost");
    zassert_is_null(safebuffer_reserve(&sb, 1), "Reserving in a full buffer should fail");
    zassert_true(sb.overflow, "Overflow should be set when reserving fails");
    zassert_equal(sb.write_index, 8, "Write index should not change when reserving fails");
    zassert_not_null(safebuffer_reserve(&sb, 0), "Reserving nothing is allowed");
    zassert_equal(buffer[8], 0xAA, "Buffer should not have been modified past its end");

    static const uint8_t data[] = {1};
    safebuffer_init_const(&sb, data, sizeof(data));
    zassert_is_null(safebuffer_reserve(&sb, 0), "Reserving in a read only buffer should fail");
}

ZTEST(safebuffer, test_safebuffer_is_empty_and_data_len)
{
    safebuffer_t sb;
    uint8_t buffer[4];
    uint8_t c;
    safebuffer_init(&sb, buffer, sizeof(buffer));

    zassert_true(safebuffer_is_empty(&sb), "A new buffer should be empty");
    zassert_equal(safebuffer_data_len(&sb), 0, "A new buffer has no data");
    safebuffer_add_data(&sb, "AB", 2);
    zassert_false(safebuffer_is_empty(&sb), "Buffer with data should not be empty");
    zassert_equal(safebuffer_data_len(&sb), 2, "Data length should be 2");
    safebuffer_read_char(&sb, &c);
    zassert_equal(safebuffer_data_len(&sb), 2, "Reading does not change the data length");
    safebuffer_reset(&sb);
    zassert_true(safebuffer_is_empty(&sb), "Buffer should be empty after reset");
    zassert_true(safebuffer_is_empty(NULL), "NULL is treated as empty");
    zassert_equal(safebuffer_data_len(NULL), 0, "NULL has no data");
}

ZTEST(safebuffer, test_safebuffer_compact)
{
    safebuffer_t sb;
    uint8_t buffer[6];
    uint8_t c;
    safebuffer_init(&sb, buffer, sizeof(buffer));

    safebuffer_add_data(&sb, "ABCDEF", 6);
    zassert_true(sb.full, "Buffer should be full");
    safebuffer_skip(&sb, 4);
    zassert_equal(safebuffer_compact(&sb), 0, "Compact should succeed");
    zassert_equal(sb.read_index, 0, "Read index should be 0 after compact");
    zassert_equal(sb.write_index, 2, "Only the unread data should be left");
    zassert_false(sb.full, "Buffer should have space after compact");
    zassert_equal(memcmp(buffer, "EF", 2), 0, "Unread data should be moved to the start");
    zassert_equal(safebuffer_add_data(&sb, "GHIJ", 4), 0, "The space should be available again");
    zassert_equal(memcmp(buffer, "EFGHIJ", 6), 0, "New data should follow the unread data");
    zassert_equal(safebuffer_read_char(&sb, &c), 0, "Read should succeed");
    zassert_equal(c, 'E', "Reading should continue with the unread data");

    // Compact when everything was read empties the buffer
    safebuffer_skip(&sb, 5);
    zassert_equal(safebuffer_compact(&sb), 0, "Compact should succeed");
    zassert_true(safebuffer_is_empty(&sb), "Buffer should be empty if everything was read");

    // Nothing read yet, data stays where it is
    safebuffer_add_data(&sb, "AB", 2);
    zassert_equal(safebuffer_compact(&sb), 0, "Compact should succeed");
    zassert_equal(sb.write_index, 2, "Data should stay when nothing was read");
    zassert_equal(memcmp(buffer, "AB", 2), 0, "Data should not move when nothing was read");

    static const uint8_t data[] = {1, 2};
    safebuffer_init_const(&sb, data, sizeof(data));
    zassert_equal(safebuffer_compact(&sb), -EACCES, "Compacting a read only buffer should fail");
}

ZTEST_SUITE(safebuffer, NULL, NULL, NULL, NULL, NULL);
