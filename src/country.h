#ifndef __COUNTRY_H
#define __COUNTRY_H

/** include files **/
#include "atomic.h"     // class Atomic
#include "except.h"     // class InvalidMessageException

#define ATOMIC_MODEL_NAME "Country"

/** declarations **/
class Country : public Atomic
{
public:
    Country( const std::string &name = ATOMIC_MODEL_NAME );				  // Default constructor

	virtual std::string className() const
		{return ATOMIC_MODEL_NAME;}

protected:
	Model &initFunction() ;

	Model &externalFunction( const ExternalMessage & );

	Model &internalFunction( const InternalMessage & );

	Model &outputFunction( const CollectMessage & );

private:
	const Port &demand;
	Port &supply;
    Tuple<Real> lastYearExports;

    void updateExports( const Tuple<Real> & );

};	// class Country


#endif   //__COUNTRY_H
