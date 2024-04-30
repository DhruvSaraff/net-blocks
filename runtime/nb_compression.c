#include "nb_compression.h"
#include "lz4.h"

int compress(char* in, int in_len, char* out, int out_len, int* compressed_len) {
    int ret = LZ4_compress_default(in, out, in_len, out_len);
    *compressed_len = ret;
    return ret == 0; // error is zero returned value
}

int decompress(char* in, int in_len, char* out, int out_len, int* decompressed_len) {
    int ret = LZ4_decompress_safe(in, out, in_len, out_len);
    *decompressed_len = ret;
    return ret < 0; // error is negative returned value
}
