[top]
components : market@Market p0@Product p1@Product

out : out0 out1

link : supply_p0@market out0
link : supply_p1@market out1

link : supply_p0@market supply@p0
link : demand@p0 demand_p0@market

link : supply_p1@market supply@p1
link : demand@p1 demand_p1@market

[market]
productQuantity : 2

[p0]
lastYearDemand : 1000
growthRate : 1

[p1]
lastYearDemand : 1000
growthRate : 1
