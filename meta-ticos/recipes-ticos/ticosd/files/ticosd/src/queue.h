//! @file
//!
//! Copyright (c) Ticos, Inc.
//! See License.txt for details
//!
//! @brief
//! File-backed transmit queue management definition
//!

#ifndef __TICOS_QUEUE_H
#define __TICOS_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "ticosd.h"

typedef struct TicosdQueue sTicosdQueue;

sTicosdQueue *ticosd_queue_init(sTicosd *ticosd, int size);
void ticosd_queue_destroy(sTicosdQueue *handle);
void ticosd_queue_reset(sTicosdQueue *handle);
bool ticosd_queue_write(sTicosdQueue *handle, const uint8_t *payload,
                           uint32_t payload_size_bytes);
uint8_t *ticosd_queue_read_head(sTicosdQueue *handle, uint32_t *payload_size_bytes);
bool ticosd_queue_complete_read(sTicosdQueue *handle);

#ifdef __cplusplus
}
#endif
#endif
