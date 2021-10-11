/** include files **/
#include "country.h"       // base header
#include "message.h"       // class InternalMessage
#include "parsimu.h"      // class Simulator
#include "strutil.h"       // str2Value( ... )
#include "utils.h"
#import "tuple_value.h"
#include <iostream>
#include <utility>
#include <algorithm>

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

void Country::updateExports(const Tuple<Real> & demand) {
    this->conservativeStrategy(demand);
}

void Country::egalitarianStrategy( const Tuple<Real> & demand) {
    Real budget = this->budgetProportion * this->totalExports();
    // TODO: calcular la inversion requerida para producir extra
    vector<Real> requiredInvestment(this->productQuantity, 1);

    vector<pair<double, int>> equalityPerInvestment(this->productQuantity);
    for (int i = 0; i < this->productQuantity; i++) {
        // Mayor PGI es mayor desigualdad, asi que igualdad = 1/pgi
        // Luego, la igualdad por inversion es igualdad/inversion
        equalityPerInvestment[i] = make_pair((Real::one / (PGIs[i] * requiredInvestment[i])).value(), i);
    }
    // Ordeno los productos por cuan eficientes son para "aumentar la igualdad"
    sort(equalityPerInvestment.begin(), equalityPerInvestment.end(), std::greater<pair<double, int>>());

    vector<Real> exports(this->productQuantity);
    for (int i = 0; i < this->productQuantity; i++) {
        // Busco primero aumentar las exportaciones de los productos con menor pgi por inversion requerida
        int product = equalityPerInvestment[i].second;
        Real diff = demand[i] - this->lastYearExports[i];
        if (budget > 0 && diff > 0) {
            Real diffRequiredInvestment = diff * requiredInvestment[product];
            if (diffRequiredInvestment < budget) {
                // Si me alcanza el presupuesto, exporto el extra que me pidieron
                exports[product] = demand[product];
                budget = budget - diffRequiredInvestment;
            } else {
                // Sino, exporto lo que llego con el presupuesto
                exports[product] = this->lastYearExports[product] + budget/requiredInvestment[product];
                budget = 0;
            }
        } else {
            exports[product] = min(this->lastYearExports[product], demand[product]);
        }
    }
    this->lastYearExports = Tuple<Real>(&exports);
}

void Country::conservativeStrategy( const Tuple<Real> & demand) {
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

