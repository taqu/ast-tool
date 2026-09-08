#include "service/inventory_service.h"
#include "service/order_service.h"

int main() {
    store::InventoryService inv;
    store::OrderService orders(inv);

    store::Product p1{1, "Widget", 9.99};
    store::Product p2{2, "Gadget", 19.99};

    inv.save(p1);
    inv.save(p2);

    orders.save(p1);
    orders.update(1, 8.99);
    orders.process(2);

    return 0;
}
