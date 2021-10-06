#include "pmodeladm.h"
#include "register.h"

#include "product.h"
#include "market.h"
#include "country.h"


void register_atomics_on(ParallelModelAdmin &admin)
{
    admin.registerAtomic(NewAtomicFunction<Product>(), "Product");
    admin.registerAtomic(NewAtomicFunction<Market>(), "Market");
    admin.registerAtomic(NewAtomicFunction<Country>(), "Country");
}
