// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "config.h"
#include "Thread.h"
#include "Chrono.h"
#include <iostream>

Thread::Thread()
{
    // Initialize the sync timer
    targetTime = util::Time::now();
    
    // Start the thread and enter the main function
    thread = std::thread(&Thread::main, this);
}

Thread::~Thread()
{
    // Wait until the thread has terminated
    join();
}

template <> void
Thread::execute <Thread::SyncMode::Periodic> ()
{
    // Call the execution function
    loadClock.go();
    execute();
    loadClock.stop();
}

template <> void
Thread::execute <Thread::SyncMode::Pulsed> ()
{
    // Call the execution function
    loadClock.go();
    execute();
    loadClock.stop();
    
}

template <> void
Thread::sleep <Thread::SyncMode::Periodic> ()
{
    auto now = util::Time::now();

    // Only proceed if we're not running in warp mode
    if (warpMode) return;
        
    // Check if we're running too slow...
    if (now > targetTime) {
        
        // Check if we're completely out of sync...
        if ((now - targetTime).asMilliseconds() > 200) {
                        
            warn("Emulation is way too slow: %f\n",(now - targetTime).asSeconds());

            // Restart the sync timer
            targetTime = util::Time::now();
        }
    }
    
    // Check if we're running too fast...
    if (now < targetTime) {
        
        // Check if we're completely out of sync...
        if ((targetTime - now).asMilliseconds() > 200) {
            
            warn("Emulation is way too slow: %f\n",(targetTime - now).asSeconds());

            // Restart the sync timer
            targetTime = util::Time::now();
        }
    }
        
    // Sleep for a while
    // std::cout << "Sleeping... " << targetTime.asMilliseconds() << std::endl;
    // std::cout << "Delay = " << delay.asNanoseconds() << std::endl;
    targetTime += delay;
    targetTime.sleepUntil();
}

template <> void
Thread::sleep <Thread::SyncMode::Pulsed> ()
{
    // Wait for the next pulse
    if (!warpMode) waitForWakeUp();
}

void
Thread::main()
{
    debug(RUN_DEBUG, "main()\n");
          
    while (++loopCounter) {
           
        if (isRunning()) {
                        
            switch (mode) {
                case SyncMode::Periodic: execute<SyncMode::Periodic>(); break;
                case SyncMode::Pulsed: execute<SyncMode::Pulsed>(); break;
            }
        }
        
        if (!warpMode || isPaused()) {

            switch (mode) {
                case SyncMode::Periodic: sleep<SyncMode::Periodic>(); break;
                case SyncMode::Pulsed: sleep<SyncMode::Pulsed>(); break;
            }
        }
        
        // Are we requested to enter or exit warp mode?
        while (newWarpMode != warpMode) {
            
            C64Component::warpOnOff(newWarpMode);
            warpMode = newWarpMode;
            break;
        }

        // Are we requested to enter or exit warp mode?
        while (newDebugMode != debugMode) {
            
            C64Component::debugOnOff(newDebugMode);
            debugMode = newDebugMode;
            break;
        }

        // Are we requested to change state?
        while (newState != state) {
            
            if (state == EXEC_OFF && newState == EXEC_PAUSED) {
                
                C64Component::powerOn();
                state = newState;
                break;
            }

            if (state == EXEC_OFF && newState == EXEC_RUNNING) {
                
                C64Component::powerOn();
                C64Component::run();
                state = newState;
                break;
            }

            if (state == EXEC_PAUSED && newState == EXEC_OFF) {
                
                C64Component::powerOff();
                state = newState;
                break;
            }

            if (state == EXEC_PAUSED && newState == EXEC_RUNNING) {
                
                C64Component::run();
                state = newState;
                break;
            }

            if (state == EXEC_RUNNING && newState == EXEC_OFF) {
                
                C64Component::pause();
                C64Component::powerOff();
                state = newState;
                break;
            }

            if (state == EXEC_RUNNING && newState == EXEC_PAUSED) {
                
                C64Component::pause();
                state = newState;
                break;
            }
            
            if (newState == EXEC_HALTED) {
                
                C64Component::halt();
                state = newState;
                return;
            }
            
            // Invalid state transition
            fatalError;
            break;
        }
        
        // Compute the CPU load once in a while
        if (loopCounter % 32 == 0) {
            
            auto used  = loadClock.getElapsedTime().asSeconds();
            auto total = nonstopClock.getElapsedTime().asSeconds();
            
            cpuLoad = used / total;
            
            loadClock.restart();
            loadClock.stop();
            nonstopClock.restart();
        }
    }
}

void
Thread::setSyncDelay(util::Time newDelay)
{
    delay = newDelay;
}

void
Thread::setMode(SyncMode newMode)
{
    mode = newMode;
}

void
Thread::setWarpLock(bool value)
{
    warpLock = value;
}

void
Thread::setDebugLock(bool value)
{
    debugLock = value;
}

void
Thread::powerOn(bool blocking)
{
    debug(RUN_DEBUG, "powerOn()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (isPoweredOff()) {
        
        // Throw an exception if the emulator is not ready to power on
        isReady();

        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_PAUSED, blocking);
    }
}

void
Thread::powerOff(bool blocking)
{
    debug(RUN_DEBUG, "powerOff()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (!isPoweredOff()) {
                
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_OFF, blocking);
    }
}

void
Thread::run(bool blocking)
{
    debug(RUN_DEBUG, "run()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (!isRunning()) {
        
        // Throw an exception if the emulator is not ready to power on
        isReady();
        
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_RUNNING, blocking);
    }
}

void
Thread::pause(bool blocking)
{
    debug(RUN_DEBUG, "pause()\n");

    // Never call this function inside the emulator thread
    assert(!isEmulatorThread());
    
    if (isRunning()) {
                
        // Request a state change and wait until the new state has been reached
        changeStateTo(EXEC_PAUSED, blocking);
    }
}

void
Thread::halt(bool blocking)
{
    changeStateTo(EXEC_HALTED, blocking);
}

void
Thread::warpOn(bool blocking)
{
    if (!warpLock) changeWarpTo(true, blocking);
}

void
Thread::warpOff(bool blocking)
{
    if (!warpLock) changeWarpTo(false, blocking);
}

void
Thread::debugOn(bool blocking)
{
    if (!debugLock) changeDebugTo(true, blocking);
}

void
Thread::debugOff(bool blocking)
{
    if (!debugLock) changeDebugTo(false, blocking);
}

void
Thread::changeStateTo(ExecutionState requestedState, bool blocking)
{
    newState = requestedState;
    if (blocking) while (state != newState) { };
}

void
Thread::changeWarpTo(bool value, bool blocking)
{
    newWarpMode = value;
    if (blocking) while (warpMode != newWarpMode) { };
}

void
Thread::changeDebugTo(bool value, bool blocking)
{
    newDebugMode = value;
    if (blocking) while (debugMode != newDebugMode) { };
}

void
Thread::wakeUp()
{
    if (mode == SyncMode::Pulsed) wakeUp();
}
