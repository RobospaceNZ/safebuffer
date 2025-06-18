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

ZTEST_SUITE(safebuffer, NULL, NULL, NULL, NULL, NULL);
