#include <stdio.h>
#include <complex>
#include <vector>
#include <math.h>
#include <iostream>
#include <random> 
#include <fstream>
#include <cmath>


//использована модуляция по инструкциям 3gpp (Specification # 38.883)
class QAM_modulate{
public:
    std::vector<std::complex<float>> map(const std::vector<bool>& array, int& type_QAM){
        std::vector<std::complex<float>> samples; 

        if (type_QAM == 4){
            for (size_t i = 0 ; i < (array.size()); i += 2) {
                std::complex <float> val;
                val =  std::complex<float>( ( (1/sqrt(2)) * (1 - 2*array[i]) ) ,  ( (1/sqrt(2)) * (1.0f* (1 - 2*array[i+1]) ) ) )  ;   
                samples.push_back(val);
            }
        }

        else if (type_QAM == 16){
            for (size_t i = 0 ; i < (array.size()); i += 4) {
                std::complex <float> val;
                float real = ( (1-2*array[i]) * (2 - (1 - 2*array[i+2]) ) )  / (sqrt(10)); 
                float image = 1.0f* ( (1-2*array[i+1]) * (2 - (1 - 2*array[i+3]) ) ) / (sqrt(10));
                val = std::complex<float>(real, image);
                samples.push_back(val);
            }
        }

        else if (type_QAM == 64){
            for (size_t i = 0 ; i < (array.size() ); i += 6) {
                std::complex <float> val;
                float real = ((1-2*array[i]) * (4*array[i+2] + 2*array[i+4] + 1) )  / (sqrt(42)); 
                float image = 1.0f* ((1-2*array[i+1]) * (4*array[i+3] + 2*array[i+5] + 1)) / (sqrt(42));
                val = std::complex<float>(real, image);
                samples.push_back(val);
            }
        }

        return samples;
}
};


class AWGN{
public:
    std::vector <std::complex<float>> noise(const std::vector<std::complex<float>>& symbols, float dispers){
    std::vector <std::complex<float>> signal_AWGN(symbols.size());
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f,  sqrtf(dispers));
    
    for (size_t i = 0 ; i < symbols.size(); i ++) {
        signal_AWGN[i] = symbols[i] + std::complex<float>(dist(gen), dist(gen));
    } 

    return signal_AWGN;
}
};

class QAM_demodulate {
public:
    std::vector<bool> demodulate(const std::vector<std::complex<float>>& array, int type_QAM) {
        std::vector<bool> bits(array.size() * (type_QAM == 4 ? 2 : type_QAM == 16 ? 4 : 6));
        
        if (type_QAM == 4) {
            for (size_t i = 0; i < array.size(); i++) {
                bits[i*2]   = array[i].real() < 0;
                bits[i*2+1] = array[i].imag() < 0;
            }
        }
        else if (type_QAM == 16) {
            float norm = std::sqrt(10.0f);
            for (size_t i = 0; i < array.size(); i++) {
                float re = array[i].real() * norm;
                float im = array[i].imag() * norm;
                
                bits[i*4]   = re < 0;
                bits[i*4+1] = im < 0;
                bits[i*4+2] = (std::fabs(re) > 2);
                bits[i*4+3] = (std::fabs(im) > 2);
            }
        }
        else if (type_QAM == 64) {
            
            for (size_t i = 0; i < array.size(); i++) {
                float re = array[i].real();
                float im = array[i].imag();
                
                bits[i*6] = (re < 0);
                
                if ((re >  (6.0f / std::sqrt(42.0f))) || (re < - (6.0f / std::sqrt(42.0f)))) {
                    bits[i*6+2] = 1;
                    bits[i*6+4] = 1;
                } else if ((re > (4.0f / std::sqrt(42.0f))) || (re < -(4.0f / std::sqrt(42.0f)))) {
                    bits[i*6+2] = 1;
                    bits[i*6+4] = 0;
                } else if ((re > (2.0f / std::sqrt(42.0f))) || (re < -(2.0f / std::sqrt(42.0f)))) {
                    bits[i*6+2] = 0;
                    bits[i*6+4] = 0;
                } else {
                    bits[i*6+2] = 0;
                    bits[i*6+4] = 1;
                }
                
                bits[i*6+1] = (im < 0);
                
                if ((im >  (6.0f / std::sqrt(42.0f))) || (im < - (6.0f / std::sqrt(42.0f)))) {
                    bits[i*6+3] = 1;
                    bits[i*6+5] = 1;
                } else if ((im > (4.0f / std::sqrt(42.0f))) || (im < -(4.0f / std::sqrt(42.0f)))) {
                    bits[i*6+3] = 1;
                    bits[i*6+5] = 0;
                } else if ((im > (2.0f / std::sqrt(42.0f))) || (im < -(2.0f / std::sqrt(42.0f)))) {
                    bits[i*6+3] = 0;
                    bits[i*6+5] = 0;
                } else {
                    bits[i*6+3] = 0;
                    bits[i*6+5] = 1;
                }
            }
        }
        return bits;
    }
};

int main() {

    int type_QAM;
    std::cout << "write type QAM (4 - QPSK, 16 - QAM16, 64 - QAM64) = "  << std::endl;
    std::cin >> type_QAM;
    while (type_QAM != 4 && type_QAM != 16 && type_QAM != 64) {
        std::cerr << "Ошибка: допустимы только 4, 16, 64\n";
        std::cin >> type_QAM;
    }


    int bps = (type_QAM==4?2:type_QAM==16?4:6);
    int N = 1000 * bps;
   
    std::vector<bool> bits(N);
    srand(time(NULL));  
    for (int i = 0; i < bits.size(); i++) {
        bits[i] = rand() % 2;  
    }
    std::cout << "original bits" << std::endl;
    for(size_t i = 0; i < 20; i++) 
    {
        std::cout << bits[i] << " "  ;
    }
    std::cout << std::endl;

    QAM_modulate map;
    AWGN noise;            
    QAM_demodulate demap;

    std::vector<std::complex<float>> symbols = map.map(bits, type_QAM);
    std::cout << "QAM" << std::endl;
    for(size_t i = 0; i < 10; i++) 
    {
        std::cout << symbols[i] << " "  ;
    }
    std::cout << std::endl;
    

    std::vector<float> dispers = {0.01, 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5, 0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0};
    std::ofstream out("ber_results.csv");
    out << "dispers,errors,ber\n";

    for ( int a = 0; a < dispers.size(); a++)
    {
        std::vector<std::complex<float>> noise_signal = noise.noise(symbols, dispers[a]);
        std::vector<bool> decoded_bits = demap.demodulate(noise_signal, type_QAM);
        int errors = 0;
        for(size_t i = 0; i < bits.size(); i++) 
        {
            if ( decoded_bits[i] != bits[i] ) 
            { 
                errors += 1;
            } 
        }
        float ber = (float)errors / bits.size();

        std::cout << "" << std::endl ;
        std::cout << "значение дисперсии = " << dispers[a] << std::endl ;
        std::cout << "error bits = " << errors << std::endl << "ber = " << ber << std::endl ;
        out << dispers[a] << "," << errors << "," << ber << "\n";
    }

    out.close();

}
