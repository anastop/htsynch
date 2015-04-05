#!/bin/bash
echo "mwmon barriers:" > barriers_results.txt
for ((i=0; i<40; i++))
do
	./barriers.mwmon $sync_period >> barriers_results.txt 
done
echo >> barriers_results.txt 
echo >> barriers_results.txt

echo "pthread barriers:" >> barriers_results.txt
for ((i=0; i<40; i++))
do
	./barriers.pthread $sync_period >> barriers_results.txt 
done
echo >> barriers_results.txt
echo >> barriers_results.txt

echo "spinpause barriers:" >> barriers_results.txt
for ((i=0; i<40; i++))
do
	./barriers.spinpause $sync_period >> barriers_results.txt 
done
echo >> barriers_results.txt
echo >> barriers_results.txt


echo "spinhalt barriers:" >> barriers_results.txt
for ((i=0; i<40; i++))
do
	./barriers.spinhalt $sync_period >> barriers_results.txt 
done
echo >> barriers_results.txt
echo >> barriers_results.txt
