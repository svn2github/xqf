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

#include <sys/types.h>
#include <stdio.h>	/* FILE, fprintf, fopen, fclose */
#include <string.h>	/* strlen, strcpy, strcmp, strtok */
#include <stdlib.h>	/* strtol */
#include <unistd.h>	/* stat */
#include <sys/stat.h>	/* stat, chmod */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */

#include <gtk/gtk.h>

#include "xqf.h"
#include "pref.h"
#include "launch.h"
#include "dialogs.h"
#include "utils.h"
#include "pixmaps.h"
#include "game.h"
#include "stat.h"


static struct player *poqs_parse_player(char *tokens[], int num);
static struct player *qw_parse_player(char *tokens[], int num);
static struct player *q2_parse_player(char *tokens[], int num);
#ifdef QSTAT23
static struct player *q3_parse_player(char *tokens[], int num);
#endif
static struct player *hl_parse_player(char *tokens[], int num);
#ifdef QSTAT_HAS_UNREAL_SUPPORT
static struct player *un_parse_player(char *tokens[], int num);
static void un_analyze_serverinfo (struct server *s);
#endif

static void quake_parse_server (char *tokens[], int num, struct server *s);

static void qw_analyze_serverinfo (struct server *s);
static void q2_analyze_serverinfo (struct server *s);
static void hl_analyze_serverinfo (struct server *s);
#ifdef QSTAT23
static void q3_analyze_serverinfo (struct server *s);
#endif

static int quake_config_is_valid (struct server *s);
#ifdef QSTAT23
static int quake3_config_is_valid (struct server *s);
#endif
static int config_is_valid_generic (struct server *s);

static int write_quake_variables (const struct condef *con);

static int q1_exec_generic (const struct condef *con, int forkit);
static int qw_exec (const struct condef *con, int forkit);
#ifdef QSTAT23
static int q3_exec (const struct condef *con, int forkit);
#endif
static int q2_exec_generic (const struct condef *con, int forkit);
static int ut_exec (const struct condef *con, int forkit);

static GList *q1_custom_cfgs (char *dir, char *game);
static GList *qw_custom_cfgs (char *dir, char *game);
static GList *q2_custom_cfgs (char *dir, char *game);
#ifdef QSTAT23
static GList *q3_custom_cfgs (char *dir, char *game);
#endif

static void quake_save_info (FILE *f, struct server *s);


struct game games[] = {
  {
    Q1_SERVER, 
    GAME_CONNECT | GAME_RECORD | GAME_QUAKE1_PLAYER_COLORS | GAME_QUAKE1_SKIN,
    "Quake",
    Q1_DEFAULT_PORT,
    0,
    "QS",
    "QS",
#ifdef QSTAT23
    "-qs",
#else
    NULL,
#endif
    NULL,
    &q1_pix,

    poqs_parse_player,
    quake_parse_server,
    NULL,
    quake_config_is_valid,
    write_quake_variables,
    q1_exec_generic,
    q1_custom_cfgs,
    quake_save_info
  },
  {
    QW_SERVER,
    GAME_CONNECT | GAME_RECORD | GAME_SPECTATE | GAME_PASSWORD | GAME_RCON | 
                                 GAME_QUAKE1_PLAYER_COLORS | GAME_QUAKE1_SKIN,
    "QuakeWorld",
    QW_DEFAULT_PORT,
    QWM_DEFAULT_PORT,

#ifdef QSTAT23
    "QWS",
    "QWS",
#else
    "QW",
    "QW",
#endif

    "-qws",

#ifdef QSTAT23
    "-qwm",
#else
    "-qw",
#endif

    &q_pix,

    qw_parse_player,
    quake_parse_server,
    qw_analyze_serverinfo,
    quake_config_is_valid,
    write_quake_variables,
    qw_exec,
    qw_custom_cfgs,
    quake_save_info
  },
  {
    Q2_SERVER,
    GAME_CONNECT | GAME_RECORD | GAME_SPECTATE | GAME_PASSWORD | GAME_RCON,
    "Quake2",
    Q2_DEFAULT_PORT,
    Q2M_DEFAULT_PORT,

#ifdef QSTAT23
    "Q2S",
    "Q2S",
#else
    "Q2",
    "Q2",
#endif

    "-q2s",
    "-q2m",
    &q2_pix,

    q2_parse_player,
    quake_parse_server,
    q2_analyze_serverinfo,
    quake_config_is_valid,
    write_quake_variables,
    qw_exec,
    q2_custom_cfgs,
    quake_save_info
  },

#ifdef QSTAT23
  {
    Q3_SERVER,
    GAME_CONNECT | GAME_PASSWORD | GAME_RCON,
    "Quake3: Arena",
    Q3_DEFAULT_PORT,
    Q3M_DEFAULT_PORT,
    "Q3S",
    "Q3S",
    "-q3s",
    "-q3m,48",
    &q3_pix,

    q3_parse_player,
    quake_parse_server,
    q3_analyze_serverinfo,
    quake3_config_is_valid,
    write_quake_variables,
    q3_exec,
    q3_custom_cfgs,
    quake_save_info
  },
#endif

  {
    H2_SERVER,
    GAME_CONNECT | GAME_QUAKE1_PLAYER_COLORS,
    "Hexen2",
    H2_DEFAULT_PORT,
    0,
    "H2S",
    "H2S",
    "-h2s",
    NULL,
    &hex_pix,

    poqs_parse_player,
    quake_parse_server,
    NULL,
    NULL,
    NULL,
    q1_exec_generic,
    NULL,
    quake_save_info
  },
  {
    HW_SERVER,
    GAME_CONNECT | GAME_QUAKE1_PLAYER_COLORS | GAME_RCON,
    "HexenWorld",
    HW_DEFAULT_PORT,
    0,
    "HWS",
    "HWS",
    "-hws",
    NULL,
    &hw_pix,

    qw_parse_player,
    quake_parse_server,
    qw_analyze_serverinfo,
    NULL,
    NULL,
    NULL,
    NULL,
    quake_save_info
  },
  {
    SN_SERVER,
    GAME_CONNECT | GAME_RCON,
    "Sin",
    SN_DEFAULT_PORT,
    0,
    "SNS",
    "SNS",
    "-sns",
    NULL,
    &sn_pix,

    q2_parse_player,
    quake_parse_server,
    q2_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q2_exec_generic,
    NULL,
    quake_save_info
  },
  {
    HL_SERVER,
    GAME_CONNECT | GAME_RCON,
    "Half-Life",
    HL_DEFAULT_PORT,
    HLM_DEFAULT_PORT,
    "HLS",
    "HLS",
    "-hls",

#ifdef QSTAT23
    "-hlm",
#else
    NULL,
#endif

    &hl_pix,

    hl_parse_player,
    quake_parse_server,
    hl_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q2_exec_generic,
    NULL,
    quake_save_info
  },
  {
    KP_SERVER,
    GAME_CONNECT | GAME_RCON,
    "Kingpin",
    KP_DEFAULT_PORT,
    0,
    "Q2S:KP",
    "Q2S",
    "-q2s",
    NULL,
    &kp_pix,

    q2_parse_player,
    quake_parse_server,
    q2_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q2_exec_generic,
    NULL,
    quake_save_info
  },
  {
    SFS_SERVER,
    GAME_CONNECT | GAME_RCON,
    "Soldier of Fortune",
    SFS_DEFAULT_PORT,
    0,
    "SFS",
    "SFS",
    "-sfs",
    "-q2s",  // assume a standard Quake2 style master.  May be wrong.
    &sfs_pix,

    q2_parse_player,
    quake_parse_server,
    q2_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q2_exec_generic,
    NULL,
    quake_save_info
  },
  {
    T2_SERVER,
    GAME_CONNECT | GAME_RCON,
    "Tribes 2",
    T2_DEFAULT_PORT,
    0,
    "T2S",
    "T2S",
    "-t2s",
    "-t2m", 
    &t2_pix,

    q2_parse_player,
    quake_parse_server,
    q2_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q2_exec_generic,
    NULL,
    quake_save_info
  },
  {
    HR_SERVER,
    GAME_CONNECT | GAME_RCON,
    "Heretic2",
    HR_DEFAULT_PORT,
    0,
    "Q2S:HR",
    "Q2S",
    "-q2s",
    NULL,
    &hr_pix,

    q2_parse_player,
    quake_parse_server,
    q2_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q2_exec_generic,
    NULL,
    quake_save_info
  },

#ifdef QSTAT_HAS_UNREAL_SUPPORT
  {
    UN_SERVER,
    GAME_CONNECT,
    "Unreal",
    UN_DEFAULT_PORT,
    0,
    "UNS",
    "UNS",
    "-uns",
    "uns",
    &un_pix,

    un_parse_player,
    quake_parse_server,
    un_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    ut_exec,
    NULL,
    quake_save_info
  },
#endif

  {
    UNKNOWN_SERVER,
    0,
    "unknown",
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  }
};


enum server_type id2type (const char *id) {
  int i;

  for (i = 0; i < GAMES_TOTAL; i++) {
    if (g_strcasecmp (id, games[i].id) == 0)
      return games[i].type;
  }

#ifdef QSTAT23

  if (g_strcasecmp (id, "QW") == 0)
    return QW_SERVER;
  if (g_strcasecmp (id, "Q2") == 0)
    return Q2_SERVER;

#else

  if (g_strcasecmp (id, "QWS") == 0)
    return QW_SERVER;
  if (g_strcasecmp (id, "Q2S") == 0)
    return Q2_SERVER;

#endif

  return UNKNOWN_SERVER;
}


/*
 *  This function should be used only for configuration saving to make
 *  possible qstat2.2<->qstat2.3 migration w/o loss of some game-specific 
 *  preferences.
 */

const char *type2id (enum server_type type) {
#ifdef QSTAT23
  return games[type].id;
#else
  switch (type) {

  case QW_SERVER:
    return "QWS";

  case Q2_SERVER:
    return "Q2S";

  default:
    return games[type].id;
  }
#endif
}


GtkWidget *game_pixmap_with_label (enum server_type type) {
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *pixmap;

  hbox = gtk_hbox_new (FALSE, 4);

  if (games[type].pix) {
    pixmap = gtk_pixmap_new (games[type].pix->pix, games[type].pix->mask);
    gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);
    gtk_widget_show (pixmap);
  }

  label = gtk_label_new (games[type].name);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  return hbox;
}


#ifdef QSTAT23
static void q3_unescape (char *dst, char *src) {

  while (*src) {
    if (src[0] != '^' || src[1] == '\0' || src[1] < '0' || src[1] > '9')
      *dst++ = *src++;
    else
      src += 2;
  }
  *dst = '\0';
}
#endif


static const char delim[] = " \t\n\r";


static struct player *poqs_parse_player (char *token[], int n) {
  struct player *player = NULL;
  long tmp;

  if (n < 7)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[1]) + 1);
  player->time  = strtol (token[4], NULL, 10);
  player->frags = strtosh (token[3]);
  player->ping = -1;

  tmp = strtol (token[5], NULL, 10);
  player->shirt = fix_qw_player_color (tmp);

  tmp = strtol (token[6], NULL, 10);
  player->pants = fix_qw_player_color (tmp);

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, token[1]);

  return player;
}


static struct player *qw_parse_player (char *token[], int n) {
  struct player *player = NULL;
  char *ptr;
  long tmp;
  int name_strlen;
  int skin_strlen;

  if (n < 8)
    return NULL;

  name_strlen = strlen (token[1]) + 1;
  skin_strlen = strlen (token[7]) + 1;

  player = g_malloc0 (sizeof (struct player) + name_strlen + skin_strlen);

  player->time  = strtol (token[3], NULL, 10);
  player->frags = strtosh (token[2]);
  player->ping  = strtosh (token[6]);

  tmp = strtol (token[4], NULL, 10);
  player->shirt = fix_qw_player_color (tmp);

  tmp = strtol (token[5], NULL, 10);
  player->pants = fix_qw_player_color (tmp);

  ptr = (char *) player + sizeof (struct player);
  player->name = strcpy (ptr, token[1]);

  player->skin = strcpy (ptr + name_strlen, token[7]);

  return player;
}


static struct player *q2_parse_player (char *token[], int n) {
  struct player *player = NULL;

  if (n < 3)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->time  = -1;
  player->frags = strtosh (token[1]);
  player->ping  = strtosh (token[2]);

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, token[0]);

  return player;
}


static struct player *q3_parse_player (char *token[], int n) {
  struct player *player = NULL;

  if (n < 3)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->time  = -1;
  player->frags = strtosh (token[1]);
  player->ping  = strtosh (token[2]);

  player->name = (char *) player + sizeof (struct player);
  q3_unescape (player->name, token[0]);

  return player;
}


static struct player *hl_parse_player (char *token[], int n) {
  struct player *player = NULL;

  if (n < 3)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->frags = strtosh (token[1]);
  player->time  = strtosh (token[2]);
  player->ping  = -1;

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, token[0]);

  return player;
}


#ifdef QSTAT_HAS_UNREAL_SUPPORT

static struct player *un_parse_player (char *token[], int n) {
  struct player *player = NULL;
  char *ptr;
  int name_strlen;
  int skin_strlen;
  int mesh_strlen;
  long tmp;

  /* player name, frags, ping time, team number, skin, mesh, face */

  if (n < 6)
    return NULL;

  name_strlen = strlen (token[0]) + 1;
  skin_strlen = strlen (token[4]) + 1;
  mesh_strlen = strlen (token[5]) + 1;

  player = g_malloc0 (sizeof (struct player) + name_strlen + 
                                                   skin_strlen + mesh_strlen);

  ptr = (char *) player + sizeof (struct player);
  player->name = strcpy (ptr, token[0]);
  ptr += name_strlen; 

  player->frags = strtosh (token[1]);
  player->ping = strtosh (token[2]);
  player->time  = -1;

  /* store team number to 'pants' */

  tmp = strtol (token[3], NULL, 10);
  player->pants = (tmp >= 0 && tmp <= 255)? tmp : 0;

  player->skin = strcpy (ptr, token[4]);
  ptr += skin_strlen;

  player->model = strcpy (ptr, token[5]);

  return player;
}

#endif


static void quake_parse_server (char *token[], int n, struct server *server) {
  /*
    This does both Quake (?) and Unreal servers
  */
  int poqs;
  int offs;

  /* debug (6, "quake_parse_server: Parse %s", server->name); */
  poqs = (server->type == Q1_SERVER || server->type == H2_SERVER);

  if (server->type != T2_SERVER) // T2 server only reports 6 tokens
  				 // in RAW format with qstat 2.4b/c
  				 // instead of 8 as Q2 etc..
  {
    if ((poqs && n != 10) || (!poqs && n != 8))
      return;
  }

#ifdef QSTAT23
  if (*(token[2])) {		/* if name is not empty */
    if (server->type != Q3_SERVER) {
      server->name = g_strdup (token[2]);
    }
    else {
      server->name = g_malloc (strlen (token[2]) + 1);
      q3_unescape (server->name, token[2]);
    }
  }
#else
  if (*(token[2]))		/* if name is not empty */
    server->name = g_strdup (token[2]);
#endif

  offs = (poqs)? 5 : 3;

  if (*(token[offs]))            /* if map is not empty */
    server->map  = g_strdup (token[offs]);

  if (server->type != T2_SERVER) // max/cur backwards in qstat 2.4b/c!
  {
    server->maxplayers = strtoush (token[offs + 1]);
    server->curplayers = strtoush (token[offs + 2]);
  }
  else
  {
    server->curplayers = strtoush (token[offs + 1]);
    server->maxplayers = strtoush (token[offs + 2]);
  }

  server->ping = strtosh (token[offs + 3]);
  server->retries = strtosh (token[offs + 4]);
}


static void qw_analyze_serverinfo (struct server *server) {
  char **info_ptr;
  long n;

  /* debug( 6, "qw_analyze_serverinfo: Analyze %s", server->name ); */
  if ((games[server->type].flags & GAME_SPECTATE) != 0)
    server->flags |= SERVER_SPECTATE;

  for (info_ptr = server->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "*gamedir") == 0) {
      server->game = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "*cheats") == 0) {
      server->flags |= SERVER_CHEATS;
    }
    else if (strcmp (*info_ptr, "maxspectators") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if (n <= 0)
        server->flags &= ~SERVER_SPECTATE;
    }
    else if (strcmp (*info_ptr, "needpass") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if ((n & 1) != 0)
	server->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	server->flags |= SERVER_SP_PASSWORD;
    }
  }
}


#ifdef QSTAT_HAS_UNREAL_SUPPORT
static void un_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->game = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "gamestyle") == 0) {
      s->gametype = info_ptr[1];
    }

    //password required?
    else if (strcmp (*info_ptr, "password") == 0 && strcmp(info_ptr[1],"False")) {
      s->flags |= SERVER_PASSWORD;
    }
  }

}
#endif //QSTAT_HAS_UNREAL_SUPPORT


static void q2_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gamedir") == 0) {
      s->game = info_ptr[1];
    }
    //determine mod
    else if (strcmp (*info_ptr, "gamename") == 0) {
	    switch (s->type)
	    {
		    case Q2_SERVER:
			/* We only set the mod if name is not baseq2 */
		    	if (strcmp(info_ptr[1],"baseq2"))
				s->gametype = info_ptr[1];
			break;
		    default:
				s->gametype = info_ptr[1];
			break;
	    }
    }

    else if (strcmp (*info_ptr, "cheats") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_CHEATS;
    }
    else if (s->type == Q2_SERVER && strcmp (*info_ptr, "protocol") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if (n < 34)
        s->flags &= ~SERVER_SPECTATE;
    }
    else if (strcmp (*info_ptr, "maxspectators") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if (n <= 0)
        s->flags &= ~SERVER_SPECTATE;
    }
    else if (strcmp (*info_ptr, "needpass") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if ((n & 1) != 0)
	s->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	s->flags |= SERVER_SP_PASSWORD;
    }

  }
  /* We only set the mod if gamedir is set */
  if (!s->game)
  {
	  s->gametype=NULL;
  }
}

static void hl_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gamedir") == 0) {
      s->game = info_ptr[1];
    }
    //determine mod
    else if (strcmp (*info_ptr, "gamename") == 0) {
        s->gametype = info_ptr[1];
    }
    
    //cheats enabled?
    else if (strcmp (*info_ptr, "sv_cheats") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_CHEATS;
    }

    //password required?
    else if (strcmp (*info_ptr, "sv_password") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_PASSWORD;
    }
  }

  // unset Mod if gamedir is valve
  if (s->game && !strcmp(s->game,"valve"))
  {
	  s->gametype=NULL;
  }
}


#ifdef QSTAT23

static char *q3a_gametypes[9] = {
  "FFA",		/* 0 = Free for All */
  "1v1",	 	/* 1 = Tournament */
  NULL,  		/* 2 = Single Player */
  "TDM",		/* 3 = Team Deathmatch */
  "CTF",		/* 4 = Capture the Flag */
  "1FCTF",		/* 5 = One Flag Capture the Flag */
  "OVR",		/* 6 = Overload */
  "HRV",		/* 7 = Harvester */
  " mod ",              /* 8+ is usually a client-side mod */
};

#define MAX_Q3A_OSP_TYPES 7
static char *q3a_osp_gametypes[MAX_Q3A_OSP_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "1v1",	 	/* 1 = Tournament */
  "FFA Comp",  		/* 2 = FFA, Competition */
  "TDM",		/* 3 = Team Deathmatch */
  "CTF",		/* 4 = Capture the Flag */
  "Clan Arena",		/* 5 = Clan Arena */
  "Custom OSP",         /* 6+ is usually a custom OSP setting */
};


static void q3_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  char *endptr;
  long n;
  int newtypes;

  int is_osp;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  // Get the server version first.
  // If it is 1.2+, the new game types are used
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
  if (strcmp (*info_ptr, "version") == 0) {
      if (info_ptr[1][3] >= '1')	// eg: 1 of 1.27
        if (info_ptr[1][5] >= '2')	// eg: 2 of 1.27
	  newtypes=1;
      else
        newtypes=0;
    }
  }

  /*
    Next figure out if it is an OSP server, if it is
    then the g_gametype has a different meaning
  */
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gamename") == 0) {
      if (strcmp (info_ptr[1], "osp") == 0) {
	is_osp = 1;
      } else {
	is_osp = 0;
      }
    }
  }


  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
 
    /*
      fs_game sets the active directory and is how one chooses
      a mod on the command line.  This should not show up in
      the server string but some times it does.  We will
      take either fs_game or gamename as the "game" string.
      --baa
    */
    if (strcmp (*info_ptr, "fs_game") == 0) {
      if (strcmp (info_ptr[1], "baseq3")) {
	s->game  = info_ptr[1];
      }
    }
    else if (strcmp (*info_ptr, "gamename") == 0) {
      if (strcmp (info_ptr[1], "baseq3")) {
	/* We only set the mod if the name is NOT baseq3. */
	s->game  = info_ptr[1];
      }
    }
    else if (!s->gametype && strcmp (*info_ptr, "g_gametype") == 0) {
      /* A value of 5 is usually a mod (Q3 1.17 and below) */
      /* New id missionpack defines 5-7 for other types.. */
      /* So only show new values if the protocol is 48+ */
      n = strtol (info_ptr[1], &endptr, 10);

      if (endptr == info_ptr[1]) {
	s->gametype = info_ptr[1];
      }
      else {
	if (is_osp == 1)
	{
	  if( n >= MAX_Q3A_OSP_TYPES )
	    n = MAX_Q3A_OSP_TYPES - 1;

	  s->gametype = q3a_osp_gametypes[n];
	  
	} 
	else if (newtypes == 1)
        {
  	  if (n < 10)
	    s->gametype = q3a_gametypes[n];
	}
	else
	{
  	  if (n < 5)
	    s->gametype = q3a_gametypes[n];
	  else
	    s->gametype = q3a_gametypes[8];
	}
      }
    }
    else if (strcmp (*info_ptr, "g_needpass") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if ((n & 1) != 0)
	s->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	s->flags |= SERVER_SP_PASSWORD;
    }
    else if (strcmp (*info_ptr, "cheats") == 0) {
      s->flags |= SERVER_CHEATS;
    }
    else if (strcmp (*info_ptr, "sv_privateClients") == 0) {
      s->private_client = strtol (info_ptr[1], NULL, 10);
    }
 

  }
}

#endif


static int quake_config_is_valid (struct server *s) {
  struct stat stat_buf;
  char *cfgdir;
  char *path;
  struct game *g = &games[s->type];

  switch (s->type) {

  case Q1_SERVER:
    cfgdir = "id1";
    break;

  case QW_SERVER:
    cfgdir = "id1";
    break;

  case Q2_SERVER:
    cfgdir = "baseq2";
    break;

  default:
    return FALSE;

  }

  if (g->cmd == NULL || g->cmd[0] == '\0') {
    dialog_ok (NULL, "%s command line is empty.", g->name);
    return FALSE;
  }

  if (g->real_dir != NULL && g->real_dir[0] != '\0') {
    if (stat (g->real_dir, &stat_buf) != 0 || !S_ISDIR (stat_buf.st_mode)) {
      dialog_ok (NULL, "\"%s\" is not a directory\n"
	              "Please specify correct %s working directory.", 
                       g->real_dir, g->name);
      return FALSE;
    }
  }

  path = file_in_dir (g->real_dir, cfgdir);

  if (stat (path, &stat_buf) || !S_ISDIR (stat_buf.st_mode)) {
    if (!g->real_dir || g->real_dir[0] == '\0') {
      dialog_ok (NULL, "Please specify correct %s working directory.", 
                                                                     g->name);
    }
    else {
      dialog_ok (NULL,  
		 "Directory \"%s\" doesn\'t contain \"%s\" subdirectory.\n"
		 "Please specify correct %s working directory.", 
		 g->real_dir, cfgdir, g->name);
    }
    g_free (path);
    return FALSE;
  }

  g_free (path);
  return TRUE;
}


#ifdef QSTAT23

static char *quake3_data_dir (char *dir) {
  struct stat stat_buf;
  char *rpath = NULL;
  char *path;

  if (dir == NULL || *dir == '\0') {
    dir = rpath = expand_tilde ("~/.q3a");
    if (!rpath)
      return NULL;
  }

  path = file_in_dir (dir, "baseq3");
  if (stat (path, &stat_buf) || !S_ISDIR (stat_buf.st_mode)) {
    g_free (path);
    path = file_in_dir (dir, "demoq3");
    if (stat (path, &stat_buf) || !S_ISDIR (stat_buf.st_mode)) {
      g_free (path);
      if (rpath) g_free (rpath);
      return NULL;
    }
  }
  if (rpath) g_free (rpath);
  return path;
}


static int quake3_config_is_valid (struct server *s) {
  struct game *g = &games[s->type];
  char *path;

  if (g->cmd == NULL || g->cmd[0] == '\0') {
    dialog_ok (NULL, "%s command line is empty.", g->name);
    return FALSE;
  }

  path = quake3_data_dir (g->real_dir);

  if (path == NULL) {
    if (!g->real_dir || g->real_dir[0] == '\0') {
      dialog_ok (NULL, 
		 "~/.q3a directory doesn\'t exist or doesn\'t contain\n"
		 "\"baseq3\" (\"demoq3\") subdirectory.\n"
		 "Please run %s client at least once before running XQF\n"
		 "or specify correct %s working directory.",
		 g->name, g->name);
    }
    else {
      dialog_ok (NULL, 
		 "\"%s\" directory doesn\'t exist or doesn\'t contain "
		 "\"baseq3\" (\"demoq3\") subdirectory.\n"
		 "Please specify correct %s working directory\n"
  	         "or leave it empty (~/.q3a is used by default)", 
		 g->real_dir, g->name);
    }
    return FALSE;
  }
  
  g_free (path);
  return TRUE;
}

#endif


static int config_is_valid_generic (struct server *s) {
  struct stat stat_buf;
  struct game *g = &games[s->type];

  if (g->cmd == NULL || g->cmd[0] == '\0') {
    dialog_ok (NULL, "%s command line is empty.", g->name);
    return FALSE;
  }

  if (g->real_dir != NULL && g->real_dir[0] != '\0') {
    if (stat (g->real_dir, &stat_buf) != 0 || !S_ISDIR (stat_buf.st_mode)) {
      dialog_ok (NULL, "\"%s\" is not a directory\n"
	              "Please specify correct %s working directory.", 
                       g->real_dir, g->name);
      return FALSE;
    }
  }

  return TRUE;
}


static FILE *open_cfg (const char *filename, int secure) {
  FILE *f;

  f = fopen (filename, "w");
  if (f) {
    if (secure)
      chmod (filename, S_IRUSR | S_IWUSR);

    fprintf (f, "//\n// generated by XQF, do not modify\n//\n");
  }
  return f;
}


static int real_password (const char *password) {
  if (!password || (password[0] == '1' && password[1] == '\0'))
    return FALSE;
  return TRUE;
}


static int write_passwords (const char *filename, const struct condef *con) {
  FILE *f;

  f = open_cfg (filename, TRUE);
  if (!f) 
    return FALSE;

  if (con->password)
    fprintf (f, "password \"%s\"\n", con->password);

  if (con->spectator_password)
    fprintf (f, "spectator \"%s\"\n", con->spectator_password);

  if (con->rcon_password)
    fprintf (f, "rcon_password \"%s\"\n", con->rcon_password);

  fclose (f);
  return TRUE;
}


static int write_q1_vars (const char *filename, const struct condef *con) {
  FILE *f;

  f = open_cfg (filename, FALSE);
  if (!f)
    return FALSE;

  if (default_name)
    fprintf (f, "name \"%s\"\n", default_name);

  fprintf (f, "color %d %d\n", default_q1_top_color, default_q1_bottom_color);

  if (games[Q1_SERVER].game_cfg)
    fprintf (f, "exec \"%s\"\n", games[Q1_SERVER].game_cfg);

  if (con->custom_cfg)
    fprintf (f, "exec \"%s\"\n", con->custom_cfg);

  fclose (f);
  return TRUE;
}


static int write_qw_vars (const char *filename, const struct condef *con) {
  FILE *f;
  int auto_pl;

  f = open_cfg (filename, FALSE);
  if (!f)
    return FALSE;

  if (default_name)
    fprintf (f, "name \"%s\"\n", default_name);

  if (default_qw_skin)
    fprintf (f, "skin \"%s\"\n", default_qw_skin);

  fprintf (f, "team \"%s\"\n", (default_team)? default_team : "");
  fprintf (f, "topcolor    \"%d\"\n", default_qw_top_color);
  fprintf (f, "bottomcolor \"%d\"\n", default_qw_bottom_color);

  switch (pushlatency_mode) {

  case 1:	/* automatic pushlatency */
    if (con->s->ping <= 0)
      auto_pl = 10;				/* "min" value */
    else if (con->s->ping >= 2000)
      auto_pl = 1000;				/* "max" value */
    else {
      auto_pl = con->s->ping / 2;
      auto_pl = ((auto_pl + 9) / 10) * 10;	/* beautify it */
    }
    fprintf (f, "pushlatency %d\n", -auto_pl);
    break;

  case 2:	/* fixed value */
    fprintf (f, "pushlatency %d\n", pushlatency_value);
    break;

  default:	/* do not touch */
    break;

  }

  fprintf (f, "rate        \"%d\"\n", default_rate);
  fprintf (f, "cl_nodelta  \"%d\"\n", default_cl_nodelta);
  fprintf (f, "cl_predict_players \"%d\"\n", default_cl_predict);
  fprintf (f, "noaim       \"%d\"\n", default_noaim);
  fprintf (f, "noskins     \"%d\"\n", default_noskins);
  if (default_w_switch >= 0)
    fprintf (f, "setinfo w_switch \"%d\"\n", default_w_switch);
  if (default_b_switch >= 0)
    fprintf (f, "setinfo b_switch \"%d\"\n", default_b_switch);

  if (games[QW_SERVER].game_cfg)
    fprintf (f, "exec \"%s\"\n", games[QW_SERVER].game_cfg);

  if (con->custom_cfg)
    fprintf (f, "exec \"%s\"\n", con->custom_cfg);

  fclose (f);
  return TRUE;
}


static int write_q2_vars (const char *filename, const struct condef *con) {
  FILE *f;

  f = open_cfg (filename, FALSE);
  if (!f)
    return FALSE;

  if (default_name)
    fprintf (f, "set name \"%s\"\n", default_name);

  if (default_q2_skin)
    fprintf (f, "set skin \"%s\"\n", default_q2_skin);

  fprintf (f, "set rate        \"%d\"\n", default_rate);
  fprintf (f, "set cl_nodelta  \"%d\"\n", default_cl_nodelta);
  fprintf (f, "set cl_predict  \"%d\"\n", default_cl_predict);
  fprintf (f, "set cl_noskins  \"%d\"\n", default_noskins);

  if (games[Q2_SERVER].game_cfg)
    fprintf (f, "exec \"%s\"\n", games[Q2_SERVER].game_cfg);

  if (con->custom_cfg)
    fprintf (f, "exec \"%s\"\n", con->custom_cfg);

  fclose (f);
  return TRUE;
}


#ifdef QSTAT23

static int write_q3_vars (const char *filename, const struct condef *con) {
  FILE *f;

  f = open_cfg (filename, FALSE);
  if (!f)
    return FALSE;

  if (default_name)
    fprintf (f, "set name \"%s\"\n", default_name);

  if (games[Q3_SERVER].game_cfg)
    fprintf (f, "exec \"%s\"\n", games[Q3_SERVER].game_cfg);

  if (con->custom_cfg)
    fprintf (f, "exec \"%s\"\n", con->custom_cfg);

  fclose (f);
  return TRUE;
}

#endif


static int write_quake_variables (const struct condef *con) {
#ifdef QSTAT23
  char *path;
#endif
  char *file = NULL;
  int res = TRUE;

  switch (con->s->type) {

  case Q1_SERVER:
    file = file_in_dir (games[Q1_SERVER].real_dir, "id1/" EXEC_CFG);
    res =  write_q1_vars (file, con);
    break;

  case QW_SERVER:
    file = file_in_dir (games[QW_SERVER].real_dir, "id1/" EXEC_CFG);
    res =  write_qw_vars (file, con);
    break;

  case Q2_SERVER:
    file = file_in_dir (games[Q2_SERVER].real_dir, "baseq2/" EXEC_CFG);
    res = write_q2_vars (file, con);
    break;

#ifdef QSTAT23
  case Q3_SERVER:
    path = quake3_data_dir (games[Q3_SERVER].dir);
    if (path) {
      file = file_in_dir (path, EXEC_CFG);
      g_free (path);
      res = write_q3_vars (file, con);
    }
    break;
#endif

  default:
    return FALSE;

  }

  if (file) {
    if (!res) {
      if (!dialog_yesno (NULL, 1, "Launch", "Cancel", 
             "Cannot write to file \"%s\".\n\nLaunch client anyway?", file)) {
	g_free (file);
	return FALSE;
      }
    }
    g_free (file);
  }

  return TRUE;
}


static int is_dir (const char *dir, const char *gamedir) {
  struct stat stat_buf;
  char *path;
  int res = FALSE;

  if (dir && gamedir) {
    path = file_in_dir (dir, gamedir);
    res = (stat (path, &stat_buf) == 0 && S_ISDIR (stat_buf.st_mode));
    g_free (path);
  }
  return res;
}


static int q1_exec_generic (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char buf[16];
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (default_nosound)
    argv[argi++] = "-nosound";

  if (default_nocdaudio)
    argv[argi++] = "-nocdaudio";

  if (con->gamedir) {
    argv[argi++] = "-game";
    argv[argi++] = con->gamedir;
  }

  argv[argi++] = "+exec";
  argv[argi++] = EXEC_CFG;

  if (con->demo) {
    argv[argi++] = "+record";
    argv[argi++] = con->demo;
  }
  
  if (con->server) {
    if (con->s->port != g->default_port) {
      g_snprintf (buf, 16, "%d", con->s->port);
      argv[argi++] = "+port";
      argv[argi++] = buf;
    }
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}


static int qw_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *q_passwd;
  char *cmd;
  char *file;
  struct game *g = &games[con->s->type];
  int retval;

  switch (con->s->type) {

  case QW_SERVER:
    q_passwd = "id1/" PASSWORD_CFG;
    break;

  case Q2_SERVER:
    q_passwd = "baseq2/" PASSWORD_CFG;
    break;

  default:
    return;
  }

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  switch (con->s->type) {

  case QW_SERVER:
    if (default_nosound)
      argv[argi++] = "-nosound";

    if (default_nocdaudio)
      argv[argi++] = "-nocdaudio";

    if (con->gamedir) {
      argv[argi++] = "-gamedir";
      argv[argi++] = con->gamedir;
    }

    break;

  case Q2_SERVER:
    if (default_nosound) {
      argv[argi++] = "+set";
      argv[argi++] = "s_initsound";
      argv[argi++] = "0";
    }

    argv[argi++] = "+set";
    argv[argi++] = "cd_nocd";
    argv[argi++] = (default_nocdaudio)? "1" : "0";

    if (con->gamedir) {
      argv[argi++] = "+set";
      argv[argi++] = "game";
      argv[argi++] = con->gamedir;
    }

    break;

  default:
    break;

  }

  if (con->password || con->rcon_password || 
                                    real_password (con->spectator_password)) {
    file = file_in_dir (g->real_dir, q_passwd);

    if (!write_passwords (file, con)) {
      if (!dialog_yesno (NULL, 1, "Launch", "Cancel", 
             "Cannot write to file \"%s\".\n\nLaunch client anyway?", file)) {
	g_free (file);
	g_free (cmd);
	return;
      }
    }

    g_free (file);

    argv[argi++] = "+exec";
    argv[argi++] = PASSWORD_CFG;
  }
  else {
    if (con->spectator_password) {
	argv[argi++] = "+spectator";
	argv[argi++] = con->spectator_password;
    }
  }

  if (con->s->type == Q2_SERVER || (con->s->type == QW_SERVER && 
				    (!con->gamedir || 
				     g_strcasecmp (con->gamedir, "qw") == 0 ||
				     !is_dir (g->real_dir, con->gamedir)) ) ) {
    argv[argi++] = "+exec";
    argv[argi++] = EXEC_CFG;
  }

  if (con->server) {
    if (!con->demo || con->s->type == Q2_SERVER) {
      argv[argi++] = "+connect";
      argv[argi++] = con->server;
    }
    if (con->demo) {
      argv[argi++] = "+record";
      argv[argi++] = con->demo;
      if (con->s->type == QW_SERVER)
	argv[argi++] = con->server;
    }
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}


#ifdef QSTAT23

static int q3_exec (const struct condef *con, int forkit) {
  char *argv[64];
  int argi = 0;
  char *cmd;
  char *game_dir;
  char *file;

  char *protocol;
  char *tmp_cmd;
  FILE* tmp_fp;
  char *fs_game;

  struct game *g = &games[con->s->type];
  int retval;

  int is_so_mod = 0;

  cmd = strdup_strip (g->cmd);
  /*
    Figure out what protocal the server
    is running so we can try to connect
    with a specialized quake3 script.
    Please not that for this to work you
    have to specify the full path to quake3
    or have it in your cwd.  You need to name 
    the scripts like quake3proto48. --baa
  */
  protocol = find_server_setting_for_key ("protocol", con->s->info);
  debug (5, "q3_exec() -- Command: '%s', protocol '%s'", cmd, protocol);
  tmp_cmd = g_malloc0 (sizeof (char) * (strlen (cmd) + 10 ));
  if( strcspn (cmd, " ")) {
    strncpy (tmp_cmd, cmd, strcspn (cmd, " "));
  } else {
    strcpy (tmp_cmd, cmd);
  }
  strcat (tmp_cmd, "proto" ); 
  strcat (tmp_cmd, protocol);
  strcat (tmp_cmd, "\0");
  debug (5, "q3_exec() -- Check for '%s' as a command", tmp_cmd);
  if (tmp_fp = fopen( tmp_cmd, "r" )){
    fclose (tmp_fp);
    debug (5, "q3_exec() -- Could open %s, use it to run q3a.", tmp_cmd);
    argv[argi++] = tmp_cmd;
    strtok (cmd, delim);
  } else {
    argv[argi++] = strtok (cmd, delim);
  }

  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (default_nosound) {
    argv[argi++] = "+set";
    argv[argi++] = "s_initsound";
    argv[argi++] = "0";
  }

  if (con->password || con->rcon_password) {
    game_dir = quake3_data_dir (g->real_dir);

    if (game_dir) {
      file = file_in_dir (game_dir, PASSWORD_CFG);
      if (!write_passwords (file, con)) {
	if (!dialog_yesno (NULL, 1, "Launch", "Cancel", 
             "Cannot write to file \"%s\".\n\nLaunch client anyway?", file)) {
	  g_free (file);
	  g_free (cmd);
	  return;
	}
      }
      g_free (file);
      g_free (game_dir);
    }

    argv[argi++] = "+exec";
    argv[argi++] = PASSWORD_CFG;
  }

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  if (con->demo) {
    argv[argi++] = "+record";
    argv[argi++] = con->demo;
  }

  /* If the fs_game or -- more likely -- gamename is set, we want to
     put fs_game on the command line so that the mod is loaded when
     we connect. */
  if( fs_game =  find_server_setting_for_key ("fs_game", con->s->info)){
    /* This will rarely (never?) be called. */
    argv[argi++] = "+set fs_game";
    argv[argi++] = fs_game;
    if (strcmp( fs_game, "arena") == 0)  is_so_mod = 1;

    
  } else if( fs_game =  find_server_setting_for_key ("gamename", con->s->info)){
    if (strcmp( fs_game, "arena") == 0 )  is_so_mod = 1;
    argv[argi++] = "+set fs_game";
    argv[argi++] = fs_game;
  }

  /* FIX ME
    BAD! special case for rocket arena 3 aka "arena", it needs sv_pure 0
    to run properly.  This is for at least 1.27g.
  */

  if (is_so_mod){
    argv[argi++] = "+set sv_pure 0 +set vm_game 0 +set vm_cgame 0 +set vm_ui 0";
  }

  argv[argi] = NULL;

#if 1
  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);
#else
  if (get_debug_level()){
    char **argptr = argv;
    fprintf (stderr, "q3_exec() -- Would have EXEC> ");
    while (*argptr)
      fprintf (stderr, "%s ", *argptr++);
    fprintf (stderr, "\n");
  }

  retval = 1;
#endif
  g_free (cmd);
  g_free (tmp_cmd);
  return retval;
}

#endif


static int q2_exec_generic (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (default_nosound) {
    argv[argi++] = "+set";
    argv[argi++] = "s_initsound";
    argv[argi++] = "0";
  }

  argv[argi++] = "+set";
  argv[argi++] = "cd_nocd";
  argv[argi++] = (default_nocdaudio)? "1" : "0";

  if (con->gamedir) {
    argv[argi++] = "+set";
    argv[argi++] = "game";
    argv[argi++] = con->gamedir;
  }

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  if (con->demo) {
    argv[argi++] = "+record";
    argv[argi++] = con->demo;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}

static int ut_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

// Pass server IP address first otherwise it won't work.
// Make sure ut/ut script (from installed game) contains
// exec "./ut-bin" $* -log and not -log $* at the end
// otherwise XQF you can not connect via the command line!

  if (con->server) {
    argv[argi++] = con->server;
  }

  if (default_nosound) {
    argv[argi++] = "-nosound";
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}


static char *dir_custom_cfg_filter (const char *dir, const char *str) {
  static const char *cfgext[] = { ".cfg", ".scr", ".rc", NULL };
  const char **ext;
  int len;

  if (!str)
    return NULL;

  len = strlen (str);

  for (ext = cfgext; *ext; ext++) {
    if (len > strlen (*ext) && 
                          strcasecmp (str + len - strlen (*ext), *ext) == 0) {
      return g_strdup (str);
    }
  }

  return NULL;
}


static GList *custom_cfg_filter (GList *list) {
  GList *tmp;
  GList *res = NULL;
  char *str;

  for (tmp = list; tmp; tmp = tmp->next) {
    str = (char *) tmp->data;
    if (strcmp (str, EXEC_CFG) && strcmp (str, PASSWORD_CFG) && 
        strcmp (str, "__qf__.cfg") && strcasecmp (str, "config.cfg") && 
        strcasecmp (str, "server.cfg") && strcasecmp (str, "autoexec.cfg") &&
        strcasecmp (str, "quake.rc") && strcasecmp (str, "q3config.cfg")) {
      res = g_list_prepend (res, str);
    }
    else {
      g_free (str);
    }
  }

  g_list_free (list);
  res = g_list_reverse (res);
  return res;
}


static GList *quake_custom_cfgs (const char *path, const char *mod_path) {
  GList *cfgs = NULL;
  GList *mod_cfgs = NULL;

  if (path) {
    cfgs = dir_to_list (path, dir_custom_cfg_filter);

    if (mod_path) {
      mod_cfgs = dir_to_list (mod_path, dir_custom_cfg_filter);
      cfgs = merge_sorted_string_lists (cfgs, mod_cfgs);
    }

    cfgs = custom_cfg_filter (cfgs);
  }

  return cfgs;
}


static GList *q1_custom_cfgs (char *dir, char *game) {
  GList *cfgs;
  char *qdir;
  char *path = NULL;
  char *mod_path = NULL;

  qdir = expand_tilde ((dir)? dir : games[Q1_SERVER].dir);

  path = file_in_dir (qdir, "id1");
  if (game)
    mod_path = file_in_dir (qdir, game);

  g_free (qdir);

  cfgs = quake_custom_cfgs (path, mod_path);

  g_free (path);
  if (mod_path)
    g_free (mod_path);

  return cfgs;
}


static GList *qw_custom_cfgs (char *dir, char *game) {
  GList *cfgs;
  char *qdir;
  char *path = NULL;
  char *mod_path = NULL;

  qdir = expand_tilde ((dir)? dir : games[QW_SERVER].dir);

  path = file_in_dir (qdir, "id1");
  mod_path = file_in_dir (qdir, (game)? game : "qw");

  g_free (qdir);

  cfgs = quake_custom_cfgs (path, mod_path);

  g_free (path);
  if (mod_path)
    g_free (mod_path);

  return cfgs;
}


static GList *q2_custom_cfgs (char *dir, char *game) {
  GList *cfgs;
  char *qdir;
  char *path = NULL;
  char *mod_path = NULL;

  qdir = expand_tilde ((dir)? dir : games[Q2_SERVER].dir);

  path = file_in_dir (qdir, "baseq2");
  if (game)
    mod_path = file_in_dir (qdir, game);

  g_free (qdir);

  cfgs = quake_custom_cfgs (path, mod_path);

  g_free (path);
  if (mod_path)
    g_free (mod_path);

  return cfgs;
}


#ifdef QSTAT23

static GList *q3_custom_cfgs (char *dir, char *game) {
  GList *cfgs;
  char *qdir;
  char *path = NULL;
  char *mod_path = NULL;

  qdir = expand_tilde ((dir)? dir : games[Q3_SERVER].dir);

  path = quake3_data_dir (qdir);
  if (game)
    mod_path = file_in_dir (qdir, game);

  g_free (qdir);

  cfgs = quake_custom_cfgs (path, mod_path);

  g_free (path);
  if (mod_path)
    g_free (mod_path);

  return cfgs;
}

#endif


static void quake_save_server_rules (FILE *f, struct server *s) {
  char **info;

  if (!s || !s->info)
    return;

  info = s->info;

  while (info[0]) {

    if (info[1])
      fprintf (f, "%s=%s", info[0], info[1]);
    else
      fprintf (f, "%s", info[0]);

    info += 2;

    if (info[0])
      fputc (QSTAT_DELIM, f);
  }

  fputc ('\n', f);
}


static void quake_save_info (FILE *f, struct server *s) {
  struct player *p;
  GSList *list;

  if (!s->name && !s->map && !s->players && !s->info && s->ping < 0)
    return;

  fprintf (f, "%ld\n", s->refreshed);

  switch (s->type) {

    /* POQS */

  case Q1_SERVER:
  case H2_SERVER: 
    fprintf (f, 
	     "%s" QSTAT_DELIM_STR
	     "%s:%d" QSTAT_DELIM_STR
	     "%s" QSTAT_DELIM_STR
	     "%s:%d" QSTAT_DELIM_STR
	     "0"  QSTAT_DELIM_STR
	     "%s" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d\n", 
	     games[s->type].id,
	     inet_ntoa (s->host->ip), s->port,
	     (s->name)? s->name : "",
	     inet_ntoa (s->host->ip), s->port,
	     (s->map)? s->map : "",
	     s->maxplayers,
	     s->curplayers,
	     s->ping,
	     s->retries);
    break;

    /* QW, Q2 */

  default:
    fprintf (f, 
	     "%s" QSTAT_DELIM_STR
	     "%s:%d" QSTAT_DELIM_STR
	     "%s" QSTAT_DELIM_STR
	     "%s" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d\n", 
	     games[s->type].id,
	     inet_ntoa (s->host->ip), s->port,
	     (s->name)? s->name : "",
	     (s->map)? s->map : "",
	     s->maxplayers, 
	     s->curplayers,
	     s->ping,
	     s->retries);
    break;

  } /* switch (s->type) */

  quake_save_server_rules (f, s);

  if (default_save_plrinfo) {

    for (list = s->players; list; list = list->next) {
      p = (struct player *) list->data;

      switch (s->type) {

	/* POQS */

      case Q1_SERVER:
      case H2_SERVER: 
	fprintf (f, 
		 "0"  QSTAT_DELIM_STR
		 "%s" QSTAT_DELIM_STR
	              QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d\n",
		 (p->name)? p->name : "",
		 p->frags,
		 p->time,
		 p->shirt,
		 p->pants);
	break;

	/* QW */

      case QW_SERVER:
      case HW_SERVER:
	fprintf (f, 
		 "0"  QSTAT_DELIM_STR
		 "%s" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%s\n",
		 (p->name)? p->name : "",
		 p->frags,
		 p->time,
		 p->shirt,
		 p->pants,
		 p->ping,
		 (p->skin)? p->skin : "");
	break;

#ifdef QSTAT_HAS_UNREAL_SUPPORT

      case UN_SERVER:
	fprintf (f, 
		 "%s" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%s" QSTAT_DELIM_STR 
		 "%s" QSTAT_DELIM_STR
		 "\n",
		 (p->name)? p->name : "",
		 p->frags,
		 p->ping, 
		 p->pants, 
		 (p->skin)? p->skin : "",
		 (p->model)? p->model : "");
	break;

#endif

	/* Q2, etc... */

      default:
	fprintf (f, 
		 "%s" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d\n",
		 (p->name)? p->name : "",
		 p->frags,
		 (s->type == HL_SERVER)? p->time : p->ping);
	break;

      } /* switch (s->type) */

    } /* for (list = s->players ... */

  }  /* if (default_save_plrinfo) */

  fputc ('\n', f);
}

