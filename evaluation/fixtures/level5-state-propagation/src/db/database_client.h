#pragma once
#include <string>
#include <vector>
#include "model/context.h"
#include "model/transaction.h"

namespace db {

class DatabaseClient {
public:
    void save_order(const std::string& order_id, const std::string& user_id, const model::RequestContext& context) {
        last_saved_order_id_ = order_id;
        last_saved_user_id_ = user_id;
        last_saved_correlation_id_ = context.correlation_id;
        last_saved_request_id_ = context.request_id;
    }

    void write_transaction(const model::Transaction& tx, const model::TransactionOptions& options) {
        last_tx_id_ = tx.id;
        last_tx_amount_ = tx.amount;
        last_tx_dry_run_ = options.dry_run;
        if (!options.dry_run) {
            committed_tx_ids_.push_back(tx.id);
        }
    }

    std::string last_saved_order_id_;
    std::string last_saved_user_id_;
    std::string last_saved_correlation_id_;
    std::string last_saved_request_id_;

    std::string last_tx_id_;
    double last_tx_amount_ = 0.0;
    bool last_tx_dry_run_ = false;
    std::vector<std::string> committed_tx_ids_;
};

} // namespace db
