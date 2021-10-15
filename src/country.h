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
    Port &gdpPort;
    Tuple<Real> lastYearExports;
    int productQuantity;
    Real budgetProportion;
    int strategy;
    Real gdpOverExports;

    void updateExports( const Tuple<Real> & );
    void conservativeStrategy( const Tuple<Real> & );
    void egalitarianStrategy( const Tuple<Real> & );
    Real totalExports();
    vector<Real> requiredInvestmentForProducts();
    vector<Real> affinityWithProducts();
    Real budget();
    Real gdp();

};	// class Country


#endif   //__COUNTRY_H
