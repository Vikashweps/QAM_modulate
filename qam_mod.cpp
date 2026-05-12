#include <stdio.h>
#include <complex>
#include <vector>
#include <math.h>
#include <iostream>
#include <random> 
#include <fstream>
#include <cmath>

// std::vector<std::complex<float>> QAM_MAP(const std::vector<int>& array, int& type_QAM){
//     std::vector<std::complex<float>> samples; 

//     if (type_QAM == 4){
//         for (size_t i = 0 ; i < (array.size()); i += 2) {
//             std::complex <float> val;
//             val =  std::complex<float>( ( (1/sqrt(2)) * (1 - 2*array[i]) ) ,  ( (1/sqrt(2)) * (1.0f* (1 - 2*array[i+1]) ) ) )  ;   
//             samples.push_back(val);
//         }
//     }

//     else if (type_QAM == 16){
//         for (size_t i = 0 ; i < (array.size()); i += 4) {
//             std::complex <float> val;
//             float real = ( (1-2*array[i]) * (2 - (1 - 2*array[i+2]) ) )  / (sqrt(10)); 
//             float image = 1.0f* ( (1-2*array[i+1]) * (2 - (1 - 2*array[i+3]) ) ) / (sqrt(10));
//             val = std::complex<float>(real, image);
//             samples.push_back(val);
//         }
//     }

//     else if (type_QAM == 64){
//         for (size_t i = 0 ; i < (array.size() ); i += 6) {
//             std::complex <float> val;
//             float real = ((1-2*array[i]) * (4*array[i+2] + 2*array[i+4] + 1) )  / (sqrt(42)); 
//             float image = 1.0f* ((1-2*array[i+1]) * (4*array[i+3] + 2*array[i+5] + 1)) / (sqrt(42));
//             val = std::complex<float>(real, image);
//             samples.push_back(val);
//         }
//     }

//     return samples;
// }


class QAM_grey {

private:
    int type_QAM;
    int k;
    float norm;
    int bps;

public:
    std::vector<std::complex<float>> map(const std::vector<bool>& bits, int M) {
        int k = (int)(log2(M) / 2); 
        int bps = 2*k;
        float norm = sqrt(3.0/(2*(M-1)));
        std::vector<std::complex<float>> sym(bits.size()/bps);
        for(size_t i=0; i<sym.size(); ++i) {
            int I=0, Q=0;
            for(int b=0; b<k; ++b) {
                I = (I<<1) | bits[i*bps + b];
                Q = (Q<<1) | bits[i*bps + k + b];
            }
            int gI = I ^ (I>>1), gQ = Q ^ (Q>>1);
            int amp = (1<<k)-1;
            sym[i] = {(2.0f*gI - amp)*norm, (2.0f*gQ - amp)*norm};
        }
        return sym;
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

class QAM_demapper {
private:
    std::vector<std::complex<float>> symbols; 
    int bps; 

public:
    QAM_demapper(int type_QAM) {
        QAM_grey mapper;
        bps = (type_QAM == 4) ? 2 : (type_QAM == 16) ? 4 : 6;

        for (int i = 0; i < type_QAM; ++i) {
            std::vector<bool> b(bps);
            for (int j = 0; j < bps; ++j){
                b[j] = (i >> (bps - 1 - j)) & 1;
                }
            symbols.push_back(mapper.map(b, type_QAM)[0]);
        }
    }

    std::vector<bool> demap(const std::vector<std::complex<float>>& rx) {
        std::vector<bool> out_bits;

        for (size_t idx = 0; idx < rx.size(); ++idx) {
            int best_idx = 0;
            float best_dist = std::norm(rx[idx] - symbols[0]);
            for (size_t i = 1; i < symbols.size(); ++i) {
                float dist = std::norm(rx[idx] - symbols[i]);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_idx = i;
                }
            }
            for (int j = 0; j < bps; ++j)
                out_bits.push_back((best_idx >> (bps - 1 - j)) & 1);
        }
        return out_bits;
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

    QAM_grey map;
    AWGN noise;            
    QAM_demapper demap(type_QAM);

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
        std::vector<bool> decoded_bits = demap.demap(noise_signal);
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
