#ifndef _XKB_H
#define _XKB_H

extern int ProcXkbUseExtension(ClientPtr client);
extern int ProcXkbSelectEvents(ClientPtr client);
extern int ProcXkbBell(ClientPtr client);
extern int ProcXkbGetState(ClientPtr client);
extern int ProcXkbLatchLockState(ClientPtr client);
extern int ProcXkbGetControls(ClientPtr client);
extern int ProcXkbSetControls(ClientPtr client);
extern int ProcXkbGetMap(ClientPtr client);
extern int ProcXkbSetMap(ClientPtr client);
extern int ProcXkbGetCompatMap(ClientPtr client);
extern int ProcXkbSetCompatMap(ClientPtr client);
extern int ProcXkbGetIndicatorState(ClientPtr client);
extern int ProcXkbGetIndicatorMap(ClientPtr client);
extern int ProcXkbSetIndicatorMap(ClientPtr client);
extern int ProcXkbGetNamedIndicator(ClientPtr client);
extern int ProcXkbSetNamedIndicator(ClientPtr client);
extern int ProcXkbGetNames(ClientPtr client);
extern int ProcXkbSetNames(ClientPtr client);
extern int ProcXkbGetGeometry(ClientPtr client);
extern int ProcXkbSetGeometry(ClientPtr client);
extern int ProcXkbPerClientFlags(ClientPtr client);
extern int ProcXkbListComponents(ClientPtr client);
extern int ProcXkbGetKbdByName(ClientPtr client);
extern int ProcXkbGetDeviceInfo(ClientPtr client);
extern int ProcXkbSetDeviceInfo(ClientPtr client);
extern int ProcXkbSetDebuggingFlags(ClientPtr client);
#endif
