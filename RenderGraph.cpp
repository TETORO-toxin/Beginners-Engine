#include "RenderGraph.h"

// RenderGraph implementation file kept minimal because most logic is header-only.
// This file ensures translation unit for the class when linking across multiple files.


// Optionally expose a debug dump
void DumpRenderGraph(const RenderGraph& rg) {
    // This is intentionally empty; hooks for debug UI can be added later.
}
