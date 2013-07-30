/*
Copyright 2013 Jay Sorg

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

xrdp mouse module

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* this should be before all X11 .h files */
#include <xorg-server.h>

/* all driver need this */
#include <xf86.h>
#include <xf86_OSproc.h>

#include "xf86Xinput.h"

#include <mipointer.h>
#include <fb.h>
#include <micmap.h>
#include <mi.h>
#include <exevents.h>
#include <xserver-properties.h>

#include "rdp.h"
#include "rdpInput.h"

/******************************************************************************/
#define LOG_LEVEL 1
#define LLOGLN(_level, _args) \
    do { if (_level < LOG_LEVEL) { ErrorF _args ; ErrorF("\n"); } } while (0)

#define XRDP_DRIVER_NAME "XRDPMOUSE"
#define XRDP_NAME "XRDPMOUSE"
#define XRDP_VERSION 1000

#define PACKAGE_VERSION_MAJOR 1
#define PACKAGE_VERSION_MINOR 0
#define PACKAGE_VERSION_PATCHLEVEL 0

static DeviceIntPtr g_pointer = 0;

static int g_cursor_x = 0;
static int g_cursor_y = 0;

static int g_old_button_mask = 0;
static int g_button_mask = 0;

/******************************************************************************/
static void
rdpmouseDeviceInit(void)
{
    LLOGLN(0, ("rdpmouseDeviceInit:"));
}

/******************************************************************************/
static void
rdpmouseDeviceOn(DeviceIntPtr pDev)
{
    LLOGLN(0, ("rdpmouseDeviceOn:"));
}

/******************************************************************************/
static void
rdpmouseDeviceOff(void)
{
    LLOGLN(0, ("rdpmouseDeviceOff:"));
}

/******************************************************************************/
static void
rdpmouseCtrl(DeviceIntPtr pDevice, PtrCtrl *pCtrl)
{
    LLOGLN(0, ("rdpmouseCtrl:"));
}

/******************************************************************************/
static int
l_bound_by(int val, int low, int high)
{
    if (val > high)
    {
        val = high;
    }

    if (val < low)
    {
        val = low;
    }

    return val;
}

/******************************************************************************/
static void
rdpEnqueueMotion(int x, int y)
{
    int i;
    int n;
    int valuators[2];
    EventListPtr rdp_events;
    xEvent *pev;

    miPointerSetPosition(g_pointer, &x, &y);
    valuators[0] = x;
    valuators[1] = y;

    GetEventList(&rdp_events);
    n = GetPointerEvents(rdp_events, g_pointer, MotionNotify, 0,
                         POINTER_ABSOLUTE | POINTER_SCREEN,
                         0, 2, valuators);

    for (i = 0; i < n; i++)
    {
        pev = (rdp_events + i)->event;
        mieqEnqueue(g_pointer, (InternalEvent *)pev);
    }
}

/******************************************************************************/
static void
rdpEnqueueButton(int type, int buttons)
{
    int i;
    int n;
    EventListPtr rdp_events;
    xEvent *pev;

    i = GetEventList(&rdp_events);
    n = GetPointerEvents(rdp_events, g_pointer, type, buttons, 0, 0, 0, 0);

    for (i = 0; i < n; i++)
    {
        pev = (rdp_events + i)->event;
        mieqEnqueue(g_pointer, (InternalEvent *)pev);
    }
}

/******************************************************************************/
void
PtrAddEvent(int buttonMask, int x, int y)
{
    int i;
    int type;
    int buttons;

    rdpEnqueueMotion(x, y);

    for (i = 0; i < 5; i++)
    {
        if ((buttonMask ^ g_old_button_mask) & (1 << i))
        {
            if (buttonMask & (1 << i))
            {
                type = ButtonPress;
                buttons = i + 1;
                rdpEnqueueButton(type, buttons);
            }
            else
            {
                type = ButtonRelease;
                buttons = i + 1;
                rdpEnqueueButton(type, buttons);
            }
        }
    }

    g_old_button_mask = buttonMask;
}

/******************************************************************************/
static int
rdpInputMouse(rdpPtr dev, int msg,
              long param1, long param2,
              long param3, long param4)
{
    LLOGLN(0, ("rdpInputMouse:"));

    switch (msg)
    {
        case 100:
            /* without the minus 2, strange things happen when dragging
               past the width or height */
            g_cursor_x = l_bound_by(param1, 0, dev->width - 2);
            g_cursor_y = l_bound_by(param2, 0, dev->height - 2);
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 101:
            g_button_mask = g_button_mask & (~1);
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 102:
            g_button_mask = g_button_mask | 1;
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 103:
            g_button_mask = g_button_mask & (~4);
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 104:
            g_button_mask = g_button_mask | 4;
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 105:
            g_button_mask = g_button_mask & (~2);
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 106:
            g_button_mask = g_button_mask | 2;
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 107:
            g_button_mask = g_button_mask & (~8);
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 108:
            g_button_mask = g_button_mask | 8;
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 109:
            g_button_mask = g_button_mask & (~16);
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
        case 110:
            g_button_mask = g_button_mask | 16;
            PtrAddEvent(g_button_mask, g_cursor_x, g_cursor_y);
            break;
    }
    return 0;
}

/******************************************************************************/
static int
rdpmouseControl(DeviceIntPtr device, int what)
{
    BYTE map[6];
    DevicePtr pDev;
    Atom btn_labels[6];
    Atom axes_labels[2];

    LLOGLN(0, ("rdpmouseControl: what %d", what));
    pDev = (DevicePtr)device;

    switch (what)
    {
        case DEVICE_INIT:
            rdpmouseDeviceInit();
            map[0] = 0;
            map[1] = 1;
            map[2] = 2;
            map[3] = 3;
            map[4] = 4;
            map[5] = 5;

            btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
            btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
            btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
            btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
            btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);

            axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
            axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

            InitPointerDeviceStruct(pDev, map, 5, btn_labels, rdpmouseCtrl,
                                    GetMotionHistorySize(), 2, axes_labels);
            g_pointer = device;
            rdpRegisterInputCallback(1, rdpInputMouse);
            break;
        case DEVICE_ON:
            pDev->on = 1;
            rdpmouseDeviceOn(device);
            break;
        case DEVICE_OFF:
            pDev->on = 0;
            rdpmouseDeviceOff();
            break;
        case DEVICE_CLOSE:

            if (pDev->on)
            {
                rdpmouseDeviceOff();
            }

            break;
    }

    return Success;
}

#if XORG_VERSION_CURRENT < (((1) * 10000000) + ((9) * 100000) + ((0) * 1000) + 1)

/* debian 6
   ubuntu 10.04 */

/******************************************************************************/
static InputInfoPtr
rdpmousePreInit(InputDriverPtr drv, IDevPtr dev, int flags)
{
    InputInfoPtr info;

    LLOGLN(0, ("rdpmousePreInit: drv %p dev %p, flags 0x%x",
           drv, dev, flags));
    info = xf86AllocateInput(drv, 0);
    info->name = dev->identifier;
    info->device_control = rdpmouseControl;
    info->flags = XI86_CONFIGURED | XI86_ALWAYS_CORE | XI86_SEND_DRAG_EVENTS |
                  XI86_CORE_POINTER | XI86_POINTER_CAPABLE;
    info->type_name = "Mouse";
    info->fd = -1;
    info->conf_idev = dev;

    return info;
}

#else

/* debian 7
   ubuntu 12.04 */

/******************************************************************************/
static int
rdpmousePreInit(InputDriverPtr drv, InputInfoPtr info, int flags)
{
    LLOGLN(0, ("rdpmousePreInit: drv %p info %p, flags 0x%x",
           drv, info, flags));
    info->device_control = rdpmouseControl;
    info->type_name = "Mouse";
    return 0;
}

#endif

/******************************************************************************/
static void
rdpmouseUnInit(InputDriverPtr drv, InputInfoPtr info, int flags)
{
    LLOGLN(0, ("rdpmouseUnInit: drv %p info %p, flags 0x%x",
           drv, info, flags));
}

/******************************************************************************/
static InputDriverRec rdpmouse =
{
    PACKAGE_VERSION_MAJOR,    /* version   */
    XRDP_NAME,                /* name      */
    NULL,                     /* identify  */
    rdpmousePreInit,          /* preinit   */
    rdpmouseUnInit,           /* uninit    */
    NULL,                     /* module    */
    0                         /* ref count */
};

/******************************************************************************/
static void
rdpmouseUnplug(pointer p)
{
    LLOGLN(0, ("rdpmouseUnplug:"));
}

/******************************************************************************/
static pointer
rdpmousePlug(pointer module, pointer options, int *errmaj, int *errmin)
{
    LLOGLN(0, ("rdpmousePlug:"));
    xf86AddInputDriver(&rdpmouse, module, 0);
    return module;
}

/******************************************************************************/
static XF86ModuleVersionInfo rdpmouseVersionRec =
{
    XRDP_DRIVER_NAME,
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    PACKAGE_VERSION_MAJOR,
    PACKAGE_VERSION_MINOR,
    PACKAGE_VERSION_PATCHLEVEL,
    ABI_CLASS_XINPUT,
    ABI_XINPUT_VERSION,
    MOD_CLASS_XINPUT,
    { 0, 0, 0, 0 }
};

/******************************************************************************/
XF86ModuleData xrdpmouseModuleData =
{
    &rdpmouseVersionRec,
    rdpmousePlug,
    rdpmouseUnplug
};