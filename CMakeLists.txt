#
# Copyright (c) 2025 Hubert Jetschko
#
# SPDX-License-Identifier: MIT
#

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/safebuffer.c)
