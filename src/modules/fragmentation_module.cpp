#include "modules/fragmentation_module.h"

namespace net_blocks {

fragmentation_module fragmentation_module::instance;

void fragmentation_module::init_module(void) 
{
    framework::instance.register_module(this);
}

module::hook_status fragmentation_module::hook_send(builder::dyn_var<connection_t*> c, packet_t p, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len)
{
    // TODO: Should headroom be subtracted from this
    auto header_size = net_packet.get_total_size() - 1;
    auto available_size = MTU_SIZE - header_size;

    if (len <= available_size) {
        return hook_status::HOOK_CONTINUE;
    }

    builder::dyn_var<int> sent = 0;
    while (len > 0) {
        builder::dyn_var<int> to_send = available_size;
        if (len < available_size) {
            to_send = len;
        }
        len -= to_send;

        sent += framework::instance.run_send_path(c, buff, to_send, m_sequence + 1);
        buff += to_send;
    }

    ret_len[0] = sent;
    return hook_status::HOOK_DROP;
}

module::hook_status fragmentation_module::hook_ingress(packet_t p)
{
    return hook_status::HOOK_CONTINUE;
}

}
