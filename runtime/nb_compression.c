#include "nb_compression.h"
#include "zlib.h"

static z_stream init_zstream(char* in, int in_len, char* out, int out_len) {
    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    stream.avail_in = in_len;
    stream.next_in = (Bytef*)in;
    stream.avail_out = out_len;
    stream.next_out = (Bytef*)out;

    return stream;
}

int zlib_compress(char* in, int in_len, char* out, int out_len, int* compressed_len) {
    uLongf out_len_ulong = out_len;
    int ret = compress(out, &out_len_ulong, in, in_len);
    *compressed_len = out_len_ulong;
    return ret;
    // z_stream def_stream = init_zstream(in, in_len, out, out_len);
    // deflateInit(&def_stream, Z_BEST_COMPRESSION);
    // int deflate_ret = deflate(&def_stream, Z_FINISH);
    // deflateEnd(&def_stream);
    // *compressed_len = def_stream.total_out;
    // return deflate_ret;
}

int zlib_decompress(char* in, int in_len, char* out, int out_len, int* decompressed_len) {
    uLongf out_len_ulong = out_len;
    int ret = uncompress(out, &out_len_ulong, in, in_len);
    *decompressed_len = out_len_ulong;
    return ret;
    // z_stream inf_stream = init_zstream(in, in_len, out, out_len);
    // inflateInit(&inf_stream);
    // int inflate_ret = inflate(&inf_stream, Z_NO_FLUSH);
    // inflateEnd(&inf_stream);
    // *decompressed_len = inf_stream.total_out;
    // return inflate_ret;
}
