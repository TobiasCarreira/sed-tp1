import csv
from jinja2 import Environment, FileSystemLoader, select_autoescape

products = [{"initialVolume": 1000, "b": 1}, {"initialVolume": 2000, "b": 3}]
with open("../datos_productos.csv") as f:
    reader = csv.DictReader(f)
    products = sorted([(r["producto_code"], {"initialVolume": r["initialVolume"], "b": r["b"]}) for r in reader], key=lambda x: x[0])

products = [x for _, x in products]

countries = [{"productExports": [1] * len(products)}]


env = Environment(
    loader=FileSystemLoader("."),
    autoescape=select_autoescape(),
)
template = env.get_template("simulation.ma.j2")

with open("simulation.ma", "w") as f:
    f.write(template.render(products=products, countries=countries))