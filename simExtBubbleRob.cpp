#include "simExtBubbleRob.h"
#include "scriptFunctionData.h"
#include <iostream>
#include "simLib.h"

#ifdef _WIN32
    #ifdef QT_COMPIL
        #include <direct.h>
    #else
        #include <shlwapi.h>
        #pragma comment(lib, "Shlwapi.lib")
    #endif
#endif
#if defined (__linux) || defined (__APPLE__)
    #include <unistd.h>
    #define WIN_AFX_MANAGE_STATE
#endif

#define CONCAT(x,y,z) x y z
#define strConCat(x,y,z)    CONCAT(x,y,z)

#define PLUGIN_NAME "BubbleRob"

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
// simExtBubble_create
// --------------------------------------------------------------------------------------
#define LUA_CREATE_COMMAND "simBubble.create"

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
    if (D.readDataFromStack(cb->stackID,inArgs_CREATE,inArgs_CREATE[0],LUA_CREATE_COMMAND))
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
// simExtBubble_destroy
// --------------------------------------------------------------------------------------
#define LUA_DESTROY_COMMAND "simBubble.destroy"

const int inArgs_DESTROY[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_DESTROY_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_DESTROY,inArgs_DESTROY[0],LUA_DESTROY_COMMAND))
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
            simSetLastError(LUA_DESTROY_COMMAND,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------
// simExtBubble_start
// --------------------------------------------------------------------------------------
#define LUA_START_COMMAND "simBubble.start"

const int inArgs_START[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_START_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_START,inArgs_START[0],LUA_START_COMMAND))
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
            simSetLastError(LUA_START_COMMAND,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// simExtBubble_stop
// --------------------------------------------------------------------------------------
#define LUA_STOP_COMMAND "simBubble.stop"

const int inArgs_STOP[]={
    1,
    sim_script_arg_int32,0,
};

void LUA_STOP_CALLBACK(SScriptCallBack* cb)
{
    CScriptFunctionData D;
    bool success=false;
    if (D.readDataFromStack(cb->stackID,inArgs_STOP,inArgs_STOP[0],LUA_STOP_COMMAND))
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
            simSetLastError(LUA_STOP_COMMAND,"Invalid BubbleRob handle.");
    }
    D.pushOutData(CScriptFunctionDataItem(success));
    D.writeDataToStack(cb->stackID);
}
// --------------------------------------------------------------------------------------

SIM_DLLEXPORT unsigned char simStart(void*,int)
{ // This is called just once, at the start of CoppeliaSim.
    // Dynamically load and bind CoppeliaSim functions:
    char curDirAndFile[1024];
#ifdef _WIN32
    #ifdef QT_COMPIL
        _getcwd(curDirAndFile, sizeof(curDirAndFile));
    #else
        GetModuleFileName(NULL,curDirAndFile,1023);
        PathRemoveFileSpec(curDirAndFile);
    #endif
#elif defined (__linux) || defined (__APPLE__)
    getcwd(curDirAndFile, sizeof(curDirAndFile));
#endif

    std::string currentDirAndPath(curDirAndFile);
    std::string temp(currentDirAndPath);

#ifdef _WIN32
    temp+="\\coppeliaSim.dll";
#elif defined (__linux)
    temp+="/libcoppeliaSim.so";
#elif defined (__APPLE__)
    temp+="/libcoppeliaSim.dylib";
#endif

    simLib=loadSimLibrary(temp.c_str());
    if (simLib==NULL)
    {
        printf("simExtBubbleRob: error: could not find or correctly load coppeliaSim.dll. Cannot start the plugin.\n"); // cannot use simAddLog here.
        return(0); // Means error, CoppeliaSim will unload this plugin
    }
    if (getSimProcAddresses(simLib)==0)
    {
        printf("simExtBubbleRob: error: could not find all required functions in coppeliaSim.dll. Cannot start the plugin.\n"); // cannot use simAddLog here.
        unloadSimLibrary(simLib);
        return(0); // Means error, CoppeliaSim will unload this plugin
    }

    // Register the new functions:
    simRegisterScriptCallbackFunction(strConCat(LUA_CREATE_COMMAND,"@",PLUGIN_NAME),strConCat("number bubbleRobHandle=",LUA_CREATE_COMMAND,"(table_2 motorJointHandles,number sensorHandle,table_2 backRelativeVelocities)"),LUA_CREATE_CALLBACK);
    simRegisterScriptCallbackFunction(strConCat(LUA_DESTROY_COMMAND,"@",PLUGIN_NAME),strConCat("boolean result=",LUA_DESTROY_COMMAND,"(number bubbleRobHandle)"),LUA_DESTROY_CALLBACK);
    simRegisterScriptCallbackFunction(strConCat(LUA_START_COMMAND,"@",PLUGIN_NAME),strConCat("boolean result=",LUA_START_COMMAND,"(number bubbleRobHandle)"),LUA_START_CALLBACK);
    simRegisterScriptCallbackFunction(strConCat(LUA_STOP_COMMAND,"@",PLUGIN_NAME),strConCat("boolean result=",LUA_STOP_COMMAND,"(number bubbleRobHandle)"),LUA_STOP_CALLBACK);

    return(11); // initialization went fine, we return the version number of this plugin (can be queried with simGetModuleName)
    // version 8 is for CoppeliaSim versions after CoppeliaSim 3.3.0 (using stacks for data exchange with scripts)
    // version 9 is for CoppeliaSim versions after CoppeliaSim 3.4.0 (new API notation)
    // version 10 is for CoppeliaSim versions after CoppeliaSim 4.1.0 (threads via coroutines)
    // version 11 is for CoppeliaSim versions after CoppeliaSim 4.2.0
}

SIM_DLLEXPORT void simEnd()
{ // This is called just once, at the end of CoppeliaSim
    unloadSimLibrary(simLib); // release the library
}

SIM_DLLEXPORT void* simMessage(int message,int* auxiliaryData,void* customData,int* replyData)
{ // This is called quite often. Just watch out for messages/events you want to handle
    void* retVal=NULL;

    if ( (message==sim_message_eventcallback_simulationactuation)&&(auxiliaryData[0]==0) )
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

    if (message==sim_message_eventcallback_scriptstatedestroyed)
    { // script state was destroyed. Destroy all associated BubbleRob instances:
        int index=getBubbleRobIndexFromScriptHandle(auxiliaryData[0]);
        while (index>=0)
        {
            allBubbleRobs.erase(allBubbleRobs.begin()+index);
            index=getBubbleRobIndexFromScriptHandle(auxiliaryData[0]);
        }
    }

    return(retVal);
}
