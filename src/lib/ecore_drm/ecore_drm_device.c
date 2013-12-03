#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ecore_drm_private.h"

/**
 * @defgroup Ecore_Drm_Device_Group Device manipulation functions
 * 
 * Functions that deal with finding, opening, closing, and otherwise using 
 * the DRM device itself.
 */

/**
 * Find a drm device in the system.
 *
 * @param name The name of the device to find. If NULL, this function will 
 *             search for the default drm device.
 * @param seat The name of the seat where this device may be found. If NULL, 
 *             this function will use a default seat name 'seat0'.
 * 
 * @return An opaque Ecore_Drm_Device structure representing the card.
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Ecore_Drm_Device *
ecore_drm_device_find(const char *name, const char *seat)
{
   Ecore_Drm_Device *dev = NULL;
   struct udev_enumerate *uenum;
   struct udev_list_entry *uentry;
   struct udev_device *udevice, *tmpdevice = NULL, *pcidevice;
   const char *path = NULL, *devseat = NULL;

   /* check for existing udev reference */
   if (!udev) return NULL;

   /* setup udev enumerator */
   uenum = udev_enumerate_new(udev);
   udev_enumerate_add_match_subsystem(uenum, "drm");
   udev_enumerate_add_match_subsystem(uenum, "card[0-9]*");

   /* ask udev for list of drm devices */
   udev_enumerate_scan_devices(uenum);

   /* loop list of returned devices */
   udev_list_entry_foreach(uentry, udev_enumerate_get_list_entry(uenum))
     {
        /* get device path */
        path = udev_list_entry_get_name(uentry);

        /* get udev device */
        if (!(udevice = udev_device_new_from_syspath(udev, path)))
          continue;

        /* if we are looking for a certain device, then compare names */
        if (name)
          {
             if (strcmp(name, udev_device_get_devnode(udevice)))
               {
                  udev_device_unref(udevice);
                  continue;
               }
          }

        /* get this devices' seat */
        devseat = udev_device_get_property_value(udevice, "ID_SEAT");
        if (!devseat) devseat = "seat0";

        /* if we are looking for a device on a certain seat, compare it */
        if (seat)
          {
             if (strcmp(seat, devseat))
               {
                  udev_device_unref(udevice);
                  continue;
               }
          }
        else
          {
             /* no seat name passed to use. check default */
             if (strcmp(devseat, "seat0"))
               {
                  udev_device_unref(udevice);
                  continue;
               }
          }

        /* try to find the boot_vga attribute */
        if ((pcidevice = 
             udev_device_get_parent_with_subsystem_devtype(udevice, "pci", NULL)))
          {
             const char *id;

             if ((id = udev_device_get_sysattr_value(pcidevice, "boot_vga")))
               {
                  if (!strcmp(id, "1"))
                    {
                       if (tmpdevice) udev_device_unref(tmpdevice);
                       tmpdevice = udevice;
                       break;
                    }
               }
          }

        if (!tmpdevice) 
          tmpdevice = udevice;
        else
          udev_device_unref(udevice);
     }

   /* destroy the enumerator */
   udev_enumerate_unref(uenum);

   if (tmpdevice)
     {
        DBG("Found Drm Device");
        DBG("\tFilename: %s", udev_device_get_devnode(tmpdevice));
        DBG("\tDriver: %s", udev_device_get_driver(tmpdevice));
        DBG("\tDevpath: %s", udev_device_get_devpath(tmpdevice));
        DBG("\tSyspath: %s", udev_device_get_syspath(tmpdevice));
        DBG("\tSysname: %s", udev_device_get_sysname(tmpdevice));

        /* try to allocate space for return device structure */
        if ((dev = calloc(1, sizeof(Ecore_Drm_Device))))
          {
             /* set device name */
             dev->devname = 
               eina_stringshare_add(udev_device_get_devnode(tmpdevice));

             /* set device path */
             dev->devpath = 
               eina_stringshare_add(udev_device_get_syspath(tmpdevice));
          }
     }

   /* release device reference */
   udev_device_unref(tmpdevice);

   return dev;
}

/**
 * Free an Ecore_Drm_Device
 *
 * This function will cleanup and free any previously allocated Ecore_Drm_Device.
 * 
 * @param dev The Ecore_Drm_Device to free
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI void 
ecore_drm_device_free(Ecore_Drm_Device *dev)
{
   /* check for valid device */
   if (!dev) return;

   /* free device name */
   if (dev->devname) eina_stringshare_del(dev->devname);

   /* free device path */
   if (dev->devpath) eina_stringshare_del(dev->devpath);

   /* free structure */
   free(dev);
}

/**
 * Open an Ecore_Drm_Device
 *
 * This function will open an existing Ecore_Drm_Device for use.
 * 
 * @param dev The Ecore_Drm_Device to try and open
 * 
 * @return EINA_TRUE on success, EINA_FALSE on failure
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Eina_Bool 
ecore_drm_device_open(Ecore_Drm_Device *dev)
{
   /* check for valid device */
   if ((!dev) || (!dev->devpath)) return EINA_FALSE;

   return EINA_TRUE;
}

/**
 * Close an Ecore_Drm_Device
 *
 * This function will close a previously opened Ecore_Drm_Device
 * 
 * @param dev The Ecore_Drm_Device to free
 * 
 * @return EINA_TRUE on success, EINA_FALSE on failure
 * 
 * @ingroup Ecore_Drm_Device_Group
 */
EAPI Eina_Bool 
ecore_drm_device_close(Ecore_Drm_Device *dev)
{
   /* check for valid device */
   if (!dev) return EINA_TRUE;

   return EINA_TRUE;
}