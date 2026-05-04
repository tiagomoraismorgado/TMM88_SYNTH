#ifndef VSS_REVERB_H
#define VSS_REVERB_H

#include "tmm88/framework.h"
#include <vector>

enum class VSSRoomPreset {
    SmallRoom = 0,
    MediumHall,
    LargeHall,
    Plate,
    Cathedral,
    Ambience,
    Chamber,
    Room1,
    Room2,
    Hall1,
    Hall2,
    Studio,
    VocalPlate,
    DrumRoom,
    ConcertHall
};

class VSSProcessor : public AudioComponent {
public:
    VSSProcessor();
    virtual ~VSSProcessor() override = default;

    bool init() override;
    void cleanup() override;

    void setSampleRate(double rate);
    void setRoomSize(double size); // 0.1 (Closet) to 2.0 (Cathedral)
    void setDecayTime(double seconds);
    void setHighCut(double freq);
    void setPreset(VSSRoomPreset preset);
    VSSRoomPreset getPreset() const { return m_currentPreset; }
    void setAbsorption(double amount); // 0.0 (Pristine) to 1.0 (Dark/Absorbed)
    void setDryWet(double mix);

    void process(double inputL, double inputR, double& outL, double& outR);

private:
    double sampleRate_;
    double mix_;
    double decay_;
    double roomSize_;
    double absorption_;
    
    VSSRoomPreset m_currentPreset;
    std::vector<int> m_taps; // Delay times in samples
    std::vector<double> m_tapGains;

    // Early Reflection Taps (VSS Logic)
    std::vector<double> erBufferL_, erBufferR_;
    size_t erWritePos_;
    
    // Late Reverb Combs/All-Pass (Diffuse Tail)
    double combFilter[4][2]; // States for L/R
    double apFilter[2][2];   
    
    double highCut_;
    double filterStateL_, filterStateR_;
    double dampStateL_, dampStateR_;
};

#endif // VSS_REVERB_H