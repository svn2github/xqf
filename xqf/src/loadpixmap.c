/* XQF - Quake server browser and launcher
 * Functions for loading image files from disk
 * Copyright (C) 2002 Ludwig Nussel <l-n@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

// modified version of what glade generates

#include "gnuconfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "loadpixmap.h"
#include "i18n.h"

/* This is an internally used function to check if a pixmap file exists. */
static gchar* check_file_exists        (const gchar     *directory,
                                        const gchar     *filename);

/* This is an internally used function to create pixmaps. */
static GtkWidget* create_dummy_pixmap  (GtkWidget       *widget);


/* This is a dummy pixmap we use when a pixmap can't be found. */
static char *dummy_pixmap_xpm[] = {
  /* columns rows colors chars-per-pixel */
  "1 1 1 1",
  "  c None",
  /* pixels */
  " "
};

/* This is an internally used function to create pixmaps. */
static GtkWidget*
create_dummy_pixmap                    (GtkWidget       *widget)
{
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;

  colormap = gtk_widget_get_colormap (widget);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &mask,
                                                     NULL, dummy_pixmap_xpm);
  if (gdkpixmap == NULL)
    g_error ("Couldn't create replacement pixmap.");
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);
  return pixmap;
}

static GList *pixmaps_directories = NULL;

/* Use this function to set the directory containing installed pixmaps. */
void
add_pixmap_directory                   (const gchar     *directory)
{
  pixmaps_directories = g_list_prepend (pixmaps_directories,
                                        g_strdup (directory));
}

gchar* find_pixmap_directory(const gchar* filename)
{
  gchar* found_filename = NULL;
  GList *elem;

  elem = pixmaps_directories;
  while (elem)
  {
    found_filename = check_file_exists ((gchar*)elem->data, filename);
    if (found_filename)
      break;
    elem = elem->next;
  }
  return found_filename;
}

/* This is an internally used function to create pixmaps. */
GtkWidget*
load_pixmap                          (GtkWidget       *widget,
				      const gchar     *filename)
{
  gchar *found_filename = NULL;
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;

#ifdef USE_GTK2
  GError *err = NULL;
#endif


  if (!filename || !filename[0])
    return create_dummy_pixmap (widget);

  found_filename = find_pixmap_directory(filename);

#if 0 //crap...
  /* If we haven't found the pixmap, try the source directory. */
  if (!found_filename)
  {
    found_filename = check_file_exists ("src/xpm", filename);
  }
#endif

  if (!found_filename)
  {
    g_warning (_("Couldn't find pixmap file: %s"), filename);
    return create_dummy_pixmap (widget);
  }

  if(strlen(found_filename)>4 && !strcmp(found_filename+strlen(found_filename)-4,".xpm"))
  {
    colormap = gtk_widget_get_colormap (widget);
    gdkpixmap = gdk_pixmap_colormap_create_from_xpm (NULL, colormap, &mask,
						   NULL, found_filename);
  }
  else
  {

/*FIXME_GTK2: need GError*/
#ifdef USE_GTK2
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(found_filename, &err);
#else
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(found_filename);
#endif
    if (pixbuf == NULL)
    {
      g_warning (_("Error loading pixmap file: %s"), found_filename);
      g_free (found_filename);
      return create_dummy_pixmap (widget);
    }

    gdk_pixbuf_render_pixmap_and_mask(pixbuf,&gdkpixmap,&mask,255);

    gdk_pixbuf_unref(pixbuf);
  }

  if (gdkpixmap == NULL)
  {
    g_warning (_("Error loading pixmap file: %s"), found_filename);
    g_free (found_filename);
    return create_dummy_pixmap (widget);
  }
  g_free (found_filename);
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  if(mask) gdk_bitmap_unref (mask);
  return pixmap;
}

/* This is an internally used function to check if a pixmap file exists. */
static gchar*
check_file_exists                      (const gchar     *directory,
                                        const gchar     *filename)
{
  gchar *full_filename;
  struct stat s;
  gint status;

  full_filename = (gchar*) g_malloc (strlen (directory) + 1
                                     + strlen (filename) + 1);
  strcpy (full_filename, directory);
  strcat (full_filename, G_DIR_SEPARATOR_S);
  strcat (full_filename, filename);

  status = stat (full_filename, &s);
  if (status == 0 && S_ISREG (s.st_mode))
    return full_filename;
  g_free (full_filename);
  return NULL;
}

