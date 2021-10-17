import csv
import pandas as pd
from jinja2 import Environment, FileSystemLoader, select_autoescape

normalization = 1_000_000.0

with open("products.csv") as f:
    reader = csv.DictReader(f)
    products = [{"initialVolume": float(r["initialVolume"]) / normalization, "b": float(r["b"]) / normalization, "label": r["label"]} for r in reader]

gdps_df = pd.read_csv("countries.csv")


def countries_data(strategy):
    countries = []
    for _, row in gdps_df.iterrows():
        countries.append({
            "initialGDP": float(row["1995"]) / normalization,
            "gdpOverExports": row["gdp_over_exports"],
            "productExports": [{"label": i, "export": float(row[p["label"]]) / normalization} for i, p in enumerate(products)],
            "iso3": row["location_code"],
            "strategy": strategy,
        })
    return countries

env = Environment(
    loader=FileSystemLoader("."),
    autoescape=select_autoescape(),
)
template = env.get_template("simulation.ma.j2")

with open("simulationConservative.ma", "w") as f:
    f.write(template.render(products=products, countries=countries_data(1)))
with open("simulationEgalitarian.ma", "w") as f:
    f.write(template.render(products=products, countries=countries_data(2)))
