#!/bin/bash

set -o pipefail

GETF0="get_pitch"

POT=${1:--42}
R1=${2:-0.47}
RMAX=${3:-0.33}

for fwav in pitch_db/train/*.wav; do
    ff0=${fwav/.wav/.f0}
    echo "$GETF0 $fwav $ff0 ----"
	$GETF0 --alpha0=$POT --alpha1=$R1 --alpha2=$RMAX $fwav $ff0 > /dev/null || { echo -e "\nError in $GETF0 $fwav $ff0" && exit 1; }
done

pitch_evaluate pitch_db/train/*.f0ref

exit 0
