# Heuristics
![Heuristics Logo](https://raw.githubusercontent.com/migcubemighull/heuristics/master/Logo.png)

Two novel heuristics for process migration in BSP applications

This git page allows to anyone to use the MigCube and MigHull heuristics presented on the paper "USING COMPUTATIONAL GEOMETRY TO IMPROVE PROCESS RESCHEDULING IN ROUND-BASED PARALLEL APPLICATIONS" (Referece pending).

# MigCube
The following image present the ideia of MigCube:

![MigCube Structure](https://raw.githubusercontent.com/migcubemighull/heuristics/master/imageMigCube.png)

Processes and cube representation in MigCube heuristic. Those processes that are located inside the cube will be appointed as candidate processes for migration.

The Algorithm presents the pseudocode of MigCube heuristic for selecting the candidate process for migration.

![MigCube Algorithm](https://raw.githubusercontent.com/migcubemighull/heuristics/master/algo_MigCube.png)

# MigHull
The following image present the ideia of MigHull:

![MigHull Structure](https://raw.githubusercontent.com/migcubemighull/heuristics/master/imageMigHull.png)

Selection of candidate process for migration with MigHull: (a) Creating three plans (x-y, x-z and y-z) from the three-dimensional space; (b) partially selecting the candidate process in the x-y plan. Those processes that appear comcommitantly in the three plans are finally chosen for the next rescheduling step, the tests of migration viability.

The Algorithm presents the pseudocode of MigHull heuristic for selecting the candidate process for migration.

![MigHull Algorithm](https://raw.githubusercontent.com/migcubemighull/heuristics/master/algo_MigHull1.png)

The Algorithm 2 is the "distance" equation used in the Algorithm 3 below.

![MigHull Algorithm](https://raw.githubusercontent.com/migcubemighull/heuristics/master/algo_MigHull2.png)

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
