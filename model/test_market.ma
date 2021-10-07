[top]
components : market@Market country@Country p0@Product p1@Product

out : supply_p0 supply_p1

link : supply_p0@market supply_p0
link : supply_p1@market supply_p1

link : supply_p0@market supply@p0
link : demand@p0 demand_p0@market

link : supply_p1@market supply@p1
link : demand@p1 demand_p1@market

link : demand_c0@market demand@country
link : supply@country supply_c0@market

[market]
productQuantity : 2
countryQuantity : 1

[p0]
initialVolume : 1000
growthRate : 1

[p1]
initialVolume : 1000
growthRate : 1

[country]
productQuantity : 2
initialExports_p0 : 1800
initialExports_p1 : 3100