#ifndef NET_BLOCKS_KEEPALIVE_MODULE_H
#define NET_BLOCKS_KEEPALIVE_MODULE_H

#include "core/framework.h"

#define KEEPALIVE_TIMEOUT_MS 7000

namespace net_blocks {

class keepalive_module : public module {
public:
    void init_module(void) override;

	module::hook_status hook_establish(builder::dyn_var<connection_t*> c, 
		builder::dyn_var<unsigned int> remote_host, builder::dyn_var<unsigned int> remote_app, 
		builder::dyn_var<unsigned int> local_app) override;
	module::hook_status hook_send(builder::dyn_var<connection_t*> c, packet_t, 
		builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len);

	module::hook_status hook_ingress(packet_t);

	void gen_timer_callback(std::ostream &oss);

private:
    keepalive_module() = default;

public:
    static keepalive_module instance;
};

}


#endif
