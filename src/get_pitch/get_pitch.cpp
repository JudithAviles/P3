/// @file

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <map>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"

#include "docopt.h"

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator

Usage:
    get_pitch [options] <input-wav> <output-txt>

Options:
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
    std::map<std::string, docopt::value> args = docopt::docopt(USAGE,
        {argv + 1, argv + argc},
        true,
        "2.0");

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

    float max_abs = 0.0F;
    for (size_t n = 0; n < x.size(); ++n) {
        float abs_val = fabs(x[n]);
        if (abs_val > max_abs)
            max_abs = abs_val;
    }
    if (max_abs > 0.0F) {
        for (size_t n = 0; n < x.size(); ++n)
            x[n] /= max_abs;
    }

    int n_len   = (int)(rate * frame_len   + 0.5F);
    int n_shift = (int)(rate * frame_shift + 0.5F);

    PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::HAMMING, min_f0, max_f0);

    vector<float>::iterator iX;
    vector<float> f0;
    for (iX = x.begin(); iX + n_len < x.end(); iX = iX + n_shift) {
        float f = analyzer(iX, iX + n_len);
        f0.push_back(f);
    }

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
