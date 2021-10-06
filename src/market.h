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
    const int portsQuantity;
    Real value;
    int lastInputPort;
	vector<const Port*> in;
	vector<Port*> out;

    vector<const Port*> inputPorts(int quantity);
    vector<Port*> outputPorts(int quantity);
			
};	// class ConstGenerator


#endif   //__MARKET_H
