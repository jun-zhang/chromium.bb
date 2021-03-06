// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// IPC messages for HTML5 Blob and Stream.
// Multiply-included message file, hence no include guard.

#include <stddef.h>

#include "content/common/content_export.h"
#include "content/public/common/common_param_traits.h"
#include "ipc/ipc_message_macros.h"
#include "storage/common/data_element.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START BlobMsgStart

// Blob messages sent from the renderer to the browser.

IPC_MESSAGE_CONTROL1(BlobHostMsg_StartBuilding,
                     std::string /*uuid */)
IPC_MESSAGE_CONTROL2(BlobHostMsg_AppendBlobDataItem,
                     std::string /* uuid */,
                     storage::DataElement)
IPC_SYNC_MESSAGE_CONTROL3_0(BlobHostMsg_SyncAppendSharedMemory,
                            std::string /*uuid*/,
                            base::SharedMemoryHandle,
                            uint32_t /* buffer size */)
IPC_MESSAGE_CONTROL2(BlobHostMsg_FinishBuilding,
                     std::string /* uuid */,
                     std::string /* content_type */)

IPC_MESSAGE_CONTROL1(BlobHostMsg_IncrementRefCount,
                     std::string /* uuid */)
IPC_MESSAGE_CONTROL1(BlobHostMsg_DecrementRefCount,
                     std::string /* uuid */)
IPC_MESSAGE_CONTROL2(BlobHostMsg_RegisterPublicURL,
                     GURL,
                     std::string /* uuid */)
IPC_MESSAGE_CONTROL1(BlobHostMsg_RevokePublicURL,
                     GURL)

// Stream messages sent from the renderer to the browser.

// Registers a stream as being built.
IPC_MESSAGE_CONTROL2(StreamHostMsg_StartBuilding,
                     GURL /* url */,
                     std::string /* content_type */)

// Appends data to a stream being built.
IPC_MESSAGE_CONTROL2(StreamHostMsg_AppendBlobDataItem,
                     GURL /* url */,
                     storage::DataElement)

// Appends data to a stream being built.
IPC_SYNC_MESSAGE_CONTROL3_0(StreamHostMsg_SyncAppendSharedMemory,
                            GURL /* url */,
                            base::SharedMemoryHandle,
                            uint32_t /* buffer size */)

// Flushes contents buffered in the stream.
IPC_MESSAGE_CONTROL1(StreamHostMsg_Flush,
                     GURL /* url */)

// Finishes building a stream.
IPC_MESSAGE_CONTROL1(StreamHostMsg_FinishBuilding,
                     GURL /* url */)

// Aborts building a stream.
IPC_MESSAGE_CONTROL1(StreamHostMsg_AbortBuilding,
                     GURL /* url */)

// Creates a new stream that's a clone of an existing src stream.
IPC_MESSAGE_CONTROL2(StreamHostMsg_Clone,
                     GURL /* url */,
                     GURL /* src_url */)

// Removes a stream.
IPC_MESSAGE_CONTROL1(StreamHostMsg_Remove,
                     GURL /* url */)
