/// @file

#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <map>
#include <algorithm>

#include "wavfile_mono.h"
#include "pitch_analyzer.h"
#include "digital_filter.h"

#include "docopt.h"

using namespace std;
using namespace upc;

static const char USAGE[] = R"(
get_pitch - Pitch Estimator

Usage:
    get_pitch [options] <input-wav> <output-txt>

Options:
    --alpha0=<dB>              Power threshold for unvoiced decision [default: -40]
    --alpha1=<f>               r1/r0 threshold for unvoiced decision [default: 0.30]
    --alpha2=<f>               rmax/r0 threshold for unvoiced decision [default: 0.40]

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

    float alpha0 = stof(args["--alpha0"].asString());
    float alpha1 = stof(args["--alpha1"].asString());
    float alpha2 = stof(args["--alpha2"].asString());

    float frame_len   = 0.030F;
    float frame_shift = 0.015F;
    float min_f0      = 50.0F;
    float max_f0      = 500.0F;
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

    // Preprocessing: LPF + decimation (20 kHz -> 10 kHz)
    if (rate > 10000) {
        vector<float> b_lpf = {0.2929F, 0.5858F, 0.2929F};
        vector<float> a_lpf = {1.0F, 0.0F, 0.1716F};
        DigitalFilter lpf(a_lpf, b_lpf);
        vector<float> x_filt(x.size());
        for (size_t i = 0; i < x.size(); ++i)
            x_filt[i] = lpf(x[i]);
        vector<float> x_dec;
        for (size_t i = 0; i < x_filt.size(); i += 2)
            x_dec.push_back(x_filt[i]);
        x.swap(x_dec);
        rate = 10000;
    }

    int n_len   = (int)(rate * frame_len   + 0.5F);
    int n_shift = (int)(rate * frame_shift + 0.5F);

    PitchAnalyzer analyzer(n_len, rate, PitchAnalyzer::HAMMING, min_f0, max_f0,
                           alpha0, alpha1, alpha2);

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
