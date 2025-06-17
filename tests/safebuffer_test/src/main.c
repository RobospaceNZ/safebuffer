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

ZTEST_SUITE(safebuffer, NULL, NULL, NULL, NULL, NULL);
