#pragma once
#include <stdlib.h>
#include <string.h>

#include "include/doca_buf.h"
#include "include/doca_buf_inventory.h"
#include "include/doca_log.h"
#include "ray/object_manager/plasma/common.h"

#include "dma_copy_core.h"

int RunDmaExport(const plasma::Allocation &allocation, size_t &export_desc_len, char **export_desc);
