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
    this->productQuantity = productQuantity;
    this->lastYearExports = Tuple<Real>(&initialExports);
    this->budgetProportion = 0.01; // TODO: hacer configurable?
}

/*******************************************************************
* Function Name: initFunction
********************************************************************/
Model &Country::initFunction() {
	this->passivate();
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

Real Country::totalExports() {
    Real total = 0;
    for (int i = 0; i < this->productQuantity; i++) total = total + this->lastYearExports[i];
    return total;
}

void Country::updateExports( const Tuple<Real> & demand) {
    // Estrategia conservadora
    // Calculo cuanto extra deberia invertir
    Real extraInvestment = 0;
    Real budget = this->budgetProportion * this->totalExports();
    // TODO: calcular la inversion requerida para producir extra
    vector<Real> requiredInvestment(this->productQuantity, 1);
    for (int i = 0; i < this->productQuantity; i++) {
        // TODO: usar RCA en vez de this->lastYearExports[i] > 0?
        if (this->lastYearExports[i] > 0 && demand[i] > this->lastYearExports[i]) {
            extraInvestment = extraInvestment + (demand[i] - this->lastYearExports[i]) * requiredInvestment[i];
        }
    }
    vector<Real> exports(this->productQuantity);
    for (int i = 0; i < this->productQuantity; i++) {
        if (this->lastYearExports[i] > 0 && demand[i] > this->lastYearExports[i]) {
            if (extraInvestment > budget) {
                // No alcanza el presupuesto para invertir lo que me ofreció el mercado
                exports[i] = this->lastYearExports[i] + (demand[i] - this->lastYearExports[i]) * (budget / extraInvestment);
            } else {
                exports[i] = demand[i];
            }
        } else {
            exports[i] = min(this->lastYearExports[i], demand[i]);
        }
    }
    this->lastYearExports = Tuple<Real>(&exports);
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

