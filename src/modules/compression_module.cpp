#include "modules/compression_module.h"

namespace net_blocks {

namespace runtime {
    builder::dyn_var<int(char*, int, char*, int, int*)> compress = builder::as_global("compress");
    builder::dyn_var<int(char*, int, char*, int, int*)> decompress = builder::as_global("decompress");
}

dynamic_layout compressed_net_packet;

compression_module compression_module::instance;

void compression_module::init_module(void) {
    framework::instance.register_module(this);
    net_state.register_member<builder::dyn_var<char*>>("compression_buffer");
	compressed_net_packet.add_member("length", new generic_integer_member<uint32_t>((int)member_flags::aligned), 1);
	compressed_net_packet.add_member("payload", new generic_integer_member<char>((int)member_flags::aligned), 1);
    compressed_net_packet.fix_layout();
}

void compression_module::hook_net_init() 
{
    get_state("compression_buffer") = runtime::malloc(MTU_SIZE);
}

module::hook_status compression_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len)
{
    if (compression_strategy == compression_strategy_t::PAYLOAD_ONLY) {
        auto header_size = net_packet.get_total_size() - get_headroom() - 1;
        auto payload_len = net_packet["total_len"]->get_integer(p) - header_size;
        builder::dyn_var<char*> packet_copy = get_state("compression_buffer");
        runtime::memcpy(packet_copy, net_packet["payload"]->get_addr(p), payload_len);

        builder::dyn_var<int> compressed_length = payload_len;
        // runtime::memcpy(net_packet["payload"]->get_addr(p), packet_copy, payload_len);
        builder::dyn_var<int> compress_status = runtime::compress(
            packet_copy, 
            payload_len, 
            net_packet["payload"]->get_addr(p), 
            MTU_SIZE - header_size - get_headroom(), 
            &compressed_length);
        // builder::dgyn_var<int> compress_status = Z_OK;
        // runtime::printf("Compressed length: %d\\n", compressed_length);

        auto total_len = header_size + compressed_length;
    	net_packet["total_len"]->set_integer(p, total_len);
        net_packet["computed_total_len"]->set_integer(p, total_len);
        return compress_status == 0
            ? hook_status::HOOK_CONTINUE
            : hook_status::HOOK_DROP;

    } else if (compression_strategy == compression_strategy_t::ENTIRE_PACKET) {
        // runtime::printf("Sending\\n");
        auto len = net_packet["total_len"]->get_integer(p);
        builder::dyn_var<char*> packet_copy = get_state("compression_buffer");
        runtime::memcpy(packet_copy, p + get_headroom(), len);

        // runtime::printf("%d\\n", len);
        for (builder::dyn_var<int> i = 0; i < len; i++) {
            // runtime::printf("%d ", (builder::dyn_var<unsigned int>)packet_copy[i]);
        }
        // runtime::printf("\\n");

        auto compressed_header_size = compressed_net_packet.get_total_size() - 1;
        builder::dyn_var<int> compressed_length;
        // runtime::printf("Fuck3\\n");
        builder::dyn_var<int> compress_status = runtime::compress(
            packet_copy,
            len,
            compressed_net_packet["payload"]->get_addr(p + get_headroom()),
            MTU_SIZE - compressed_header_size - get_headroom(),
            &compressed_length);
        // runtime::printf("Fuck4 %d\\n", compress_status);
        // builder::dyn_var<int> compressed_length = len;
        // builder::dyn_var<int> compress_status = Z_OK;
        // runtime::memcpy(
        //     compressed_net_packet["payload"]->get_addr(p + get_headroom()),
        //     packet_copy,
        //     len);

        compressed_net_packet["length"]->set_integer(p + get_headroom(), compressed_length);
        // runtime::printf("%d\\n", compressed_net_packet["length"]->get_integer(p + get_headroom()));
        for (builder::dyn_var<int> i = 0; i < compressed_length; i++) {
            // runtime::printf("%d ", (builder::dyn_var<unsigned int>)compressed_net_packet["payload"]->get_addr(p + get_headroom())[i]);
        }
        // runtime::printf("\\n");
        net_packet["computed_total_len"]->set_integer(p, compressed_length + compressed_header_size);
        // runtime::printf("%d\\n", compress_status);
        return compress_status == 0
            ? hook_status::HOOK_CONTINUE
            : hook_status::HOOK_DROP;
    }
    return hook_status::HOOK_CONTINUE;
}

module::hook_status compression_module::hook_ingress(packet_t p) {    
    if (compression_strategy == compression_strategy_t::PAYLOAD_ONLY) {
        auto header_size = net_packet.get_total_size() - get_headroom() - 1;
        builder::dyn_var<int> len = net_packet["total_len"]->get_integer(p) - header_size; 
        builder::dyn_var<char*> compressed_copy = get_state("compression_buffer");
        runtime::memcpy(compressed_copy, net_packet["payload"]->get_addr(p), len);

        builder::dyn_var<int> uncompressed_length = len;
        // runtime::memcpy(net_packet["payload"]->get_addr(p), compressed_copy, len);
        builder::dyn_var<int> decompress_status = runtime::decompress(
            compressed_copy, 
            len, 
            net_packet["payload"]->get_addr(p), 
            MTU_SIZE - header_size - get_headroom(),
            &uncompressed_length);
        // builder::dyn_var<int> decompress_status = Z_OK;

        auto total_len = (net_packet.get_total_size() - get_headroom() - 1) + uncompressed_length;
        net_packet["total_len"]->set_integer(p, total_len);
        net_packet["computed_total_len"]->set_integer(p, total_len);
        return decompress_status == 0
            ? hook_status::HOOK_CONTINUE
            : hook_status::HOOK_DROP;

    } else if (compression_strategy == compression_strategy_t::ENTIRE_PACKET) {
        // runtime::printf("Receiving\\n");
        // runtime::printf("%d\\n", compressed_net_packet["length"]->get_integer(p + get_headroom()));
        auto len = compressed_net_packet["length"]->get_integer(p + get_headroom());
        // if (len > 53)
        //     len = 53;
        builder::dyn_var<char*> compressed_copy = get_state("compression_buffer");
        runtime::memcpy(compressed_copy, compressed_net_packet["payload"]->get_addr(p + get_headroom()), len);

        for (builder::dyn_var<int> i = 0; i < len; i++) {
            // runtime::printf("%d ", (builder::dyn_var<unsigned int>)compressed_copy[i]);
        }
        // runtime::printf("\\n");
        // runtime::printf("Fuck1\\n");
        builder::dyn_var<int> uncompressed_length;
        builder::dyn_var<int> decompress_status = runtime::decompress(
            compressed_copy,
            len,
            p + get_headroom(),
            MTU_SIZE - get_headroom(),
            &uncompressed_length);
        // runtime::printf("Fuck2: %d\\n", decompress_status);
        // builder::dyn_var<int> uncompressed_length = len;
        // builder::dyn_var<int> decompress_status = Z_OK;
        // runtime::memcpy(
        //     p + get_headroom(),
        //     compressed_copy,
        //     len);

        // TODO: Mirror group 0 of net_packet into compressed_packet
        // Link the two net packets together
        auto total_len = net_packet["total_len"]->get_integer(p);
        net_packet["computed_total_len"]->set_integer(p, total_len);
        // runtime::printf("%d\\n", decompress_status);
        // runtime::printf("%d\\n", total_len);
        // for (builder::dyn_var<int> i = 0; i < total_len; i++) {
        //     packet_t uc = (packet_t)runtime::to_void_ptr(p + get_headroom());
        //     // runtime::printf("%d ", (builder::dyn_var<unsigned int>)uc[i]);
        // }
        // runtime::printf("\\n");
        return decompress_status == 0
            ? hook_status::HOOK_CONTINUE
            : hook_status::HOOK_DROP;
    }
    return hook_status::HOOK_CONTINUE;
}

}
