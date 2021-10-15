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
      supply(addOutputPort("supply")),
      gdpPort(addOutputPort("gdp")) {
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
    this->strategy = str2Int( simulator.getParameter( description(), "strategy" ) );
    this->gdpOverExports = str2Real( simulator.getParameter( description(), "gdpOverExports" ) );
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
    switch (this->strategy) {
        case 1:
            this->conservativeStrategy(demand);
            break;
        case 2:
            this->egalitarianStrategy(demand);
    }
}

vector<Real> Country::affinityWithProducts() {
    // TODO: deberia hacerlo el mercado y dejarlo global?
    vector<Real> affinity(this->productQuantity);
    for (int p = 0; p < this->productQuantity; p++) {
        // TODO: Cambiar por RCA > 1
        if (this->lastYearExports[p] > 0) {
            affinity[p] = 1.0;
        } else {
            // Si no exporto el producto, tomo la mejor afinidad con los productos que si exporto
            affinity[p] = 0.0;
            for (int p2 = 0; p2 < this->productQuantity; p2++) {
                if (this->lastYearExports[p2] > 0)
                    affinity[p] = max(affinity[p], productsAffinity[p][p2]);
            }
        }
    }
    return affinity;
}

vector<Real> Country::requiredInvestmentForProducts() {
    vector<Real> affinities = affinityWithProducts();

    vector<Real> requiredInvestment(this->productQuantity);
    for (int p = 0; p < this->productQuantity; p++) {
        requiredInvestment[p] = (Real::one - affinities[p]) + (Real::one - PGIs[p]);
        MASSERTMSG( requiredInvestment[p] >= 0, string("La inversion requerida no puede ser negativa") );
    }
    return requiredInvestment;
}

Real Country::gdp() {
    return this->gdpOverExports * this->totalExports();
}

Real Country::budget() {
    return this->budgetProportion * this->gdp();
}

void Country::egalitarianStrategy( const Tuple<Real> & demand) {
    vector<Real> requiredInvestment = requiredInvestmentForProducts();

    vector<pair<double, int>> equalityPerInvestment(this->productQuantity);
    for (int i = 0; i < this->productQuantity; i++) {
        // Mayor PGI es mayor desigualdad, asi que igualdad = 1/pgi
        // Luego, la igualdad por inversion es igualdad/inversion
        equalityPerInvestment[i] = make_pair((Real::one / (PGIs[i] * requiredInvestment[i])).value(), i);
    }
    // Ordeno los productos por cuan eficientes son para "aumentar la igualdad"
    sort(equalityPerInvestment.begin(), equalityPerInvestment.end(), std::greater<pair<double, int>>());

    vector<Real> exports(this->productQuantity);
    Real remainingBudget = this->budget();
    for (int i = 0; i < this->productQuantity; i++) {
        // Busco primero aumentar las exportaciones de los productos con menor pgi por inversion requerida
        int product = equalityPerInvestment[i].second;
        Real diff = demand[product] - this->lastYearExports[product];
        if (remainingBudget > 0 && diff > 0) {
            Real diffRequiredInvestment = diff * requiredInvestment[product];
            if (diffRequiredInvestment < remainingBudget) {
                // Si me alcanza el presupuesto, exporto el extra que me pidieron
                exports[product] = demand[product];
                remainingBudget = remainingBudget - diffRequiredInvestment;
            } else {
                // Sino, exporto lo que llego con el presupuesto
                exports[product] = this->lastYearExports[product] + remainingBudget/requiredInvestment[product];
                MASSERTMSG( exports[product] <= demand[product],
                            string("Export: ") + exports[product].asString() + ", demand: " +  demand[product].asString() + "\n" +
                            "lastYear: " + this->lastYearExports[product].asString() + ", rem budget: " +  remainingBudget.asString() + "\n" +
                            "diff: " + diff.asString() + ", req: " +  requiredInvestment[product].asString() + "\n" +
                            "diffReq: " + diffRequiredInvestment.asString());
                remainingBudget = 0;
            }
        } else {
            exports[product] = min(this->lastYearExports[product], demand[product]);
        }
        MASSERTMSG( exports[product] >= 0, string("La exportacion no puede ser negativa") );
        MASSERTMSG( exports[product] <= demand[product], string("La exportacion no puede ser mayor a lo demandado") );
    }
    // TODO: update GDP
    this->lastYearExports = Tuple<Real>(&exports);
}

void Country::conservativeStrategy( const Tuple<Real> & demand) {
    // Estrategia conservadora
    // Calculo cuanto extra deberia invertir
    Real extraInvestment = 0;
    Real totalBudget = this->budget();
    MASSERTMSG( totalBudget >= 0, string("El presupuesto no puede ser negativo") );

    vector<Real> requiredInvestment = this->requiredInvestmentForProducts();
    for (int i = 0; i < this->productQuantity; i++) {
        MASSERTMSG( demand[i] >= 0, string("La demanda no puede ser negativa") );
        if (this->lastYearExports[i] > 0 && demand[i] > this->lastYearExports[i]) {
            extraInvestment = extraInvestment + (demand[i] - this->lastYearExports[i]) * requiredInvestment[i];
        }
    }
    vector<Real> exports(this->productQuantity);
    for (int i = 0; i < this->productQuantity; i++) {
        if (this->lastYearExports[i] > 0 && demand[i] > this->lastYearExports[i]) {
            if (extraInvestment > totalBudget) {
                MASSERTMSG( demand[i] - this->lastYearExports[i] > 0, string("El incremento anual no puede ser negativo") );
                MASSERTMSG( totalBudget / extraInvestment > 0, string("El porcentaje de inversion no puede ser negativo") );
                // No alcanza el presupuesto para invertir lo que me ofreció el mercado
                exports[i] = this->lastYearExports[i] + (demand[i] - this->lastYearExports[i]) * (totalBudget / extraInvestment);
            } else {
                exports[i] = demand[i];
            }
        } else {
            exports[i] = min(this->lastYearExports[i], demand[i]);
        }
        MASSERTMSG( exports[i] >= 0, string("La exportacion no puede ser negativa") );
        MASSERTMSG( exports[i] <= demand[i], string("La exportacion no puede ser mayor a lo demandado") );
    }

    // TODO: update GDP
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
    sendOutput(msg.time(), this->gdpPort, this->gdp());
	return *this;
}

