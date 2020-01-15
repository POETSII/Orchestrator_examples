#include "Supervisor.h"

#ifdef _APPLICATION_SUPERVISOR_

extern "C"
{
/* note that by enclosing the entire extern declaration in braces, variables
   still have internal linkage. The alternative, no braces around the extern
   declaration, makes the variables external as well. May need some experimentation
   if this doesn't work.
*/
int SupervisorCall(PMsg_p* In, PMsg_p* Out)
{
  int numMsgs = 0;
  int sentErr = 0;
  char outMsgBuf[P_MSG_MAX_SIZE];
  P_Sup_Msg_t* devMsg = In->Get<P_Sup_Msg_t>(0, numMsgs); // decode the message
  if (!devMsg) return -1;                                 // bad message
  for (int msg=0; msg < numMsgs; msg++)
  {
      // run as necessary any OnReceive handler
      if (devMsg[msg].header.destPin >= Supervisor::inputs.size()) return -1;  // invalid pin
      supInputPin* dest = Supervisor::inputs[devMsg[msg].header.destPin];
      // more thought needed about accumulation of error messages vs. send indications
      sentErr += dest->OnReceive(dest->properties, dest->state, &devMsg[msg], Out, outMsgBuf);
  }
  return sentErr;
}

int SupervisorExit()
{
  for (vector<supInputPin*>::iterator ipin = Supervisor::inputs.begin(); ipin != Supervisor::inputs.end(); ipin++)
      delete *ipin;
  for (vector<supOutputPin*>::iterator opin = Supervisor::outputs.begin(); opin != Supervisor::outputs.end(); opin++)
      delete *opin;
  return 0;
} 
}

#else
extern "C"
{
int SupervisorCall(PMsg_p* In, PMsg_p* Out)
{
  return -1;
}

int SupervisorExit()
{
  return 0;
}

int SupervisorInit()
{
  return 0;
}
}

#endif

supInputPin::supInputPin(Sup_OnReceive_t recvHandler, 
                         Sup_PinTeardown_t pinTeardown, 
                         const void* props, void* st)
{
      OnReceive = recvHandler;
      PinTeardown = pinTeardown;
      properties = props;
      state = st;
}

supInputPin::~supInputPin()
{
    if(PinTeardown) PinTeardown(properties, state);
    //if (properties) delete properties;
    //if (state) delete state;
}

supOutputPin::supOutputPin(Sup_OnSend_t sendHandler)
{
      OnSend = sendHandler;
}

supOutputPin::~supOutputPin()
{
}


          #include <fstream>
          #include <iostream>
          #include <stdint.h>
          #include <map>
          #include <vector>
          
          #define USEDEBUG
          //#define VERBOSEDEBUG
          
          #ifdef USEDEBUG
            #define DEBUG_PRINT(x) std::cout << std::setprecision(2) << x << std::endl
          #else
            #define DEBUG_PRINT(x)
          #endif
          
          #ifdef VERBOSEDEBUG
            #define VERBOSE_PRINT(x) std::cout << std::setprecision(2) << x << std::endl
          #else
            #define VERBOSE_PRINT(x)
          #endif
          
          map<uint32_t, vector<uint32_t>*> spikeRaster;          

        

vector<supInputPin*> Supervisor::inputs;
vector<supOutputPin*> Supervisor::outputs;

unsigned super_InPin_SpikeReceiver_Recv_handler (const void* pinProps, void* pinState, const P_Sup_Msg_t* inMsg, PMsg_p* outMsg, void* msgBuf)
{
   const super_InPin_SpikeReceiver_props_t* sEdgeProperties = static_cast<const super_InPin_SpikeReceiver_props_t*>(pinProps);
   super_InPin_SpikeReceiver_state_t* sEdgeState = static_cast<super_InPin_SpikeReceiver_state_t*>(pinState);
   const s_msg_spike_pyld_t* message = static_cast<const s_msg_spike_pyld_t*>(static_cast<const void*>(inMsg->data));

      if (message->srcAddr > sEdgeProperties->maxIdx)
      {
	 std::cout << "Error: Received a putative message from a neuron ID " << message->srcAddr << " but highest index is " << sEdgeProperties->maxIdx << std::endl;
	 return 0;
      }
      if (message->srcAddr < sEdgeProperties->minIdx)
      {
	 std::cout << "Error: Received a putative message from a neuron ID " << message->srcAddr << " but lowest index is " << sEdgeProperties->minIdx << std::endl;
	 return 0;
      }
      if (sEdgeState->numFinished > sEdgeProperties->numNeurons)
      {
	 std::cout << "Error: Received an unexpected message from neuron ID " << message->srcAddr << " at time " << message->tick << " when the simulation was complete" << std::endl;
	 return 0;
      }
      if (message->tick > sEdgeProperties->duration+1)
      {
         std::cout << "Error: Received a message from neuron ID " <<  message->srcAddr << " at time " << message->tick << " beyond the end of the simulation tick " << sEdgeProperties->duration << std::endl;
         std::cout.flush();
	 return 0;
      }
      std::cout << "Received a spike from neuron ID " << message->srcAddr << " at time " << message->tick << std::endl;	 
      vector<uint32_t>* spikeTimes = spikeRaster[message->srcAddr];
      if (spikeTimes == 0)
      {
	 #ifdef USEDEBUG
	 std::cout << "New neuron " << message->srcAddr << " reporting" << std::endl;
         #endif
	 spikeRaster[message->srcAddr] = (spikeTimes = new vector<uint32_t>);
      }	 
      if (message->tick >= sEdgeProperties->duration)
      {
 	 #ifdef USEDEBUG
	 std::cout << "Neuron " << message->srcAddr << " has reached the end of the simulation" << std::endl;
         #endif
	 if (++sEdgeState->numFinished == sEdgeProperties->numNeurons)
	 {
	    #ifdef USEDEBUG
	    std::cout << "All neurons have reached the end of the simulation. Dumping spikes to file" << std::endl;
            #endif
	    std::ofstream rasterFile("spike_times.csv", ios_base::out|ios_base::trunc);
	    for (map<uint32_t, vector<uint32_t>*>::iterator nu = spikeRaster.begin(); nu != spikeRaster.end(); nu++)
	    {
	        for (vector<uint32_t>::iterator t = nu->second->begin(); t != nu->second->end(); t++)
		{
		    rasterFile << nu->first << ", " << *t << std::endl; 
		}
		delete nu->second;
	    }
	    spikeRaster.clear();
	    rasterFile.close();
	    std::cout << "Simulation complete." << std::endl;
	    ++sEdgeState->numFinished;
	 }
	 return 0;
      }
      else
      {
         #ifdef USEDEBUG
         std::cout << "Recording spike at time " << message->tick << " for neuron " << message->srcAddr << std::endl;
         #endif
         spikeTimes->push_back(message->tick);
	 #ifdef USEDEBUG
         std::cout << "Spike recorded" << std::endl;
         #endif
      }
      return 0;
          
   return 0;
}

unsigned super_InPin_SpikeReceiver_PinTeardown (const void* pinProps, void* pinState)
{
    delete static_cast<const super_InPin_SpikeReceiver_props_t*>(pinProps);
    delete static_cast<super_InPin_SpikeReceiver_state_t*>(pinState);
    return 0;
}



extern "C"{
int SupervisorInit()
{
std::cout << "Starting Application Supervisor for application Synfire_Ring_2_2" << std::endl;
Supervisor::inputs.push_back(new supInputPin(&super_InPin_SpikeReceiver_Recv_handler,&super_InPin_SpikeReceiver_PinTeardown,new const super_InPin_SpikeReceiver_props_t {1070, 10, 19, 0},new super_InPin_SpikeReceiver_state_t {0}));
return 0;
}
}

