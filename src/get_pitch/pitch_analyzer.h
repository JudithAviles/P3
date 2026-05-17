/// @file

#ifndef PITCH_ANALYZER_H
#define PITCH_ANALYZER_H

#include <vector>
#include <algorithm>

namespace upc {
  const float MIN_F0 = 20.0F;    ///< Minimum value of pitch in Hertzs
  const float MAX_F0 = 500.0F; ///< Maximum value of pitch in Hertzs

  ///
  /// PitchAnalyzer: class that computes the pitch (in Hz) from a signal frame.
  /// No pre-processing or post-processing has been included
  ///
  // ^Doxygen comment, appears in doxygen documentation
  class PitchAnalyzer {
  public:
	/// Wndow type + Unvoiced choice type
    enum Params {
		RECT, 						          ///< Rectangular window
		HAMMING,						        ///< Hamming window
    CORRELACIO = 1, 				    ///< Autocorrelation
    AMDF = 2, 						      ///< AMDF
    CEPSTRUM = 3 						    ///< Cepstrum
  };

    void set_window(Params type); ///< pre-compute window

  private:
    std::vector<float> window; ///< precomputed window
    unsigned int frameLen, ///< length of frame (in samples). Has to be set in the constructor call
      samplingFreq, ///< sampling rate (in samples per second). Has to be set in the constructor call
      npitch_min, ///< minimum value of pitch period, in samples
      npitch_max; ///< maximum value of pitch period, in samples
    int unvoiced_choice;
    float pot_threshold;
    float r1norm_threshold;
    float rmaxnorm_threshold;
    float zcr_threshold;
 
	///
	/// Computes correlation from lag=0 to r.size()
	///
    void autocorrelation(const std::vector<float> &x, std::vector<float> &r) const;

  ///
	/// Computes AMDF from lag=0 to d.size()
	///
    void compute_AMDF(const std::vector<float> &x, std::vector<float> &d) const;

  ///
	/// Computes cepstrum from lag=0 to r.size()
	///
    void cepstrum(const std::vector<float> &x, std::vector<float> &c) const;

  ///
	/// Computes zero crossings of input frame x
	///
    float compute_zcr(const std::vector<float> &x, size_t N, float fm) const;

	///
	/// Returns the pitch (in Hz) of input frame x
	///
    float compute_pitch(std::vector<float> & x) const;
	
	///
	/// Returns true is the frame is unvoiced
	///
    bool unvoiced(float pot, float r1norm, float rmaxnorm, float zcr) const;


  public:
    PitchAnalyzer(	unsigned int fLen,			///< Frame length in samples
					unsigned int sFreq,			///< Sampling rate in Hertzs
					Params w = PitchAnalyzer::HAMMING,	///< Window type
          Params method_choice = PitchAnalyzer::CORRELACIO,	  ///< Method used for the unvoiced choice
					float min_F0 = 20,		        ///< Pitch range should be restricted to be above this value
					float max_F0 = 500,		        ///< Pitch range should be restricted to be below this value
					float potTh = -42.0F,         ///< Llindar de potència per unvoiced decision
					float r1nTh = 0.47F,          ///< Llindar de r[1]/r[0] per unvoiced decision
					float rmaxnTh = 0.33F,        ///< Llindar de r[P]/r[0] per unvoiced decision
          float zcrTh = 0.012F        ///< Llindar de zcr per unvoiced decision
				 )
	  {
      frameLen = fLen;
      samplingFreq = sFreq;
      pot_threshold = potTh;
      r1norm_threshold = r1nTh;
      rmaxnorm_threshold = rmaxnTh;
      zcr_threshold = zcrTh;
      unvoiced_choice = method_choice;
      set_f0_range(min_F0, max_F0);
      //set_unvoiced_thresholds(potTh, r1nTh, rmaxnTh);
      set_window(w);
    }

	///
    /// Operator (): computes the pitch for the given vector x
	///
    float operator()(const std::vector<float> & _x) const {
      if (_x.size() != frameLen)
        return -1.0F;

      std::vector<float> x(_x); //local copy of input frame
      return compute_pitch(x);
    }

	///
    /// Operator (): computes the pitch for the given "C" vector (float *).
    /// N is the size of the vector pointer by pt.
	///
    float operator()(const float * pt, unsigned int N) const {
      if (N != frameLen)
        return -1.0F;

      std::vector<float> x(N); //local copy of input frame, size N
      std::copy(pt, pt+N, x.begin()); ///copy input values into local vector x
      return compute_pitch(x);
    }

	///
    /// Operator (): computes the pitch for the given vector, expressed by the begin and end iterators
	///
    float operator()(std::vector<float>::const_iterator begin, std::vector<float>::const_iterator end) const {

      if (end-begin != frameLen)
        return -1.0F;

      std::vector<float> x(end-begin); //local copy of input frame, size N
      std::copy(begin, end, x.begin()); //copy input values into local vector x
      return compute_pitch(x);
    }
    
	///
    /// Sets pitch range: takes min_F0 and max_F0 in Hz, sets npitch_min and npitch_max in samples
	///
    void set_f0_range(float min_F0, float max_F0);

    //void set_unvoiced_thresholds(float pot, float r1, float rmax);
  };
}
#endif
