/** include files **/
#include "country.h"       // base header
#include "message.h"       // class InternalMessage 
#include "parsimu.h"      // class Simulator
#include "strutil.h"       // str2Value( ... )
#include "utils.h"
#import "tuple_value.h"
#include <iostream>

using namespace std;

/*******************************************************************
* Function Name: Country
* Description: constructor
********************************************************************/
Country::Country( const string &name )
	: Atomic( name ),
      demand(addInputPort("demand")),
	  supply(addOutputPort("supply")) {
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
    const ParallelMainSimulator& simulator = ParallelMainSimulator::Instance();
    int productQuantity = str2Int( simulator.getParameter( description(), "productQuantity" ) );
    vector<Real> initialExports(productQuantity);
    for (int i = 0; i < productQuantity; i++) {
        initialExports[i] = str2Real( simulator.getParameter( description(), "initialExports_p" + to_string(i) ) );
    }
    this->lastYearExports = Tuple<Real>(&initialExports);
}

/*******************************************************************
* Function Name: initFunction
********************************************************************/
Model &Country::initFunction() {
	holdIn(AtomicState::active, VTime::Zero);
	return *this ;
}

/*******************************************************************
* Function Name: externalFunction
* Description: This method executes when an external event is received.
* Remember you can use the msg object (mgs.port(), msg.value()) and you should set the next TA (you might use the holdIn method).
********************************************************************/
Model &Country::externalFunction(const ExternalMessage &msg) {
    if(msg.port() == this->demand) {
        this->updateExports(Tuple<Real>::from_value(msg.value()));
        holdIn(AtomicState::active, VTime::Zero) ;
    }

    return *this;
}

void Country::updateExports( const Tuple<Real> & demand) {
    // TODO: agregar estrategia con respecto a lo exportado el año pasado y los limites de crecimiento
    this->lastYearExports = demand;
}
/*******************************************************************
* Function Name: internalFunction
* Description: This method executes when the TA has expired, right after the outputFunction has finished.
* The new state and TA should be set.
********************************************************************/
Model &Country::internalFunction( const InternalMessage & ) {
	this->passivate();
	return *this;
}

/*******************************************************************
* Function Name: outputFunction
* Description: This method executes when the TA has expired. After this method the internalFunction is called.
* Output values can be send through output ports
********************************************************************/
Model &Country::outputFunction( const CollectMessage &msg ) {
	sendOutput(msg.time(), this->supply, this->lastYearExports);
	return *this;
}

