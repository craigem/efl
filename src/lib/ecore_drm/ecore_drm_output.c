#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ecore_drm_private.h"

#define ALEN(array) (sizeof(array) / sizeof(array)[0])

static const char *conn_types[] = 
{
   "None", "VGA", "DVI", "DVI", "DVI",
   "Composite", "TV", "LVDS", "CTV", "DIN",
   "DP", "HDMI", "HDMI", "TV", "eDP",
};

/* local functions */
#ifdef HAVE_GBM
static Eina_Bool 
_ecore_drm_output_hardware_setup(Ecore_Drm_Device *dev, Ecore_Drm_Output *output)
{
   unsigned int i = 0;
   int flags = 0;
   int w = 0, h = 0;

   if ((!dev) || (!output) || (!dev->use_hw_accel)) return EINA_FALSE;

   if (output->current_mode)
     {
        w = output->current_mode->width;
        h = output->current_mode->height;
     }
   else
     {
        w = 1024;
        h = 768;
     }

   flags = (GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

   if (!(output->surface = 
         gbm_surface_create(dev->gbm, w, h, dev->format, flags)))
     {
        ERR("Could not create output surface: %m");
        return EINA_FALSE;
     }

   flags = (GBM_BO_USE_CURSOR_64X64 | GBM_BO_USE_WRITE);
   for (i = 0; i < NUM_FRAME_BUFFERS; i++)
     {
        if (output->cursor[i]) continue;
        if (!(output->cursor[i] = 
              gbm_bo_create(dev->gbm, 64, 64, dev->format, flags)))
          {
             ERR("Could not create cursor surface: %m");
             continue;
          }
     }

   if ((!output->cursor[0]) || (!output->cursor[1]))
     {
        WRN("Hardware Cursor Buffers not available");
        dev->cursors_broken = EINA_TRUE;
     }

   return EINA_TRUE;
}
#endif

static Eina_Bool 
_ecore_drm_output_software_setup(Ecore_Drm_Device *dev, Ecore_Drm_Output *output)
{
   unsigned int i = 0;
   int w = 0, h = 0;

   if ((!dev) || (!output)) return EINA_FALSE;

   if (output->current_mode)
     {
        w = output->current_mode->width;
        h = output->current_mode->height;
     }
   else
     {
        w = 1024;
        h = 768;
     }

   for (i = 0; i < NUM_FRAME_BUFFERS; i++)
     {
        if (!(output->dumb[i] = _ecore_drm_fb_create(dev, w, h)))
          {
             ERR("Could not create dumb framebuffer %d", i);
             goto err;
          }
     }

   return EINA_TRUE;

err:
   for (i = 0; i < NUM_FRAME_BUFFERS; i++)
     {
        if (output->dumb[i]) _ecore_drm_fb_destroy(output->dumb[i]);
        output->dumb[i] = NULL;
     }

   return EINA_FALSE;
}

static int 
_ecore_drm_output_crtc_find(Ecore_Drm_Device *dev, drmModeRes *res, drmModeConnector *conn)
{
   drmModeEncoder *enc;
   unsigned int p;
   int i = 0, j = 0;

   for (j = 0; j < conn->count_encoders; j++)
     {
        /* get the encoder on this connector */
        if (!(enc = drmModeGetEncoder(dev->drm.fd, conn->encoders[j])))
          {
             ERR("Failed to get encoder: %m");
             return -1;
          }

        p = enc->possible_crtcs;
        drmModeFreeEncoder(enc);

        for (i = 0; i < res->count_crtcs; i++)
          {
             if ((p & (1 << i)) && 
                 (!(dev->crtc_allocator & (1 << res->crtcs[i]))))
               {
                  return i;
               }
          }
     }

   return -1;
}

static Ecore_Drm_Output_Mode *
_ecore_drm_output_mode_add(Ecore_Drm_Output *output, drmModeModeInfo *info)
{
   Ecore_Drm_Output_Mode *mode;
   uint64_t refresh;

   /* try to allocate space for mode */
   if (!(mode = malloc(sizeof(Ecore_Drm_Output_Mode))))
     {
        ERR("Could not allocate space for mode");
        return NULL;
     }

   mode->flags = 0;
   mode->width = info->hdisplay;
   mode->height = info->vdisplay;

   refresh = (info->clock * 1000000LL / info->htotal + info->vtotal / 2) / info->vtotal;
   if (info->flags & DRM_MODE_FLAG_INTERLACE)
     refresh *= 2;
   if (info->flags & DRM_MODE_FLAG_DBLSCAN)
     refresh /= 2;
   if (info->vscan > 1)
     refresh /= info->vscan;

   mode->refresh = refresh;
   mode->info = *info;

   /* DBG("Added Mode: %dx%d@%d to Output %d",  */
   /*     mode->width, mode->height, mode->refresh, output->crtc_id); */

   output->modes = eina_list_append(output->modes, mode);

   return mode;
}

static Ecore_Drm_Output *
_ecore_drm_output_create(Ecore_Drm_Device *dev, drmModeRes *res, drmModeConnector *conn, int x, int y)
{
   Ecore_Drm_Output *output;
   Ecore_Drm_Output_Mode *mode;
   const char *conn_name;
   char name[32];
   int i = 0;
   drmModeEncoder *enc;
   drmModeModeInfo crtc_mode;
   drmModeCrtc *crtc;
   Eina_List *l;

   i = _ecore_drm_output_crtc_find(dev, res, conn);
   if (i < 0)
     {
        ERR("Could not find crtc or encoder for connector");
        return NULL;
     }

   /* try to allocate space for output */
   if (!(output = calloc(1, sizeof(Ecore_Drm_Output))))
     {
        ERR("Could not allocate space for output");
        return NULL;
     }

   output->x = x;
   output->y = y;

   output->subpixel = conn->subpixel;
   output->make = eina_stringshare_add("unknown");
   output->model = eina_stringshare_add("unknown");

   if (conn->connector_type < ALEN(conn_types))
     conn_name = conn_types[conn->connector_type];
   else
     conn_name = "UNKNOWN";

   snprintf(name, sizeof(name), "%s%d", conn_name, conn->connector_type_id);
   output->name = eina_stringshare_add(name);

   output->crtc_id = res->crtcs[i];
   dev->crtc_allocator |= (1 << output->crtc_id);
   output->conn_id = conn->connector_id;
   output->crtc = drmModeGetCrtc(dev->drm.fd, output->crtc_id);

   memset(&mode, 0, sizeof(mode));
   if ((enc = drmModeGetEncoder(dev->drm.fd, conn->encoder_id)))
     {
        crtc = drmModeGetCrtc(dev->drm.fd, enc->crtc_id);
        drmModeFreeEncoder(enc);
        if (!crtc) goto mode_err;
        if (crtc->mode_valid) crtc_mode = crtc->mode;
        drmModeFreeCrtc(crtc);
     }

   for (i = 0; i < conn->count_modes; i++)
     {
        if (!(mode = _ecore_drm_output_mode_add(output, &conn->modes[i])))
          {
             ERR("Failed to add mode to output");
             goto mode_err;
          }
     }

   EINA_LIST_REVERSE_FOREACH(output->modes, l, mode)
     {
        if (!memcmp(&crtc_mode, &mode->info, sizeof(crtc_mode)))
          {
             output->current_mode = mode;
             break;
          }
     }

   if (!output->current_mode)
     output->current_mode = _ecore_drm_output_mode_add(output, &crtc_mode);

#ifdef HAVE_GBM
   if (dev->use_hw_accel)
     {
        if (!_ecore_drm_output_hardware_setup(dev, output))
          {
             ERR("Could not setup output for hardware acceleration");
             if (!_ecore_drm_output_software_setup(dev, output))
               goto mode_err;
             else
               DBG("Setup Output %d for Software Rendering", output->crtc_id);
          }
        else
          DBG("Setup Output %d for Hardware Acceleration", output->crtc_id);
     }
   else
#endif
     {
        if (!_ecore_drm_output_software_setup(dev, output))
          goto mode_err;
        else
          DBG("Setup Output %d for Software Rendering", output->crtc_id);
     }

   /* TODO: output_init_pixman/output_init_egl ?? */

   return output;

mode_err:
   EINA_LIST_FREE(output->modes, mode)
     free(mode);
   drmModeFreeCrtc(output->crtc);
   dev->crtc_allocator &= ~(1 << output->crtc_id);
   free(output);
   return NULL;
}

/* public functions */
EAPI Eina_Bool 
ecore_drm_outputs_create(Ecore_Drm_Device *dev)
{
   Eina_Bool ret = EINA_TRUE;
   Ecore_Drm_Output *output;
   drmModeConnector *conn;
   drmModeRes *res;
   drmModeCrtc *crtc;
   int i = 0, x = 0, y = 0;

   /* DBG("Create outputs for %d", dev->drm.fd); */

   /* get the resources */
   if (!(res = drmModeGetResources(dev->drm.fd)))
     {
        ERR("Could not get resources for drm card: %m");
        return EINA_FALSE;
     }

   if (!(dev->crtcs = calloc(res->count_crtcs, sizeof(unsigned int))))
     {
        ERR("Could not allocate space for crtcs");
        /* free resources */
        drmModeFreeResources(res);
        return EINA_FALSE;
     }

   dev->crtc_count = res->count_crtcs;
   memcpy(dev->crtcs, res->crtcs, sizeof(unsigned int) * res->count_crtcs);

   dev->min_width = res->min_width;
   dev->min_height = res->min_height;
   dev->max_width = res->max_width;
   dev->max_height = res->max_height;

   for (i = 0; i < res->count_connectors; i++)
     {
        /* get the connector */
        if (!(conn = drmModeGetConnector(dev->drm.fd, res->connectors[i])))
          continue;

        if ((conn->connection == DRM_MODE_CONNECTED) && 
            (conn->count_modes > 0))
          {
             drmModeEncoder *enc;

             /* create output for this connector */
             if (!(output = _ecore_drm_output_create(dev, res, conn, x, y)))
               {
                  /* free the connector */
                  drmModeFreeConnector(conn);
                  continue;
               }

             dev->outputs = eina_list_append(dev->outputs, output);

             if (!(enc = drmModeGetEncoder(dev->drm.fd, conn->encoder_id)))
               {
                  drmModeFreeConnector(conn);
                  continue;
               }

             if (!(crtc = drmModeGetCrtc(dev->drm.fd, enc->crtc_id)))
               {
                  drmModeFreeEncoder(enc);
                  drmModeFreeConnector(conn);
                  continue;
               }

             x += crtc->width;

             drmModeFreeCrtc(crtc);
             drmModeFreeEncoder(enc);
          }

        /* free the connector */
        drmModeFreeConnector(conn);
     }

   ret = EINA_TRUE;
   if (eina_list_count(dev->outputs) < 1) 
     {
        ret = EINA_FALSE;
        free(dev->crtcs);
     }

   /* free resources */
   drmModeFreeResources(res);

   /* TODO: add hook for udev drm output updates */

   return ret;
}

EAPI void 
ecore_drm_output_free(Ecore_Drm_Output *output)
{
   Ecore_Drm_Output_Mode *mode;

   /* check for valid output */
   if (!output) return;

   /* free modes */
   EINA_LIST_FREE(output->modes, mode)
     free(mode);

   /* free strings */
   if (output->name) eina_stringshare_del(output->name);
   if (output->model) eina_stringshare_del(output->model);
   if (output->make) eina_stringshare_del(output->make);

   free(output);
}