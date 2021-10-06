/** include files **/
#include "market.h"       // base header
#include "message.h"       // class InternalMessage 
#include "parsimu.h"      // class Simulator
#include "strutil.h"       // str2Value( ... )
#include <string>
#include <iostream>
using namespace std;

vector<const Port*> Market::inputPorts(int productQuantity) {
    vector<const Port*> ports(productQuantity);
    for (int i = 0; i < productQuantity; i++) {
        ports[i] = &addInputPort("demand_p" + to_string(i));
    }
    return ports;
}

vector<Port*> Market::outputPorts(int productQuantity) {
    vector<Port*> ports(productQuantity);
    for (int i = 0; i < productQuantity; i++) {
        ports[i] = &addInputPort("supply_p" + to_string(i));
    }
    return ports;
}
/*******************************************************************
* Function Name: Market
* Description: constructor
********************************************************************/
Market::Market( const string &name )
	: Atomic( name ),
      productQuantity(str2Int( ParallelMainSimulator::Instance().getParameter( description(), "productQuantity" ) )),
      value(0),
	  // TODO: productsIn deberia ser un dict con nombre puerto -> puerto
	  productsIn(inputPorts(productQuantity)),

	  // TODO: productsOut deberia ser un dict con nombre puerto -> puerto
	  productsOut(outputPorts(productQuantity)),
	  productDemands(productQuantity) {
	// add initialization code here. (reading parameters, initializing private vars, etc)
	// Code templates for reading parameters:
	// read string parameter:
	// 		stringVar = ParallelMainSimulator::Instance().getParameter( description(), "paramName" );
	// read int parameter:
	// 		intVar = str2Int( ParallelMainSimulator::Instance().getParameter( description(), "initial" ) );
	// read time parameter:
	//		timeVar = string time( ParallelMainSimulator::Instance().getParameter( description(), "preparation" ) ) ;
	// read distribution parameters:
	//		dist = Distribution::create( ParallelMainSimulator::Instance().getParameter( description(), "distribution" ) );
	//		MASSERT( dist ) ;
	//		for ( register int i = 0; i < dist->varCount(); i++ )
	//		{
	//			string parameter( ParallelMainSimulator::Instance().getParameter( description(), dist->getVar( i ) ) ) ;
	//			dist->setVar( i, str2Value( parameter ) ) ;
	//		}
}

/*******************************************************************
* Function Name: initFunction
********************************************************************/
Model &Market::initFunction() {
	this->passivate();
	return *this ;
}

void calculoOfTheShit() {}

/*******************************************************************
* Function Name: externalFunction
* Description: This method executes when an external event is received.
* Remember you can use the msg object (mgs.port(), msg.value()) and you should set the next TA (you might use the holdIn method).
********************************************************************/
Model &Market::externalFunction(const ExternalMessage &msg) {
    auto demandedQuantity = Real::from_value(msg.value());
	if (this->lastChange() < msg.time()) {
		cout << "nuevo pedido" << endl;
	}
    for (int i = 0; i < this->productQuantity; i++) {
        if (msg.port() == *this->productsIn[i]) {
			this->productDemands[i] = demandedQuantity;
		}
    }
	this->demands += 1;
	if (this->demands == this->productQuantity) {
		calculoOfTheShit();
    	this->holdIn(AtomicState::active, VTime::Zero);
	} else {
		this->passivate();
	}
    return *this;
}

/*******************************************************************
* Function Name: internalFunction
* Description: This method executes when the TA has expired, right after the outputFunction has finished.
* The new state and TA should be set.
********************************************************************/
Model &Market::internalFunction( const InternalMessage & ) {
	this->demands = 0;
	this->passivate();
	return *this;
}

/*******************************************************************
* Function Name: outputFunction
* Description: This method executes when the TA has expired. After this method the internalFunction is called.
* Output values can be send through output ports
********************************************************************/
Model &Market::outputFunction( const CollectMessage &msg ) {
	for (int i = 0; i < this->productQuantity; i++) {
    	sendOutput(msg.time(), *this->productsOut[i], this->productDemands[i]);
    }
	return *this;
}

