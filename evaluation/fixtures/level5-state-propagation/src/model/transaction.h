#pragma once
#include <string>

namespace model {

struct Transaction {
    std::string id;
    double amount;
};

struct TransactionOptions {
    bool dry_run = false;
    bool bypass_cache = false;

    static TransactionOptions default_options() {
        return TransactionOptions{false, false};
    }
};

} // namespace model
