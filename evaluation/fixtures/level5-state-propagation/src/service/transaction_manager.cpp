#include "transaction_manager.h"

namespace service {

void TransactionManager::execute(const model::Transaction& tx, const model::TransactionOptions& options) {
    db_.write_transaction(tx, options);
}

} // namespace service
