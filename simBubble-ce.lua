local codeEditorInfos = [[
int bubbleRobHandle = simBubble.create(int[2] motorJointHandles, int sensorHandle, float[2] backRelativeVelocities)
bool result = simBubble.destroy(int bubbleRobHandle)
bool result = simBubble.start(int bubbleRobHandle)
bool result = simBubble.stop(int bubbleRobHandle)
]]

registerCodeEditorInfos("simBubble", codeEditorInfos)
