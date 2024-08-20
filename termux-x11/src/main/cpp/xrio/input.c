#include "input.h"
#include "math.h"

#include <sys/time.h>
#include <string.h>

void XrInputInit(struct XrEngine* engine, struct XrInput* input) {
    if (input->Initialized)
        return;
    memset(input, 0, sizeof(input));

    // Actions
    input->ActionSet = XrInputCreateActionSet(engine->Instance, "running_action_set", "Actionset");
    input->IndexLeft = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "index_left", "Index left", 0, NULL);
    input->IndexRight = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "index_right","Index right", 0, NULL);
    input->ButtonMenu = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "menu_action", "ButtonMenu", 0, NULL);
    input->ButtonA = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_a", "XrButton A", 0, NULL);
    input->ButtonB = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_b", "XrButton B", 0, NULL);
    input->ButtonX = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_x", "XrButton X", 0, NULL);
    input->ButtonY = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "button_y", "XrButton Y", 0, NULL);
    input->GripLeft = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_FLOAT_INPUT, "grip_left", "Grip left", 0, NULL);
    input->GripRight = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_FLOAT_INPUT, "grip_right", "Grip right", 0, NULL);
    input->JoystickLeft = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_VECTOR2F_INPUT, "move_on_left_joy","Move on left Joy", 0, NULL);
    input->JoystickRight = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_VECTOR2F_INPUT, "move_on_right_joy","Move on right Joy", 0, NULL);
    input->ThumbLeft = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "thumbstick_left","Thumbstick left", 0, NULL);
    input->ThumbRight = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_BOOLEAN_INPUT, "thumbstick_right","Thumbstick right", 0, NULL);
    input->VibrateLeftFeedback = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_VIBRATION_OUTPUT, "vibrate_left_feedback","Vibrate Left Controller", 0, NULL);
    input->VibrateRightFeedback = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_VIBRATION_OUTPUT, "vibrate_right_feedback","Vibrate Right Controller", 0, NULL);

    OXR(xrStringToPath(engine->Instance, "/user/hand/left", &input->LeftHandPath));
    OXR(xrStringToPath(engine->Instance, "/user/hand/right", &input->RightHandPath));
    input->HandPoseLeft = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_POSE_INPUT, "hand_pose_left", NULL,1, &input->LeftHandPath);
    input->HandPoseRight = XrInputCreateAction(input->ActionSet, XR_ACTION_TYPE_POSE_INPUT, "hand_pose_right", NULL,1, &input->RightHandPath);

    XrPath interactionProfilePath = XR_NULL_PATH;
    if (engine->PlatformFlag[PLATFORM_CONTROLLER_QUEST]) {
        OXR(xrStringToPath(engine->Instance, "/interaction_profiles/oculus/touch_controller",&interactionProfilePath));
    } else if (engine->PlatformFlag[PLATFORM_CONTROLLER_PICO]) {
        OXR(xrStringToPath(engine->Instance, "/interaction_profiles/pico/neo3_controller",&interactionProfilePath));
    }

    // Map bindings
    XrInstance instance = engine->Instance;
    XrActionSuggestedBinding bindings[32];  // large enough for all profiles
    int curr = 0;

    if (engine->PlatformFlag[PLATFORM_CONTROLLER_QUEST]) {
        bindings[curr++] = XrInputGetBinding(instance, input->IndexLeft, "/user/hand/left/input/trigger");
        bindings[curr++] = XrInputGetBinding(instance, input->IndexRight, "/user/hand/right/input/trigger");
        bindings[curr++] = XrInputGetBinding(instance, input->ButtonMenu, "/user/hand/left/input/menu/click");
    } else if (engine->PlatformFlag[PLATFORM_CONTROLLER_PICO]) {
        bindings[curr++] = XrInputGetBinding(instance, input->IndexLeft, "/user/hand/left/input/trigger/click");
        bindings[curr++] = XrInputGetBinding(instance, input->IndexRight, "/user/hand/right/input/trigger/click");
        bindings[curr++] = XrInputGetBinding(instance, input->ButtonMenu, "/user/hand/left/input/back/click");
        bindings[curr++] = XrInputGetBinding(instance, input->ButtonMenu, "/user/hand/right/input/back/click");
    }
    bindings[curr++] = XrInputGetBinding(instance, input->ButtonX, "/user/hand/left/input/x/click");
    bindings[curr++] = XrInputGetBinding(instance, input->ButtonY, "/user/hand/left/input/y/click");
    bindings[curr++] = XrInputGetBinding(instance, input->ButtonA, "/user/hand/right/input/a/click");
    bindings[curr++] = XrInputGetBinding(instance, input->ButtonB, "/user/hand/right/input/b/click");
    bindings[curr++] = XrInputGetBinding(instance, input->GripLeft, "/user/hand/left/input/squeeze/value");
    bindings[curr++] = XrInputGetBinding(instance, input->GripRight, "/user/hand/right/input/squeeze/value");
    bindings[curr++] = XrInputGetBinding(instance, input->JoystickLeft, "/user/hand/left/input/thumbstick");
    bindings[curr++] = XrInputGetBinding(instance, input->JoystickRight, "/user/hand/right/input/thumbstick");
    bindings[curr++] = XrInputGetBinding(instance, input->ThumbLeft, "/user/hand/left/input/thumbstick/click");
    bindings[curr++] = XrInputGetBinding(instance, input->ThumbRight, "/user/hand/right/input/thumbstick/click");
    bindings[curr++] = XrInputGetBinding(instance, input->VibrateLeftFeedback, "/user/hand/left/output/haptic");
    bindings[curr++] = XrInputGetBinding(instance, input->VibrateRightFeedback, "/user/hand/right/output/haptic");
    bindings[curr++] = XrInputGetBinding(instance, input->HandPoseLeft, "/user/hand/left/input/aim/pose");
    bindings[curr++] = XrInputGetBinding(instance, input->HandPoseRight, "/user/hand/right/input/aim/pose");

    XrInteractionProfileSuggestedBinding suggested_bindings = {};
    suggested_bindings.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
    suggested_bindings.next = NULL;
    suggested_bindings.interactionProfile = interactionProfilePath;
    suggested_bindings.suggestedBindings = bindings;
    suggested_bindings.countSuggestedBindings = curr;
    OXR(xrSuggestInteractionProfileBindings(engine->Instance, &suggested_bindings));

    // Attach actions
    XrSessionActionSetsAttachInfo attach_info = {};
    attach_info.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attach_info.next = NULL;
    attach_info.countActionSets = 1;
    attach_info.actionSets = &input->ActionSet;
    OXR(xrAttachSessionActionSets(engine->Session, &attach_info));

    // Enumerate actions
    char string_buffer[256];
    XrPath action_paths_buffer[32];
    XrAction actions_to_enumerate[] = {input->IndexLeft,
                                       input->IndexRight,
                                       input->ButtonMenu,
                                       input->ButtonA,
                                       input->ButtonB,
                                       input->ButtonX,
                                       input->ButtonY,
                                       input->GripLeft,
                                       input->GripRight,
                                       input->JoystickLeft,
                                       input->JoystickRight,
                                       input->ThumbLeft,
                                       input->ThumbRight,
                                       input->VibrateLeftFeedback,
                                       input->VibrateRightFeedback,
                                       input->HandPoseLeft,
                                       input->HandPoseRight};
    for (int i = 0; i < sizeof(actions_to_enumerate) / sizeof(XrAction); i++) {
        XrBoundSourcesForActionEnumerateInfo e = {};
        e.type = XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO;
        e.next = NULL;
        e.action = actions_to_enumerate[i];

        // Get Count
        uint32_t count_output = 0;
        OXR(xrEnumerateBoundSourcesForAction(engine->Session, &e, 0, &count_output, NULL));

        if (count_output < 32) {
            OXR(xrEnumerateBoundSourcesForAction(engine->Session, &e, 32, &count_output,
                                                 action_paths_buffer));
            for (uint32_t a = 0; a < count_output; ++a) {
                XrInputSourceLocalizedNameGetInfo name_info = {};
                name_info.type = XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO;
                name_info.next = NULL;
                name_info.sourcePath = action_paths_buffer[a];
                name_info.whichComponents = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
                                            XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
                                            XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

                uint32_t str_count = 0u;
                OXR(xrGetInputSourceLocalizedName(engine->Session, &name_info, 0, &str_count, NULL));
                if (str_count < 256) {
                    OXR(xrGetInputSourceLocalizedName(engine->Session, &name_info, 256, &str_count,
                                                      string_buffer));
                    char path_str[256];
                    uint32_t str_len = 0;
                    OXR(xrPathToString(engine->Instance, action_paths_buffer[a],
                                       (uint32_t)sizeof(path_str), &str_len, path_str));
                    ALOGV("mapped %s -> %s", path_str, string_buffer);
                }
            }
        }
    }
    input->Initialized = true;
}

uint32_t XrInputGetButtonState(struct XrInput* input, int controller) {
    switch (controller) {
        case 0:
            return input->ButtonsLeft;
        case 1:
            return input->ButtonsRight;
        default:
            return 0;
    }
}

XrVector2f XrInputGetJoystickState(struct XrInput* input, int controller) {
    return input->JoystickState[controller].currentState;
}

XrPosef XrInputGetPose(struct XrInput* input, int controller) {
    return input->ControllerPose[controller].pose;
}

void XrInputUpdate(struct XrEngine* engine, struct XrInput* input) {
    // sync action data
    XrActiveActionSet activeActionSet = {};
    activeActionSet.actionSet = input->ActionSet;
    activeActionSet.subactionPath = XR_NULL_PATH;

    XrActionsSyncInfo sync_info = {};
    sync_info.type = XR_TYPE_ACTIONS_SYNC_INFO;
    sync_info.next = NULL;
    sync_info.countActiveActionSets = 1;
    sync_info.activeActionSets = &activeActionSet;
    OXR(xrSyncActions(engine->Session, &sync_info));

    // query input action states
    XrActionStateGetInfo get_info = {};
    get_info.type = XR_TYPE_ACTION_STATE_GET_INFO;
    get_info.next = NULL;
    get_info.subactionPath = XR_NULL_PATH;

    XrSession session = engine->Session;
    XrInputProcessHaptics(input, session);

    if (input->LeftControllerSpace == XR_NULL_HANDLE) {
        input->LeftControllerSpace = XrInputCreateActionSpace(session, input->HandPoseLeft, input->LeftHandPath);
    }
    if (input->RightControllerSpace == XR_NULL_HANDLE) {
        input->RightControllerSpace = XrInputCreateActionSpace(session, input->HandPoseRight, input->RightHandPath);
    }

    // button mapping
    input->ButtonsLeft = 0;
    if (XrInputGetActionStateBoolean(session, input->ButtonMenu).currentState)
        input->ButtonsLeft |= (int)Enter;
    if (XrInputGetActionStateBoolean(session, input->ButtonX).currentState)
        input->ButtonsLeft |= (int)X;
    if (XrInputGetActionStateBoolean(session, input->ButtonY).currentState)
        input->ButtonsLeft |= (int)Y;
    if (XrInputGetActionStateBoolean(session, input->IndexLeft).currentState)
        input->ButtonsLeft |= (int)Trigger;
    if (XrInputGetActionStateFloat(session, input->GripLeft).currentState > 0.5f)
        input->ButtonsLeft |= (int)Grip;
    if (XrInputGetActionStateBoolean(session, input->ThumbLeft).currentState)
        input->ButtonsLeft |= (int)LThumb;
    input->ButtonsRight = 0;
    if (XrInputGetActionStateBoolean(session, input->ButtonA).currentState)
        input->ButtonsRight |= (int)A;
    if (XrInputGetActionStateBoolean(session, input->ButtonB).currentState)
        input->ButtonsRight |= (int)B;
    if (XrInputGetActionStateBoolean(session, input->IndexRight).currentState)
        input->ButtonsRight |= (int)Trigger;
    if (XrInputGetActionStateFloat(session, input->GripRight).currentState > 0.5f)
        input->ButtonsRight |= (int)Grip;
    if (XrInputGetActionStateBoolean(session, input->ThumbRight).currentState)
        input->ButtonsRight |= (int)RThumb;

    // thumbstick
    input->JoystickState[0] = XrInputGetActionStateVector2(session, input->JoystickLeft);
    input->JoystickState[1] = XrInputGetActionStateVector2(session, input->JoystickRight);
    if (input->JoystickState[0].currentState.x > 0.5)
        input->ButtonsLeft |= (int)Right;
    if (input->JoystickState[0].currentState.x < -0.5)
        input->ButtonsLeft |= (int)Left;
    if (input->JoystickState[0].currentState.y > 0.5)
        input->ButtonsLeft |= (int)Up;
    if (input->JoystickState[0].currentState.y < -0.5)
        input->ButtonsLeft |= (int)Down;
    if (input->JoystickState[1].currentState.x > 0.5)
        input->ButtonsRight |= (int)Right;
    if (input->JoystickState[1].currentState.x < -0.5)
        input->ButtonsRight |= (int)Left;
    if (input->JoystickState[1].currentState.y > 0.5)
        input->ButtonsRight |= (int)Up;
    if (input->JoystickState[1].currentState.y < -0.5)
        input->ButtonsRight |= (int)Down;

    // pose
    for (int i = 0; i < 2; i++) {
        memset(&input->ControllerPose[i], 0, sizeof(input->ControllerPose[i]));
        input->ControllerPose[i].type = XR_TYPE_SPACE_LOCATION;
        XrSpace aim_space[] = {input->LeftControllerSpace, input->RightControllerSpace};
        xrLocateSpace(aim_space[i], engine->CurrentSpace,
                      (XrTime)(engine->PredictedDisplayTime), &input->ControllerPose[i]);
    }
}

void XrInputVibrate(struct XrInput* input, int duration, int chan, float intensity) {
    for (int i = 0; i < 2; ++i) {
        int channel = i & chan;
        if (channel) {
            if (input->VibrationChannelDuration[channel] > 0.0f)
                return;

            if (input->VibrationChannelDuration[channel] == -1.0f && duration != 0.0f)
                return;

            input->VibrationChannelDuration[channel] = (float)duration;
            input->VibrationChannelIntensity[channel] = intensity;
        }
    }
}

XrAction XrInputCreateAction(XrActionSet output_set, XrActionType type, const char* name,
                             const char* desc, int count_subaction_path, XrPath* subaction_path) {
    XrActionCreateInfo action_info = {};
    action_info.type = XR_TYPE_ACTION_CREATE_INFO;
    action_info.next = NULL;
    action_info.actionType = type;
    if (count_subaction_path > 0) {
        action_info.countSubactionPaths = count_subaction_path;
        action_info.subactionPaths = subaction_path;
    }
    strcpy(action_info.actionName, name);
    strcpy(action_info.localizedActionName, desc ? desc : name);
    XrAction output = XR_NULL_HANDLE;
    OXR(xrCreateAction(output_set, &action_info, &output));
    return output;
}

XrActionSet XrInputCreateActionSet(XrInstance instance, const char* name, const char* desc) {
    XrActionSetCreateInfo action_set_info = {};
    action_set_info.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    action_set_info.next = NULL;
    action_set_info.priority = 1;
    strcpy(action_set_info.actionSetName, name);
    strcpy(action_set_info.localizedActionSetName, desc);
    XrActionSet output = XR_NULL_HANDLE;
    OXR(xrCreateActionSet(instance, &action_set_info, &output));
    return output;
}

XrSpace XrInputCreateActionSpace(XrSession session, XrAction action, XrPath subaction_path) {
    XrActionSpaceCreateInfo action_space_info = {};
    action_space_info.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
    action_space_info.action = action;
    action_space_info.poseInActionSpace.orientation.w = 1.0f;
    action_space_info.subactionPath = subaction_path;
    XrSpace output = XR_NULL_HANDLE;
    OXR(xrCreateActionSpace(session, &action_space_info, &output));
    return output;
}

XrActionStateBoolean XrInputGetActionStateBoolean(XrSession session, XrAction action) {
    XrActionStateGetInfo get_info = {};
    get_info.type = XR_TYPE_ACTION_STATE_GET_INFO;
    get_info.action = action;

    XrActionStateBoolean state = {};
    state.type = XR_TYPE_ACTION_STATE_BOOLEAN;

    OXR(xrGetActionStateBoolean(session, &get_info, &state));
    return state;
}

XrActionStateFloat XrInputGetActionStateFloat(XrSession session, XrAction action) {
    XrActionStateGetInfo get_info = {};
    get_info.type = XR_TYPE_ACTION_STATE_GET_INFO;
    get_info.action = action;

    XrActionStateFloat state = {};
    state.type = XR_TYPE_ACTION_STATE_FLOAT;

    OXR(xrGetActionStateFloat(session, &get_info, &state));
    return state;
}

XrActionStateVector2f XrInputGetActionStateVector2(XrSession session, XrAction action) {
    XrActionStateGetInfo get_info = {};
    get_info.type = XR_TYPE_ACTION_STATE_GET_INFO;
    get_info.action = action;

    XrActionStateVector2f state = {};
    state.type = XR_TYPE_ACTION_STATE_VECTOR2F;

    OXR(xrGetActionStateVector2f(session, &get_info, &state));
    return state;
}

XrActionSuggestedBinding XrInputGetBinding(XrInstance instance, XrAction action, const char* name) {
    XrPath bindingPath;
    OXR(xrStringToPath(instance, name, &bindingPath));

    XrActionSuggestedBinding output;
    output.action = action;
    output.binding = bindingPath;
    return output;
}

int XrInputGetMilliseconds(struct XrInput* input) {
    struct timeval tp;

    gettimeofday(&tp, NULL);

    if (!input->SysTimeBase) {
        input->SysTimeBase = tp.tv_sec;
        return tp.tv_usec / 1000;
    }

    return (tp.tv_sec - input->SysTimeBase) * 1000 + tp.tv_usec / 1000;
}

void XrInputProcessHaptics(struct XrInput* input, XrSession session) {
    static float last_frame_timestamp = 0.0f;
    float timestamp = (float)(XrInputGetMilliseconds(input));
    float frametime = timestamp - last_frame_timestamp;
    last_frame_timestamp = timestamp;

    for (int i = 0; i < 2; ++i) {
        if (input->VibrationChannelDuration[i] > 0.0f || input->VibrationChannelDuration[i] == -1.0f) {
            // fire haptics using output action
            XrHapticVibration vibration = {};
            vibration.type = XR_TYPE_HAPTIC_VIBRATION;
            vibration.next = NULL;
            vibration.amplitude = input->VibrationChannelIntensity[i];
            vibration.duration = ToXrTime(input->VibrationChannelDuration[i]);
            vibration.frequency = 3000;
            XrHapticActionInfo haptic_info = {};
            haptic_info.type = XR_TYPE_HAPTIC_ACTION_INFO;
            haptic_info.next = NULL;
            haptic_info.action = i == 0 ? input->VibrateLeftFeedback : input->VibrateRightFeedback;
            OXR(xrApplyHapticFeedback(session, &haptic_info, (const XrHapticBaseHeader*)&vibration));

            if (input->VibrationChannelDuration[i] != -1.0f) {
                input->VibrationChannelDuration[i] -= frametime;

                if (input->VibrationChannelDuration[i] < 0.0f) {
                    input->VibrationChannelDuration[i] = 0.0f;
                    input->VibrationChannelIntensity[i] = 0.0f;
                }
            }
        } else {
            // Stop haptics
            XrHapticActionInfo haptic_info = {};
            haptic_info.type = XR_TYPE_HAPTIC_ACTION_INFO;
            haptic_info.next = NULL;
            haptic_info.action = i == 0 ? input->VibrateLeftFeedback : input->VibrateRightFeedback;
            OXR(xrStopHapticFeedback(session, &haptic_info));
        }
    }
}
