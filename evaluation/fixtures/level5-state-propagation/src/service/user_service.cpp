#include "user_service.h"

namespace service {

void UserService::process_transaction(const std::string& user_id, const model::Transaction& tx, bool dry_run) {
    // BUG: Options struct is populated using default settings but forgets to assign the dry_run parameter.
    // It should perform: options.dry_run = dry_run;
    model::TransactionOptions options = model::TransactionOptions::default_options();
    tx_manager_.execute(tx, options);
}

} // namespace service
