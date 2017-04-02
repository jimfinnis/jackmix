/**
 * @file process.h
 * @brief Encapsulates audio processing and the ring buffers
 * for communicating between the main and audio threads. Note
 * that everything is static.
 *
 */

#ifndef __PROCESS_H
#define __PROCESS_H

#include "ringbuffer.h"
#include "monitor.h"
#include "proccmds.h"

struct Process {
    static jack_client_t *client;

    static volatile bool parsedAndReady;
    // process thread -> main thread, monitoring data
    static RingBuffer<MonitorData> monring;
    // main thread -> process thread, commands
    static RingBuffer<ProcessCommand> moncmdring;
    /// sample rate 
    static uint32_t samprate;
    
    /// peak monitors for the master channel
    static PeakMonitor masterMonL,masterMonR;
    // have to be ptrs so they get registered
    static Value *masterPan,*masterGain;
    
    // initialise data
    static void init();
    
    // initialise jack
    static void initJack();
    
    // force a shutdown
    static void shutdown();
    
    // poll the monitoring ring from the main thread; gives
    // the last block of data from the ring. returns false if
    // no data present.
    static bool pollMonRing(MonitorData *p);
    
    /// add a command to be communicated to the process thread.
    /// Actually queues commands to be sent with sendCmds(),
    /// which is done in the display thread.
    static void writeCmd(ProcessCommand cmd);
    
    /// called from the display thread - copies commands into the process queue
    /// and blocks that thread until processing is done.
    static void sendCmds();
    
    
    
    
    // handle a command coming in on the command ring buffer
    static void processCommand(ProcessCommand& c);
    
    
    // sample rate change callback
    static int callbackSrate(jack_nframes_t nframes, void *arg);
    
    // callback for when jack shuts down
    static void callbackShutdown(void *arg);
        

    // this is called repeatedly by process to do the mixing.
    static void subproc(float *left,float *right,
                        jack_nframes_t offset,
                        jack_nframes_t n);
    
    // the main process - static so it's just a function and can
    // be used as a callback
    static int callbackProcess(jack_nframes_t nframes, void *arg);
};

#endif /* __PROCESS_H */
