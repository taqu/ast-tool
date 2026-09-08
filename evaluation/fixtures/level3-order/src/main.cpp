#include "gateway/payment_gateway.h"
#include "repository/inventory_repository.h"
#include "repository/order_repository.h"
#include "service/admin_service.h"
#include "service/checkout_service.h"
#include "service/inventory_service.h"
#include "service/notification_service.h"
#include "service/order_service.h"
#include "service/payment_service.h"
#include "web/admin_order_handler.h"
#include "web/mobile_checkout_handler.h"
#include "web/web_checkout_handler.h"
#include "worker/retry_worker.h"

int main() {
    gateway::PaymentGateway gateway;
    repository::OrderRepository order_repo;
    repository::InventoryRepository inv_repo;

    service::AdminService admin_svc;
    service::InventoryService inv_svc(inv_repo);
    service::PaymentService pay_svc(gateway);
    service::OrderService order_svc(inv_svc, pay_svc, order_repo);
    service::NotificationService notif_svc;
    service::CheckoutService checkout_svc(order_svc, notif_svc);

    web::WebCheckoutHandler web_handler(checkout_svc);
    web::MobileCheckoutHandler mobile_handler(checkout_svc);
    web::AdminOrderHandler admin_handler(admin_svc);
    worker::RetryWorker retry_worker(pay_svc);

    web_handler.handle("alice", "item-1");
    mobile_handler.handle("bob", "item-2");
    admin_handler.handle("admin", "item-3", 5);
    retry_worker.retry("order-99", 29.99);

    return 0;
}
