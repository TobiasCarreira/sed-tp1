/** include files **/
#include "market.h"       // base header
#include "message.h"       // class InternalMessage 
#include "parsimu.h"      // class Simulator
#include "strutil.h"       // str2Value( ... )
#include <string>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
using namespace std;

vector<const Port*> Market::productDemandPorts(int productQuantity) {
    vector<const Port*> ports(productQuantity);
    for (int i = 0; i < productQuantity; i++) {
        ports[i] = &addInputPort("demand_p" + to_string(i));
    }
    return ports;
}

vector<Port*> Market::productSupplyPorts(int productQuantity) {
    vector<Port*> ports(productQuantity);
    for (int i = 0; i < productQuantity; i++) {
        ports[i] = &addOutputPort("supply_p" + to_string(i));
    }
    return ports;
}

vector<const Port*> Market::countrySupplyPorts(int countryQuantity) {
    vector<const Port*> ports(countryQuantity);
    for (int i = 0; i < countryQuantity; i++) {
        ports[i] = &addInputPort("supply_c" + to_string(i));
    }
    return ports;
}

vector<Port*> Market::countryDemandPorts(int countryQuantity) {
    vector<Port*> ports(countryQuantity);
    for (int i = 0; i < countryQuantity; i++) {
        ports[i] = &addOutputPort("demand_c" + to_string(i));
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
	  countryQuantity(str2Int( ParallelMainSimulator::Instance().getParameter( description(), "countryQuantity" ) )),
	  // TODO: productsIn deberia ser un dict con nombre puerto -> puerto
	  productsIn(productDemandPorts(productQuantity)),

	  // TODO: productsOut deberia ser un dict con nombre puerto -> puerto
	  productsOut(productSupplyPorts(productQuantity)),

	  // TODO: countriesIn deberia ser un dict con nombre puerto -> puerto
	  countriesIn(countrySupplyPorts(countryQuantity)),

	  // TODO: countriesOut deberia ser un dict con nombre puerto -> puerto
	  countriesOut(countryDemandPorts(countryQuantity)),
	  productDemands(productQuantity),
	  exports(countryQuantity),
	  permutationIndeces(countryQuantity) {
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
	srand(time(0));

	for (int i = 0; i < this->countryQuantity; i++) {
		// TODO: sacar estos datos de la "realidad"
		this->exports[i] = vector<Real>(this->productQuantity);
	}

	for (int c = 0; c < this->countryQuantity; c++) permutationIndeces[c] = c;
}

vector<vector<Real>>* Market::getRCAMatrix() {
	vector<Real> exportsCountries(this->countryQuantity, 0);
	vector<Real> exportsProducts(this->productQuantity, 0);

	for (int c = 0; c < this->countryQuantity; c++) {
		for (int p = 0; p < this->productQuantity; p++) {
			Real exported = this->exports[c][p];
			exportsCountries[c] = exportsCountries[c] + exported;
			exportsProducts[p] = exportsProducts[p] + exported;
		}
	}

	Real totalExports = 0;
	for (auto e : exportsCountries) {
		totalExports = totalExports + e;
	}

	vector<vector<Real>> RCA(this->countryQuantity);
	for (int c = 0; c < this->countryQuantity; c++) {
		// TODO: sacar estos datos de la "realidad"
		RCA[c] = vector<Real>(this->productQuantity);
		for (int p = 0; p < this->productQuantity; p++) {
			RCA[c][p] = (this->exports[c][p] / exportsCountries[c]) / (exportsProducts[p] / totalExports);
		}
	}
	return &RCA;
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
	if (msg.port().name().rfind("supply_c", 0) == 0) {
		// procesar lo que vino del pais
		// podriamos mandar directamente la tupla
		this->updateDemandsAfterCountry(msg);
		if (this->demandedToCountriesCount == this->countryQuantity) {
			// termino de asignar los recursos
			this->updateEffectiveExports();
		}
		// comunicamos o nuevos pedidos o lo que se pudo hacer
		this->holdIn(AtomicState::active, VTime::Zero);
	} else {
		// procesar lo que vino del producto
		auto demandedQuantity = Real::from_value(msg.value());
		for (int i = 0; i < this->productQuantity; i++) {
			if (msg.port() == *this->productsIn[i]) {
				this->productDemands[i] = demandedQuantity;
			}
		}
		this->demands += 1;
		if (this->demands == this->productQuantity) {
			this->determineDemandsForCountries();
			this->holdIn(AtomicState::active, VTime::Zero);
		} else {
			this->passivate();
		}
	}
    return *this;
}

void Market::determineDemandsForCountries(){
	random_shuffle(this->permutationIndeces.begin(), this->permutationIndeces.end());

	// asignar las demandas a los paises con el criterio del documento
	this->demandedToCountries = vector<vector<Real>>(this->countryQuantity);
	Real ratio = 1.0 / this->countryQuantity;
	for (int c = 0; c < this->countryQuantity; c++) {
		this->demandedToCountries[c] = vector<Real>(this->productQuantity);
		for (int p = 0; p < this->productQuantity; p++) {
			this->demandedToCountries[c][p] = ratio * this->productDemands[p];
		}
	}
}

void Market::updateDemandsAfterCountry(const ExternalMessage &msg) {
	cout << "HERE " << endl;
	// calcula la diferencia entre lo que se le pidio al pais y lo que va a producir
	// asigna proporcionalmente esa diferenica a los paises que lo tuvieron asignados
	// setea en demandedToCountries el valor a producir
}

void Market::updateEffectiveExports() {
	cout << "FINISHED" << endl;

	// copia de demandedToCountries a exports
	// calcula los nuevos parametros del ProductSpace
}

/*******************************************************************
* Function Name: internalFunction
* Description: This method executes when the TA has expired, right after the outputFunction has finished.
* The new state and TA should be set.
********************************************************************/
Model &Market::internalFunction( const InternalMessage & ) {
	if (this->demands == this->productQuantity && this->demandedToCountriesCount < this->countryQuantity) {
		this->demandedToCountriesCount++;
	} else if (this->demands == this->productQuantity && this->demandedToCountriesCount == this->countryQuantity) {
		this->demandedToCountriesCount = 0;
		this->demands = 0;
	}
	this->passivate();
	return *this;
}

/*******************************************************************
* Function Name: outputFunction
* Description: This method executes when the TA has expired. After this method the internalFunction is called.
* Output values can be send through output ports
********************************************************************/
Model &Market::outputFunction( const CollectMessage &msg ) {
	if (this->demands == this->productQuantity && this->demandedToCountriesCount < this->countryQuantity) {
		int country = this->permutationIndeces[this->demandedToCountriesCount];
		sendOutput(msg.time(), *this->countriesOut[country], Tuple<Real>(&this->demandedToCountries[country]));

	} else if (this->demands == this->productQuantity && this->demandedToCountriesCount == this->countryQuantity) {
		for (int i = 0; i < this->productQuantity; i++) {
    		sendOutput(msg.time(), *this->productsOut[i], this->productDemands[i]);
    	}
	}

	return *this;
}

