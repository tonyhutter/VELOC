#include "module_manager.hpp"

#define __DEBUG
#include "common/debug.hpp"

module_manager_t::module_manager_t() { }

void module_manager_t::add_default_modules(const config_t &cfg, MPI_Comm comm, bool ec_active) {
    watchdog = new client_watchdog_t(cfg);
    add_module([this](const command_t &c) { return watchdog->process_command(c); });
    if (ec_active) {
	redset = new ec_module_t(cfg, comm);
	ec_agg = new client_aggregator_t(
	    [this](const std::vector<command_t> &cmds) {
		return redset->process_commands(cmds);
	    },
	    [this](const command_t &c) {
		return redset->process_command(c);
	    });
	add_module([this](const command_t &c) { return ec_agg->process_command(c); });
    }
    transfer = new transfer_module_t(cfg);
    add_module([this](const command_t &c) { return transfer->process_command(c); });
}

module_manager_t::~module_manager_t() {
    delete watchdog;
    delete ec_agg;
    delete redset;
    delete transfer;
}

int module_manager_t::notify_command(const command_t &c) {
    int ret = VELOC_SUCCESS;
    for (auto &f : sig) {
	int mod_ret = f(c);
	if (c.command == command_t::TEST && mod_ret != VELOC_FAILURE)
	    ret = std::max(ret, mod_ret);
	else
	    ret = std::min(ret, mod_ret);
    }
    return ret;
}
