#ifndef SOLVER_HELPERS_HPP
#define SOLVER_HELPERS_HPP

#define _USE_MATH_DEFINES
#include <vector>
#include <math.h>
#include <memory>

namespace {
template <typename T>
inline void hannWindowMultiply(std::vector<T>& inputData)
{
    const unsigned int length = inputData.length();
    /* HANNING WINDOW */
    for(int i = 0; i < length; i++)
    {
        inputData[i] *= (0.5 * (1 - cos(2*M_PI*i/length)));
    }
}

template <typename T>
inline std::vector<T> hannWindow(const unsigned int length)
{
    std::vector<T> hann(length);
    /* HANNING WINDOW */
    for(int i = 0; i < length; i++)
    {
        hann[i] = (0.5 * (1 - cos(2*M_PI*i/length)));
    }
    return hann;
}

struct timeVec
{
    float currTime = 0;
    float dt;
    
    timeVec(float dt_s) : dt(dt_s) {};
    
    timeVec& operator++()
    {
        currTime += dt;
        return *this;
    }
};
}


inline std::unique_ptr<std::vector<float>> getExtortionSingleSin() {
    constexpr float simTime{100e-5}, dt{1e-8};
    constexpr uint numOfTimeSteps = static_cast<uint>(simTime/dt);
    auto pExtortion = std::make_unique<std::vector<float>> (std::vector<float>(numOfTimeSteps, 0.0f));
    unsigned int length = ((1/1.75e5) * 14)/dt;
//    length=numOfTimeSteps/3;
    timeVec timeValue(dt);
    for(auto it = (*pExtortion.get()).begin(); it != (*pExtortion.get()).begin()+length; it++)
    {
        *it = 1e-6 * sin(2*M_PI*1.75e5*timeValue.currTime);
        ++timeValue;
    }
    //Multiply though hanning window
    std::vector<float> hann = hannWindow<float>(length);
    std::transform(
       (*pExtortion.get()).begin(), (*pExtortion.get()).begin()+length,
       hann.begin(),
       (*pExtortion.get()).begin(), std::multiplies<float>());
    
    return pExtortion;
}

inline std::unique_ptr<std::vector<float>> getExtortionWaveMix() {
    constexpr float simTime{100e-5}, dt{1e-8};
    constexpr uint numOfTimeSteps = static_cast<uint>(simTime/dt);
    auto pExtortion = std::make_unique<std::vector<float>> (std::vector<float>(numOfTimeSteps, 0.0f));
    constexpr float f1 = 68e3, f2 = 285e3, amplitude = 1e-6;
    
    timeVec timeValue(dt);
    for(auto it = (*pExtortion.get()).begin(); it != (*pExtortion.get()).begin()+numOfTimeSteps; it++)
    {
        *it =  1e-6 * sin(2*M_PI*f1*timeValue.currTime) + amplitude * sin(2*M_PI*f2*timeValue.currTime);
        ++timeValue;
    }
    return pExtortion;
}

inline std::unique_ptr<std::vector<float>> getExtortionLG() {
    constexpr float simTime{100e-5}, dt{1e-8};
    constexpr uint numOfTimeSteps = static_cast<uint>(simTime/dt);
    auto pExtortion = std::make_unique<std::vector<float>> (std::vector<float>(numOfTimeSteps, 0.0f));
    constexpr float f1 = 185e3, f2 = 750e3, fm = 41e3, amplitude = 1e-6;
    
    timeVec timeValue(dt);

    int version = 1;    // 0 - normal, 1 - amplitude studies
    
    switch (version) {
        case 1:
        {
            auto pDisturbing = std::make_unique<std::vector<float>> (std::vector<float>(numOfTimeSteps, 0.0f));
            auto pWanted = std::make_unique<std::vector<float>> (std::vector<float>(numOfTimeSteps, 0.0f));
            for(int i {0}; i<numOfTimeSteps; i++)
            {
                pDisturbing->at(i) = (1 + 1.5 * sin(2* M_PI * fm * timeValue.currTime))*cos(2 * M_PI * f2 * timeValue.currTime);
                pWanted->at(i) = sin(2 * M_PI * f1 * timeValue.currTime);
                ++timeValue;
            }
            // Scale
            auto scale = [](std::unique_ptr<std::vector<float>>& v, const float amp){
                const float maxVal = *(std::max_element(v->begin(), v->end()));
                std::for_each(v->begin(), v->end(), [maxVal, amp](float& f){
                    f /= maxVal;
                    f *= amp;
                });
            };
            constexpr float ampDisturbing = 1e-6, ampWanted = 1e-6;
            scale(pDisturbing,ampDisturbing);
            scale(pWanted,ampWanted);
            // Combine
            int i = 0;
            for(auto it = (*pExtortion.get()).begin(); it != (*pExtortion.get()).begin()+numOfTimeSteps; it++, i++)
            {
                *it = pDisturbing->at(i) + pWanted->at(i);
            }
        }
        break;
        default:
            // Normal version
            for(auto it = (*pExtortion.get()).begin(); it != (*pExtortion.get()).begin()+numOfTimeSteps; it++)
            {
                // Modulation around f2
                *it = (1 + 1.5 * sin(2* M_PI * fm * timeValue.currTime))*cos(2 * M_PI * f2 * timeValue.currTime) +
                        sin(2 * M_PI * f1 * timeValue.currTime);
                // Modulation around f1
        //        *it = (1 + 1.5 * sin(2* M_PI * fm * timeValue.currTime))*cos(2 * M_PI * f1 * timeValue.currTime) +
        //                sin(2 * M_PI * f2 * timeValue.currTime);
                ++timeValue;
            }
            const float maxVal = *(std::max_element(pExtortion->begin(), pExtortion->end()));
            std::for_each(pExtortion->begin(), pExtortion->end(), [maxVal](float& f){
                f /= maxVal;
                f *= amplitude;
            });
        break;
    }
    
    
    return pExtortion;
}
    
    
#endif
