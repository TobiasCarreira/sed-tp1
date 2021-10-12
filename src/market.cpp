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
#include "utils.h"
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

	srand(time(0));

	for (int i = 0; i < this->countryQuantity; i++) {
		// TODO: sacar estos datos del PS 
		this->exports[i] = vector<Real>(this->productQuantity);
	}

	// TODO: sacar estos datos del PS 
    productsAffinity = vector<vector<Real> >(this->productQuantity);
	for (int p = 0; p < this->productQuantity; p++) productsAffinity[p] = vector<Real>(this->productQuantity, 0.5);
    PGIs = vector<Real>(this->productQuantity, 0.5);

	for (int c = 0; c < this->countryQuantity; c++) permutationIndeces[c] = c;
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
	if (msg.port().name().rfind("supply_c", 0) == 0) {
		// procesar lo que vino del pais
		this->updateDemandsAfterCountry(Tuple<Real>::from_value(msg.value()));
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

#define PRODUCT_COUNTRY_FIDELITY Real(0.8)
#define NEW_PRODUCT_BAG_SIZE  5

void Market::determineDemandsForCountries() {
	random_shuffle(this->permutationIndeces.begin(), this->permutationIndeces.end());

	// asignar las demandas a los paises con el criterio del documento
	this->demandedToCountries = vector<vector<Real>>(this->countryQuantity);
	Real ratio = 1.0 / this->countryQuantity;
	for (int c = 0; c < this->countryQuantity; c++)
		this->demandedToCountries[c] = vector<Real>(this->productQuantity, 0);

	for (int p = 0; p < this->productQuantity; p++) {
		// primero respetamos la fidelidad pidiendo el 80% de los bienes a los paises que ya los exportan (sosten de produccion)
		Real total = 0;
		vector<int> exporters, notExporters;
		for (int c = 0; c < this->countryQuantity; c++) {
			total = total + this->exports[c][p];
			if (this->exports[c][p] > 0)
				exporters.push_back(c);
			else
				notExporters.push_back(c);
		}
		for (const int c : exporters)
			this->demandedToCountries[c][p] = PRODUCT_COUNTRY_FIDELITY * (this->exports[c][p] / total) * this->productDemands[p];

		this->productDemands[p] = this->productDemands[p] * (Real(1) - PRODUCT_COUNTRY_FIDELITY);

		// el 10% de los bienes los asignamos equitativamente a aquellos que todavia no los exportan (nueva produccion)
		random_shuffle(notExporters.begin(), notExporters.end());
		const int bagSize = min(NEW_PRODUCT_BAG_SIZE, (int) notExporters.size());
		for (int i = 0; i < bagSize; i++) {
			const int c = notExporters[i];
			this->demandedToCountries[c][p] = this->productDemands[p] * 0.5 * (1 / bagSize);
		}
		this->productDemands[p] = this->productDemands[p] * Real(0.5);

		// el restante 10% se distribuye de manera equitativa en quienes si lo exportan (aumento de la produccion)
		for (const int c : exporters)
			this->demandedToCountries[c][p] = this->demandedToCountries[c][p] + (this->productDemands[p] / exporters.size());
	}
}

void Market::updateDemandsAfterCountry(const Tuple<Real> &supplied) {
	// calcula la diferencia entre lo que se le pidio al pais y lo que va a producir
	const int country = this->permutationIndeces[this->demandedToCountriesCount];
	vector<Real> diff(this->demandedToCountries[country]);
	for (int p = 0; p < supplied.size(); p++) {
		// actualiza exports
		this->exports[country][p] = supplied[p];
		diff[p] = diff[p] - supplied[p];
	}
	// asigna proporcionalmente esa diferenica a los paises que lo tuvieron asignados
	for (int p = 0; p < supplied.size(); p++) {
		vector<int> potentialExporters;
		for (int i = this->demandedToCountriesCount + 1; i < this->countryQuantity; i++) {
			const int c = this->permutationIndeces[i];
			if (this->demandedToCountries[c][p] > 0)
				potentialExporters.push_back(c);
		}

		// divide sobre los exportadores
		for (const int c : potentialExporters)
			this->demandedToCountries[c][p] = this->demandedToCountries[c][p] + (diff[p] / potentialExporters.size());
	}
}

void Market::updateEffectiveExports() {
	// calcula los nuevos parametros del ProductSpace
	vector<double> exportsCountries(this->countryQuantity, 0);
	vector<double> exportsProducts(this->productQuantity, 0);

	for (int c = 0; c < this->countryQuantity; c++) {
		for (int p = 0; p < this->productQuantity; p++) {
			double exported = this->exports[c][p].value();
			exportsCountries[c] += exported;
			exportsProducts[p] += exported;
		}
	}

	double totalExports = 0;
	for (auto e : exportsCountries)
		totalExports += e;

	vector<vector<double>* > *RCA = new vector<vector<double>* >(this->countryQuantity);
	for (int c = 0; c < this->countryQuantity; c++) {
		// TODO: sacar estos datos de la "realidad"
		(*RCA)[c] = new vector<double>(this->productQuantity);
		for (int p = 0; p < this->productQuantity; p++) {
			(*(*RCA)[c])[p] = (this->exports[c][p].value() / exportsCountries[c]) / (exportsProducts[p] / totalExports);
		}
	}

	vector<double> productsPRCA(this->productQuantity);
	for (int p = 0; p < this->productQuantity; p++) {
		int count = 0;
		for (int c = 0; c < this->countryQuantity; c++)
			count += (*(*RCA)[c])[p] >= 1;
		productsPRCA[p] = ((double)count) / this->countryQuantity;
	}

	for (int i = 0; i < this->productQuantity; i++) {
		for (int j = 0; j < this->productQuantity; j++) {
			const double preProximity = productsPRCA[i] / productsPRCA[j];
			productsAffinity[i][j] = Real(min(preProximity, 1 / preProximity));
		}
	}

	for (int c = 0; c < this->countryQuantity; c++)
		delete (*RCA)[c];
	delete RCA;
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

