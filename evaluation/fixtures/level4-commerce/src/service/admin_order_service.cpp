#include "admin_order_service.h"
#include <iostream>

namespace service {

AdminOrderService::AdminOrderService(InventoryService& inventory,
                                     PaymentService& payment,
                                     NotificationService& notification,
                                     repository::AdminRepository& repo)
    : inventory_(inventory), payment_(payment),
      notification_(notification), repo_(repo) {}

bool AdminOrderService::submit(const std::string& admin_id,
                               const std::string& batch_id,
                               int count) {
    std::cout << "admin-order: submitting batch=" << batch_id << "\n";
    if (!inventory_.reserve(batch_id, count)) return false;
    if (!payment_.authorize(admin_id, count * 5.0)) return false;
    repo_.bulkSave(batch_id, count);
    notification_.notify(admin_id, "batch submitted: " + batch_id);
    return true;
}

bool AdminOrderService::adminCancel(const std::string& order_id,
                                    const std::string& reason) {
    std::cout << "admin-order: cancelling order=" << order_id << "\n";
    repo_.adminUpdate(order_id, reason);
    return true;
}

} // namespace service
