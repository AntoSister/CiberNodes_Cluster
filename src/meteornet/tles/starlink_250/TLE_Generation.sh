#!/bin/bash

nOrbit=25
# nSatPerOrbit=22
nSatPerOrbit=10
inclination=" 53"
eccentricity="0000000"		# 0: circular orbit
argumentOfPerigee="000"
satelliteAltitude=550		# km
simulationDuration=86400

massEarth=`echo "5.9736 * 10^24" | bc`
gravitationalConstant=`echo "scale=14; 6.673 / 10^11" | bc`
radiusEarth=6371
radiusOrbit=$(($radiusEarth + $satelliteAltitude))
satelliteAverageSpeed=`echo "scale=8; sqrt(($gravitationalConstant * $massEarth) / ($radiusOrbit * 1000))" | bc`
orbitLength=`echo "2 * 4 * a(1) * $radiusOrbit * 1000" | bc -l`
orbitTime=`echo "scale=8; $orbitLength / $satelliteAverageSpeed" | bc`

for i in $(seq 1 $nOrbit);
do
    for j in $(seq 1 $nSatPerOrbit);
    do
	tleNumber=$(($((i-1)) * $nSatPerOrbit + $j));
        #Title line
#        echo -n -e "UNIGE_CUBESAT$j\n" >> TLE_Satellite_$tleNumber.txt

        # First line
        if [ $tleNumber -lt 10 ]; then
		    satID="0000$tleNumber"
	    elif [ $tleNumber -lt 100 ]; then
		    satID="000$tleNumber"
	    elif [ $tleNumber -lt 1000 ]; then
		    satID="00$tleNumber"
	    elif [ $tleNumber -lt 10000 ]; then
		    satID="0$tleNumber"
	    else
		    satID="$tleNumber"
	    fi
        firstLine="1 ${satID}U 17021A   16303.82790574  .00000605  00000-0  72029-4 0  001"
        firstLineChecksum_temp=0
        for k in $(seq 1 ${#firstLine})
        do
            if [ "${firstLine:k-1:1}" == "0" ] || [ "${firstLine:k-1:1}" == "1" ] || [ "${firstLine:k-1:1}" == "2" ] || [ "${firstLine:k-1:1}" == "3" ] || [ "${firstLine:k-1:1}" == "4" ] || [ "${firstLine:k-1:1}" == "5" ] || [ "${firstLine:k-1:1}" == "6" ] || [ "${firstLine:k-1:1}" == "7" ] || [ "${firstLine:k-1:1}" == "8" ] || [ "${firstLine:k-1:1}" == "9" ]; then
                ((firstLineChecksum_temp+=${firstLine:k-1:1}))
            elif [ "${firstLine:k-1:1}" == "-" ]; then
                ((firstLineChecksum_temp++))
            else
                  empty=" "
            fi
	firstLineChecksum=$(($firstLineChecksum_temp % 10));
        done
        echo -n -e "$firstLine$firstLineChecksum\n" >> TLE_Satellite_$tleNumber.txt

        # Second line
	raan_coefficient=360		# 180: polar orbit; 360: non polar orbit
        raan_temp=`echo "scale=0; $raan_coefficient * $((i-1)) / $nOrbit" | bc`
        if [ $raan_temp -lt 10 ]; then
		    raan="00$raan_temp"
	    elif [ $raan_temp -lt 100 ]; then
		    raan="0$raan_temp"
	    else
		    raan="$raan_temp"
	    fi
        meanAnomaly_temp=`echo "scale=0; 360 * $((j-1)) / $nSatPerOrbit + (360 * $((i-1)) / $nOrbit / $nSatPerOrbit)" | bc`
        if [ $meanAnomaly_temp -lt 10 ]; then
		    meanAnomaly="00$meanAnomaly_temp"
	    elif [ $meanAnomaly_temp -lt 100 ]; then
		    meanAnomaly="0$meanAnomaly_temp"
	    else
		    meanAnomaly="$meanAnomaly_temp"
	    fi
	meanMotion=`echo "scale=8; 24 * 3600 / $orbitTime" | bc`
        secondLine="2 ${satID} ${inclination}.0000 ${raan}.0000 ${eccentricity} ${argumentOfPerigee}.0000 ${meanAnomaly}.0000 ${meanMotion}46064"
        secondLineChecksum_temp=0
        for k in $(seq 1 ${#secondLine})
        do
            if [ "${secondLine:k-1:1}" == "0" ] || [ "${secondLine:k-1:1}" == "1" ] || [ "${secondLine:k-1:1}" == "2" ] || [ "${secondLine:k-1:1}" == "3" ] || [ "${secondLine:k-1:1}" == "4" ] || [ "${secondLine:k-1:1}" == "5" ] || [ "${secondLine:k-1:1}" == "6" ] || [ "${secondLine:k-1:1}" == "7" ] || [ "${secondLine:k-1:1}" == "8" ] || [ "${secondLine:k-1:1}" == "9" ]; then
                ((secondLineChecksum_temp+=${secondLine:k-1:1}))
            elif [ "${secondLine:k-1:1}" == "-" ]; then
                ((secondLineChecksum_temp++))
            else
                  empty=" "
            fi
        done
	secondLineChecksum=$(($secondLineChecksum_temp % 10));
        secondLineTimes="000000.00 ${simulationDuration}.00 001.00"
        echo -n -e "$secondLine$secondLineChecksum $secondLineTimes\n" >> TLE_Satellite_$tleNumber.txt
    done
done

exit 0
