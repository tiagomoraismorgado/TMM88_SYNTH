#ifndef DSP_H
#define DSP_H

/**
 * DSP Framework - Unified Header
 * 
 * Include this single header to access all components:
 *   #include "dsp.h"
 */

#include "tmm88/framework.h"
#include "synthesis/oscillator.h"
#include "synthesis/vca.h"
#include "synthesis/stereo_panner.h"
#include "synthesis/sub_oscillator.h"
#include "filters/lpf.h"
#include "effects/adsr.h"
#include "effects/overdrive.h"
#include "effects/crystals_delay.h"
#include "effects/eventide_filter.h"
#include "effects/tc_dynamics.h"
#include "effects/vss_reverb.h"
#include "midi/midi_handler.h"
#include "midi/osc_handler.h"
#include "midi/spectral_delay.h"

#endif // DSP_H