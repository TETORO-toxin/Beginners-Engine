#pragma once

#include "miniz_export.h"
#include "miniz_common.h"

/* zlib-style API definitions and prototypes used by miniz_upstream.c */

#ifdef __cplusplus
extern "C" {
#endif

    /* For more compatibility with zlib, miniz.c uses unsigned long for some parameters/struct members. */
    typedef unsigned long mz_ulong;

    MINIZ_EXPORT void mz_free(void *p);

#define MZ_ADLER32_INIT (1)
    MINIZ_EXPORT mz_ulong mz_adler32(mz_ulong adler, const unsigned char *ptr, size_t buf_len);

#define MZ_CRC32_INIT (0)
    MINIZ_EXPORT mz_ulong mz_crc32(mz_ulong crc, const unsigned char *ptr, size_t buf_len);

    typedef struct mz_stream_s mz_stream;
    typedef mz_stream *mz_streamp;

    /* Memory allocation callbacks (provided by miniz_upstream or user) */
    MINIZ_EXPORT void *miniz_def_alloc_func(void *opaque, size_t items, size_t size);
    MINIZ_EXPORT void miniz_def_free_func(void *opaque, void *address);
    MINIZ_EXPORT void *miniz_def_realloc_func(void *opaque, void *address, size_t items, size_t size);

    MINIZ_EXPORT const char *mz_version(void);

    /* tinfl_uncompress: lightweight inflate function implemented in miniz.c (when not using upstream).
       Declared here so C++ code can call it. Returns 0 on success, non-zero on failure. */
    MINIZ_EXPORT int tinfl_uncompress(void *pBuf, size_t *pBuf_size, const void *pSrc, size_t src_size);

#ifndef MINIZ_NO_ZLIB_APIS
    MINIZ_EXPORT int mz_deflateInit(mz_streamp pStream, int level);
    MINIZ_EXPORT int mz_deflateInit2(mz_streamp pStream, int level, int method, int window_bits, int mem_level, int strategy);
    MINIZ_EXPORT int mz_deflateReset(mz_streamp pStream);
    MINIZ_EXPORT int mz_deflate(mz_streamp pStream, int flush);
    MINIZ_EXPORT int mz_deflateEnd(mz_streamp pStream);
    MINIZ_EXPORT mz_ulong mz_deflateBound(mz_streamp pStream, mz_ulong source_len);
    MINIZ_EXPORT int mz_compress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len, int level);
    MINIZ_EXPORT int mz_compress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
    MINIZ_EXPORT mz_ulong mz_compressBound(mz_ulong source_len);

    MINIZ_EXPORT int mz_inflateInit(mz_streamp pStream);
    MINIZ_EXPORT int mz_inflateInit2(mz_streamp pStream, int window_bits);
    MINIZ_EXPORT int mz_inflateReset(mz_streamp pStream);
    MINIZ_EXPORT int mz_inflate(mz_streamp pStream, int flush);
    MINIZ_EXPORT int mz_inflateEnd(mz_streamp pStream);
    MINIZ_EXPORT int mz_uncompress(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong source_len);
    MINIZ_EXPORT int mz_uncompress2(unsigned char *pDest, mz_ulong *pDest_len, const unsigned char *pSource, mz_ulong *pSource_len);

    MINIZ_EXPORT const char *mz_error(int err);
#endif

#ifdef __cplusplus
}
#endif
