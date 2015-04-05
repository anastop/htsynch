#!/bin/bash
echo "mwmon primitives:" > mwmon_primitives_results.txt
#for sync_period in 5 10 15 20 50 100 150 200 250 300 400 500
for sync_period in 500
do
	echo SYNC_PERIOD= $sync_period >> mwmon_primitives_results.txt
	for ((i=0; i<40; i++))
	do
		./primitives.mwmon $sync_period >> mwmon_primitives_results.txt 
	done

	echo >> mwmon_primitives_results.txt 
	echo >> mwmon_primitives_results.txt
done


echo "pthread primitives:" > pthread_primitives_results.txt
#for sync_period in 5 10 15 20 50 100 150 200 250 300 400 500
for sync_period in 500
do
	echo SYNC_PERIOD= $sync_period >> pthread_primitives_results.txt
	for ((i=0; i<40; i++))
	do
		./primitives.pthread $sync_period >> pthread_primitives_results.txt 
	done

	echo >> pthread_primitives_results.txt 
	echo >> pthread_primitives_results.txt
done


echo "spinpause primitives:" > spinpause_primitives_results.txt
#for sync_period in 5 10 15 20 50 100 150 200 250 300 400 500
for sync_period in 500
do
	echo SYNC_PERIOD= $sync_period >> spinpause_primitives_results.txt
	for ((i=0; i<40; i++))
	do
		./primitives.spinpause $sync_period >> spinpause_primitives_results.txt 
	done

	echo >> spinpause_primitives_results.txt 
	echo >> spinpause_primitives_results.txt
done




echo "spinhalt primitives:" > spinhalt_primitives_results.txt
#for sync_period in 5 10 15 20 50 100 150 200 250 300 400 500
for sync_period in 500
do
	echo SYNC_PERIOD= $sync_period >> spinhalt_primitives_results.txt
	for ((i=0; i<40; i++))
	do
		./primitives.spinhalt $sync_period >> spinhalt_primitives_results.txt 
	done

	echo >> spinhalt_primitives_results.txt 
	echo >> spinhalt_primitives_results.txt
done


