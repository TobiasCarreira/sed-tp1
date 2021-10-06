#include "pmodeladm.h"
#include "register.h"

#include "product.h"
#include "market.h"


void register_atomics_on(ParallelModelAdmin &admin)
{
    admin.registerAtomic(NewAtomicFunction<Product>(), "Product");
    admin.registerAtomic(NewAtomicFunction<Market>(), "Market");
}
