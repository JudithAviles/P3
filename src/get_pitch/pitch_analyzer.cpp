/// @file

#include <iostream>
#include <math.h>
#include "pitch_analyzer.h"

using namespace std;

/// Name space of UPC
namespace upc {
  void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {

    for (unsigned int l = 0; l < r.size(); ++l) {
  		/**
      \TODO Compute the normalized autocorrelation r[l]
      \DONE Autocorrelació calculada:
      \f[
      r[l] = \frac{1}{N} \sum_{n=l}^{N} x[n] \cdot x[n-l]
      \f]
      1. Inicialitzem \f$r[l]\f$ a zero
      2. Acumulem el producte de \f$x[n]\f$ per \f$x[n-l]\f$ per a \f$l \leq n < N\f$
      3. Dividim el resultat per \f$N\f$
      */
      // La autocorrelación es sesgada; la senyal está enventanada y la consideramos cero
      r[l] = 0;
      for (unsigned int n = l; n < x.size(); ++n){
        r[l] += x[n]*x[n-l];
      }
      r[l] = r[l]/x.size();
    }

    if (r[0] == 0.0F) //to avoid log() and divide zero 
      r[0] = 1e-10; 
  }

  void PitchAnalyzer::set_window(Window win_type) {
    if (frameLen == 0)
      return;

    window.resize(frameLen);

    switch (win_type) {
    case HAMMING:
      /// \TODO Implement the Hamming window
      /// \DONE Finestra de Hamming implementada
      for (unsigned int n = 0; n < frameLen; ++n){
        window[n] = 0.54 - 0.46*cos(2*M_PI*n/(frameLen-1));
      }
      break;
    case RECT:
    default:
      window.assign(frameLen, 1);
      break;
    }
  }

  void PitchAnalyzer::set_f0_range(float min_F0, float max_F0) {
    npitch_min = (unsigned int) samplingFreq/max_F0;
    if (npitch_min < 2)
      npitch_min = 2;  // samplingFreq/2

    npitch_max = 1 + (unsigned int) samplingFreq/min_F0;

    //frameLen should include at least 2*T0
    if (npitch_max > frameLen/2)
      npitch_max = frameLen/2;
  }

  void PitchAnalyzer::set_unvoiced_thresholds(float pot, float r1, float rmax) {
    pot_threshold = pot;
    r1norm_threshold = r1;
    rmaxnorm_threshold = rmax;
  }

  /*
  float PitchAnalyzer::compute_zcr(const vector<float> &x, unsigned int N, float fm) const {
    int sum = 0;
    int i;
    for(i = 1; i < N; i++) {
        if ((x[i] >= 0 && x[i-1] < 0) || (x[i] <= 0 && x[i-1] > 0)) {
            sum++;
        }
    }
    float zcr = (sum * fm) / (2*(N-1));
    return zcr;
  }
  */

  bool PitchAnalyzer::unvoiced(float pot, float r1norm, float rmaxnorm) const {
    /**
    \TODO Implement a rule to decide whether the sound is voiced or not.
    * You can use the standard features (pot, r1norm, rmaxnorm),
        or compute and use other ones.
    \DONE Decision rule improved:
    * pot: 10*log10(r[0]), log-power in dB (after normalization to [-1,1])
    * r1norm: r[1]/r[0], normalized correlation at lag 1
    * rmaxnorm: r[lag_max]/r[0], normalized correlation at pitch period
    */

    // Señales sonoras tienden a tener bajas frecuencias por la resonancia con el tracto vocal --> r[1] >0
    // Las sordas tienden a ser de alta frecuencia  --> r[1] < 0
    // Siempre referido a fm/4 --> Varía con fm

    // Para implementar --> Zero-Crossings, Cepstrum, AMDF
    if (pot < pot_threshold){
      return true;
    } else if (r1norm >= r1norm_threshold && rmaxnorm >= rmaxnorm_threshold){
      return false;
    } else{
      return true;
    }
  }

  float PitchAnalyzer::compute_pitch(vector<float> & x) const {
    if (x.size() != frameLen){
      return -1.0F;
    }

    //Window input frame
    for (unsigned int i=0; i<x.size(); ++i){
      x[i] *= window[i];
    }

    vector<float> r(npitch_max);

    //Compute correlation
    autocorrelation(x, r);

    vector<float>::const_iterator iR = r.begin(), iRMax = r.begin() + npitch_min;

  /* 
  \TODO 
	Find the lag of the maximum value of the autocorrelation away from the origin.<br>
	Choices to set the minimum value of the lag are:
	  - The first negative value of the autocorrelation.
	  - The lag corresponding to the maximum value of the pitch.
  .
	In either case, the lag should not exceed that of the minimum value of the pitch.
  \DONE A basic search for the maximum value of the autocorrelation away from the origin, as well as a rule for unvoiced segments have been implemented.
  */

    for(iR = iRMax; (iR < r.begin()+npitch_max-1 && iR < r.end()); iR++){
      if(*iR > *iRMax){
        iRMax = iR;
      }
    }

    unsigned int lag = iRMax - r.begin();

    float pot = 10 * log10(r[0]);

    //You can print these (and other) features, look at them using wavesurfer
    //Based on that, implement a rule for unvoiced
    //change to #if 1 and compile
#if 1
    if (r[0] > 0.0F)
      cout << pot << '\t' << r[1]/r[0] << '\t' << r[lag]/r[0] << endl;
#endif
    
    if (unvoiced(pot, r[1]/r[0], r[lag]/r[0]))
      return 0;
    else
      return (float) samplingFreq/(float) lag;
  }
}
