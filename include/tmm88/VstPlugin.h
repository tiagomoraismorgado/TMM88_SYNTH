#ifndef VST_PLUGIN_H
#define VST_PLUGIN_H

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "dsp.h"

class HybridVstPlugin : public Steinberg::Vst::AudioEffect {
public:
    HybridVstPlugin();
    static Steinberg::FUnknown* createInstance(void* context);

     Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) override;
     Steinberg::tresult PLUGIN_API process(Steinberg::Vst::ProcessData& data) override;
     Steinberg::tresult PLUGIN_API setState(Steinberg::IBStream* state) override;
     Steinberg::tresult PLUGIN_API getState(Steinberg::IBStream* state) override;

private:
    Oscillator m_osc{WaveformType::Sawtooth};
    EventideFilter m_filter;
    VCA m_vca;
    ADSR m_adsr;
    VSSProcessor m_reverb;
    CrystalsDelay m_crystals;
    TCDynamics m_comp;
};

#endif // VST_PLUGIN_H