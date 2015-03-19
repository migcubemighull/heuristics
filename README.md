# Heuristics
Two novel heuristics for process migration in BSP applications

This git page allows to anyone to use the MigCube and MigHull heuristics presented on the paper "USING COMPUTATIONAL GEOMETRY TO IMPROVE PROCESS RESCHEDULING IN ROUND-BASED PARALLEL APPLICATIONS" (Referece pending).

# How to Use
First you should install the SimGrid with MSG module. Reference: http://simgrid.gforge.inria.fr/simgrid/latest/doc/install.html

Is also necessary the c libraries

# Running
Unsing the following commands:

make app1-s1 for scenario with application without heuristics and migrations

make app1-s2 for scenario with application with heuristics but without migrations

make app1-s2 for scenario with application with heuristics and with migrations

To run the application use:

./migbsp platform_file.xml deployment_file.xml

Webpage: http://migcubemighull.github.io/heuristics
![Heuristics Logo](https://raw.githubusercontent.com/migcubemighull/heuristics/master/Logo.png)
![MigCube Structure](https://raw.githubusercontent.com/migcubemighull/heuristics/master/imageMigHull.png)
![MigHull Structure](https://raw.githubusercontent.com/migcubemighull/heuristics/master/imageMigCube.png)
![MigCube Algorithm](https://raw.githubusercontent.com/migcubemighull/heuristics/master/algo_MigCube.png)
![MigHull Algorithm](https://raw.githubusercontent.com/migcubemighull/heuristics/master/algo_MigHull1.png)
![MigHull Algorithm](https://raw.githubusercontent.com/migcubemighull/heuristics/master/algo_MigHull2.png)
