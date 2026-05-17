/// @file

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <map>
#include <algorithm>

#include <complex>
#include <numeric>
#include <assert.h>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"
#include "digital_filter.h"

#include "docopt.h"

//#define FRAME_LEN   0.030 /* 30 ms. */
//#define FRAME_SHIFT 0.015 /* 15 ms. */

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -h, --help  Show this screen
    --version   Show the version of the project
    --min-f0=<Hz>              Minimum F0 in Hz [default: 20]
    --max-f0=<Hz>              Maximum F0 in Hz [default: 500]
    --frame-len=<s>            Frame length in seconds [default: 0.030]
    --frame-shift=<s>          Frame shift in seconds [default: 0.015]
    --alpha0=<dB>              Power threshold for unvoiced decision [default: -42]
    --alpha1=<f>               r1/r0 threshold for unvoiced decision [default: 0.48]
    --alpha2=<f>               rmax/r0 threshold for unvoiced decision [default: 0.34]
    --alpha3=<f>               rmax/r0 threshold for unvoiced decision [default: 0.012]
    --method=<name>            Method: autocorr, amdf, cepstrum [default: autocorrelació]

Arguments:
    input-wav   Wave file with the audio signal
    output-txt  Output file: ASCII file with the result of the estimation:
                    - One line per frame with the estimated f0
                    - If considered unvoiced, f0 must be set to f0 = 0
)";


int main(int argc, const char *argv[]) {
	/**
   \TODO 
	 Modify the program syntax and the call to **docopt()** in order to
	 add options and arguments to the program.
   \DONE S'han afegit les opcions min_f0 [default: 50] i max_f0 [default: 500], per establir les freqüències de pitch llindar per les que el programa pot buscar,
   i frame-length [default: 0.030] i frame-shift [default: 0.015], que permeten canviar la llargada dels frames i el desplaçament 
  */

    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},	// array of arguments, without the program name
        true,    // show help if requested
        "2.0");  // version string

    std::string input_wav = args["<input-wav>"].asString();
    std::string output_txt = args["<output-txt>"].asString();

    float frame_len   = stof(args["--frame-len"].asString());
    float frame_shift = stof(args["--frame-shift"].asString());
    float min_f0      = stof(args["--min-f0"].asString());
    float max_f0      = stof(args["--max-f0"].asString());
    float alpha0 = stof(args["--alpha0"].asString());
    float alpha1 = stof(args["--alpha1"].asString());
    float alpha2 = stof(args["--alpha2"].asString());
    float alpha3 = stof(args["--alpha3"].asString());

    string method_str = args["--method"].asString();
    PitchAnalyzer::Params method;
    if (method_str == "amdf")
      method = PitchAnalyzer::AMDF;
    else if (method_str == "cepstrum")
      method = PitchAnalyzer::CEPSTRUM;
    else
      method = PitchAnalyzer::CORRELACIO;

    // Read input sound file
    unsigned int rate;
    vector<float> x;
    if (readwav_mono(input_wav, rate, x) != 0) {
        cerr << "Error reading input file " << input_wav << " (" << strerror(errno) << ")\n";
        return -2;
    }

    int n_len   = (int)(rate * frame_len);
    int n_shift = (int)(rate * frame_shift);

    // Define analyzer
    PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::HAMMING, method, min_f0, max_f0, alpha0, alpha1, alpha2, alpha3);

    /// \TODO
    /// Preprocess the input signal in order to ease pitch estimation. For instance,
    /// central-clipping or low pass filtering may be used.
    /// \DONE S'ha implementat center-clipping per reduir els efectes dels formants.
    /// També es normalitza el senyal per poder comparar de manera objectiva les característiques d'aquesta. 

    // Filtre passbaix rudimentari (averaging filter)
    for (size_t n = 0; n < x.size(); ++n) {
        x[n] = (3*x[n] + x[n-1] + x[n-2] + x[n-3])/6;
    }

    // Center Clipping+ Normalització
    float max_abs = 0.0F;
    float abs_val = 0.0F;
    for (size_t n = 0; n < x.size(); ++n) {
        abs_val = fabs(x[n]);
        if (abs_val > max_abs){
          max_abs = abs_val;
        }
    }
    if (max_abs > 0.0F) {
        for (size_t n = 0; n < x.size(); ++n)
            x[n] /= max_abs;
    }

    float C_L = 0.01;
    for (size_t n = 0; n < x.size(); ++n) {
        if(x[n] >= C_L){
          x[n] = x[n] - C_L;
        } else if (x[n] <= -C_L){
          x[n] = x[n] + C_L;
        } else {
          x[n] = 0;
        }
    }

    // Iterate for each frame and save values in f0 vector
    vector<float>::iterator iX;
    vector<float> f0;
    for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
        float f = analyzer(iX, iX + n_len);
        f0.push_back(f);
    }

    /// \TODO
    /// Postprocess the estimation in order to supress errors. For instance, a median filter
    /// or time-warping may be used.
    /// \DONE S'ha implementat un filtre de mediana de mida 3 al pitch, i s'examina la distància entre dos mostres contínues.
    /// Si la distància entre els dos pitch estimats és més gran que el threshold, es tracta la més llunyana de la mitjana com un error i és corregit

    // Filtre de mediana
    std:: vector<float> mediana;
    float mean = 0;
    for (iX = f0.begin()+1; iX != f0.end()-1; ++iX){
      mediana = {*(iX-1), *iX, *(iX+1)};
      stable_sort(mediana.begin(), mediana.end());
      *iX = mediana[1];
      mean = mean + *iX;
    }
    mean = mean/f0.size();

    // Error correction
    float dist_threshold = 360;
    float dist = 0;
    for (iX = f0.begin()+1; iX != f0.end()-1; ++iX){
      dist = fabs(*iX - *(iX-1));
      if(dist > dist_threshold){
        if(fabs(*iX-mean) >= fabs(*(iX-1)-mean)){
          *iX = *(iX-1);
        } else{
          *(iX-1) = *iX;
        }
      }
    }

    // Write f0 contour into the output file
    ofstream os(output_txt);
    if (!os.good()) {
        cerr << "Error opening output file " << output_txt << " (" << strerror(errno) << ")\n";
        return -3;
    }

    os << 0 << '\n';
    for (iX = f0.begin(); iX != f0.end(); ++iX)
        os << *iX << '\n';
    os << 0 << '\n';

    return 0;
}
