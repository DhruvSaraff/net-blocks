#ifndef NET_BLOCKS_COMPRESSION_MODULE_H
#define NET_BLOCKS_COMPRESSION_MODULE_H

#include "core/framework.h"

namespace net_blocks {

class compression_module : public module {
public:
    enum class compression_strategy_t {
        NO_COMPRESSION,
        ENTIRE_PACKET,
        PAYLOAD_ONLY
    };

    void init_module(void) override;
    
    void configureCompression(compression_strategy_t strategy) {
        compression_strategy = strategy;
    }

    void hook_net_init(void) override;

	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) override;
	module::hook_status hook_ingress(packet_t) override;

private:
    compression_module() = default;
    compression_strategy_t compression_strategy = compression_strategy_t::NO_COMPRESSION;

public:
    static compression_module instance;
};

}

#endif
