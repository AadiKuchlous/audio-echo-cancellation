#include <iostream>
#include <fstream>
#include <string>
#include <cfloat>
#include <cmath>
#include <vector>
#include <algorithm>

#include "barge_detected.h"

// #include <sndfile.h>
// #include <fmt/format.h>

using namespace std;

struct RIFFHeader{
    char chunk_id[4];
    uint32_t chunk_size;
    char format[4];
};

struct ChunkInfo{
    char chunk_id[4];
    uint32_t chunk_size;
};

struct FmtChunk{
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

struct DataChunk
// We assume 16-bit monochannel samples
{  
    int16_t* data_stereo;
    vector<int16_t> data_stereo_vec;
    vector<int16_t> data_mono;
    int nb_of_samples;
    DataChunk(int s) {
        nb_of_samples = s;        
        data_stereo = new int16_t[s];
    }
    void CastArrayToVector() {
        data_stereo_vec.assign(data_stereo, data_stereo + nb_of_samples);
    }
    void StereoToMono() {
        for (size_t i = 0; i < data_stereo_vec.size()/2; i++) {
            data_mono.push_back((data_stereo_vec[i*2] + data_stereo_vec[(i*2)+1])/2);
        }
        
    }

    ~DataChunk(){delete[] data_stereo;}
};

/*
vector<int16_t> readWAVFile(string filename) {
    vector<int16_t> error_vec;
    
    const char riff_id[4] = {'R','I','F','F'};
    const char format[4] = {'W','A','V','E'};
    const char fmt_id[4] = {'f','m','t',' '};
    const char data_id[4] = {'d','a','t','a'};

    ifstream ifs(filename, ios_base::binary);
    if (!ifs){
        cerr << "Cannot open file" << endl;
        return error_vec;
    }
    
    // first read RIFF header
    RIFFHeader h;
    ifs.read((char*)(&h), sizeof(h));
    if (!ifs || memcmp(h.chunk_id, riff_id, 4) || memcmp(h.format, format, 4)){
        cerr << "Bad formatting" << endl;
        return error_vec;
    }
    
    // read chunk infos iteratively
    ChunkInfo ch;
    FmtChunk fmt;
    bool fmt_read = false;
    bool data_read = false;
    while(ifs.read((char*)(&ch), sizeof(ch))){
        // if fmt chunk?
        if (memcmp(ch.chunk_id, fmt_id, 4) == 0){
            ifs.read((char*)(&fmt), ch.chunk_size);
            fmt_read = true;
        }
        // is data chunk?
        else if(memcmp(ch.chunk_id, data_id, 4) == 0){
            DataChunk dat_chunk(ch.chunk_size/sizeof(int16_t));
            ifs.read((char*)dat_chunk.data_stereo, ch.chunk_size);
            dat_chunk.CastArrayToVector();
            if (fmt.num_channels == 2) {
                dat_chunk.StereoToMono();
            }
            data_read = true;
            return dat_chunk.data_mono;
        }
        // otherwise skip the chunk
        else{
            ifs.seekg(ch.chunk_size, ios_base::cur);
        }
    }
    if (!data_read || !fmt_read){
        cout << "Problem when reading data" << endl;
        return error_vec;
    }
    return error_vec;
}
*/
/*
void writeWavFile(const string& filename, const vector<int16_t>& samples, int sampleRate) {
    SF_INFO info;
    info.samplerate = sampleRate;
    info.channels = 1; // Mono
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* file = sf_open(filename.c_str(), SFM_WRITE, &info);
    if (!file) {
        std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
        return;
    }

    sf_write_short(file, samples.data(), samples.size());
    sf_close(file);

}
*/

float signalPower(vector<int16_t> bitstream) {
    // Running average of squares
    float running_avearge = pow(bitstream[0]/pow(2, 7), 2);
    for (size_t i = 1; i < bitstream.size(); i++) {
        running_avearge += (pow(bitstream[i]/pow(2, 7), 2) - running_avearge)/i;
    }
    return running_avearge;
}

vector<int16_t> amplifySignal(vector<int16_t> bitstream, float fraction) {
   for_each(
        bitstream.begin(),
        bitstream.end(),
        [fraction](int16_t &x) { x = x * fraction; }
    );
    return bitstream;
}

vector<int16_t> subtractSignal(vector<int16_t> signal1, vector<int16_t> signal2, int n_samples_delay_signal_2 = 0) {
    // Signal 1 - Signal 2
    vector<int16_t> result;

    int signal2_padded_size = signal2.size() + n_samples_delay_signal_2;

    for (size_t i = 0; i < fmax(signal1.size(), signal2_padded_size); i++) {
        if (i < n_samples_delay_signal_2) {
            result.push_back(signal1[i]);
        }
        else if (i > signal2_padded_size) {
            result.push_back(signal1[i]);
        }
        else if (i > signal1.size()){
            result.push_back(0-signal2[i]);
        }
        else {
            result.push_back(signal1[i] - signal2[i-n_samples_delay_signal_2]);
        }
    }
    return result;
}

pair<int, float> crossCorrelation(vector<int16_t> f, vector<int16_t> g, int first_delay_check, int n_samples_check) {
    // Change in correlation is proportional to change in amplitude of f in g
    int f_length = f.size();
    int g_length = g.size();

    vector<float> correlation(n_samples_check, 0);

    int index_max_correlation = 0;

    for (int sample_delay = first_delay_check; sample_delay < n_samples_check; sample_delay++){
        int correlation_index = sample_delay - first_delay_check;
        int n_samples = fmin(f_length, g_length - sample_delay);
        for (int i = 0; i < n_samples; i++) {
            // Running average
            float f_i_8bit = f[i]/pow(2, 7);
            float g_i_8bit = g[i+sample_delay]/pow(2, 7);
            float product = f_i_8bit*g_i_8bit;
            if (i == 0) {
                correlation[correlation_index] += product;
            }
            else {
                correlation[correlation_index] += (product - correlation[correlation_index])/i;
            }
        }

        if (correlation[correlation_index] > correlation[index_max_correlation]) {
            index_max_correlation = correlation_index;
        }

    }

    pair<int, float> max_correlation;
    max_correlation.first = index_max_correlation + first_delay_check;
    max_correlation.second = correlation[index_max_correlation];
    cout << "Max correlation of: "  << max_correlation.second << ", at: " <<  max_correlation.first << endl; 
    return max_correlation;
    
}

// Export functions for Flutter Package

bool bargeDetected(int buffer_size, int16_t ai_voice[], int16_t microphone_signal[]) {
    vector<int16_t> ai_voice_vec(ai_voice, ai_voice + buffer_size);
    vector<int16_t> microphone_signal_vec(microphone_signal, microphone_signal + buffer_size);

    float signal_power = signalPower(microphone_signal_vec);
    pair<int, float> max_correlation_pair = crossCorrelation(ai_voice_vec, microphone_signal_vec, 0, 50);
    int delay_max_correlation = max_correlation_pair.first;
    float max_correlation_value = max_correlation_pair.second;
    float atten_amount = max_correlation_value / signal_power;
    vector<int16_t> ai_attenuated = amplifySignal(ai_voice_vec, atten_amount);
    vector<int16_t> clean_signal = subtractSignal(microphone_signal_vec, ai_attenuated, delay_max_correlation);
    float clean_signal_power = signalPower(clean_signal);

    if (clean_signal_power < 0.5 * signal_power) {
        return true;
    }
    else {
        return false;
    }

    return false;
}

int main(){
    // vector<int16_t> testA = readWAVFile("./audio_files/A440.wav");
    // vector<int16_t> testA_small(testA.begin(), testA.begin() + 50);
    // vector<int16_t> testB = readWAVFile("./audio_files/1000.wav");
    // vector<int16_t> testB_small(testB.begin(), testB.begin() + 50);
    // vector<int16_t> test_mix = readWAVFile("./audio_files/mixed_sine.wav");
    // vector<int16_t> ai_only = readWAVFile("./audio_files/ai_only.wav");
    // vector<int16_t> mixed = readWAVFile("./audio_files/mixed_delay.wav");
    // vector<int16_t> mixed_noise = readWAVFile("./audio_files/mixed_delay_noise.wav");
    // vector<int16_t> mixed_dim3 = readWAVFile("./audio_files/mixed_delay_dim3.wav");
    // vector<int16_t> mixed_dim6 = readWAVFile("./audio_files/mixed_delay_dim6.wav");
    // vector<int16_t> mixed_dim10 = readWAVFile("./audio_files/mixed_delay_dim10.wav");

    // vector<int16_t> clean_signal = mixed_noise;
    // float noise_power = signalPower(ai_only);
    // pair<int, float> max_correlation_pair = crossCorrelation(ai_only, clean_signal, 0, 50);
    // int delay_max_correlation = max_correlation_pair.first;
    // float max_correlation_value = max_correlation_pair.second;
    // float atten_amount = max_correlation_value / noise_power;
    // vector<int16_t> atten_noise = amplifySignal(ai_only, atten_amount);

    return 1;
}
