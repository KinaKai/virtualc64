// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v2
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "C64.h"

SIDBridge::SIDBridge(C64 &ref) : C64Component(ref)
{
	setDescription("SIDBridge");
        
    subComponents = vector<HardwareComponent *> {
        
        &resid[0],
        &resid[1],
        &resid[2],
        &resid[3],
        &fastsid[0],
        &fastsid[1],
        &fastsid[2],
        &fastsid[3]
    };
    
    config.engine = ENGINE_RESID;
    config.enabled = 1;
    
    for (int i = 0; i < 4; i++) {
        resid[i].setClockFrequency(PAL_CLOCK_FREQUENCY);
        fastsid[i].setClockFrequency(PAL_CLOCK_FREQUENCY);
    }
}

void
SIDBridge::_reset()
{
    RESET_SNAPSHOT_ITEMS
    
    clearRingbuffer();
}

long
SIDBridge::getConfigItem(ConfigOption option)
{
    switch (option) {
            
        case OPT_SID_REVISION:
            return config.revision;
            
        case OPT_SID_FILTER:
            return config.filter;
            
        case OPT_SID_ENGINE:
            return config.engine;
            
        case OPT_SID_SAMPLING:
            return config.sampling;
            
        case OPT_AUDVOLL:
            return config.volL;

        case OPT_AUDVOLR:
            return config.volR;
            
        default:
            assert(false);
    }
}

long
SIDBridge::getConfigItem(ConfigOption option, long id)
{
    
    switch (option) {
            
        case OPT_SID_ENABLE:
            return GET_BIT(config.enabled, id);
            
        case OPT_SID_ADDRESS:
            return config.address[id];
            
        case OPT_AUDVOL:
            return config.vol[id];

        case OPT_AUDPAN:
            return config.pan[id];
                        
        default:
            assert(false);
    }
}

bool
SIDBridge::setConfigItem(ConfigOption option, long value)
{
    bool wasMuted = isMuted();
        
    switch (option) {
            
        case OPT_VIC_REVISION:
        {
            u32 newFrequency = VICII::getFrequency((VICRevision)value);
                                    
            suspend();
            setClockFrequency(newFrequency);
            resume();
            
            return true;
        }
            
        case OPT_SID_REVISION:
            
            if (!isSIDRevision(value)) {
                warn("Invalid SID revision: %d\n", value);
                return false;
            }
            if (config.revision == value) {
                return false;
            }
            
            suspend();
            config.revision = (SIDRevision)value;
            setRevision((SIDRevision)value);
            resume();
            
            return true;
            
        case OPT_SID_FILTER:
            
            if (config.filter == value) {
                return false;
            }

            suspend();
            config.filter = value;
            setAudioFilter(value);
            resume();
            
            return true;
            
        case OPT_SID_ENGINE:
            
            if (!isAudioEngine(value)) {
                warn("Invalid SID engine: %d\n", value);
                return false;
            }
            if (config.engine == value) {
                return false;
            }
            suspend();
            config.engine = (SIDEngine)value;
            resume();
            
            return true;
            
        case OPT_SID_SAMPLING:
            
            if (!isSamplingMethod(value)) {
                warn("Invalid sampling method: %d\n", value);
                return false;
            }
            if (config.sampling == value) {
                return false;
            }
            suspend();
            config.sampling = (SamplingMethod)value;
            setSamplingMethod((SamplingMethod)value);
            resume();
            
            return true;
            
        case OPT_AUDVOLL:
            
            config.volL = MIN(100, MAX(0, value));
            volL = pow((double)config.volL / 50, 1.4);

            if (wasMuted != isMuted()) {
                messageQueue.put(isMuted() ? MSG_MUTE_ON : MSG_MUTE_OFF);
            }
            return true;
            
        case OPT_AUDVOLR:

            config.volR = MIN(100, MAX(0, value));
            volR = pow((double)config.volR / 50, 1.4);

            if (wasMuted != isMuted()) {
                messageQueue.put(isMuted() ? MSG_MUTE_ON : MSG_MUTE_OFF);
            }
            return true;
            
        default:
            return false;
    }
}

bool
SIDBridge::setConfigItem(ConfigOption option, long id, long value)
{
    bool wasMuted = isMuted();

    switch (option) {
                     
        case OPT_SID_ENABLE:
                                  
            // The built-in SID can't be disabled
            if (id == 0 && value == false) {
                warn("SID 0 can't be disabled.\n");
                return false;
            }
            
            assert(id >= 0 && id <= 3);
            if (!!GET_BIT(config.enabled, id) == value) {
                return false;
            }
            
            suspend();
            REPLACE_BIT(config.enabled, id, value);
            clearSampleBuffer(id);
            
            for (int i = 0; i < 4; i++) {
                resid[i].reset();
                fastsid[i].reset();
            }
            resume();
            return true;
            
        case OPT_SID_ADDRESS:

            assert(id >= 0 && id <= 3);
            if (value < 0xD400 || value > 0xD7E0 || (value & 0x1F)) {
                warn("Invalid SID address: %x\n", value);
                warn("       Valid values: D400, D420, ... D7E0\n");
                return false;
            }

            if (config.address[id] == value) {
                return false;
            }
            
            suspend();
            config.address[id] = value;
            clearSampleBuffer(id);
            debug("config.address[%d] = %x\n", id, config.address);
            resume();
            return true;
            
        case OPT_AUDVOL:
            
            assert(id >= 0 && id <= 3);

            config.vol[id] = MIN(100, MAX(0, value));
            vol[id] = pow((double)config.vol[id] / 100, 1.4) * 0.0000025;

            if (wasMuted != isMuted()) {
                messageQueue.put(isMuted() ? MSG_MUTE_ON : MSG_MUTE_OFF);
            }

            return true;
            
        case OPT_AUDPAN:
            
            assert(id >= 0 && id <= 3);
            if (value < 0 || value > 200) {
                warn("Invalid pan: %d\n", value);
                warn("       Valid values: 0 ... 200\n");
                return false;
            }

            config.pan[id] = value; //  MAX(0.0, MIN(value / 100.0, 1.0));
            
            if (value <= 50) pan[id] = (50 + value) / 100.0;
            else if (value <= 150) pan[id] = (150 - value) / 100.0;
            else if (value <= 200) pan[id] = (value - 150) / 100.0;
            return true;

        default:
            return false;
    }
}

bool
SIDBridge::isMuted()
{
    if (config.volL == 0 && config.volR == 0) return true;
    
    return
    config.vol[0] == 0 &&
    config.vol[1] == 0 &&
    config.vol[2] == 0 &&
    config.vol[3] == 0;
}

u32
SIDBridge::getClockFrequency()
{
    u32 result = resid[0].getClockFrequency();
    
    for (int i = 0; i < 4; i++) {
        assert(resid[i].getClockFrequency() == result);
        assert(fastsid[i].getClockFrequency() == result);
    }
    
    return result;
}

void
SIDBridge::setClockFrequency(u32 frequency)
{
    debug(SID_DEBUG, "Setting clock frequency to %d\n", frequency);

    // suspend();
    
    for (int i = 0; i < 4; i++) {
        resid[i].setClockFrequency(frequency);
        fastsid[i].setClockFrequency(frequency);
    }
    
    // resume();
}

SIDRevision
SIDBridge::getRevision()
{
    SIDRevision result = resid[0].getRevision();
    
    for (int i = 0; i < 4; i++) {
        assert(resid[i].getRevision() == result);
        assert(fastsid[i].getRevision() == result);
    }
    
    return result;
}

void
SIDBridge::setRevision(SIDRevision revision)
{
    debug(SID_DEBUG, "Setting SID revision to %s\n", sidRevisionName(revision));

    for (int i = 0; i < 4; i++) {
        resid[i].setRevision(revision);
        fastsid[i].setRevision(revision);
    }
}

double
SIDBridge::getSampleRate()
{
    double result = resid[0].getSampleRate();
    
    for (int i = 0; i < 4; i++) {
        assert(resid[i].getSampleRate() == result);
        assert(fastsid[i].getSampleRate() == result);
    }
    
    return result;
}

void
SIDBridge::setSampleRate(double rate)
{
    debug(SID_DEBUG, "Setting sample rate to %f\n", rate);

    for (int i = 0; i < 4; i++) {
        resid[i].setSampleRate(rate);
        fastsid[i].setSampleRate(rate);
    }
}

bool
SIDBridge::getAudioFilter()
{
    bool result = resid[0].getAudioFilter();
    
    for (int i = 0; i < 4; i++) {
        assert(resid[i].getAudioFilter() == result);
        assert(fastsid[i].getAudioFilter() == result);
    }
    
    return result;
}

void
SIDBridge::setAudioFilter(bool enable)
{
    debug(SID_DEBUG, "%s audio filter\n", enable ? "Enabling" : "Disabling");

    for (int i = 0; i < 4; i++) {
        resid[i].setAudioFilter(enable);
        fastsid[i].setAudioFilter(enable);
    }
}

SamplingMethod
SIDBridge::getSamplingMethod()
{
    SamplingMethod result = resid[0].getSamplingMethod();
    
    for (int i = 0; i < 4; i++) {
        assert(resid[i].getSamplingMethod() == result);
        // Note: fastSID has no such option
    }
    
    return result;
}

void
SIDBridge::setSamplingMethod(SamplingMethod method)
{
    debug(SID_DEBUG, "Setting sampling method to %s\n",sidSamplingMethodName(method));

    for (int i = 0; i < 4; i++) {
        resid[i].setSamplingMethod(method);
        // Note: fastSID has no such option
    }
}

size_t
SIDBridge::didLoadFromBuffer(u8 *buffer)
{
    clearRingbuffer();
    return 0;
}

void
SIDBridge::_run()
{
    clearRingbuffer();
}

void
SIDBridge::_pause()
{
    clearRingbuffer();
}

void 
SIDBridge::_dump()
{
    _dump(0);
}

void
SIDBridge::_dump(int nr)
{
    SIDRevision residRev = resid[nr].getRevision();
    SIDRevision fastsidRev = fastsid[nr].getRevision();

    msg("ReSID:\n");
    msg("------\n");
    msg("    Chip model: %d (%s)\n", residRev, sidRevisionName(residRev));
    msg(" Sampling rate: %d\n", resid[nr].getSampleRate());
    msg(" CPU frequency: %d\n", resid[nr].getClockFrequency());
    msg("Emulate filter: %s\n", resid[nr].getAudioFilter() ? "yes" : "no");
    msg("\n");
    _dump(resid[nr].getInfo());

    msg("FastSID:\n");
    msg("--------\n");
    msg("    Chip model: %d (%s)\n", fastsidRev, sidRevisionName(fastsidRev));
    msg(" Sampling rate: %d\n", fastsid[nr].getSampleRate());
    msg(" CPU frequency: %d\n", fastsid[nr].getClockFrequency());
    msg("Emulate filter: %s\n", fastsid[nr].getAudioFilter() ? "yes" : "no");
    msg("\n");
    _dump(fastsid[nr].getInfo());
}

void
SIDBridge::_dump(SIDInfo info)
{
    u8 ft = info.filterType;
    msg("        Volume: %d\n", info.volume);
    msg("   Filter type: %s\n",
        (ft == FASTSID_LOW_PASS) ? "LOW PASS" :
        (ft == FASTSID_HIGH_PASS) ? "HIGH PASS" :
        (ft == FASTSID_BAND_PASS) ? "BAND PASS" : "NONE");
    msg("Filter cut off: %d\n\n", info.filterCutoff);
    msg("Filter resonance: %d\n\n", info.filterResonance);
    msg("Filter enable bits: %d\n\n", info.filterEnableBits);

    for (unsigned i = 0; i < 3; i++) {
        VoiceInfo vinfo = getVoiceInfo(i);
        u8 wf = vinfo.waveform;
        msg("Voice %d:       Frequency: %d\n", i, vinfo.frequency);
        msg("             Pulse width: %d\n", vinfo.pulseWidth);
        msg("                Waveform: %s\n",
            (wf == FASTSID_NOISE) ? "NOISE" :
            (wf == FASTSID_PULSE) ? "PULSE" :
            (wf == FASTSID_SAW) ? "SAW" :
            (wf == FASTSID_TRIANGLE) ? "TRIANGLE" : "NONE");
        msg("         Ring modulation: %s\n", vinfo.ringMod ? "yes" : "no");
        msg("               Hard sync: %s\n", vinfo.hardSync ? "yes" : "no");
        msg("             Attack rate: %d\n", vinfo.attackRate);
        msg("              Decay rate: %d\n", vinfo.decayRate);
        msg("            Sustain rate: %d\n", vinfo.sustainRate);
        msg("            Release rate: %d\n", vinfo.releaseRate);
    }
}

void
SIDBridge::_setWarp(bool enable)
{
    if (enable) {
        
        // Warping has the unavoidable drawback that audio playback gets out of
        // sync. To cope with this issue, we ramp down the volume when warping
        // is switched on and fade in smoothly when it is switched off.
        sid.rampDown();
        
    } else {
        
        sid.rampUp();
        sid.alignWritePtr();
    }
}

SIDInfo
SIDBridge::getInfo()
{
    SIDInfo info;
    
    switch (config.engine) {
            
        case ENGINE_FASTSID: info = fastsid[0].getInfo(); break;
        case ENGINE_RESID:   info = resid[0].getInfo(); break;
    }
    
    info.potX = mouse.readPotX();
    info.potY = mouse.readPotY();
    
    return info;
}

VoiceInfo
SIDBridge::getVoiceInfo(unsigned voice)
{
    VoiceInfo info;
    
    switch (config.engine) {
            
        case ENGINE_FASTSID: info = fastsid[0].getVoiceInfo(voice); break;
        case ENGINE_RESID:   info = resid[0].getVoiceInfo(voice); break;
    }
    
    return info;
}

void
SIDBridge::rampUp()
{
    // Only proceed if the emulator is not running in warp mode
    if (warpMode) return;
    
    volume.target = Volume::maxVolume;
    volume.delta = 3;
    ignoreNextUnderOrOverflow();
}

void
SIDBridge::rampUpFromZero()
{
    volume.current = 0;
    rampUp();
}
 
void
SIDBridge::rampDown()
{
    volume.target = 0;
    volume.delta = 50;
    ignoreNextUnderOrOverflow();
}

int
SIDBridge::mappedSID(u16 addr)
{
    addr &= 0xFFE0;
    
    if (isEnabled(1) && addr == config.address[1]) return 1;
    if (isEnabled(2) && addr == config.address[2]) return 2;
    if (isEnabled(3) && addr == config.address[3]) return 3;

    return 0;
}

u8 
SIDBridge::peek(u16 addr)
{
    // Get SIDs up to date
    executeUntil(cpu.cycle);
 
    // Select the target SID
    int sidNr = config.enabled > 1 ? mappedSID(addr) : 0;

    addr &= 0x1F;

    if (sidNr == 0) {
        if (addr == 0x19) return mouse.readPotX();
        if (addr == 0x1A) return mouse.readPotY();
    }
    
    switch (config.engine) {
        case ENGINE_FASTSID: return fastsid[sidNr].peek(addr);
        case ENGINE_RESID:   return resid[sidNr].peek(addr);
    }
    
    assert(false);
    return 0;
}

u8
SIDBridge::spypeek(u16 addr)
{
    return peek(addr);
}

void 
SIDBridge::poke(u16 addr, u8 value)
{
    // Get SID up to date
    executeUntil(cpu.cycle);
 
    // Select the target SID
    int sidNr = config.enabled > 1 ? mappedSID(addr) : 0;

    addr &= 0x1F;
    
    // Keep both SID implementations up to date
    resid[sidNr].poke(addr, value);
    fastsid[sidNr].poke(addr, value);
    
    // Run ReSID for at least one cycle to make pipelined writes work
    if (config.engine != ENGINE_RESID) {
        for (int i = 0; i < 4; i++) resid[i].clock();
    }
}

void
SIDBridge::executeUntil(u64 targetCycle)
{
    u64 missingCycles = targetCycle - cycles;
    
    if (missingCycles > PAL_CYCLES_PER_SECOND) {
        debug(SID_DEBUG, "Far too many SID cycles missing.\n");
        missingCycles = PAL_CYCLES_PER_SECOND;
    }
    
    execute(missingCycles);
    cycles = targetCycle;
}

void
SIDBridge::execute(u64 numCycles)
{
    if (numCycles == 0) return;
  
    // Check for a buffer underflow
    if (signalUnderflow) {
        signalUnderflow = false;
        handleBufferUnderflow();
    }

    //
    // Synthesize samples
    //
    
    u64 numSamples;
    
    switch (config.engine) {
            
        case ENGINE_FASTSID:

            // Run the primary SID (which is always enabled)
            numSamples = fastsid[0].execute(numCycles);
            
            // Run all other SIDS (if any)
            if (config.enabled > 1) {
                for (int i = 1; i < 4; i++) {
                    if (isEnabled(i)) {
                        u64 numSamples2 = fastsid[i].execute(numCycles);
                        assert(numSamples2 == numSamples);
                    }
                }
            }
            break;

        case ENGINE_RESID:

            // Run the primary SID (which is always enabled)
            numSamples = resid[0].execute(numCycles);
            
            // Run all other SIDS (if any)
            if (config.enabled > 1) {
                for (int i = 1; i < 4; i++) {
                    if (isEnabled(i)) {
                        u64 numSamples2 = resid[i].execute(numCycles);
                        if (numSamples2 != numSamples) {
                            warn("SID sample mismatch %d %d\n", numSamples, numSamples2);
                            _dump(0);
                            _dump(1);
                            assert(false);
                        }
                    }
                }
            }
            break;

        default:
            assert(false);
    }
    
    //
    // Mix channels
    //
    
    stream.lock();
    
    // Check for buffer overflow
    if (stream.free() < numSamples) {
        handleBufferOverflow();
    }
    
    // Adjust volume
    /*
    if (volume != targetVolume) {
        if (volume < targetVolume) {
            volume += MIN(volumeDelta, targetVolume - volume);
        } else {
            volume -= MIN(volumeDelta, volume - targetVolume);
        }
    }
    */
    
    // Convert sound samples to floating point values and write into ringbuffer
    for (unsigned i = 0; i < numSamples; i++) {
        
        float ch0, ch1, ch2, ch3, l, r;
        
        ch0 = (float)samples[0][i] * vol[0];
        ch1 = (float)samples[1][i] * vol[1];
        ch2 = (float)samples[2][i] * vol[2];
        ch3 = (float)samples[3][i] * vol[3];

        // Compute left channel output
        l =
        ch0 * (1 - pan[0]) + ch1 * (1 - pan[1]) +
        ch2 * (1 - pan[2]) + ch3 * (1 - pan[3]);

        // Compute right channel output
        r =
        ch0 * pan[0] + ch1 * pan[1] +
        ch2 * pan[2] + ch3 * pan[3];

        // Apply master volume
        l *= config.volL;
        r *= config.volR;

        stream.write(SamplePair { l, r } );
    }
    
    stream.unlock();
}

void
SIDBridge::clearSampleBuffer(long nr)
{
    memset(samples[nr], 0, sizeof(samples[nr]));
}

void
SIDBridge::clearRingbuffer()
{
    stream.clear();
    alignWritePtr();
}

float
SIDBridge::ringbufferData(size_t offset)
{
    SamplePair &pair = stream.current((int)offset);
    return (pair.left + pair.right) / 2.0;
}

void
SIDBridge::handleBufferUnderflow()
{
    // There are two common scenarios in which buffer underflows occur:
    //
    // (1) The consumer runs slightly faster than the producer.
    // (2) The producer is halted or not startet yet.
    
    debug(SID_DEBUG, "BUFFER UNDERFLOW (r: %ld w: %ld)\n", stream.r, stream.w);

    // Determine the elapsed seconds since the last pointer adjustment.
    u64 now = Oscillator::nanos();
    double elapsedTime = (double)(now - lastAlignment) / 1000000000.0;
    lastAlignment = now;

    // Adjust the sample rate, if condition (1) holds
    if (elapsedTime > 10.0) {
        
        bufferUnderflows++;
        
        // Increase the sample rate based on what we've measured
        int offPerSecond = (int)(samplesAhead / elapsedTime);
        setSampleRate(getSampleRate() + offPerSecond);
    }

    // Reset the write pointer
    alignWritePtr();
}

void
SIDBridge::handleBufferOverflow()
{
    // There are two common scenarios in which buffer overflows occur:
    //
    // (1) The consumer runs slightly slower than the producer
    // (2) The consumer is halted or not startet yet
    
    debug(SID_DEBUG, "BUFFER OVERFLOW (r: %ld w: %ld)\n", stream.r, stream.w);
    
    // Determine the elapsed seconds since the last pointer adjustment
    u64 now = Oscillator::nanos();
    double elapsedTime = (double)(now - lastAlignment) / 1000000000.0;
    lastAlignment = now;
    
    // Adjust the sample rate, if condition (1) holds
    if (elapsedTime > 10.0) {
        
        bufferOverflows++;
        
        // Decrease the sample rate based on what we've measured
        int offPerSecond = (int)(samplesAhead / elapsedTime);
        setSampleRate(getSampleRate() - offPerSecond);
    }
    
    // Reset the write pointer
    alignWritePtr();
}

void
SIDBridge::ignoreNextUnderOrOverflow()
{
    lastAlignment = Oscillator::nanos();
}

void
SIDBridge::copyMono(float *target, size_t n)
{
    stream.lock();
    
    // Check for a buffer underflow
    if (stream.count() < n) handleBufferUnderflow();

    // Copy sound samples
    stream.copyMono(target, n, volume.current, volume.target, volume.delta);
    
    stream.unlock();
}

void
SIDBridge::copyStereo(float *target1, float *target2, size_t n)
{
    stream.lock();
    
    // Check for a buffer underflow
    if (stream.count() < n) handleBufferUnderflow();

    // Copy sound samples
    stream.copy(target1, target2, n, volume.current, volume.target, volume.delta);
    
    stream.unlock();
}

void
SIDBridge::copyInterleaved(float *target, size_t n)
{
    stream.lock();
    
    // Check for a buffer underflow
    if (stream.count() < n) handleBufferUnderflow();

    // Read sound samples
    stream.copyInterleaved(target, n, volume.current, volume.target, volume.delta);
    
    stream.unlock();
}
