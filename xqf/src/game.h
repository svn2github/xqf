/* XQF - Quake server browser and launcher
 * Copyright (C) 1998-2000 Roman Pozlevich <roma@botik.ru>
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

#ifndef __GAME_H__
#define __GAME_H__

#include <sys/types.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "xqf.h"
#include "launch.h"
#include "pixmaps.h"


// game->flags
enum {
  GAME_CONNECT			= 0x0001,
  GAME_RECORD			= 0x0002,
  GAME_SPECTATE			= 0x0004,
  GAME_PASSWORD			= 0x0008,
  GAME_RCON			= 0x0010,
  GAME_QUAKE1_PLAYER_COLORS	= 0x0100,
  GAME_QUAKE1_SKIN		= 0x0200,
  GAME_QUAKE3_MASTERPROTOCOL	= 0x0400, // master server protocol version is in games_data["masterprotocol"]
  GAME_LAUNCH_HOSTPORT		= 0x0800, // use hostport rule as port when launching
};

struct game {
  enum server_type type;
  unsigned long flags;

  char *name;
  unsigned short default_port;
  unsigned short default_master_port;

  char *id;
  char *qstat_str;
  char *qstat_option;
  char *qstat_master_option;
  const char* icon; // xpm symbol name
  struct pixmap *pix;

  struct player * (*parse_player) (char *tokens[], int num, struct server *s);
  void (*parse_server) (char *tokens[], int num, struct server *s);
  void (*analyze_serverinfo) (struct server *s);
  int (*config_is_valid) (struct server *s);
  int (*write_config) (const struct condef *con);
  int (*exec_client) (const struct condef *con, int forkit);
  GList * (*custom_cfgs) (char *dir, char *game);
  void (*save_info) (FILE *f, struct server *s);

  /* map functions */
  /** determine installed maps, destroys previous data */
  void (*init_maps)(enum server_type);
  /** return true if s->map is installed, false otherwise */
  gboolean (*has_map)(struct server* s);
  /** acquire image data, function allocates space in buf, returns size. buf
   * must be freed by caller */
  size_t (*get_mapshot)(struct server* s, guchar** buf);

  char *arch_identifier;
  enum CPU (*identify_cpu) (struct server *s, const char *versionstr);
  enum OS (*identify_os) (struct server *s, char *versionstr);

  char *cmd;
  char *dir;
  char *real_dir;
  char *game_cfg;
  GData *games_data;
  GSList *custom_args;
 /** game specific private data */
  void *pd;
};

extern struct game games[];

extern	enum server_type id2type (const char *id);
extern	const char *type2id (enum server_type type);
extern	GtkWidget *game_pixmap_with_label (enum server_type type);

// retreive game specific value that belongs to key, do not free return value!
const char* game_get_attribute(enum server_type type, const char* key);
// set game specific key/value pair, value is _not_ copied
const char* game_set_attribute(enum server_type type, const char* key, char* value);

void init_games(void);

#endif /* __GAME_H__ */
