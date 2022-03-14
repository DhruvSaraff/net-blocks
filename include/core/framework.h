#ifndef NET_BLOCKS_FRAMEWORK_H
#define NET_BLOCKS_FRAMEWORK_H

#include <vector>
#include "core/module.h"
#include "core/packet.h"

namespace net_blocks {

extern dynamic_layout net_packet;

class module;

class framework {
private:
	std::vector<module*> m_registered_modules;
	// Singleton class framework
	framework() = default;
public:
	static framework instance;

	void register_module(module*);	
	
	// Implementations for various paths	
	builder::dyn_var<connection_t*> run_establish_path(builder::dyn_var<unsigned int>, builder::dyn_var<unsigned int>);
	void run_destablish_path(builder::dyn_var<connection_t*>);
	builder::dyn_var<int> run_send_path(builder::dyn_var<connection_t*>, builder::dyn_var<char*>, builder::dyn_var<int>);
	void run_ingress_path(packet_t);
};



}
#endif
