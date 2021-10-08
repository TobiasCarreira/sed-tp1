#ifndef __MARKET_H
#define __MARKET_H

/** include files **/
#include "atomic.h"     // class Atomic
#include "except.h"     // class InvalidMessageException
#include <vector>

#define ATOMIC_MODEL_NAME "Market"

/** declarations **/
class Market : public Atomic
{
public:
	Market( const std::string &name = ATOMIC_MODEL_NAME );				  // Default constructor

	virtual std::string className() const
		{return ATOMIC_MODEL_NAME;}

protected:
	Model &initFunction() ;

	Model &externalFunction( const ExternalMessage & );

	Model &internalFunction( const InternalMessage & );

	Model &outputFunction( const CollectMessage & );

private:
    const int productQuantity;
	const int countryQuantity;

	const vector<const Port*> productsIn;
	const vector<Port*> productsOut;

	const vector<const Port*> countriesIn;
	const vector<Port*> countriesOut;

	int demands = 0;
	int demandedToCountriesCount = 0;

	vector<Real> productDemands;
	vector<vector<Real>> demandedToCountries;

	vector<vector<Real>> exports;

	vector<int> permutationIndeces;

    vector<const Port*> productDemandPorts(int productQuantity);
    vector<Port*> productSupplyPorts(int productQuantity);

	vector<Port*> countryDemandPorts(int countryQuantity);
	vector<const Port*> countrySupplyPorts(int countryQuantity);

	vector<vector<Real>> *getRCAMatrix();

	void determineDemandsForCountries();
	void updateEffectiveExports();
	void updateDemandsAfterCountry(const Tuple<Real> &);

};	// class ConstGenerator


#endif   //__MARKET_H
