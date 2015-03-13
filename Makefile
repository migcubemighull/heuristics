app1-s1:
	gcc -DAPP1 -DS1 migbsp.c -Wall -o migbsp -I /opt/simgrid/include/ -L /opt/simgrid/lib/ -l simgrid -lm

app1-s2:
	gcc -DAPP1 -DS2 migbsp.c -Wall -o migbsp -I /opt/simgrid/include/ -L /opt/simgrid/lib/ -l simgrid -lm

app1-s3:
	gcc -DAPP1 -DS3 migbsp.c -Wall -o migbsp -I /opt/simgrid/include/ -L /opt/simgrid/lib/ -l simgrid -lm