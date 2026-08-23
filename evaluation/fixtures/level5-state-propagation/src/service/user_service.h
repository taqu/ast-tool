#pragma once
#include <string>
#include "model/transaction.h"
#include "transaction_manager.h"

namespace service {

class UserService {
public:
    explicit UserService(TransactionManager& tx_manager) : tx_manager_(tx_manager) {}

    void process_transaction(const std::string& user_id, const model::Transaction& tx, bool dry_run);

private:
    TransactionManager& tx_manager_;
};

} // namespace service
