/** include files **/
#include "market.h"       // base header
#include "message.h"       // class InternalMessage 
#include "parsimu.h"      // class Simulator
#include "strutil.h"       // str2Value( ... )
#include <string>
#include <iostream>
using namespace std;

vector<const Port*> Market::inputPorts(int quantity) {
    vector<const Port*> ports(quantity);
    for (int i = 0; i < quantity; i++) {
        ports[i] = &addInputPort("in" + to_string(i));
        cout << "PUERTO: " << "in" + to_string(i) << endl;
    }
    return ports;
}

vector<Port*> Market::outputPorts(int quantity) {
    vector<Port*> ports(quantity);
    for (int i = 0; i < quantity; i++) {
        ports[i] = &addInputPort("out" + to_string(i));
    }
    return ports;
}
/*******************************************************************
* Function Name: Market
* Description: constructor
********************************************************************/
Market::Market( const string &name )
	: Atomic( name ),
      portsQuantity(str2Int( ParallelMainSimulator::Instance().getParameter( description(), "quantity" ) )),
      value(0),
	  in(inputPorts(portsQuantity)),
	  out(outputPorts(portsQuantity)) {
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

/*******************************************************************
* Function Name: externalFunction
* Description: This method executes when an external event is received.
* Remember you can use the msg object (mgs.port(), msg.value()) and you should set the next TA (you might use the holdIn method).
********************************************************************/
Model &Market::externalFunction(const ExternalMessage &msg) {
    this->value = Real::from_value(msg.value());
    for (int i = 0; i < this->portsQuantity; i++) {
        if (msg.port() == *this->in[i]) this->lastInputPort = i;
    }
    this->holdIn(AtomicState::active, VTime::Zero);
    return *this;
}

/*******************************************************************
* Function Name: internalFunction
* Description: This method executes when the TA has expired, right after the outputFunction has finished.
* The new state and TA should be set.
********************************************************************/
Model &Market::internalFunction( const InternalMessage & ) {
	this->passivate();
	return *this;
}

/*******************************************************************
* Function Name: outputFunction
* Description: This method executes when the TA has expired. After this method the internalFunction is called.
* Output values can be send through output ports
********************************************************************/
Model &Market::outputFunction( const CollectMessage &msg ) {
    sendOutput(msg.time(), *this->out[this->lastInputPort], this->value);
	return *this;
}

