// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v2
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// This class is a wrapper around the third-party reSID library.
//
// List of modifications applied to reSID:
// 1. Changed visibility of some objects from protected to public
//
// Good candidate for testing sound emulation: INTERNAT.P00

#ifndef _RESID_H
#define _RESID_H

#include "C64Component.h"
#include "resid/sid.h"

class ReSID : public C64Component {

    // Reference to the connected bridge object
     SIDBridge &bridge;
    
    // Entry point to the reSID backend
    reSID::SID *sid;
    
    // Result of the latest inspection
    SIDInfo info;
    VoiceInfo voiceInfo[3];
    
public:
    
    void clock() { sid->clock(); }
    
private:
    
    // ReSID state
    reSID::SID::State st;
    
    // The emulated chip model
    SIDRevision model;
    
    // Clock frequency
    u32 clockFrequency;
    
    // Sample rate (usually set to 44.1 kHz)
    double sampleRate;
    
    // Sampling method
    SamplingMethod samplingMethod;
    
    // Switches filter emulation on or off
    bool emulateFilter;
    
    
    //
    // Constructing and serializing
    //
    
public:
    
	ReSID(C64 &ref, SIDBridge &bridgeref);
	~ReSID();
    
private:
    
    void _reset() override;

    
    //
    // Analyzing
    //
    
public:
    
    SIDInfo getInfo() { return HardwareComponent::getInfo(info); }
    VoiceInfo getVoiceInfo(unsigned nr) { return HardwareComponent::getInfo(voiceInfo[nr]); }
    
private:
    
    void _inspect() override;
    
    
    //
    // Serializing
    //
    
private:
    
    template <class T>
    void applyToPersistentItems(T& worker)
    {
        worker
        
        & sampleRate
        & emulateFilter
        
        & st.sid_register
        & st.bus_value
        & st.bus_value_ttl
        & st.write_pipeline
        & st.write_address
        & st.voice_mask
        & st.accumulator[0]
        & st.accumulator[1]
        & st.accumulator[2]
        & st.shift_register[0]
        & st.shift_register[1]
        & st.shift_register[2]
        & st.shift_register_reset[0]
        & st.shift_register_reset[1]
        & st.shift_register_reset[2]
        & st.shift_pipeline[0]
        & st.shift_pipeline[1]
        & st.shift_pipeline[2]
        & st.pulse_output[0]
        & st.pulse_output[1]
        & st.pulse_output[2]
        & st.floating_output_ttl[0]
        & st.floating_output_ttl[1]
        & st.floating_output_ttl[2]
        & st.rate_counter[0]
        & st.rate_counter[1]
        & st.rate_counter[2]
        & st.rate_counter_period[0]
        & st.rate_counter_period[1]
        & st.rate_counter_period[2]
        & st.exponential_counter[0]
        & st.exponential_counter[1]
        & st.exponential_counter[2]
        & st.exponential_counter_period[0]
        & st.exponential_counter_period[1]
        & st.exponential_counter_period[2]
        & st.envelope_counter[0]
        & st.envelope_counter[1]
        & st.envelope_counter[2]
        & st.envelope_state[0]
        & st.envelope_state[1]
        & st.envelope_state[2]
        & st.hold_zero[0]
        & st.hold_zero[1]
        & st.hold_zero[2]
        & st.envelope_pipeline[0]
        & st.envelope_pipeline[1]
        & st.envelope_pipeline[2];
    }
    
    template <class T>
    void applyToResetItems(T& worker)
    {
    }
    
    size_t _size() override { COMPUTE_SNAPSHOT_SIZE }
    size_t _load(u8 *buffer) override { LOAD_SNAPSHOT_ITEMS }
    size_t _save(u8 *buffer) override { SAVE_SNAPSHOT_ITEMS }
    size_t didLoadFromBuffer(u8 *buffer) override;
    size_t willSaveToBuffer(u8 *buffer) override;

    
    //
    // Methods from HardwareComponent
    //
    
public:
    
    void oldDidLoadFromBuffer(u8 **buffer) override { sid->write_state(st); }
    void oldWillSaveToBuffer(u8 **buffer) override { st = sid->read_state(); }

private:

    void _setClockFrequency(u32 value) override;
    
    
    
public:
    
	//! Special peek function for the I/O memory range.
	u8 peek(u16 addr);
	
	//! Special poke function for the I/O memory range.
	void poke(u16 addr, u8 value);
	
	/* Runs reSID for the specified amount of CPU cycles and writes the
     * generated sound samples into the internal ring buffer.
     */
    void execute(u64 cycles);
	

    // Configuring
    
    //! Returns the chip model
    SIDRevision getRevision() {
        assert((SIDRevision)sid->sid_model == model);
        return model;
    }
    
    //! Sets the chip model
    void setRevision(SIDRevision m);
    
    //! Returns the clock frequency
    u32 getClockFrequency() {
        assert((u32)sid->clock_frequency == clockFrequency);
        return (u32)sid->clock_frequency;
    }

    //! Returns the sample rate
    double getSampleRate() { return sampleRate; }
    
    //! Sets the sample rate
    void setSampleRate(double rate);
    
    //! Returns true iff audio filters should be emulated.
    bool getAudioFilter() { return emulateFilter; }
    
    //! Enable or disable audio filter emulation
	void setAudioFilter(bool enable);

    //! Get sampling method
    SamplingMethod getSamplingMethod() {
        assert((SamplingMethod)sid->sampling == samplingMethod);
        return samplingMethod;
    }
    
    //! Set sampling method
    void setSamplingMethod(SamplingMethod value);
};

#endif
