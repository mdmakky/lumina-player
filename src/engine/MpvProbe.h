#pragma once

#include "storage/LibraryRepository.h"

// Reads duration, resolution, codec, tags and cover-art presence with a short-lived libmpv
// handle (vo=null, ao=null, paused, no decoding). Blocking: call from a worker thread only.
namespace MpvProbe {

// Fills the metadata fields of `item` (path must be set); false if mpv cannot open the file.
bool probe(LibraryItem &item, int timeoutMs = 6000);

} // namespace MpvProbe
