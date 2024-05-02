#ifndef NET_BLOCKS_FRAGMENTATION_MODULE_H
#define NET_BLOCKS_FRAGMENTATION_MODULE_H

#include "core/framework.h"

namespace net_blocks {

class fragmentation_module : public module {
public:
    void init_module(void) override;

	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len) override;
	module::hook_status hook_ingress(packet_t) override;

private:
    fragmentation_module() = default;

public:
    static fragmentation_module instance;
};

}

#endif
