#ifndef __PRODUCT_H
#define __PRODUCT_H

/** include files **/
#include "atomic.h"     // class Atomic
#include "except.h"     // class InvalidMessageException

#define ATOMIC_MODEL_NAME "Product"

/** declarations **/
class Product : public Atomic
{
public:
	Product( const std::string &name = ATOMIC_MODEL_NAME );				  // Default constructor

	virtual std::string className() const
		{return ATOMIC_MODEL_NAME;}

protected:
	Model &initFunction() ;

	Model &externalFunction( const ExternalMessage & );

	Model &internalFunction( const InternalMessage & );

	Model &outputFunction( const CollectMessage & );

private:
	const Port &supply;
	Port &demand;
	Real initialVolume;
    Real growthRate;

	Real lastVolume;

};	// class ConstGenerator


#endif   //__PRODUCT_H
