#pragma once
#include <string>
#include "inventory_service.h"
#include "payment_service.h"
#include "notification_service.h"
#include "repository/admin_repository.h"

namespace service {

class AdminOrderService {
public:
    AdminOrderService(InventoryService& inventory,
                      PaymentService& payment,
                      NotificationService& notification,
                      repository::AdminRepository& repo);
    bool submit(const std::string& admin_id,
                const std::string& batch_id,
                int count);
    bool adminCancel(const std::string& order_id, const std::string& reason);

private:
    InventoryService& inventory_;
    PaymentService& payment_;
    NotificationService& notification_;
    repository::AdminRepository& repo_;
};

} // namespace service
