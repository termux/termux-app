/***********************************************************

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************/

#ifndef DIX_ACCESS_H
#define DIX_ACCESS_H

/* These are the access modes that can be passed in the last parameter
 * to several of the dix lookup functions.  They were originally part
 * of the Security extension, now used by XACE.
 *
 * You can or these values together to indicate multiple modes
 * simultaneously.
 */

#define DixUnknownAccess	0       /* don't know intentions */
#define DixReadAccess		(1<<0)  /* inspecting the object */
#define DixWriteAccess		(1<<1)  /* changing the object */
#define DixDestroyAccess	(1<<2)  /* destroying the object */
#define DixCreateAccess		(1<<3)  /* creating the object */
#define DixGetAttrAccess	(1<<4)  /* get object attributes */
#define DixSetAttrAccess	(1<<5)  /* set object attributes */
#define DixListPropAccess	(1<<6)  /* list properties of object */
#define DixGetPropAccess	(1<<7)  /* get properties of object */
#define DixSetPropAccess	(1<<8)  /* set properties of object */
#define DixGetFocusAccess	(1<<9)  /* get focus of object */
#define DixSetFocusAccess	(1<<10) /* set focus of object */
#define DixListAccess		(1<<11) /* list objects */
#define DixAddAccess		(1<<12) /* add object */
#define DixRemoveAccess		(1<<13) /* remove object */
#define DixHideAccess		(1<<14) /* hide object */
#define DixShowAccess		(1<<15) /* show object */
#define DixBlendAccess		(1<<16) /* mix contents of objects */
#define DixGrabAccess		(1<<17) /* exclusive access to object */
#define DixFreezeAccess		(1<<18) /* freeze status of object */
#define DixForceAccess		(1<<19) /* force status of object */
#define DixInstallAccess	(1<<20) /* install object */
#define DixUninstallAccess	(1<<21) /* uninstall object */
#define DixSendAccess		(1<<22) /* send to object */
#define DixReceiveAccess	(1<<23) /* receive from object */
#define DixUseAccess		(1<<24) /* use object */
#define DixManageAccess		(1<<25) /* manage object */
#define DixDebugAccess		(1<<26) /* debug object */
#define DixBellAccess		(1<<27) /* audible sound */
#define DixPostAccess		(1<<28) /* post or follow-up call */

#endif                          /* DIX_ACCESS_H */
