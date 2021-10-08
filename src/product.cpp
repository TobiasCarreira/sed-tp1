/** include files **/
#include "product.h"       // base header
#include "message.h"       // class InternalMessage 
#include "parsimu.h"      // class Simulator
#include "strutil.h"       // str2Value( ... )
#include "utils.h"

using namespace std;

/*******************************************************************
* Function Name: Product
* Description: constructor
********************************************************************/
Product::Product( const string &name )
	: Atomic( name ),
	  supply(addInputPort("supply")),
	  demand(addOutputPort("demand")) {
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
    this->initialVolume = str2Real( ParallelMainSimulator::Instance().getParameter( description(), "initialVolume" ) );
    this->growthRate = str2Real( ParallelMainSimulator::Instance().getParameter( description(), "growthRate" ) ) ;
	this->lastVolume = this->initialVolume;
}

/*******************************************************************
* Function Name: initFunction
********************************************************************/
Model &Product::initFunction() {
	holdIn(AtomicState::active, VTime::Zero);
	return *this ;
}

/*******************************************************************
* Function Name: externalFunction
* Description: This method executes when an external event is received.
* Remember you can use the msg object (mgs.port(), msg.value()) and you should set the next TA (you might use the holdIn method).
********************************************************************/
Model &Product::externalFunction(const ExternalMessage &msg) {
    if(msg.port() == this->supply) {
		// cuidado con el overflow
        this->lastVolume = Real::from_value(msg.value()) + this->growthRate;
        holdIn(AtomicState::active, CYCLE_TIME) ;
    }

    return *this;
}

/*******************************************************************
* Function Name: internalFunction
* Description: This method executes when the TA has expired, right after the outputFunction has finished.
* The new state and TA should be set.
********************************************************************/
Model &Product::internalFunction( const InternalMessage & ) {
	holdIn(AtomicState::active, CYCLE_TIME) ;
	return *this;
}

/*******************************************************************
* Function Name: outputFunction
* Description: This method executes when the TA has expired. After this method the internalFunction is called.
* Output values can be send through output ports
********************************************************************/
Model &Product::outputFunction( const CollectMessage &msg ) {
	sendOutput(msg.time(), this->demand, this->lastVolume);
	return *this;
}

