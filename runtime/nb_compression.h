#ifndef NB_COMPRESSION_H
#define NB_COMPRESSION_H

#ifdef __cplusplus
extern "C" {
#endif

int zlib_compress(char* in, int in_len, char* out, int out_len, int* compressed_len);

int zlib_decompress(char* in, int len, char* out, int out_len, int* decompressed_len);

#ifdef __cplusplus 
}
#endif

#endif
