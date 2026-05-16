/// @file

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <map>

#include <complex>
#include <numeric>
#include <assert.h>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

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
    --min-f0=<Hz>              Minimum F0 in Hz [default: 50]
    --max-f0=<Hz>              Maximum F0 in Hz [default: 500]
    --frame-len=<s>            Frame length in seconds [default: 0.030]
    --frame-shift=<s>          Frame shift in seconds [default: 0.015]

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
    PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::HAMMING, min_f0, max_f0);

    /// \TODO
    /// Preprocess the input signal in order to ease pitch estimation. For instance,
    /// central-clipping or low pass filtering may be used.
    /// \DONE S'ha implementat center-clipping per reduir els efectes dels formants.
    /// També es normalitza el senyal per poder comparar de manera objectiva les característiques d'aquesta. 

    // Filtre passbaix Butterworth 4 ordre
    /*vector<float> y = x;
    float fc = max_f0+50;
    //float w_cd = 2*M_PI*(max_f0+50)/rate;
    //float w_ac = 2*rate*tan(w_dc/2);
    //float gamma = 2*rate/w_ac; 
    float gamma = 1/tan(M_PI*fc/rate);
    float alpha = -2*cos(5*M_PI/8);
    float beta = -2*cos(7*M_PI/8);

    float a0 = pow(gamma,4) + pow(gamma,3)*(alpha+beta) + pow(gamma,2)*(alpha*beta+2) + gamma*(alpha+beta) +1;
    float a1 = -4*pow(gamma,4) - 2*pow(gamma,3)*(alpha+beta) + 2*gamma*(alpha+beta) + 4;
    float a2 = 6*pow(gamma,4) - 2*pow(gamma,2)*(alpha*beta+2) + 6;
    float a3 = -4*pow(gamma,4) + 2*pow(gamma,3)*(alpha+beta) - 2*gamma*(alpha+beta) + 4;
    float a4 = pow(gamma,4) - pow(gamma,3)*(alpha+beta) + pow(gamma,2)*(alpha*beta+2) - gamma*(alpha+beta) + 1;
    for(size_t n = 0; n < 4; n++){
      y[n] = x[n];
    }
    for (size_t n = 4; n < x.size(); ++n) {
        y[n] = (3*x[n] + x[n-1] + x[n-2] + x[n-3])/6;
        //a0=1,a1=2⋅Re{pk},a2=|pk|^2
        //y[n] = 1/a0*(x[n] + 4*x[n-1] + 6*x[n-2] + 4*x[n-3] + x[n-4] - a1*y[n-1] - a2*y[n-2] - a3*y[n-3] - a4*y[n-4]);
    }*/

    // Filtre passbaix rudimentari (averaging filter)
    for (size_t n = 0; n < x.size(); ++n) {
        x[n] = (3*x[n] + x[n-1] + x[n-2] + x[n-3])/6;
    }

    // Center Clipping
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

    float C_L = 0.017;
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
    /*
    float dist_threshold = 100;
    float dist = 0;
    for (iX = f0.begin()+1; iX != f0.end()-1; ++iX){
      dist = *iX - *(iX-1);
      if(dist > dist_threshold){
        if((*iX-mean) >= (*(iX-1)-mean)){
          *iX = mean;
        } else{
          *(iX-1) = mean;
        }
      }
    }
    */

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
