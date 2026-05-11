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



std::vector<std::complex<float>> QAM_gray(const std::vector<int>& array, int& type_QAM) {
    int k = 0;
    switch (type_QAM) {
        case 4:  k = 1; break;
        case 16: k = 2; break;
        case 64: k = 3; break;
        default: k = 0;
    }
    
    float norm = std::sqrt(3.0f / (2.0f * (type_QAM - 1)));
    int bps = 2 * k;
    std::vector<std::complex<float>> samples_grei(array.size() / bps);

    for (size_t i = 0; i < samples_grei.size(); ++i) {
        int I = 0, Q = 0;
        for (int b = 0; b < k; ++b) {
            I = (I << 1) | array[i * bps + b];
            Q = (Q << 1) | array[i * bps + k + b];
        }
        int gI = I ^ (I >> 1);
        int gQ = Q ^ (Q >> 1);
        float amp = (1 << k) - 1;
        samples_grei[i] = { (2.0f * gI - amp) * norm, (2.0f * gQ - amp) * norm };
    }
    return samples_grei;
}

std::vector<std::complex<float>> AWGN(const std::vector<std::complex<float>>& symbols, float dispers){
    std::vector <std::complex<float>> signal_AWGN(symbols.size());
    float sigma = sqrtf(dispers);
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::normal_distribution<float> dist(0.0f, sigma);
    
    for (size_t i = 0 ; i < symbols.size(); i ++) {
        signal_AWGN[i] = symbols[i] + std::complex<float>(dist(gen), dist(gen));
    } 

    return signal_AWGN;
}


std::vector<int> QAM_demapper(const std::vector<std::complex<float>>& symbols, int& type_QAM) {
    int k = 0;
    switch (type_QAM) {
        case 4:  k = 1; break;
        case 16: k = 2; break;
        case 64: k = 3; break;
        default: k = 0;
    }
    float norm = std::sqrt(3.0f / (2.0f * (type_QAM - 1)));
    int amp = (1 << k) - 1;
    int bps = 2 * k;
    std::vector<int> bits(symbols.size() * bps);
    
    // Таблицы преобразования Грея -> двоичный
    static const int gray2bin[3][8] = {
        {0, 1},                   // k=1
        {0, 1, 3, 2},            // k=2
        {0, 1, 3, 2, 6, 7, 5, 4} // k=3
    };
    
    for (size_t i = 0; i < symbols.size(); ++i) {
        float re = symbols[i].real() / norm;
        float im = symbols[i].imag() / norm;
        
        int levI = (int)(re + (re >= 0 ? 0.5f : -0.5f));
        int levQ = (int)(im + (im >= 0 ? 0.5f : -0.5f));
        
        // Насыщение
        levI = std::max(-amp, std::min(amp, levI));
        levQ = std::max(-amp, std::min(amp, levQ));
        
        int gI = (levI + amp) / 2;
        int gQ = (levQ + amp) / 2;
        
        int bI = gray2bin[k-1][gI];
        int bQ = gray2bin[k-1][gQ];
        
        int base = i * bps;
        for (int b = 0; b < k; ++b) {
            bits[base + b]       = (bI >> (k - 1 - b)) & 1;
            bits[base + k + b]   = (bQ >> (k - 1 - b)) & 1;
        }
    }
    return bits;
}


int main() {

    std::cout << "write type QAM (4 - QPSK, 16 - QAM16, 64 - QAM64) = "  << std::endl;
    int type_QAM;
    std::cin >> type_QAM;

    if (type_QAM != 4 && type_QAM != 16 && type_QAM != 64) {
        std::cerr << "Ошибка: допустимы только 4, 16, 64\n";
        std::cin >> type_QAM;
    }

    int bps = 0;
    switch(type_QAM) {
        case 4:  bps = 2; break;
        case 16: bps = 4; break;
        case 64: bps = 6; break;
    }
    int num_symbols = 10000;
    int N = num_symbols * bps;
   
    std::vector<int> bits(N);
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

    std::vector<std::complex<float>> symbols = QAM_gray(bits, type_QAM);
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
        std::vector<std::complex<float>> signal_AWGN = AWGN(symbols, dispers[a]);

        std::vector<int> bitiks = QAM_demapper(signal_AWGN, type_QAM);
        int errors = 0;
        for(size_t i = 0; i < bits.size(); i++) 
        {
            if (  bitiks[i] != bits[i] ) 
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
