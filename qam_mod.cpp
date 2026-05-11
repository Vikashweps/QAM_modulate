#include <stdio.h>
#include <complex>
#include <vector>
#include <math.h>
#include <iostream>
#include <random> 
#include <fstream>
#include <cmath>

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "third_party/implot/implot.h"
#include "third_party/imgui/backends/imgui_impl_sdl2.h"
#include "third_party/imgui/backends/imgui_impl_opengl3.h"
#include "third_party/imgui/imgui.h"

                 

void run_gui(const std::vector<std::complex<float>>& symbols,const std::vector<std::vector<std::complex<float>>>& all_awgn, 
            const std::vector<float>& dispers,const std::vector<float>& ber,int type_QAM)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    SDL_DisplayMode dm;
    SDL_GetCurrentDisplayMode(0, &dm);
    SDL_Window* win = SDL_CreateWindow("QAM Simulation", 0, 0, dm.w, dm.h,
                                       SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!win) return;
    SDL_GLContext gl = SDL_GL_CreateContext(win);
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(win, gl);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::vector<float> xs_orig(symbols.size()), ys_orig(symbols.size());
    for (size_t i = 0; i < symbols.size(); ++i) {
        xs_orig[i] = symbols[i].real();
        ys_orig[i] = symbols[i].imag();
    }

    int disp_idx = 0;
    bool running = true;
    std::vector<float> xs_noisy, ys_noisy;
    {
        const auto& noisy = all_awgn[disp_idx];
        xs_noisy.assign(noisy.size(), 0);
        ys_noisy.assign(noisy.size(), 0);
        for (size_t i = 0; i < noisy.size(); ++i) {
            xs_noisy[i] = noisy[i].real();
            ys_noisy[i] = noisy[i].imag();
        }
    }
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            ImGui_ImplSDL2_ProcessEvent(&e);
            if (e.type == SDL_QUIT || (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE))
                running = false;
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2(dm.w, dm.h));
        ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::BeginChild("Info", ImVec2(0, 50), true);
        ImGui::Text("Current QAM: %d (restart to change)", type_QAM);
        ImGui::EndChild();

        if (ImGui::BeginTabBar("Tabs")) {
            if (ImGui::BeginTabItem("Constellations")) {
                int prev_idx = -1;
               
                ImGui::BeginChild("DispersionButtons", ImVec2(0, 100), true, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::Text("Dispersion Buttons ");
                int btn_width = 100;
                int cols = std::max(1, (int)(ImGui::GetContentRegionAvail().x / (btn_width + 10)));
                ImGui::Columns(cols, "DispColumns", false);
                
                for (int idx = 0; idx < (int)dispers.size(); ++idx) {
                    // Формируем метку с кратким значением дисперсии
                    char label[32];
                    sprintf(label, "%.3f", dispers[idx]);
                    if (ImGui::Button(label, ImVec2(btn_width, 30))) {
                        prev_idx = disp_idx;    // запоминаем предыдущий индекс
                        disp_idx = idx;         // устанавливаем новый
                    }
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
                ImGui::EndChild();
                ImGui::Text("Dispersion = %.4f", dispers[disp_idx]);

                // Если индекс изменился, пересчитываем векторы зашумленного созвездия
                if (prev_idx != disp_idx) {
                    const auto& noisy = all_awgn[disp_idx];
                    xs_noisy.assign(noisy.size(), 0);
                    ys_noisy.assign(noisy.size(), 0);
                    for (size_t i = 0; i < noisy.size(); ++i) {
                        xs_noisy[i] = noisy[i].real();
                        ys_noisy[i] = noisy[i].imag();
                    }
                }

                float limit = 1.5f * sqrtf(float(type_QAM));
                ImVec2 sz(dm.w * 0.45f, dm.h * 0.7f);

                ImGui::BeginGroup();
                ImGui::Text("Original");
                if (ImPlot::BeginPlot("##orig", sz)) {
                    ImPlot::SetupAxes("I","Q");
                    ImPlot::SetupAxesLimits(-limit, limit, -limit, limit);
                    ImPlot::PlotScatter("##orig", xs_orig.data(), ys_orig.data(), (int)xs_orig.size());
                    ImPlot::EndPlot();
                }
                ImGui::EndGroup();
                ImGui::SameLine();
                ImGui::BeginGroup();
                ImGui::Text("Noisy (disp = %.3f)", dispers[disp_idx]);
                if (ImPlot::BeginPlot("##noisy", sz)) {
                    ImPlot::SetupAxes("I","Q");
                    ImPlot::SetupAxesLimits(-limit, limit, -limit, limit);
                    if (!xs_noisy.empty())
                        ImPlot::PlotScatter("##noisy", xs_noisy.data(), ys_noisy.data(), (int)xs_noisy.size());
                    ImPlot::EndPlot();
                }
                ImGui::EndGroup();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("BER")) {
                if (ImPlot::BeginPlot("BER vs Dispersion", ImVec2(-1,-1))) {
                    ImPlot::SetupAxes("Dispersion", "BER");
                    ImPlot::SetupAxisFormat(ImAxis_Y1, "%.2f");
                    ImPlot::PlotLine("BER", dispers.data(), ber.data(), (int)dispers.size());
                    ImPlot::PlotScatter("BER", dispers.data(), ber.data(), (int)dispers.size());
                    ImPlot::EndPlot();
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::End();

        ImGui::Render();
        glClearColor(0.1f,0.1f,0.1f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(win);
    SDL_Quit();
}


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

    std::vector<float>BER(dispers.size());
    std::vector<std::vector<std::complex<float>>> all_awgn(dispers.size());

    for ( int a = 0; a < dispers.size(); a++)
    {
        std::vector<std::complex<float>> signal_AWGN = AWGN(symbols, dispers[a]);
        all_awgn[a] = signal_AWGN;

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
        BER[a] += ber;
        
    
    }
    out.close();
    
    run_gui(symbols, all_awgn, dispers, BER, type_QAM);
    

}
