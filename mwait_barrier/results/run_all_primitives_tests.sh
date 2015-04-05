#!/bin/bash
echo "mwmon primitives:" > primitives_results.txt
#for sync_period in 5 10 15 20 50 100 150 200 250 300 400 500
for sync_period in 500
do
	echo SYNC_PERIOD= $sync_period >> primitives_results.txt
	for ((i=0; i<40; i++))
	do
		./primitives.mwmon $sync_period >> primitives_results.txt 
	done

	echo >> primitives_results.txt 
	echo >> primitives_results.txt
done


echo "pthread primitives:" >> primitives_results.txt
#for sync_period in 5 10 15 20 50 100 150 200 250 300 400 500
for sync_period in 500
do
	echo SYNC_PERIOD= $sync_period >> primitives_results.txt
	for ((i=0; i<40; i++))
	do
		./primitives.pthread $sync_period >> primitives_results.txt 
	done

	echo >> primitives_results.txt
	echo >> primitives_results.txt
done


echo "spinpause primitives:" >> primitives_results.txt
#for sync_period in 5 10 15 20 50 100 150 200 250 300 400 500
for sync_period in 500
do
	echo SYNC_PERIOD= $sync_period >> primitives_results.txt
	for ((i=0; i<40; i++))
	do
		./primitives.spinpause $sync_period >> primitives_results.txt 
	done

	echo >> primitives_results.txt
	echo >> primitives_results.txt
done




echo "spinhalt primitives:" >> primitives_results.txt
#for sync_period in 5 10 15 20 50 100 150 200 250 300 400 500
for sync_period in 500
do
	echo SYNC_PERIOD= $sync_period >> primitives_results.txt
	for ((i=0; i<40; i++))
	do
		./primitives.spinhalt $sync_period >> primitives_results.txt 
	done

	echo >> primitives_results.txt
	echo >> primitives_results.txt
done


