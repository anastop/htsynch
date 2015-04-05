#!/bin/bash
#Test overall execution time as symmetry of heavy/light threads increases

nbarriers=1000000

echo "mwmon" > barriers_2packages_varying_symmetry.txt
for((i=0; i<=10; i++))
do
	echo mwmon:$i
	heavy_work=10
	light_work=$i
	echo Heavy_work=$heavy_work  Light_work=$light_work  nbarriers=$nbarriers >> barriers_2packages_varying_symmetry.txt
	./barriers_2packages.mwmon $heavy_work $light_work $nbarriers >> barriers_2packages_varying_symmetry.txt
done
echo >> barriers_2packages_varying_symmetry.txt
echo >> barriers_2packages_varying_symmetry.txt

echo "pthread" >> barriers_2packages_varying_symmetry.txt
for((i=0; i<=10; i++))
do
	echo pthread:$i
	heavy_work=10
	light_work=$i
	echo Heavy_work=$heavy_work  Light_work=$light_work  nbarriers=$nbarriers >> barriers_2packages_varying_symmetry.txt
	./barriers_2packages.pthread $heavy_work $light_work $nbarriers >> barriers_2packages_varying_symmetry.txt
done
echo >> barriers_2packages_varying_symmetry.txt
echo >> barriers_2packages_varying_symmetry.txt


echo "spinpause" >> barriers_2packages_varying_symmetry.txt
for((i=0; i<=10; i++))
do
	echo spinpause:$i
	heavy_work=10
	light_work=$i
	echo Heavy_work=$heavy_work  Light_work=$light_work  nbarriers=$nbarriers >> barriers_2packages_varying_symmetry.txt
	./barriers_2packages.spinpause $heavy_work $light_work $nbarriers >> barriers_2packages_varying_symmetry.txt
done
