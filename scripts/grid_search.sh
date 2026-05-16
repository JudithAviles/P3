#!/bin/bash
for pot in -35 -40 -45; do 
  for r1 in 0.25 0.30 0.35; do 
    for rmax in 0.35 0.40 0.45; do
      echo -ne "a0=$pot a1=$r1 a2=$rmax\t"; 
      scripts/run_get_pitch.sh "$pot" "$r1" "$rmax" | grep TOTAL
    done 
  done
done | sort -t: -k2 -n -r
