#!/bin/bash
for pot in -41 -42 -43; do 
  for r1 in 0.46 0.47 0.48; do 
    for rmax in 0.32 0.33 0.34; do
      for zcr in 0.009 0.010 0.011 0.012; do
        echo -ne "a0=$pot a1=$r1 a2=$rmax a3=$zcr\t"; 
        scripts/run_get_pitch.sh "$pot" "$r1" "$rmax" "$zcr" | grep TOTAL
      done
    done 
  done
done | sort -t: -k2 -n -r
