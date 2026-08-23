#pragma once
#include "model/transaction.h"
#include "db/database_client.h"

namespace service {

class TransactionManager {
public:
    explicit TransactionManager(db::DatabaseClient& db) : db_(db) {}
    void execute(const model::Transaction& tx, const model::TransactionOptions& options);

private:
    db::DatabaseClient& db_;
};

} // namespace service
