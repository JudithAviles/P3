#!/bin/bash

set -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
export PATH="$PROJECT_DIR/build/release/src/get_pitch:$PATH"

GETF0="get_pitch"

POT=${1:--42}
R1=${2:-0.47}
RMAX=${3:-0.33}

for fwav in pitch_db/train/*.wav; do
    ff0=${fwav/.wav/.f0}
	$GETF0 --alpha0=$POT --alpha1=$R1 --alpha2=$RMAX $fwav $ff0 > /tmp/get_pitch_out.txt 2>&1 || { echo -e "\nError in $GETF0 $fwav $ff0" && exit 1; }
done

pitch_evaluate pitch_db/train/*.f0ref

exit 0
