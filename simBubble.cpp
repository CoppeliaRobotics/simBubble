#include "simBubble.h"
#include <simLib/scriptFunctionData.h>
#include <iostream>
#include <simLib/simLib.h>

static LIBRARY simLib;

struct sBubbleRob
{
    int handle;
    int scriptHandle;
    int motorHandles[2];
    int sensorHandle;
    float backRelativeVelocities[2];
    bool run;
    float backMovementDuration;
};

static std::vector<sBubbleRob> allBubbleRobs;
static int nextBubbleRobHandle=0;

int getBubbleRobIndexFromHandle(int bubbleRobHandle)
{
    for (size_t i=0;i<allBubbleRobs.size();i++)
    {
        if (allBubbleRobs[i].handle==bubbleRobHandle)
            return(i);
    }
    return(-1);
}

int getBubbleRobIndexFromScriptHandle(int scriptHandle)
{
    for (size_t i=0;i<allBubbleRobs.size();i++)
    {
        if (allBubbleRobs[i].scriptHandle==scriptHandle)
            return(i);
    }
    return(-1);
}

// --------------------------------------------------------------------------------------
// simBubble.create
// --------------------------------------------------------------------------------------
const int inArgs_CREATE[]={
    3,
    sim_script_arg_int32|sim_script_arg_table,2,
    sim_script_arg_int32,0,
    sim_script_arg_float|sim_script_arg_table,2,
};

void LUA_CREATE_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    int handle=-1;
    if (D.readDataFromStack(cb->stackID,inArgs_CREATE,inArgs_CREATE[0],nullptr))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        sBubbleRob bubbleRob;
        handle=nextBubbleRobHandle++;
        bubbleRob.handle=handle;
        bubbleRob.scriptHandle=cb->scriptID;
        bubbleRob.motorHandles[0]=inData->at(0).int32Data[0];
        bubbleRob.motorHandles[1]=inData->at(0).int32Data[1];
        bubbleRob.sensorHandle=inData->at(1).int32Data[0];
        bubbleRob.backRelativeVelocities[0]=inData->at(2).floatData[0];
        bubbleRob.backRelativeVelocities[1]=inData->at(2).floatData[1];
        bubbleRob.run=false;
        allBubbleRobs.push_back(bubbleRob);
    }
    D.pushOutData(CScriptFunctionDataItem(handle));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simBubble.destroy
// --------------------------------------------------------------------------------------
const int inArgs_DESTROY[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_DESTROY_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_DESTROY,inArgs_DESTROY[0],nullptr))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        int index=getBubbleRobIndexFromHandle(handle);
        if (index>=0)
        {
            allBubbleRobs.erase(allBubbleRobs.begin()+index);
            success=true;
        }
        else
            simSetLastError(nullptr,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------
// simBubble.start
// --------------------------------------------------------------------------------------
const int inArgs_START[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_START_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_START,inArgs_START[0],nullptr))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        int index=getBubbleRobIndexFromHandle(handle);
        if (index!=-1)
        {
            allBubbleRobs[index].backMovementDuration=0.0f;
            allBubbleRobs[index].run=true;
            success=true;
        }
        else
            simSetLastError(nullptr,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simBubble.stop
// --------------------------------------------------------------------------------------
const int inArgs_STOP[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_STOP_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_STOP,inArgs_STOP[0],nullptr))
    {
        std::vector<CScriptFunctionDataItem>* inData=D.getInDataPtr();
        int handle=inData->at(0).int32Data[0];
        int index=getBubbleRobIndexFromHandle(handle);
        if (index!=-1)
        {
            allBubbleRobs[index].run=false;
            simSetJointTargetVelocity(allBubbleRobs[index].motorHandles[0],0.0f);
            simSetJointTargetVelocity(allBubbleRobs[index].motorHandles[1],0.0f);
            success=true;
        }
        else
            simSetLastError(nullptr,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------

SIM_DLLEXPORT int simInit(SSimInit* info)
{ // This is called just once, at the start of CoppeliaSim.
    simLib=loadSimLibrary(info->coppeliaSimLibPath);
    if (simLib==NULL)
    {
        simAddLog(info->pluginName,sim_verbosity_errors,"could not find all required functions in the CoppeliaSim library. Cannot start the plugin.");
        return(0); // Means error, CoppeliaSim will unload this plugin
    }
    if (getSimProcAddresses(simLib)==0)
    {
        simAddLog(info->pluginName,sim_verbosity_errors,"sorry, your CoppelisSim copy is somewhat old, CoppelisSim 4.0.0 rev1 or higher is required. Cannot start the plugin.");
        unloadSimLibrary(simLib);
        return(0); // Means error, CoppeliaSim will unload this plugin
    }

    // Register the new functions:
    simRegisterScriptCallbackFunction("create",nullptr,LUA_CREATE_CALLBACK);
    simRegisterScriptCallbackFunction("destroy",nullptr,LUA_DESTROY_CALLBACK);
    simRegisterScriptCallbackFunction("start",nullptr,LUA_START_CALLBACK);
    simRegisterScriptCallbackFunction("stop",nullptr,LUA_STOP_CALLBACK);

    return(13); // initialization went fine, we return the version number of this plugin (can be queried with simGetModuleName)
    // version 8 is for CoppeliaSim versions after CoppeliaSim 3.3.0 (using stacks for data exchange with scripts)
    // version 9 is for CoppeliaSim versions after CoppeliaSim 3.4.0 (new API notation)
    // version 10 is for CoppeliaSim versions after CoppeliaSim 4.1.0 (threads via coroutines)
    // version 11 is for CoppeliaSim versions after CoppeliaSim 4.2.0
    // version 12 is for CoppeliaSim versions after CoppeliaSim 4.3.0
    // version 13 is for CoppeliaSim versions after CoppeliaSim 4.5.1
}

SIM_DLLEXPORT void simCleanup()
{ // This is called just once, at the end of CoppeliaSim
    unloadSimLibrary(simLib); // release the library
}

SIM_DLLEXPORT void simMsg(SSimMsg* info)
{ // This is called quite often. Just watch out for messages/events you want to handle
    if ( (info->msgId==sim_message_eventcallback_simulationactuation)&&(info->auxData[0]==0) )
    { // the main script's actuation section is about to be executed
        float dt=simGetSimulationTimeStep();
        for (unsigned int i=0;i<allBubbleRobs.size();i++)
        {
            if (allBubbleRobs[i].run)
            { // movement mode
                if (simReadProximitySensor(allBubbleRobs[i].sensorHandle,NULL,NULL,NULL)>0)
                    allBubbleRobs[i].backMovementDuration=3.0f; // we detected an obstacle, we move backward for 3 seconds
                if (allBubbleRobs[i].backMovementDuration>0.0f)
                { // We move backward
                    simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[0],-7.0f*allBubbleRobs[i].backRelativeVelocities[0]);
                    simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[1],-7.0f*allBubbleRobs[i].backRelativeVelocities[1]);
                    allBubbleRobs[i].backMovementDuration-=dt;
                }
                else
                { // We move forward
                    simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[0],7.0f);
                    simSetJointTargetVelocity(allBubbleRobs[i].motorHandles[1],7.0f);
                }
            }
        }
    }

    if (info->msgId==sim_message_eventcallback_scriptstatedestroyed)
    { // script state was destroyed. Destroy all associated BubbleRob instances:
        int index=getBubbleRobIndexFromScriptHandle(info->auxData[0]);
        while (index>=0)
        {
            allBubbleRobs.erase(allBubbleRobs.begin()+index);
            index=getBubbleRobIndexFromScriptHandle(info->auxData[0]);
        }
    }
}
