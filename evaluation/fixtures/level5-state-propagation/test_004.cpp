#include <cassert>
#include <iostream>
#include "service/user_service.h"
#include "service/transaction_manager.h"
#include "db/database_client.h"
#include "model/transaction.h"

int main() {
    db::DatabaseClient db;
    service::TransactionManager tx_manager(db);
    service::UserService user_service(tx_manager);

    model::Transaction tx;
    tx.id = "tx_789";
    tx.amount = 150.0;

    // Run transaction as dry_run = true
    user_service.process_transaction("usr_123", tx, true);

    // Verify dry_run is true
    if (!db.last_tx_dry_run_) {
        std::cerr << "FAIL: dry_run flag not propagated to DB" << std::endl;
        return 1;
    }

    // Verify it was NOT committed
    for (const auto& id : db.committed_tx_ids_) {
        if (id == "tx_789") {
            std::cerr << "FAIL: Dry run transaction committed to DB" << std::endl;
            return 1;
        }
    }

    std::cout << "SUCCESS" << std::endl;
    return 0;
}
