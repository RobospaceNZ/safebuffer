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
    zassert_not_null(&sb, "Failed to allocate safebuffer");

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
    zassert_equal(sb.write_index, 9, "Write index should be 9 after snprintf with exact size including a terminating zero");
    zassert_true(sb.full, "Buffer should be full after failed snprintf");
    zassert_true(sb.overflow, "Buffer should have overflow after failed snprintf");
    zassert_equal(buffer[8], 0, "Terminating zero should be at index 18 after overflow");
    zassert_equal(buffer[9], 0xAA, "Buffer should not have been modified after overflow");

    safebuffer_reset(&sb);
    memset(buffer, 0xAA, sizeof(buffer)); // Fill buffer with dummy data
    // Fill buffer with exactly the size of the buffer
    zassert_equal(safebuffer_snprintf(&sb, "123456789"), 0, "snprintf should succeed with exact buffer size");
    print_debug_data(&sb, "Exact size message to fill buffer");
    zassert_equal(sb.write_index, 9, "Write index should be 9 after snprintf with exact size including a terminating zero");
    zassert_true(sb.full, "Buffer should not be full after snprintf with exact size");
    zassert_false(sb.overflow, "Buffer should not have overflow after snprintf with exact size");
    zassert_equal(buffer[8], 0, "Buffer should have terminating zero after snprintf with exact size");
    zassert_equal(buffer[9], 0xAA, "Buffer should not have been modified after snprintf with exact size");
}

ZTEST_SUITE(safebuffer, NULL, NULL, NULL, NULL, NULL);
