#include "modules/keepalive_module.h"
#include "blocks/rce.h"

namespace net_blocks {

namespace runtime {
    // Move declarations to runtime.h and implementation to runtime.cpp
    builder::dyn_var<int(connection_t*, char*, int)> nb__send = builder::as_global("nb__send");
}

keepalive_module keepalive_module::instance;

void keepalive_module::init_module(void) {
    conn_layout.register_member<builder::dyn_var<unsigned long long>>("keepalive_timer");
    conn_layout.register_member<builder::dyn_var<unsigned long long>>("num_keepalive_sent");
    framework::instance.register_module(this);
}

static builder::dyn_var<void(timer_t*, void*, unsigned long long)> keepalive_timer_callback = builder::as_global("nb__keepalive_timer_cb");

static void keepalive_cb(builder::dyn_var<runtime::timer_t*> t, builder::dyn_var<void*> param, 
		builder::dyn_var<unsigned long long> to) 
{
    builder::dyn_var<connection_t*> c = runtime::to_void_ptr(param);
    // TODO: Do we configure number of keepalives and how do we kill the connection here
    if (conn_layout.get(c, "num_keepalive_sent") == 3) {
        runtime::printf("Reached the limit for number of keepalives\\n");
        framework::instance.run_destablish_path(c);
        // TODO: Is this good enough
        return;
    }

    conn_layout.get(c, "num_keepalive_sent") += 1;

    // Add a dummy timer as this will be removed during hook_send()
    runtime::insert_timer(t, runtime::get_time_ms_now() + KEEPALIVE_TIMEOUT_MS, keepalive_timer_callback, c);

    runtime::printf("Sending keepalive\\n");
    runtime::nb__send(c, "", 0);
}

void keepalive_module::gen_timer_callback(std::ostream &oss)
{
    auto ast = builder::builder_context().extract_function_ast(keepalive_cb, "nb__keepalive_timer_cb");
    block::eliminate_redundant_vars(ast);
    block::c_code_generator::generate_code(ast, oss);
}

module::hook_status keepalive_module::hook_establish(builder::dyn_var<connection_t*> c, 
	builder::dyn_var<unsigned int> remote_host, builder::dyn_var<unsigned int> remote_app, 
	builder::dyn_var<unsigned int> local_app) 
{
    builder::dyn_var<runtime::timer_t*> t = runtime::alloc_timer();
    conn_layout.get(c, "keepalive_timer") = runtime::to_ull(t);
    conn_layout.get(c, "num_keepalive_sent") = 0;
    runtime::insert_timer(t, runtime::get_time_ms_now() + KEEPALIVE_TIMEOUT_MS, keepalive_timer_callback, c);
    return module::hook_status::HOOK_CONTINUE;
}

module::hook_status keepalive_module::hook_send(builder::dyn_var<connection_t*> c, packet_t, 
	builder::dyn_var<char*> buff, builder::dyn_var<unsigned int> len, builder::dyn_var<int*> ret_len)
{
    builder::dyn_var<runtime::timer_t*> t = runtime::to_void_ptr(conn_layout.get(c, "keepalive_timer"));
    runtime::remove_timer(t);
    runtime::insert_timer(t, runtime::get_time_ms_now() + KEEPALIVE_TIMEOUT_MS, keepalive_timer_callback, c);
    return module::hook_status::HOOK_CONTINUE;
}

module::hook_status keepalive_module::hook_ingress(packet_t p) 
{
    builder::dyn_var<connection_t*> c = runtime::to_void_ptr(net_packet["flow_identifier"]->get_integer(p));
    conn_layout.get(c, "num_keepalive_sent") = 0;
    return module::hook_status::HOOK_CONTINUE;
}

}