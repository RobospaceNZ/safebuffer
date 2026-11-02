#
# Copyright (c) 2025 Hubert Jetschko
#
# SPDX-License-Identifier: MIT
#

if(CONFIG_SAFEBUFFER)
  zephyr_library_named(safebuffer)
  zephyr_library_sources(safebuffer.c)
  zephyr_include_directories(.)
endif()
