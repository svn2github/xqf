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

#include "gnuconfig.h"

#include <sys/types.h>
#include <stdio.h>	/* FILE, fprintf, fopen, fclose */
#include <string.h>	/* strlen, strcpy, strcmp, strtok */
#include <stdlib.h>	/* strtol */
#include <unistd.h>	/* stat */
#include <sys/stat.h>	/* stat, chmod */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "pref.h"
#include "launch.h"
#include "dialogs.h"
#include "utils.h"
#include "pixmaps.h"
#include "game.h"
#include "stat.h"
#include "server.h"
#include "statistics.h"
#include "server.h"
#include "config.h"
#include "debug.h"
#include "q3maps.h"
#include "utmaps.h"

static struct player *poqs_parse_player(char *tokens[], int num, struct server *s);
static struct player *qw_parse_player(char *tokens[], int num, struct server *s);
static struct player *q2_parse_player(char *tokens[], int num, struct server *s);
static struct player *q3_parse_player(char *tokens[], int num, struct server *s);
static struct player *t2_parse_player(char *tokens[], int num, struct server *s);
static struct player *hl_parse_player(char *tokens[], int num, struct server *s);
static struct player *un_parse_player(char *tokens[], int num, struct server *s);
static struct player *descent3_parse_player(char *tokens[], int num, struct server *s);
static struct player *savage_parse_player (char *token[], int n, struct server *s);

static void quake_parse_server (char *tokens[], int num, struct server *s);

static void qw_analyze_serverinfo (struct server *s);
static void q2_analyze_serverinfo (struct server *s);
static void hl_analyze_serverinfo (struct server *s);
static void t2_analyze_serverinfo (struct server *s);
static void q3_analyze_serverinfo (struct server *s);
static void un_analyze_serverinfo (struct server *s);
static void bf1942_analyze_serverinfo (struct server *s);
static void descent3_analyze_serverinfo (struct server *s);
static void savage_analyze_serverinfo (struct server *s);

static int quake_config_is_valid (struct server *s);
static int quake3_config_is_valid (struct server *s);
static int config_is_valid_generic (struct server *s);

static int write_quake_variables (const struct condef *con);

static int q1_exec_generic (const struct condef *con, int forkit);
static int qw_exec (const struct condef *con, int forkit);
static int q2_exec (const struct condef *con, int forkit);
static int q2_exec_generic (const struct condef *con, int forkit);
static int q3_exec (const struct condef *con, int forkit);
static int hl_exec(const struct condef *con, int forkit);
static int ut_exec (const struct condef *con, int forkit);
static int t2_exec (const struct condef *con, int forkit);
static int gamespy_exec (const struct condef *con, int forkit);
static int bf1942_exec (const struct condef *con, int forkit);
static int exec_generic (const struct condef *con, int forkit);
static int ssam_exec (const struct condef *con, int forkit);
static int savage_exec (const struct condef *con, int forkit);

static GList *q1_custom_cfgs (char *dir, char *game);
static GList *qw_custom_cfgs (char *dir, char *game);
static GList *q2_custom_cfgs (char *dir, char *game);
static GList *q3_custom_cfgs (char *dir, char *game);

static void quake_save_info (FILE *f, struct server *s);

char **get_custom_arguments(enum server_type type, const char *gamestring);

static void quake_init_maps(enum server_type);
static gboolean quake_has_map(struct server* s);

static void q3_init_maps(enum server_type);
static size_t q3_get_mapshot(struct server* s, guchar** buf);

static void unreal_init_maps(enum server_type);
static gboolean unreal_has_map(struct server* s);

struct quake_private
{
  GHashTable* maphash;
  const char* home; // until we have something useful
};

struct unreal_private
{
  GHashTable* maphash;
  const char* suffix;
};

static struct unreal_private ut_private = { NULL, ".unr" };
static struct unreal_private ut2_private = { NULL, ".ut2" };
static struct unreal_private rune_private = { NULL, ".run" };
static struct unreal_private postal2_private = { NULL, ".fuk" };
static struct unreal_private aao_private = { NULL, ".aao" };

static struct quake_private q1_private, qw_private, q2_private, hl_private;
static struct quake_private q3_private = { NULL, "~/.q3a" };
static struct quake_private wolf_private = { NULL, "~/.wolf" };
static struct quake_private wolfet_private = { NULL, "~/.etwolf" };
static struct quake_private mohaa_private = { NULL, "~/.mohaa" };
static struct quake_private cod_private = { NULL, NULL }; // no home, wine only

#if 0
struct game games[] = {
  {
    Q1_SERVER, 
    GAME_CONNECT | GAME_RECORD | GAME_QUAKE1_PLAYER_COLORS | GAME_QUAKE1_SKIN,
    "Quake",
    Q1_DEFAULT_PORT,
    0,
    "QS",
    "QS",
    "-qs",
    NULL,
    &q1_pix,

    poqs_parse_player,
    quake_parse_server,
    NULL,
    quake_config_is_valid,
    write_quake_variables,
    q1_exec_generic,
    q1_custom_cfgs,
    quake_save_info,
    quake_init_maps,	// init_maps
    quake_has_map,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&q1_private,	// pd
  },
  {
    QW_SERVER,
    GAME_CONNECT | GAME_RECORD | GAME_SPECTATE | GAME_PASSWORD | GAME_RCON | 
                                 GAME_QUAKE1_PLAYER_COLORS | GAME_QUAKE1_SKIN,
    "QuakeWorld",
    QW_DEFAULT_PORT,
    QWM_DEFAULT_PORT,
    "QWS",
    "QWS",
    "-qws",
    "-qwm",
    &q_pix,
    qw_parse_player,
    quake_parse_server,
    qw_analyze_serverinfo,
    quake_config_is_valid,
    write_quake_variables,
    qw_exec,
    qw_custom_cfgs,
    quake_save_info,
    quake_init_maps,	// init_maps
    quake_has_map,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&qw_private,	// pd
  },
  {
    Q2_SERVER,
    GAME_CONNECT | GAME_RECORD | GAME_SPECTATE | GAME_PASSWORD | GAME_RCON,
    "Quake2",
    Q2_DEFAULT_PORT,
    Q2M_DEFAULT_PORT,
    "Q2S",
    "Q2S",
    "-q2s",
    "-q2m",
    &q2_pix,
    q2_parse_player,
    quake_parse_server,
    q2_analyze_serverinfo,
    quake_config_is_valid,
    write_quake_variables,
    q2_exec,
    q2_custom_cfgs,
    quake_save_info,
    quake_init_maps,	// init_maps
    quake_has_map,		// has_map
    NULL,		// get_mapshot
    "version",		// arch_identifier
    identify_cpu,		// identify_cpu
    identify_os,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&q2_private,	// pd
  },

  {
    Q3_SERVER,
    GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    "Quake3: Arena",
    Q3_DEFAULT_PORT,
    Q3M_DEFAULT_PORT,
    "Q3S",
    "Q3S",
    "-q3s",
    "-q3m",
    &q3_pix,

    q3_parse_player,
    quake_parse_server,
    q3_analyze_serverinfo,
    quake3_config_is_valid,
    NULL,
    q3_exec,
    q3_custom_cfgs,
    quake_save_info,
    q3_init_maps,	// init_maps
    quake_has_map,	// has_map
    q3_get_mapshot,	// get_mapshot
    "version",		// arch_identifier
    identify_cpu,	// identify_cpu
    identify_os,	// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&q3_private,	// pd
  },

  {
    WO_SERVER,
    GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    "Wolfenstein",
    WO_DEFAULT_PORT,
    WOM_DEFAULT_PORT,
    "WOS",
    "Q3S",
    "-q3s",
    "-q3m",
    &wo_pix,

    q3_parse_player,
    quake_parse_server,
    q3_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q3_exec,
    NULL,
    quake_save_info,
    q3_init_maps,	// init_maps
    quake_has_map,	// has_map
    q3_get_mapshot,	// get_mapshot
    "version",		// arch_identifier
    identify_cpu,	// identify_cpu
    identify_os,	// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&wolf_private,	// pd
  },

  {
    WOET_SERVER,
  },

  {
    EF_SERVER,
    GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    "Voyager Elite Force",
    EF_DEFAULT_PORT,
    EFM_DEFAULT_PORT,
    "EFS",
    "Q3S",
    "-q3s",
    "-efm",
    &ef_pix,

    q3_parse_player,
    quake_parse_server,
    q3_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q3_exec,
    NULL,
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    "version",		// arch_identifier
    identify_cpu,		// identify_cpu
    identify_os,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },

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
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
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
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
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
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },
  {
    HL_SERVER,
    GAME_CONNECT | GAME_PASSWORD | GAME_RCON,
    "Half-Life",
    HL_DEFAULT_PORT,
    HLM_DEFAULT_PORT,
    "HLS",
    "HLS",
    "-hls",
    "-hlm",
    &hl_pix,

    hl_parse_player,
    quake_parse_server,
    hl_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    hl_exec,
    NULL,
    quake_save_info,
    quake_init_maps,	// init_maps
    quake_has_map,		// has_map
    NULL,		// get_mapshot
    "sv_os",		// arch_identifier
    NULL,		// identify_cpu
    identify_os,	// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&hl_private,	// pd
  },
  {
    KP_SERVER,
    GAME_CONNECT | GAME_RCON ,
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
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    "version",		// arch_identifier
    identify_cpu,		// identify_cpu
    identify_os,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
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
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },
  {
    SOF2S_SERVER,
    GAME_CONNECT | GAME_RCON | GAME_PASSWORD,
    "Soldier of Fortune 2",
    SOF2S_DEFAULT_PORT,
    SOF2M_DEFAULT_PORT,
    "SOF2S",
    "SOF2S",
    "-sof2s",
    "-sof2m",
    &sof2s_pix,

    q3_parse_player,
    quake_parse_server,
    q3_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    q3_exec,
    NULL,
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    "version",		// arch_identifier
    identify_cpu,		// identify_cpu
    identify_os,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },
  {
    T2_SERVER,
    GAME_CONNECT | GAME_RCON,
    "Tribes 2",
    T2_DEFAULT_PORT,
    T2M_DEFAULT_PORT,
    "T2S",
    "T2S",
    "-t2s",
    "-t2m", 
    &t2_pix,

    t2_parse_player,
    quake_parse_server,
    t2_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    t2_exec,
    NULL,
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    "linux",		// arch_identifier
    NULL,		// identify_cpu
    t2_identify_os,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
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
    quake_save_info,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },

  {
    UN_SERVER,
    GAME_CONNECT | GAME_PASSWORD,
    "Unreal / UT",
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
    quake_save_info,
    unreal_init_maps,	// init_maps
    unreal_has_map,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&ut_private,	// pd
  },
  {
    UT2_SERVER,
    GAME_CONNECT | GAME_SPECTATE | GAME_PASSWORD | GAME_LAUNCH_HOSTPORT,
    "UT 2003",
    UT2_DEFAULT_PORT,
    0,
    "UT2S",
    "UT2S",
    "-ut2s",
    "-ut2m",
    &ut2_pix,

    un_parse_player,
    quake_parse_server,
    un_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    ut_exec,
    NULL,
    quake_save_info,
    unreal_init_maps,	// init_maps
    unreal_has_map,	// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&ut2_private,	// pd
  },

  {
    RUNE_SERVER,
    GAME_CONNECT | GAME_PASSWORD,
    "Rune",
    UN_DEFAULT_PORT,
    0,
    "RUNESRV",
    "UNS",
    "-uns",
    "uns",
    &rune_pix,

    un_parse_player,
    quake_parse_server,
    un_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    ut_exec,
    NULL,
    quake_save_info,
    unreal_init_maps,	// init_maps
    unreal_has_map,	// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    (gpointer)&rune_private,	// pd
  },

  {
    POSTAL2_SERVER,
  },

  {
    AAO_SERVER,
  },

  // Descent 3
  {
    DESCENT3_SERVER,		// server_type
    GAME_CONNECT,		// flags
    "Descent 3",		// name
    DESCENT3_DEFAULT_PORT,	// default_port
    D3M_DEFAULT_PORT,		// default_master_port
    "D3P",			// id
    "D3P",			// qstat_str
    "-d3p",			// qstat_option
    "-d3m",			// qstat_master_option
    &descent3_pix,		// pixmap

    descent3_parse_player,	// parse_player
    quake_parse_server,		// parse_server
    descent3_analyze_serverinfo,	// analyze_serverinfo
    config_is_valid_generic,	// config_is_valid
    NULL,			// write_config
    exec_generic,		// exec_client
    NULL,			// custom_cfgs
    quake_save_info,		// save_info
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },

  {
    SSAM_SERVER,
    GAME_CONNECT,
    "Serious Sam",
    25600,
    0,
    "SMS",
    "SMS",
    "-sms",
    NULL,
    &ssam_pix,

    un_parse_player,
    quake_parse_server,
    un_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    ssam_exec,
    NULL,
    quake_save_info,
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },

  {
    SSAMSE_SERVER,
    GAME_CONNECT,
    "Serious Sam TSE",
    25600,
    0,
    "SMSSE",
    "SMS",
    "-sms",
    NULL,
    &ssam_pix,

    un_parse_player,
    quake_parse_server,
    un_analyze_serverinfo,
    config_is_valid_generic,
    NULL,
    ssam_exec,
    NULL,
    quake_save_info,
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },

  {
    MOHAA_SERVER,
  },

  {
    COD_SERVER,
  },

  {
    SAS_SERVER,			// server_type
    GAME_CONNECT,		// flags
    "Savage",	// name
    11235,		// default_port
    0,				// default_master_port
    "SAS",			// id
    "SAS",			// qstat_str
    "-sas",			// qstat_option
    NULL,			// qstat_master_option
    &savage_pix,		// pixmap

    savage_parse_player,		// parse_player
    quake_parse_server,		// parse_server
    savage_analyze_serverinfo,	// analyze_serverinfo
    config_is_valid_generic,	// config_is_valid
    NULL,			// write_config
    savage_exec,		// exec_client
    NULL,			// custom_cfgs
    quake_save_info,		// save_info
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },


  // any game using the gamespy protocol
  {
    GPS_SERVER,			// server_type
    GAME_CONNECT,		// flags
    N_("Generic Gamespy"),	// name
    GPS_DEFAULT_PORT,		// default_port
    0,				// default_master_port
    "GPS",			// id
    "GPS",			// qstat_str
    "-gps",			// qstat_option
    "gps",			// qstat_master_option
    &gamespy3d_pix,		// pixmap

    un_parse_player,		// parse_player
    quake_parse_server,		// parse_server
    un_analyze_serverinfo,	// analyze_serverinfo
    config_is_valid_generic,	// config_is_valid
    NULL,			// write_config
    gamespy_exec,		// exec_client
    NULL,			// custom_cfgs
    quake_save_info,		// save_info
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  },

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
    NULL,
    NULL,		// init_maps
    NULL,		// has_map
    NULL,		// get_mapshot
    NULL,		// arch_identifier
    NULL,		// identify_cpu
    NULL,		// identify_os
    NULL,		// cmd
    NULL,		// dir
    NULL,		// real_dir
    NULL,		// game_cfg
    NULL,		// games_data
    NULL,		// Custom arguments
    NULL,		// pd
  }
};
#else
#include "games.c"
#endif

struct gsname2type_s
{
	char* name;
	enum server_type type;
};

// gamespy names, used to determine the server type
static struct gsname2type_s gsname2type[] =
{
	{ "ut", UN_SERVER },
	{ "ut2", UT2_SERVER },
	{ "ut2d", UT2_SERVER },
	{ "rune", RUNE_SERVER },
	{ "serioussam", SSAM_SERVER },
	{ "serioussamse", SSAMSE_SERVER },
	{ "postal2", POSTAL2_SERVER },
	{ "postal2d", POSTAL2_SERVER },
	{ "armygame", AAO_SERVER },
	{ "bfield1942", BF1942_SERVER },
	{ NULL, UNKNOWN_SERVER }
};

// might be handy in the future instead of copy&paste
static void game_copy_static_options(enum server_type dest, enum server_type src)
{
    games[dest].flags			= games[src].flags;
    games[dest].name			= games[src].name;
    games[dest].default_port		= games[src].default_port;
    games[dest].default_master_port	= games[src].default_master_port;
    games[dest].id			= games[src].id;
    games[dest].qstat_str		= games[src].qstat_str;
    games[dest].qstat_option		= games[src].qstat_option;
    games[dest].qstat_master_option	= games[src].qstat_master_option;
    games[dest].pix			= games[src].pix;
    games[dest].parse_player		= games[src].parse_player;
    games[dest].parse_server		= games[src].parse_server;
    games[dest].analyze_serverinfo	= games[src].analyze_serverinfo;
    games[dest].config_is_valid		= games[src].config_is_valid;
    games[dest].write_config		= games[src].write_config;
    games[dest].exec_client		= games[src].exec_client;
    games[dest].custom_cfgs		= games[src].custom_cfgs;
    games[dest].save_info		= games[src].save_info;
    games[dest].init_maps		= games[src].init_maps;
    games[dest].has_map			= games[src].has_map;
    games[dest].get_mapshot		= games[src].get_mapshot;
    games[dest].arch_identifier		= games[src].arch_identifier;
    games[dest].identify_cpu		= games[src].identify_cpu;
    games[dest].identify_os		= games[src].identify_os;
}


void init_games()
{
  int i;

  debug(3,"init_games");

#if 0
  game_copy_static_options(WOET_SERVER,WO_SERVER);
  games[WOET_SERVER].name="Enemy Territory";
  games[WOET_SERVER].id="WOETS";
  games[WOET_SERVER].pd=&wolfet_private;
  games[WOET_SERVER].pix=&et_pix;

  game_copy_static_options(POSTAL2_SERVER,UN_SERVER);
  games[POSTAL2_SERVER].name="Postal 2";
  games[POSTAL2_SERVER].id="POSTAL2";
  games[POSTAL2_SERVER].pd=&postal2_private;
  games[POSTAL2_SERVER].pix=&postal2_pix;

  game_copy_static_options(AAO_SERVER,UN_SERVER);
  games[AAO_SERVER].name="America's Army";
  games[AAO_SERVER].flags |= GAME_SPECTATE;
  games[AAO_SERVER].flags |= GAME_LAUNCH_HOSTPORT;
  games[AAO_SERVER].id="AMS"; // http://qstat.uglypunk.com/
  games[AAO_SERVER].pd=&aao_private;
  games[AAO_SERVER].pix=&aao_pix;

  game_copy_static_options(MOHAA_SERVER,Q3_SERVER);
  games[MOHAA_SERVER].name="Medal of Honor: Allied Assault";
  games[MOHAA_SERVER].default_port=12204;
  games[MOHAA_SERVER].id="MHS";
  games[MOHAA_SERVER].qstat_str="MHS";
  games[MOHAA_SERVER].qstat_option="-mhs";
  games[MOHAA_SERVER].pd=&mohaa_private;
  games[MOHAA_SERVER].pix=&mohaa_pix;
  games[MOHAA_SERVER].config_is_valid=config_is_valid_generic;

  game_copy_static_options(COD_SERVER,Q3_SERVER);
  games[COD_SERVER].name="Call of Duty";
  games[COD_SERVER].default_port=27201;
  games[COD_SERVER].id="CODS";
  games[COD_SERVER].qstat_str="CODS";
  games[COD_SERVER].qstat_option="-cods";
  games[COD_SERVER].default_master_port=20510;
  games[COD_SERVER].qstat_master_option="-codm";
  games[COD_SERVER].pd=&cod_private;
  games[COD_SERVER].pix=&cod_pix;
  games[COD_SERVER].config_is_valid=config_is_valid_generic;
  games[COD_SERVER].arch_identifier=NULL;
#endif

  for (i = 0; i < GAMES_TOTAL; i++)
  {
    g_datalist_init(&games[i].games_data);
  }

  game_set_attribute(Q1_SERVER,"suggest_commands",strdup("twilight-nq:nq-sgl:nq-glx:nq-sdl:nq-x11"));
  game_set_attribute(Q2_SERVER,"suggest_commands",strdup("quake2"));
  game_set_attribute(QW_SERVER,"suggest_commands",strdup("twilight-qw:qw-client-sgl:qw-client-glx:qw-client-sdl:qw-client-x11"));
  game_set_attribute(Q3_SERVER,"suggest_commands",strdup("q3:quake3"));
  game_set_attribute(WO_SERVER,"suggest_commands",strdup("wolf"));
  game_set_attribute(WOET_SERVER,"suggest_commands",strdup("et"));
  game_set_attribute(SFS_SERVER,"suggest_commands",strdup("sof"));
  game_set_attribute(DESCENT3_SERVER,"suggest_commands",strdup("descent3"));
  game_set_attribute(T2_SERVER,"suggest_commands",strdup("tribes2"));
  game_set_attribute(UN_SERVER,"suggest_commands",strdup("ut"));
  game_set_attribute(UT2_SERVER,"suggest_commands",strdup("ut2003:ut2003_demo"));
  game_set_attribute(RUNE_SERVER,"suggest_commands",strdup("rune"));
  game_set_attribute(AAO_SERVER,"suggest_commands",strdup("armyops"));
  game_set_attribute(POSTAL2_SERVER,"suggest_commands",strdup("postal2mp:postal2mpdemo"));
  game_set_attribute(SSAM_SERVER,"suggest_commands",strdup("ssamtfe"));
  game_set_attribute(SSAMSE_SERVER,"suggest_commands",strdup("ssamtse"));
  game_set_attribute(MOHAA_SERVER,"suggest_commands",strdup("mohaa"));
  game_set_attribute(SAS_SERVER,"suggest_commands",strdup("savage"));
  game_set_attribute(COD_SERVER,"suggest_commands",strdup("codmp"));

  game_set_attribute(SFS_SERVER,"game_notes",strdup(_
   				   ("Note:  Soldier of Fortune will not connect to a server correctly\n"\
    				    "without creating a startup script for the game.  Please see the\n"\
			  	    "XQF documentation for more information."))); 
  game_set_attribute(UN_SERVER,"game_notes",strdup(_
  				   ("Note:  Unreal Tournament will not launch correctly without\n"\
    				    "modifications to the game's startup script.  Please see the\n"\
			  	    "XQF documentation for more information.")));
  game_set_attribute(HL_SERVER,"game_notes",strdup(_
  				   ("Sample Command Line:  wine hl.exe -- hl.exe -console")));

  game_set_attribute(SAS_SERVER,"game_notes",strdup(_
  				   ("Note:  Savage will not launch correctly without\n"\
    				    "modifications to the game's startup script. Please see the\n"\
			  	    "XQF documentation for more information.")));

  game_set_attribute(SSAM_SERVER,"game_notes",strdup(_
  				   ("Note: You need to create a qstat config file for this game to work.\n"\
    				    "Please see the XQF documentation for more information."))); 

  game_set_attribute(SSAMSE_SERVER,"game_notes",strdup(_
  				   ("Note: You need to create a qstat config file for this game to work.\n"\
    				    "Please see the XQF documentation for more information."))); 

}

// retreive game specific value that belongs to key, do not free return value!
const char* game_get_attribute(enum server_type type, const char* attr)
{
  return g_datalist_get_data(&games[type].games_data,attr);
}

// set game specific key/value pair, value is _not_ copied and must not be
// freed manually
const char* game_set_attribute(enum server_type type, const char* attr, char* value)
{
  g_datalist_set_data_full(&games[type].games_data,attr,value,g_free);
  return value;
}

enum server_type id2type (const char *id) {
  int i;

  g_return_val_if_fail(id != NULL, UNKNOWN_SERVER);

  for (i = 0; i < GAMES_TOTAL; i++) {
    g_return_val_if_fail(games[i].id != NULL, UNKNOWN_SERVER);
    if (g_strcasecmp (id, games[i].id) == 0)
      return games[i].type;
  }

  for (i = 0; i < GAMES_TOTAL; i++) {
    if (g_strcasecmp (id, games[i].qstat_str) == 0)
      return games[i].type;
  }
  
  // workaround for qstat beta
  if (g_strcasecmp (id, "RWS" ) == 0)
  {
    return WO_SERVER;
  }

  // for those who used the cvs version before, can be removed after some time
  // yes, ut2ds is is right, used to autoconvert to ut2s
  if (g_strcasecmp (id, "UT2DS" ) == 0)
  {
    return UT2_SERVER;
  }


  if (g_strcasecmp (id, "QW") == 0)
    return QW_SERVER;
  if (g_strcasecmp (id, "Q2") == 0)
    return Q2_SERVER;

  return UNKNOWN_SERVER;
}


/*
 *  This function should be used only for configuration saving to make
 *  possible qstat2.2<->qstat2.3 migration w/o loss of some game-specific 
 *  preferences.
 */

const char *type2id (enum server_type type) {
  switch (type) {

  default:
    return games[type].id;
  }
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

  label = gtk_label_new (_(games[type].name));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  return hbox;
}


static void q3_unescape (char *dst, char *src) {

  while (*src) {
    if (src[0] != '^' || src[1] == '\0' || src[1] < '0' || src[1] > '9')
      *dst++ = *src++;
    else
      src += 2;
  }
  *dst = '\0';
}


static const char delim[] = " \t\n\r";


static struct player *poqs_parse_player (char *token[], int n, struct server *s) {
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


static struct player *qw_parse_player (char *token[], int n, struct server *s) {
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

// Parse Tribes2 player info, abuse player->model to show when a Player is only
// a Teamname or a Bot
// player name, frags, team number, team name, player type, tribe tag
static struct player *t2_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;
  char* name=token[0];
  char* frags=token[1];
//  char* team_number=token[2];
  char* team_name=token[3];
  char* player_type=token[4];
//  char* tribe_tag=token[5];

  if (n < 6)
    return NULL;

  // show TEAM in model column
  if (!strcmp(team_name,"TEAM"))
    player_type=team_name;

  player = g_malloc0 (sizeof (struct player) + strlen(name)+1 + strlen(player_type)+1 );
  player->time  = -1;
  player->frags = strtosh (frags);
  player->ping  = -1;

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, name);

  player->model = (char *) player + sizeof (struct player) + strlen(name)+1;
  strcpy (player->model, player_type);
  if( player_type[0] == 'B' ) ++s->curbots;

  return player;
}

static struct player *q2_parse_player (char *token[], int n, struct server *s) {
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

// Descent 3: player name, frags, deaths, ping time, team
static struct player *descent3_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;

  if (n < 5)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->time  = -1;
  player->frags = strtosh (token[1]);
  player->ping  = strtosh (token[3]);

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, token[0]);

  return player;
}

static struct player *q3_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;

  if (n < 3)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->time  = -1;
  player->frags = strtosh (token[1]);
  player->ping  = strtosh (token[2]);
   /* FIXME if ( dedicated == 0 ) s->curbots-- */
  if ( player->ping == 0 ) ++s->curbots;

  player->name = (char *) player + sizeof (struct player);
  q3_unescape (player->name, token[0]);

  return player;
}


static struct player *hl_parse_player (char *token[], int n, struct server *s) {
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


static struct player *un_parse_player (char *token[], int n, struct server *s) {
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

static struct player *savage_parse_player (char *token[], int n, struct server *s)
{
    return q2_parse_player(token, n, s);
}

static void quake_parse_server (char *token[], int n, struct server *server) {
  /*
    This does both Quake (?) and Unreal servers
  */
  int poqs;
  int offs;

  /* debug (6, "quake_parse_server: Parse %s", server->name); */
  poqs = (server->type == Q1_SERVER || server->type == H2_SERVER);

  if (poqs && n < 10)
      return;
  else if(n < 8)
      return;

  if (*(token[2])) {		/* if name is not empty */
    if (server->type != Q3_SERVER) {
      server->name = g_strdup (token[2]);
    }
    else {
      server->name = g_malloc (strlen (token[2]) + 1);
      q3_unescape (server->name, token[2]);
    }
  }

  offs = (poqs)? 5 : 3;

  if (*(token[offs]))            /* if map is not empty */
    server->map  = g_strdup (token[offs]);
    if(server->map) g_strdown(server->map);

    server->maxplayers = strtoush (token[offs + 1]);
    server->curplayers = strtoush (token[offs + 2]);

  server->ping = strtosh (token[offs + 3]);
  server->retries = strtosh (token[offs + 4]);
}


static void qw_analyze_serverinfo (struct server *server) {
  char **info_ptr;
  long n;

  /* Clear out the flags */
  server->flags = 0;
  
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

static void un_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  unsigned short hostport=0;

  enum server_type oldtype = s->type;

  /* Clear out the flags */
  s->flags = 0;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;
  
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->game = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "gamestyle") == 0) {
      s->gametype = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "hostport") == 0) {
      hostport = atoi(info_ptr[1]);
    }
    else if (strcmp (*info_ptr, "gamename") == 0) {
      unsigned i;
      for( i = 0 ; gsname2type[i].name ; ++i )
      {
	if(!strcmp(info_ptr[1],gsname2type[i].name))
	{
	  s->type = gsname2type[i].type;
	  break;
	}
      }
    }

    //password required?
    // If not password=False or password=0, set SERVER_PASSWORD
    else if (g_strcasecmp (*info_ptr, "password") == 0 && 
	( g_strcasecmp(info_ptr[1],"false") && strcmp(info_ptr[1],"0") ) )
    {
      s->flags |= SERVER_PASSWORD;
      if (games[s->type].flags & GAME_SPECTATE)
	s->flags |= SERVER_SP_PASSWORD;
    }
  }

  // adjust port for unreal and rune
  if(s->type != oldtype)
  {
    switch(s->type)
    {
      case UN_SERVER:
      case UT2_SERVER:
      case RUNE_SERVER:
      case POSTAL2_SERVER:
//      case AAO_SERVER: // doesn't work on lan
	server_change_port(s,hostport);
	break;
      default:
	break;
    }
  }
}

static void bf1942_analyze_serverinfo (struct server *s)
{
  char **info_ptr;

  /* Clear out the flags */
  s->flags = 0;
  
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2)
  {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->gametype = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "Game Id") == 0) {
      s->game = info_ptr[1];
    }
    //password required?
    // If not password=False or password=0, set SERVER_PASSWORD
    else if (strcmp (*info_ptr, "_password") == 0
	&& strcmp(info_ptr[1], "0"))
    {
      s->flags |= SERVER_PASSWORD;
      if (games[s->type].flags & GAME_SPECTATE)
	s->flags |= SERVER_SP_PASSWORD;
    }
  }
}


static void savage_analyze_serverinfo (struct server *s)
{
  char **info_ptr;

  /* Clear out the flags */
  s->flags = 0;
  
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->game = info_ptr[1];
    }
  }
}

static void descent3_analyze_serverinfo (struct server *s) {
  char **info_ptr;

  /* Clear out the flags */
  s->flags = 0;
  
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->game = info_ptr[1];
    }
/*
    //password required?
    else if (strcmp (*info_ptr, "password") == 0 && strcmp(info_ptr[1],"False")) {
      s->flags |= SERVER_PASSWORD;
    }
*/
  }
}

static void q2_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;

  /* Clear out the flags */
  s->flags = 0;
  
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

static void t2_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;

  /* Clear out the flags */
  s->flags = 0;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "game") == 0) {
      if (strcmp(info_ptr[1],"base")) // If it's not 'base'
        s->game = info_ptr[1];
    }

    //determine GameType column
    else if (strcmp (*info_ptr, "mission") == 0) {
      s->gametype = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "cheats") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_CHEATS;
    }
    else if (strcmp (*info_ptr, "maxspectators") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if (n <= 0)
        s->flags &= ~SERVER_SPECTATE;
    }
    else if (strcmp (*info_ptr, "password") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if ((n & 1) != 0)
	s->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	s->flags |= SERVER_SP_PASSWORD;
    }

  }
  // unset game if game is base
//  if (!strcmp(s->game,"base"))
//  {
//  strcpy(s->gametype,"Alex");
//  }
}

static void hl_analyze_serverinfo (struct server *s) {
  char **info_ptr;

  /* Clear out the flags */
  s->flags = 0;
  
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

    //cheating death
    else if (strcmp (*info_ptr, "cdrequired") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_PUNKBUSTER;
    }

    // reserved slots
    else if (strcmp (*info_ptr, "reserve_slots") == 0) {
      s->private_client = strtol (info_ptr[1], NULL, 10);
    }
  }

  // unset Mod if gamedir is valve
  if (s->game && !strcmp(s->game,"valve"))
  {
	  s->gametype=NULL;
  }
}


// TODO: read this stuff from a config file
#define MAX_Q3A_TYPES 9
static char *q3a_gametypes[MAX_Q3A_TYPES] = {
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


#define MAX_Q3A_UT2_TYPES 9
static char *q3a_ut2_gametypes[MAX_Q3A_UT2_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "FFA",		/* 1 = Free for All */
  "FFA",		/* 2 = Free for All */
  "TDM",		/* 3 = Team Deathmatch */
  "Team Survivor",	/* 4 = Team Survivor */
  "Follow the Leader",	/* 5 = Follow the Leader */
  "Capture & Hold",     /* 6 = Capture & Hold */
  "CTF",		/* 7 = Capture the Flag */
  NULL,			/* 8+ ?? */
};

#define MAX_Q3A_UT3_TYPES 9
static char *q3a_ut3_gametypes[MAX_Q3A_UT3_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "FFA",		/* 1 = Free for All */
  "FFA",		/* 2 = Free for All */
  "TDM",		/* 3 = Team Deathmatch */
  "Team Survivor",	/* 4 = Team Survivor */
  "Follow the Leader",	/* 5 = Follow the Leader */
  "Capture & Hold",     /* 6 = Capture & Hold */
  "CTF",		/* 7 = Capture the Flag */
  "Bomb Mode",		/* 8 = Bomb Mode */
};


#define MAX_Q3A_Q3TC045_TYPES 11
static char *q3a_q3tc045_gametypes[MAX_Q3A_Q3TC045_TYPES] = {
  "FFA",		// 0 = Free for All
  NULL,			// 1 = ?
  NULL,			// 2 = ?
  "Survivor",		// 3 = Survivor
  "TDM",		// 4 = Team Deathmatch
  "CTF",		// 5 = Capture the Flag
  "Team Survivor",	// 6 = Team Survivor
  NULL,			// 7 = ?
  "Capture & Hold",     // 8 = Capture & Hold
  "King Of The Hill",	// 9 = King of the hill
  NULL,			// 10+ ??
};

#define MAX_Q3A_TRUECOMBAT_TYPES 8
static char *q3a_truecombat_gametypes[MAX_Q3A_TRUECOMBAT_TYPES] = {
  "FFA",		// 0 = Free for All
  "Survivor",		// 1 = Survivor/Last Man Standing
  NULL,			// 2 = ?
  "TDM",		// 3 = Team Deathmatch
  "Reverse CTF",	// 4 = Reverse CTF
  "CTF",		// 5 = Capture the Flag
  "Team Survivor",	// 6 = Team Survivor
  "Mission",		// 7 = Mission
};

#define MAX_Q3A_THREEWAVE_TYPES 12
static char *q3a_threewave_gametypes[MAX_Q3A_THREEWAVE_TYPES] = {
  "FFA",		// 0  - Free For All
  "1v1",		// 1  - Tournament
  NULL,			// 2  - Single Player (invalid, don't use this)
  "TDM",		// 3  - Team Deathmatch
  "ThreeWave CTF",	// 4  - ThreeWave CTF
  "One flag CTF",	// 5  - One flag CTF  (invalid, don't use this)
  "Obelisk",		// 6  - Obelisk       (invalid, don't use this)
  "Harvester",		// 7  - Harvester     (invalid, don't use this)
  "Portal", 	        // 8  - Portal        (invalid, don't use this)
  "CaptureStrike",	// 9  - CaptureStrike 
  "Classic CTF", 	// 10 - Classic CTF   
  NULL			// 11+ ???
};

#define MAX_Q3A_TRIBALCTF_TYPES 10
static char *q3a_tribalctf_gametypes[MAX_Q3A_TRIBALCTF_TYPES] = {
  NULL,			// 0 - Unknown
  NULL,			// 1 - Unknown
  NULL,			// 2 - Unknown
  NULL,			// 3 - Unknown
  NULL,			// 4 - Unknown
  NULL,			// 5 - Unknown
  "Freestyle",		// 6 - Freestyle
  "Fixed",		// 7 - Fixed
  "Roulette",		// 8 - Roulette
  NULL			// 9+ ???
};

#define MAX_Q3A_SEALS_TYPES 5
static char *q3a_seals_gametypes[MAX_Q3A_SEALS_TYPES] = {
  NULL,			/* 0 = devmode */
  NULL,			/* 1 = invalid */
  NULL,			/* 2 = invalid */
  "Operations",		/* 3 = Team Deathmatch */
  NULL			/* 4+ invalid */
};

#define MAX_Q3A_AFTERWARDS_TYPES 3
static char *q3a_afterwards_gametypes[MAX_Q3A_AFTERWARDS_TYPES] = {
  "Tactical",		// 0 = Tactical
  "FFA",		// 1 = Deatchmatch
  NULL,			// 2+ ??
};

#define MAX_Q3A_ARENA_TYPES 9
// Not sure what the proper types are, but 99% of them are a game
// type of 8.  Just call them all "arena"
static char *q3a_arena_gametypes[MAX_Q3A_ARENA_TYPES] = {
  "arena",		/* 0 = Arena */
  "arena",		/* 1 = Arena */
  "arena",		/* 2 = Arena */
  "arena",		/* 3 = Arena */
  "arena",		/* 4 = Arena */
  "arena",		/* 5 = Arena */
  "arena",		/* 6 = Arena */
  "arena",		/* 7 = Arena */
  "arena",		/* 8 = Arena */
};

#define MAX_Q3A_CPMA_TYPES 6
static char *q3a_cpma_gametypes[MAX_Q3A_CPMA_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "1v1",	 	/* 1 = Tournament */
  NULL,  		/* 2 = Single Player */
  "TDM",		/* 3 = Team Deathmatch */
  "CTF",		/* 4 = Capture the Flag */
  "Clan Arena",		/* 5 = Clan Arena */
};

#define MAX_Q3A_Q3F_TYPES 6
static char *q3a_q3f_gametypes[MAX_Q3A_Q3F_TYPES] = {
  "q3f",		/* 0 = Arena */
  "q3f",		/* 1 = Arena */
  "q3f",		/* 2 = Arena */
  "q3f",		/* 3 = Arena */
  "q3f",		/* 4 = Arena */
  "q3f",		/* 5 = Arena */
};

#define MAX_Q3A_WQ3_TYPES 6
static char *q3a_wq3_gametypes[MAX_Q3A_WQ3_TYPES] = {
  "FFA",
  "Duel",
  NULL,
  "TDM",
  "Round Teamplay",
  "Bank Robbery"
};

#define MAX_WOLF_TYPES 9
static char *wolf_gametypes[MAX_WOLF_TYPES] = {
  NULL,			// 0 - Unknown
  NULL,			// 1 - Unknown
  NULL,			// 2 - Unknown
  NULL,			// 3 - Unknown
  NULL,			// 4 - Unknown
  "WolfMP",		// 5 - standard objective mode
  "WolfSW",		// 6 - Stopwatch mode
  "WolfCP",		// 7 - Checkpoint mode
  NULL			// 8+ ???
};

#define MAX_WOLFET_TYPES 7
static char *wolfet_gametypes[MAX_WOLFET_TYPES] = {
  NULL,			// 0 - Unknown
  NULL,			// 1 - Unknown
  "Obj",		// 2 - Single-Map Objective
  "SW",			// 3 - Stopwatch
  "Cmpgn",		// 4 - Campaign
  "LMS",		// 5 - Last Man Standing
  NULL			// 6+ ???
};

struct q3a_gametype_s {
  char* mod;
  char** gametypes;
  int number;
};

struct q3a_gametype_s q3a_gametype_map[] =
{
  {
    "baseq3",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "osp",
    q3a_osp_gametypes,
    MAX_Q3A_OSP_TYPES
  },
  {
    "q3ut2",
    q3a_ut2_gametypes,
    MAX_Q3A_UT2_TYPES
  },
  {
    "q3ut3",
    q3a_ut3_gametypes,
    MAX_Q3A_UT3_TYPES
  },
  {
    "threewave",
    q3a_threewave_gametypes,
    MAX_Q3A_THREEWAVE_TYPES
  },
  {
    "seals",
    q3a_seals_gametypes,
    MAX_Q3A_SEALS_TYPES
  },
  {
    "TribalCTF",
    q3a_tribalctf_gametypes,
    MAX_Q3A_TRIBALCTF_TYPES
  },
  {
    "missionpack",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "generations",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "truecombat",
    q3a_truecombat_gametypes,
    MAX_Q3A_TRUECOMBAT_TYPES
  },
  {
    "q3tc045",
    q3a_q3tc045_gametypes,
    MAX_Q3A_Q3TC045_TYPES
  },
  {
    "freeze",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "afterwards",
    q3a_afterwards_gametypes,
    MAX_Q3A_AFTERWARDS_TYPES
  },
  {
    "cpma",
    q3a_cpma_gametypes,
    MAX_Q3A_CPMA_TYPES
  },
  {
    "arena",
    q3a_arena_gametypes,
    MAX_Q3A_ARENA_TYPES
  },
  {
    "instaunlagged",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "instagibplus",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "beryllium",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "excessive",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "q3f",
    q3a_q3f_gametypes,
    MAX_Q3A_Q3F_TYPES
  },
  {
    "q3f2",
    q3a_q3f_gametypes,
    MAX_Q3A_Q3F_TYPES
  },
  {
    "westernq3",
    q3a_wq3_gametypes,
    MAX_Q3A_WQ3_TYPES
  },
  {
    NULL,
    NULL,
    0
  }
};

struct q3a_gametype_s wolf_gametype_map[] =
{
  {
    "main",
    wolf_gametypes,
    MAX_WOLF_TYPES
  },
  {
    "wolfmp",
    wolf_gametypes,
    MAX_WOLF_TYPES
  },
  {
    "bani",
    wolf_gametypes, // exports additional gametypes via g_gametype2
    MAX_WOLF_TYPES
  },
  {
    "headshot",
    wolf_gametypes,
    MAX_WOLF_TYPES
  },
  {
    "osp",
    wolf_gametypes,
    MAX_WOLF_TYPES
  },
  {
    "shrubmod",
    wolf_gametypes,
    MAX_WOLF_TYPES
  }
};

// didn't find docu about this, so use q3a types
struct q3a_gametype_s ef_gametype_map[] =
{
  {
    "baseEF",
    q3a_gametypes,
    MAX_Q3A_TYPES
  }
};

struct q3a_gametype_s wolfet_gametype_map[] =
{
  {
    "et",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  },
  {
    "etmain",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  },
  {
    "ettest",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  },
  {
    "etpro",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  },
  {
    "shrubet",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  }
};

void q3_decode_gametype (struct server *s, struct q3a_gametype_s map[])
{
  char *endptr;
  int n;
  int found=0;
  struct q3a_gametype_s* ptr;

  if(!s->game) return;

  n = strtol (s->gametype, &endptr, 10);

  // strtol returns a pointer to the first invalid digit, if both pointers
  // are equal there was no number at all
  if (s->gametype == endptr)
    return;

  for( ptr=map; !found && ptr && ptr->mod != NULL; ptr++ )
  {
    if( !strcasecmp (s->game, ptr->mod)
	&& n >=0
	&& n < ptr->number
	&& ptr->gametypes[n] )
    {
      	s->gametype = ptr->gametypes[n];
	found=1;
    }
//    else
      // Exact match not found - use the first one in the list
      // which should be the game's original game types
//      s->gametype = map->gametypes[n];
  }
}

static void q3_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;
  char *fs_game=NULL;
  char *game=NULL;
  char *gamename=NULL;

  /* Clear out the flags */
  s->flags = 0;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
 
    /*
      fs_game sets the active directory and is how one chooses
      a mod on the command line.  This should not show up in
      the server string but some times it does.  We will
      take either fs_game, game or gamename as the "game" string.
      --baa
    */
    if (strcmp (*info_ptr, "fs_game") == 0) {
	fs_game  = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "gamename") == 0) {
      gamename  = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "game") == 0) {
      game  = info_ptr[1];
    }

    else if (strcmp (*info_ptr, "version" ) == 0) {
      if (strstr (info_ptr[1], "linux")) {
	s->sv_os = 'L';
      } else if (strstr (info_ptr[1], "win" )) {
	s->sv_os = 'W';
      } else if (strstr (info_ptr[1], "Mac" )) {
	s->sv_os = 'M';
      } else {
	s->sv_os = '?';
      }

      // check if it's really a q3 server
      if(!strncmp(info_ptr[1],"Q3",2))
      {
	s->type=Q3_SERVER;
      }
      else if(!strncmp(info_ptr[1],"Wolf",4))
      {
	s->type=WO_SERVER;
      }
      else if(!strncmp(info_ptr[1],"ET 2",4))
      {
	s->type=WOET_SERVER;
      }
      // voyager elite force
      else if(!strncmp(info_ptr[1],"ST:V HM",7))
      {
	s->type=EF_SERVER;
      }
      else if(!strncmp(info_ptr[1],"Medal",5))
      {
	s->type=MOHAA_SERVER;
      }
      else if(!strncmp(info_ptr[1],"SOF2MP",6))
      {
	s->type=SOF2S_SERVER;
      }
    }
    
    else if (!s->gametype && strcmp (*info_ptr, "g_gametype") == 0) {
	s->gametype = info_ptr[1];
    }
    else if (s->type == MOHAA_SERVER &&
	strcmp (*info_ptr, "g_gametypestring") == 0) {
	s->gametype = info_ptr[1];
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
    else if (g_strcasecmp (*info_ptr, "sv_privateClients") == 0) {
      s->private_client = strtol (info_ptr[1], NULL, 10);
    }
    else if (!strcmp(*info_ptr, "sv_punkbuster") && info_ptr[1] && info_ptr[1][0] == '1') {
      s->flags |= SERVER_PUNKBUSTER;
    }
  }

  if(fs_game)
  {
    s->game=fs_game;
  }
  else if(game)
  {
    s->game=game;
  }
  else if(gamename)
  {
    s->game=gamename;
  }

  if(s->gametype) {
    if ( s->type == Q3_SERVER)
    {
      q3_decode_gametype( s, q3a_gametype_map );
    }
    else if ( s->type == WO_SERVER)
    {
      q3_decode_gametype( s, wolf_gametype_map );
    }
    else if ( s->type == EF_SERVER)
    {
      q3_decode_gametype( s, ef_gametype_map );
    }
    else if ( s->type == WOET_SERVER)
    {
      q3_decode_gametype( s, wolfet_gametype_map );
    }

  }

  // unset game if it's no mod
  if ( s->game )
  {
    if ( s->type == Q3_SERVER && !strcasecmp (s->game, "baseq3"))
    {
      s->game=NULL;
    }
    else if ( s->type == WO_SERVER && !strcasecmp (s->game, "main"))
    {
      s->game=NULL;
    }
    else if ( s->type == EF_SERVER && !strcasecmp (s->game, "baseEF"))
    {
      s->game=NULL;
    }
  }
}


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
    cfgdir = (default_qw_is_quakeforge?"base":"id1");
    break;

  case Q2_SERVER:
    cfgdir = "baseq2";
    break;

  default:
    return FALSE;

  }

  if (g->cmd == NULL || g->cmd[0] == '\0') {
    // %s = game name e.g. QuakeWorld
    dialog_ok (NULL, _("%s command line is empty."), g->name);
    return FALSE;
  }

  if (g->real_dir != NULL && g->real_dir[0] != '\0') {
    if (stat (g->real_dir, &stat_buf) != 0 || !S_ISDIR (stat_buf.st_mode)) {
      // directory name, game name
      dialog_ok (NULL, _("\"%s\" is not a directory\n"
	              "Please specify correct %s working directory."), 
                       g->real_dir, g->name);
      return FALSE;
    }
  }

  path = file_in_dir (g->real_dir, cfgdir);

  if (stat (path, &stat_buf) || !S_ISDIR (stat_buf.st_mode)) {
    if (!g->real_dir || g->real_dir[0] == '\0') {
      // game name
      dialog_ok (NULL, _("Please specify correct %s working directory."), 
                                                                     g->name);
    }
    else {
      dialog_ok (NULL,  
		// directory, subdirectory, game name
		 _("Directory \"%s\" doesn\'t contain \"%s\" subdirectory.\n"
		 "Please specify correct %s working directory."), 
		 g->real_dir, cfgdir, g->name);
    }
    g_free (path);
    return FALSE;
  }

  g_free (path);
  return TRUE;
}


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
		 // %s Quake3
		 _("~/.q3a directory doesn\'t exist or doesn\'t contain\n"
		 "\"baseq3\" (\"demoq3\") subdirectory.\n"
		 "Please run %s client at least once before running XQF\n"
		 "or specify correct %s working directory."),
		 g->name, g->name);
    }
    else {
      dialog_ok (NULL, 
		 // %s directory, Quake3
		 _("\"%s\" directory doesn\'t exist or doesn\'t contain "
		 "\"baseq3\" (\"demoq3\") subdirectory.\n"
		 "Please specify correct %s working directory\n"
  	         "or leave it empty (~/.q3a is used by default)"), 
		 g->real_dir, g->name);
    }
    return FALSE;
  }
  
  g_free (path);
  return TRUE;
}

static int config_is_valid_generic (struct server *s) {
  struct stat stat_buf;
  struct game *g = &games[s->type];

  if (g->cmd == NULL || g->cmd[0] == '\0') {
    dialog_ok (NULL, "%s command line is empty.", g->name);
    return FALSE;
  }

  if (g->real_dir != NULL && g->real_dir[0] != '\0') {
    if (stat (g->real_dir, &stat_buf) != 0 || !S_ISDIR (stat_buf.st_mode)) {
      // %s directory, game name
      dialog_ok (NULL, _("\"%s\" is not a directory\n"
	              "Please specify correct %s working directory."), 
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
    fprintf (f, "rconpassword \"%s\"\n", con->rcon_password);

  fclose (f);
  return TRUE;
}


static int write_q1_vars (const char *filename, const struct condef *con) {
  FILE *f;

  f = open_cfg (filename, FALSE);
  if (!f)
    return FALSE;

  if (default_q1_name)
    fprintf (f, "name \"%s\"\n", default_q1_name);

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

  if (default_qw_name)
    fprintf (f, "name \"%s\"\n", default_qw_name);

  if (default_qw_skin)
    fprintf (f, "skin \"%s\"\n", default_qw_skin);

  fprintf (f, "team \"%s\"\n", (default_qw_team)? default_qw_team : "");
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

  fprintf (f, "rate        \"%d\"\n", default_qw_rate);
  fprintf (f, "cl_nodelta  \"%d\"\n", default_qw_cl_nodelta);
  fprintf (f, "cl_predict_players \"%d\"\n", default_qw_cl_predict);
  fprintf (f, "noaim       \"%d\"\n", default_noaim);
  fprintf (f, "noskins     \"%d\"\n", default_qw_noskins);
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

  if (default_q2_name)
    fprintf (f, "set name \"%s\"\n", default_q2_name);

  if (default_q2_skin)
    fprintf (f, "set skin \"%s\"\n", default_q2_skin);

  fprintf (f, "set rate        \"%d\"\n", default_q2_rate);
  fprintf (f, "set cl_nodelta  \"%d\"\n", default_q2_cl_nodelta);
  fprintf (f, "set cl_predict  \"%d\"\n", default_q2_cl_predict);
  fprintf (f, "set cl_noskins  \"%d\"\n", default_q2_noskins);

  if (games[Q2_SERVER].game_cfg)
    fprintf (f, "exec \"%s\"\n", games[Q2_SERVER].game_cfg);

  if (con->custom_cfg)
    fprintf (f, "exec \"%s\"\n", con->custom_cfg);

  fclose (f);
  return TRUE;
}

/*
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
*/

static int write_quake_variables (const struct condef *con) {
/*
#ifdef QSTAT23
  char *path;
#endif
*/
  char *file = NULL;
  int res = TRUE;

  switch (con->s->type) {

  case Q1_SERVER:
    file = file_in_dir (games[Q1_SERVER].real_dir, "id1/" EXEC_CFG);
    res =  write_q1_vars (file, con);
    break;

  case QW_SERVER:
    if(default_qw_is_quakeforge)
      file = file_in_dir (games[QW_SERVER].real_dir, "base/" EXEC_CFG);
    else
      file = file_in_dir (games[QW_SERVER].real_dir, "id1/" EXEC_CFG);
    res =  write_qw_vars (file, con);
    break;

  case Q2_SERVER:
    file = file_in_dir (games[Q2_SERVER].real_dir, "baseq2/" EXEC_CFG);
    res = write_q2_vars (file, con);
    break;

  default:
    return FALSE;

  }

  if (file) {
    if (!res) {
      if (!dialog_yesno (NULL, 1, _("Launch"), _("Cancel"), 
	    //%s frontend.cfg
             _("Cannot write to file \"%s\".\n\nLaunch client anyway?"), file)) {
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

  // Not used for Quake2 anymore, but not changed as I can't test it
  // right now

  char *argv[32];
  int argi = 0;
  char *q_passwd;
  char *cmd;
  char *file;
  struct game *g = &games[con->s->type];
  int retval=-1;
  char *to_free = NULL;
  
  switch (con->s->type) {

  case QW_SERVER:
    if(default_qw_is_quakeforge)
      q_passwd = "base/" PASSWORD_CFG;
    else
      q_passwd = "id1/" PASSWORD_CFG;
    break;

  case Q2_SERVER:
    q_passwd = "baseq2/" PASSWORD_CFG;
    break;

  default:
    return retval;
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
      //passwords file could not be written
      if (!dialog_yesno (NULL, 1, _("Launch"), _("Cancel"), 
             _("Cannot write to file \"%s\".\n\nLaunch client anyway?"), file)) {
	g_free (file);
	g_free (cmd);
	return retval;
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
  if (to_free)
    g_free (to_free);
  return retval;
}

static int q2_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *q_passwd;
  char *cmd;
  char *file;
  struct game *g = &games[con->s->type];
  int retval=-1;
  char *to_free = NULL;
  
  q_passwd = "baseq2/" PASSWORD_CFG;

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

  if (con->password || con->rcon_password || 
                                    real_password (con->spectator_password)) {
    file = file_in_dir (g->real_dir, q_passwd);

    if (!write_passwords (file, con)) {
      //passwords file could not be written
      if (!dialog_yesno (NULL, 1, _("Launch"), _("Cancel"), 
             _("Cannot write to file \"%s\".\n\nLaunch client anyway?"), file)) {
	g_free (file);
	g_free (cmd);
	return retval;
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

  argv[argi++] = "+exec";
  argv[argi++] = EXEC_CFG;

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
  if (to_free)
    g_free (to_free);
  return retval;
}

static int q2_exec_generic (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  char *to_free = NULL;

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
  if (to_free)
    g_free (to_free);

  return retval;
}

static int q3_exec (const struct condef *con, int forkit) {
  char *argv[64];
  int argi = 0;
  int cmdi = 0;

  char* cmd = NULL;
  char *protocol = NULL;
  char** cmdtokens = NULL;
  char *punkbuster;
  char* protocmdtofree = NULL;
  char** additional_args = NULL;

  char** memoryoptions = NULL;
  int i;

  struct game *g = NULL;
  int retval;
  
  int game_match_result = 0;
  
  char *to_free = NULL;

  int vmfix = 0;
  int setfs_game = 0;
  int set_punkbuster = 0;
  int pass_memory_options = 0;
  
  int vm_game_set = 0;
  int vm_cgame_set = 0;
  int vm_ui_set = 0;
  
  if(!con) return -1;
  if(!con->s) return -1;

  g = &games[con->s->type];
  if(!g) return -1;

  vmfix               = str2bool(game_get_attribute(g->type,"vmfix"));
  setfs_game          = str2bool(game_get_attribute(g->type,"setfs_game"));
  set_punkbuster      = str2bool(game_get_attribute(g->type,"set_punkbuster"));
  pass_memory_options = str2bool(game_get_attribute(g->type,"pass_memory_options"));

  cmdtokens = g_strsplit(g->cmd," ",0);

  if(cmdtokens && *cmdtokens[cmdi])
    cmd=cmdtokens[cmdi++];

  /*
    Figure out what protocal the server is running so we can try to connect
    with a specialized quake3 script. You need to name the scripts like
    quake3proto48.
  */

  protocol = strdup_strip(find_server_setting_for_key ("protocol", con->s->info));
  if (cmd && protocol)
  {
    char* tmp = g_strdup_printf("%sproto%s",cmd,protocol);
    char* tmp_cmd = expand_tilde(tmp);
    g_free(tmp);
    tmp = NULL;

    g_free(protocol);
    protocol = NULL;

    debug (1, "Check for '%s' as a command", tmp_cmd);
  
    // Check to see if we can find that generated filename
    // absolute path?
    if(tmp_cmd[0]=='/')
    {
      if(!access(tmp_cmd,X_OK))
      {
	cmd = protocmdtofree = strdup(tmp_cmd);
	debug(1,"absolute path %s",cmd);
      }
    }
    // no, check $PATH
    else
    {
      char* file = find_file_in_path(tmp_cmd);
      if(file)
      {
	cmd = protocmdtofree = file;
	debug(1,"use file %s in path",cmd);
      }
    }
    g_free(tmp_cmd);
  }

  while(cmd)
  {
    if(*cmd) // skip empty
    {
      argv[argi++] = cmd;
    }
    cmd = cmdtokens[cmdi++];
  }

  if (default_nosound) {
    argv[argi++] = "+set";
    argv[argi++] = "s_initsound";
    argv[argi++] = "0";
  }

  if (con->rcon_password)
  {
    argv[argi++] = "+rconpassword";
    argv[argi++] = con->rcon_password;
  }

  if (con->password)
  {
    argv[argi++] = "+password";
    argv[argi++] = con->password;
  }

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  if (con->demo) {
    argv[argi++] = "+record";
    argv[argi++] = con->demo;
  }

  if(g->game_cfg) {
    argv[argi++] = "+exec";
    argv[argi++] = g->game_cfg;
  }

  if (con->custom_cfg) {
    argv[argi++] = "+exec";
    argv[argi++] = con->custom_cfg;
  }

// useful for wolf too, ef doesn't have it
//  if(g->type == Q3_SERVER)
  {
    /* The 1.32 release of Q3A needs +set cl_punkbuster 1 on the command line. */
    punkbuster = find_server_setting_for_key ("sv_punkbuster", con->s->info);
    if( punkbuster != NULL && strcmp( punkbuster, "1" ) == 0 )
    {
      if( set_punkbuster )
      {
	argv[argi++] = "+set";
	argv[argi++] = "cl_punkbuster";
	argv[argi++] = "1";
      }
      else
      {
	char* option = g_strdup_printf("/" CONFIG_FILE "/Game: %s/punkbuster dialog shown",type2id(g->type));
	debug( 1, "Got %s for punkbuster\n", punkbuster );
	if(!config_get_bool (option))
	{
	  dialog_ok (NULL, _("The server has Punkbuster enabled but it is not going\nto be set on the command line.\nYou may have problems connecting.\nYou can fix this in the game preferences."));
	  config_set_bool (option,TRUE);
	}
	g_free(option);
      }
    }
  }
  
  /*
    If the s->game is set, we want to put fs_game on the command
    line so that the mod is loaded when we connect.
  */
  if((setfs_game) && con->s->game && g->type == Q3_SERVER) {
    if (setfs_game) {
      char* expandedstr = NULL;
      argv[argi++] = "+set";
      argv[argi++] = "fs_game";
      //argv[argi++] = con->s->game;
      
      // Look in home directory /.q3a first
      expandedstr = expand_tilde("~/.q3a");
      argv[argi] = to_free = find_game_dir(expandedstr, con->s->game, &game_match_result);
      g_free(expandedstr);
      expandedstr=NULL;
      debug (1, "find_game_dir result: %d",game_match_result);
      
      if (game_match_result == 0) {  		// 0=not found, 1=exact, 2=differnet case match
        if (to_free) // Holding what was returned from find_game_dir above.  Get rid of it
          g_free (to_free);

        // Didn't find in home directory /.q3a so look in real directory if defined
        argv[argi] = to_free = find_game_dir(g->real_dir, con->s->game, &game_match_result);
      }
      
      argi++;
    }
  }

  if(pass_memory_options == TRUE)
  {
    memoryoptions = g_new0(char*,5); // must be null terminated => max four entries
    argv[argi++] = "+set";
    argv[argi++] = "com_hunkmegs";
    argv[argi++] = memoryoptions[0] = strdup(game_get_attribute(g->type,"com_hunkmegs"));
    argv[argi++] = "+set";
    argv[argi++] = "com_zonemegs";
    argv[argi++] = memoryoptions[1] = strdup(game_get_attribute(g->type,"com_zonemegs"));
    argv[argi++] = "+set";
    argv[argi++] = "com_soundmegs";
    argv[argi++] = memoryoptions[2] = strdup(game_get_attribute(g->type,"com_soundmegs"));
    argv[argi++] = "+set";
    argv[argi++] = "cg_precachedmodels";
    argv[argi++] = memoryoptions[3] = strdup(game_get_attribute(g->type,"cg_precachedmodels"));
  }

  // Append additional args if needed
  i = 0;
  additional_args = get_custom_arguments(g->type, con->s->game);

  while(additional_args && additional_args[i] )
  {
    argv[argi++] = additional_args[i];

    // Check to see if vm_game, vm_cgame or vm_ui was set in custom args
    // Used to prevent them from being re-set if vmfix is enabled below.
    if (strcmp(additional_args[i], "vm_game") == 0)
      vm_game_set = 1;
    else if (strcmp(additional_args[i], "vm_cgame") == 0)
      vm_cgame_set = 1;
    else if (strcmp(additional_args[i], "vm_ui") == 0)
      vm_ui_set = 1;
    i++;
  }

  if(vmfix) {
    if (!vm_game_set)
    {
      argv[argi++] = "+set";
      argv[argi++] = "vm_game";
      argv[argi++] = "2";
    }
    if (!vm_cgame_set)
    {
      argv[argi++] = "+set";
      argv[argi++] = "vm_cgame";
      argv[argi++] = "2";
    }
    if (!vm_ui_set)
    {
      argv[argi++] = "+set";
      argv[argi++] = "vm_ui";
      argv[argi++] = "2";
    }
  }    
   
  argv[argi] = NULL;

  debug(1,"argument count %d", argi);

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (to_free);
  g_free (protocmdtofree);
  g_strfreev(additional_args);
  g_strfreev(cmdtokens);
  g_strfreev(memoryoptions);
  return retval;
}

static int hl_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->gamedir) {
    argv[argi++] = "-game";
    argv[argi++] = con->gamedir;
  }

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  if (con->password) {
    argv[argi++] = "+password";
    argv[argi++] = con->password;
  }

  if (con->rcon_password)
  {
    argv[argi++] = "+rcon_password";
    argv[argi++] = con->rcon_password;
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
  char* hostport=NULL;
  char* real_server=NULL;
  char** additional_args = NULL;
  char **info_ptr;
  int i;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;


// Pass server IP address first otherwise it won't work.
// Make sure ut/ut script (from installed game) contains
// exec "./ut-bin" $* -log and not -log $* at the end
// otherwise XQF you can not connect via the command line!

  if(g->flags & GAME_LAUNCH_HOSTPORT) {
    // go through all server rules
    for (info_ptr = con->s->info; info_ptr && *info_ptr; info_ptr += 2) {
      if (!strcmp (*info_ptr, "hostport")) {
        hostport=info_ptr[1];
      }
    }
  }
  
  if (con->server)
  {
    // gamespy port can be different from game port
    if(hostport)
    {
      real_server = g_strdup_printf ("%s:%s", inet_ntoa (con->s->host->ip), hostport);
    }
    else
      real_server = strdup(con->server);

    if (con->spectate) {
      char* tmp = NULL;
      if(real_password(con->spectator_password))
	tmp = g_strdup_printf("%s?spectatoronly=true?password=%s",real_server,con->spectator_password);
      else
	tmp = g_strdup_printf("%s?spectatoronly=true",real_server);
      g_free(real_server);
      real_server=tmp;
    }

    // Add password if exists
    if (con->password) {
      char* tmp = g_strdup_printf("%s?password=%s",real_server,con->password);
      g_free(real_server);
      real_server=tmp;
    }
  }

  if(con->s)
  {
    // Append additional args if needed
    i = 0;

    additional_args = get_custom_arguments(con->s->type, con->s->game);

//    if (!(additional_args && additional_args[i]))
      argv[argi++] = real_server;
    
    while(additional_args && additional_args[i] )
    {
      argv[argi++] = additional_args[i];
      /*
      // append first argument to server address
      if(i == 0)
      {
	tmp = g_strconcat(real_server,additional_args[i],NULL);
	g_free(real_server);
	real_server=tmp;
	argv[argi++] = real_server;
      }
      else
      {
	argv[argi++] = additional_args[i];
      }
      */
      i++;
    }
  }

  if (default_nosound) {
    argv[argi++] = "-nosound";
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (real_server);
  if(additional_args) g_strfreev(additional_args);
  return retval;
}

static int savage_exec(const struct condef *con, int forkit)
{
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;
  char* connect_arg = NULL;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {
    argv[argi++] = "set";
    argv[argi++] = "autoexec";
    
    if(con->password)
    {
      connect_arg = g_strdup_printf("set cl_password %s; connect %s",con->password, con->server);
    }
    else
    {
      connect_arg = g_strdup_printf("connect %s",con->server);
    }
    
    argv[argi++] = connect_arg;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (connect_arg);
  return retval;

  return 0;
}


// this one just passes the ip address as first parameter
static int exec_generic (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}

// Serious Sam: only supports +connect
static int ssam_exec(const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}

// launch any game that uses the gamespy protocol
// the first argument is the content of gamename field (may be empty),
// the second one the ip of the server
static int gamespy_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = NULL;
  int retval;
  char **info_ptr;
  char* gamename="";

  char* hostport=NULL;
  char* real_server=NULL;
  
  if(!con || !con->s)
    return 1;

  g = &games[con->s->type];

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  // go through all server rules
  for (info_ptr = con->s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (!strcmp (*info_ptr, "gamename")) {
      gamename=info_ptr[1];
    }
    else if (!strcmp (*info_ptr, "hostport")) {
      hostport=info_ptr[1];
    }
  }

  argv[argi++] = strdup_strip (gamename);
  
  if (con->server) {
    // gamespy port can be different from game port
    if(hostport)
    {
      real_server = g_strdup_printf ("%s:%s", inet_ntoa (con->s->host->ip), hostport);
      argv[argi++] = real_server;
    }
    else
    {
      argv[argi++] = con->server;
    }
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (real_server);
  return retval;
}

static int t2_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {

    if(default_t2_name && *default_t2_name) {   
      argv[argi++] = "-login";
      argv[argi++] = default_t2_name;
    }

    argv[argi++] = "-online";
    argv[argi++] = "-connect";
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}

static int bf1942_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = NULL;
  int retval;
  char **info_ptr;
  char* gameid=NULL;

  char* hostport=NULL;
  char* real_server=NULL;
  
  if(!con || !con->s)
    return 1;

  g = &games[con->s->type];

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  argv[argi++] = "+restart";
  argv[argi++] = "1";

  // go through all server rules
  for (info_ptr = con->s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (!strcmp (*info_ptr, "gameid")) {
      gameid=info_ptr[1];
    }
    else if (!strcmp (*info_ptr, "hostport")) {
      hostport=info_ptr[1];
    }
  }

  if(gameid)
  {
    argv[argi++] = "+game";
    argv[argi++] = gameid;
  }

//  argv[argi++] = strdup_strip (gameid);
  
  if (con->server) {
    argv[argi++] = "+joinServer";
    // gamespy port can be different from game port
    if(hostport)
    {
      real_server = g_strdup_printf ("%s:%s", inet_ntoa (con->s->host->ip), hostport);
      argv[argi++] = real_server;
    }
    else
    {
      argv[argi++] = con->server;
    }
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (real_server);
  return retval;
}

static char *dir_custom_cfg_filter (const char *dir, const char *str) {
  static const char *cfgext[] = { ".cfg", ".scr", ".rc", NULL };
  const char **ext;
  size_t len;

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

  path = file_in_dir (qdir, default_qw_is_quakeforge?"base":"id1");
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

static GList *q3_custom_cfgs (char *dir, char *game) {
  GList *cfgs;
  char *qdir;
  char *path = NULL;
  char *mod_path = NULL;

  debug (5, "q3_custom_cfgs(%s,%s)",dir,game);

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

  fprintf (f, "%ld %ld\n", s->refreshed, s->last_answer);

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

      case UN_SERVER:
      case UT2_SERVER:
      case RUNE_SERVER:
      case POSTAL2_SERVER:
      case AAO_SERVER:
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

	/* Q2, etc... */

      case T2_SERVER:
	fprintf (f, 
		 "%s" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%s" QSTAT_DELIM_STR 
		 "%s" QSTAT_DELIM_STR 
		 "\n", //tribe tag not supported yet
		 (p->name)? p->name : "",
		 p->frags,
		 0,  // team number not supported yet
		 "x",  // team name not supported yet
		 (p->model)? p->model : ""); // player_type
	break;

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

// fetch additional arguments when launching server of type 'type' and it's
// game matches 'gamestring'
// returns a newly-allocated array of strings. Use g_strfreev() to free it. 
// TODO use struct server instead of char* to be able to match any variable

char **get_custom_arguments(enum server_type type, const char *gamestring)
{
  struct game *g = &games[type];
  char *arg = NULL;
  int j;
  char conf[15];
  char *token[2];
  int n;
  char ** ret = NULL;
  GSList *temp;

  if(!gamestring) return NULL;
  
  temp = g_slist_nth(g->custom_args, 0);

  j = 0;
  while (temp) {
    g_snprintf (conf, 15, "custom_arg%d", j);
    arg = g_strdup((char *) temp->data);

    n = tokenize (arg, token, 2, ",");
    
    if(!(strcasecmp(token[0],gamestring))) {
      ret = g_strsplit(token[1]," ",0);
      debug(1, "found entry for:%s.  Returning argument:%s\n",gamestring,token[1]);
      g_free(arg);
      return ret;
    }
    temp = g_slist_next(temp);
  }
  debug(1, "get_custom_arguments: Didn't find an entry for %s",gamestring);
  g_free(arg);
  return NULL;
}


/** map functions */

static void quake_init_maps(enum server_type type)
{
  struct quake_private* pd = NULL;

  pd = (struct quake_private*)games[type].pd;
  g_return_if_fail(pd!=NULL);
  
  q3_clear_maps(pd->maphash); // important to avoid memleak when called second time
  pd->maphash = q3_init_maphash();

  findquakemaps(pd->maphash,games[type].real_dir);
}

static void q3_init_maps(enum server_type type)
{
  struct quake_private* pd = NULL;

  pd = (struct quake_private*)games[type].pd;
  g_return_if_fail(pd!=NULL);
  
  q3_clear_maps(pd->maphash); // important to avoid memleak when called second time
  pd->maphash = q3_init_maphash();
  findq3maps(pd->maphash,games[type].real_dir);

  if(pd->home)
  {
    char* home = expand_tilde(pd->home);
    findq3maps(pd->maphash,home);
    g_free(home);
  }
}


static void unreal_init_maps(enum server_type type)
{
  struct unreal_private* pd = NULL;

  pd = (struct unreal_private*)games[type].pd;
  g_return_if_fail(pd!=NULL);
  
  ut_clear_maps(pd->maphash);
  pd->maphash = ut_init_maphash();
  findutmaps_dir(pd->maphash,games[type].real_dir,pd->suffix);
}

static gboolean quake_has_map(struct server* s)
{
  struct quake_private* pd = NULL;
  GHashTable* hash = NULL;

  g_return_val_if_fail(s!=NULL,TRUE);
  
  pd = (struct quake_private*)games[s->type].pd;
  g_return_val_if_fail(pd!=NULL,TRUE);
  
  hash = (GHashTable*)pd->maphash;
  if(!hash) return TRUE;

  if(!s->map)
    return FALSE;

  return q3_lookup_map(hash,s->map);
}

static size_t q3_get_mapshot(struct server* s, guchar** buf)
{
  struct quake_private* pd = NULL;
  GHashTable* hash = NULL;

  g_return_val_if_fail(s!=NULL,TRUE);
  
  pd = (struct quake_private*)games[s->type].pd;
  g_return_val_if_fail(pd!=NULL,TRUE);
  
  hash = (GHashTable*)pd->maphash;
  if(!hash) return TRUE;

  if(!s->map)
    return FALSE;

  return q3_lookup_mapshot(hash,s->map, buf);
}

static gboolean unreal_has_map(struct server* s)
{
  struct unreal_private* pd = NULL;
  GHashTable* hash = NULL;

  g_return_val_if_fail(s!=NULL,TRUE);
  
  pd = (struct unreal_private*)games[s->type].pd;
  g_return_val_if_fail(pd!=NULL,TRUE);
  
  hash = (GHashTable*)pd->maphash;
  if(!hash) return TRUE;

  if(!s->map)
    return FALSE;

  return ut_lookup_map(hash,s->map);
}

// vim: sw=2
