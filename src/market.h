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
	const int countryQuantity = 1;
	vector<Real> productDemands;

	vector<vector<Real>> exports;

	vector<const Port*> productsIn;
	vector<Port*> productsOut;

	vector<const Port*> countriesIn;
	vector<Port*> countriesOut;

	int demands = 0;
	int demanded = 0;

	vector<vector<Real>> demandedToCountries;
	vector<int> permutationIndeces;

    vector<const Port*> inputPorts(int productQuantity);
    vector<Port*> outputPorts(int productQuantity);


	vector<vector<Real>> *getRCAMatrix();
	void determineDemandsForCountries();
	void updateEffectiveExports();
	void updateDemandsAfterCountry(const ExternalMessage &msg);

};	// class ConstGenerator


#endif   //__MARKET_H
