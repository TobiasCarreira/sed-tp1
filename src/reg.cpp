#include "pmodeladm.h"
#include "register.h"

#include "product.h"


void register_atomics_on(ParallelModelAdmin &admin)
{
    admin.registerAtomic(NewAtomicFunction<Product>(), ATOMIC_MODEL_NAME);
}
