#pragma once
#include <stdlib.h>
#include <string.h>

#include "include/doca_buf.h"
#include "include/doca_buf_inventory.h"
#include "include/doca_log.h"
#include "ray/object_manager/plasma/common.h"

#include "dma_copy_core.h"
#include "secure_channel.h"
char* RunDmaExport(BaseMetaInfo &metainfo);
