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

#include <sys/types.h>	/* FD_SETSIZE */
#include <stdio.h>	/* fprintf */
#include <stdlib.h>	/* strtol */
#include <string.h>	/* strcmp, strlen */
#include <sys/time.h>	/* FD_SETSIZE */

#include <gtk/gtk.h>

#include "xqf.h"
#include "game.h"
#include "pref.h"
#include "dialogs.h"
#include "skin.h"
#include "utils.h"
#include "srv-prop.h"
#include "pixmaps.h"
#include "xutils.h"
#include "config.h"
#include "rc.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(string) gettext(string)
#define N_(string) (string)
#else
#define _(string) (string)
#endif 


char 	*user_rcdir = NULL;

char 	*default_name = NULL;
char 	*default_team = NULL;
char 	*default_qw_skin = NULL;
char 	*default_q2_skin = NULL;
int 	default_q1_top_color = 0;
int 	default_q1_bottom_color = 0;
int 	default_qw_top_color = 0;
int 	default_qw_bottom_color = 0;

int 	default_rate;
int 	default_cl_nodelta;
int 	default_cl_predict;
int 	default_noaim;
int 	default_noskins;
int 	default_b_switch;
int 	default_w_switch;

int 	pushlatency_mode;
int 	pushlatency_value;

int 	default_nosound;
int 	default_nocdaudio;

int	show_hostnames;
int	show_default_port;

int	default_terminate;
int	default_iconify;
int	default_launchinfo;
int	default_prelaunchexec;
int	default_save_lists;
int 	default_save_srvinfo;
int 	default_save_plrinfo;
int	default_auto_favorites;
int	default_toolbar_style;
int	default_toolbar_tips;
int	default_refresh_sorts;
int	default_refresh_on_update;

int     maxretries;
int     maxsimultaneous;

/* Quake 3 settings */
char *default_q3proto = NULL;
int default_q3vmfix;
int default_q3rafix;
int default_q3setfs_game;

static	int pref_q1_top_color;
static	int pref_q1_bottom_color;
static	int pref_qw_top_color;
static	int pref_qw_bottom_color;
static	int pref_b_switch;
static	int pref_w_switch;
static	int pref_noskins;
static	char *pref_qw_skin;
static	char *pref_q2_skin;

static  GtkWidget *profile_notebook;
static  GtkWidget *games_notebook;

static  GtkWidget *rate_spinner;
static  GtkWidget *cl_nodelta_check_button;
static  GtkWidget *cl_predict_check_button;
static  GtkWidget *noaim_check_button;
static  GtkWidget *pushlat_check_button;
static  GtkWidget *nosound_check_button;
static  GtkWidget *nocdaudio_check_button;
static  GtkWidget *name_entry;
static  GtkWidget *team_entry;
static  GtkWidget *q1_skin_preview = NULL;
static  GtkWidget *qw_skin_preview = NULL;
static  GtkWidget *q2_skin_preview = NULL;
static  GtkWidget *qw_skin_combo;
static  GtkWidget *q2_skin_combo;
static  GtkWidget *q1_top_color_button;
static  GtkWidget *q1_bottom_color_button;
static  GtkWidget *qw_top_color_button;
static  GtkWidget *qw_bottom_color_button;

static  GtkWidget *terminate_check_button;
static  GtkWidget *iconify_check_button;
static  GtkWidget *launchinfo_check_button;
static  GtkWidget *prelaunchexec_check_button;
static  GtkWidget *save_lists_check_button;
static  GtkWidget *save_srvinfo_check_button;
static  GtkWidget *save_plrinfo_check_button;
static  GtkWidget *auto_favorites_check_button;
static  GtkWidget *show_hostnames_check_button;
static  GtkWidget *show_defport_check_button;
static  GtkWidget *toolbar_style_radio_buttons[3];
static  GtkWidget *toolbar_tips_check_button;
static  GtkWidget *refresh_sorts_check_button;
static  GtkWidget *refresh_on_update_check_button;

static  GtkWidget *pushlatency_mode_radio_buttons[3];
static  GtkWidget *pushlatency_value_spinner;

static  GtkWidget *maxretries_spinner;
static  GtkWidget *maxsimultaneous_spinner;

static 	guchar *q1_skin_data = NULL;
static  int q1_skin_is_valid = TRUE;

static 	guchar *qw_skin_data = NULL;
static  int qw_skin_is_valid = TRUE;

static 	guchar *q2_skin_data = NULL;
static  int q2_skin_is_valid = TRUE;

static	GtkWidget *color_menu = NULL;

/* Quake 3 settings */
static GtkWidget *vmfixbutton;
static GtkWidget *rafixbutton;
static GtkWidget *setfs_gamebutton;
static GtkWidget *q3proto_entry;

struct generic_prefs {
  char *pref_dir;
  char *real_dir;
  GtkWidget *dir_entry;
  GtkWidget *cmd_entry;
  GtkWidget *cfg_combo;
  GtkWidget *game_button;
} *genprefs = NULL;


static void get_new_defaults_for_game (enum server_type type) {
  struct game *g = &games[type];
  struct generic_prefs *prefs = &genprefs[type];
  char str[256];

  if (prefs->cmd_entry) {
    if (g->cmd) g_free (g->cmd);
    g->cmd = strdup_strip (gtk_entry_get_text (GTK_ENTRY (prefs->cmd_entry)));
  }

  if (prefs->dir_entry) {
    if (g->dir) g_free (g->dir);
    g->dir = strdup_strip (gtk_entry_get_text (GTK_ENTRY (prefs->dir_entry)));

    if (g->real_dir) g_free (g->real_dir);
    g->real_dir = expand_tilde (g->dir);

    if (prefs->pref_dir) g_free (prefs->pref_dir);
    prefs->pref_dir = NULL;

    if (prefs->real_dir) g_free (prefs->real_dir);
    prefs->real_dir = NULL;
  }

  if (prefs->cfg_combo) {
    if (g->game_cfg) g_free (g->game_cfg);
    g->game_cfg = strdup_strip (gtk_entry_get_text (
                            GTK_ENTRY (GTK_COMBO (prefs->cfg_combo)->entry)));
  }

  g_snprintf (str, 256, "/" CONFIG_FILE "/Game: %s", type2id (type));
  config_push_prefix (str);

  if (g->cmd) 
    config_set_string ("cmd", g->cmd);
  else
    config_clean_key ("cmd");

  if (g->dir) 
    config_set_string ("dir", g->dir);
  else
    config_clean_key ("dir");

  if (g->game_cfg) 
    config_set_string ("custom cfg", g->game_cfg);
  else
    config_clean_key ("custom cfg");

  config_pop_prefix ();
}


static void load_game_defaults (enum server_type type) {
  struct game *g = &games[type];
  char str[256];

  g_snprintf (str, 256, "/" CONFIG_FILE "/Game: %s", type2id (type));
  config_push_prefix (str);

  if (g->cmd) g_free (g->cmd);
  g->cmd = config_get_string ("cmd");

  if (g->dir) g_free (g->dir);
  g->dir = config_get_string ("dir");

  if (g->real_dir) g_free (g->real_dir);
  g->real_dir = expand_tilde (g->dir);

  if (g->game_cfg) g_free (g->game_cfg);
  g->game_cfg = config_get_string ("custom cfg");

  config_pop_prefix ();
}


static void get_new_defaults (void) {
  int i;
  char *str;

  /* Quake */

  config_push_prefix ("/" CONFIG_FILE "/Game: QS");

  if (pref_q1_top_color != default_q1_top_color)
    config_set_int ("top", default_q1_top_color = pref_q1_top_color);

  if (pref_q1_bottom_color != default_q1_bottom_color)
    config_set_int ("bottom", default_q1_bottom_color = pref_q1_bottom_color);

  config_pop_prefix ();

  /* QuakeWorld (some network settings are used by Q2) */

  config_push_prefix ("/" CONFIG_FILE "/Game: QWS");

  str = strdup_strip (gtk_entry_get_text (GTK_ENTRY (team_entry)));
  if (str == NULL || (default_team && strcmp (str, default_team))) {
    if (default_team) g_free (default_team);
    default_team = str;
    config_set_string ("team", (str)? str : "");
  }

  if (pref_qw_top_color != default_qw_top_color)
    config_set_int ("top", default_qw_top_color = pref_qw_top_color);

  if (pref_qw_bottom_color != default_qw_bottom_color)
    config_set_int ("bottom", default_qw_bottom_color = pref_qw_bottom_color);

  if (pref_qw_skin == NULL || 
                (default_qw_skin && strcmp (pref_qw_skin, default_qw_skin))) {
    if (default_qw_skin) g_free (default_qw_skin);
    default_qw_skin = pref_qw_skin;
    config_set_string ("skin", (pref_qw_skin)? pref_qw_skin : "");
  }
  pref_qw_skin = NULL;

  i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (rate_spinner));
  if (i != default_rate)
    config_set_int ("rate", default_rate = i);

  i = GTK_TOGGLE_BUTTON (cl_nodelta_check_button)->active;
  if (i != default_cl_nodelta)
    config_set_int ("cl_nodelta", default_cl_nodelta = i);

  i = 1 - GTK_TOGGLE_BUTTON (cl_predict_check_button)->active;
  if (i != default_cl_predict)
    config_set_int ("cl_predict", default_cl_predict = i);

  i = GTK_TOGGLE_BUTTON (noaim_check_button)->active;
  if (i != default_noaim)
    config_set_int ("noaim", default_noaim = i);

  for (i = 0; i < 3; i++) {
    if (GTK_TOGGLE_BUTTON (pushlatency_mode_radio_buttons[i])->active) {
      if (i != pushlatency_mode)
	config_set_int  ("pushlatency mode", pushlatency_mode = i);
      break;
    }
  }

  i = gtk_spin_button_get_value_as_int (
                                 GTK_SPIN_BUTTON (pushlatency_value_spinner));
  if (i != pushlatency_value)
    config_set_int ("pushlatency value", pushlatency_value = i);

  if (pref_b_switch != default_b_switch)
    config_set_int ("b_switch", default_b_switch = pref_b_switch);

  if (pref_w_switch != default_w_switch)
    config_set_int ("w_switch", default_w_switch = pref_w_switch);

  if (pref_noskins != default_noskins)
    config_set_int ("noskins", default_noskins = pref_noskins);

  config_pop_prefix ();

  /* Quake2 */

  config_push_prefix ("/" CONFIG_FILE "/Game: Q2S");

  if (pref_q2_skin == NULL || 
                (default_q2_skin && strcmp (pref_q2_skin, default_q2_skin))) {
    if (default_q2_skin) g_free (default_q2_skin);
    default_q2_skin = pref_q2_skin;
    config_set_string ("skin", (pref_q2_skin)? pref_q2_skin : "");
  }
  pref_q2_skin = NULL;

  config_pop_prefix ();

  /* Quake 3 */

  config_push_prefix ("/" CONFIG_FILE "/Game: Q3S");

  str = strdup_strip (gtk_entry_get_text (GTK_ENTRY (q3proto_entry)));
  if (default_q3proto) g_free (default_q3proto);
  default_q3proto = str;
  config_set_string ("protocol", (str)? str : "");

  i = GTK_TOGGLE_BUTTON (vmfixbutton)->active;
  if (i != default_q3vmfix)
    config_set_bool ("vmfix", default_q3vmfix = i);

  i = GTK_TOGGLE_BUTTON (rafixbutton)->active;
  if (i != default_q3rafix)
    config_set_bool ("rafix", default_q3rafix = i);

  i = GTK_TOGGLE_BUTTON (setfs_gamebutton)->active;
  if (i != default_q3setfs_game)
    config_set_bool ("setfs_game", default_q3setfs_game = i);

  config_pop_prefix ();

  /* Common part of games config */

  config_push_prefix ("/" CONFIG_FILE "/Games Config");

  str = strdup_strip (gtk_entry_get_text (GTK_ENTRY (name_entry)));
  if (str == NULL || (default_name && strcmp (str, default_name))) {
    if (default_name) g_free (default_name);
    default_name = str;
    config_set_string ("player name", (str)? str : "");
  }

  i = GTK_TOGGLE_BUTTON (nosound_check_button)->active;
  if (i != default_nosound)
    config_set_bool ("nosound", default_nosound = i);

  i = GTK_TOGGLE_BUTTON (nocdaudio_check_button)->active;
  if (i != default_nocdaudio)
    config_set_bool ("nocdaudio", default_nocdaudio = i);

  config_pop_prefix ();

  for (i = 0; i < GAMES_TOTAL; i++)
    get_new_defaults_for_game (i);

  /* Appearance */

  config_push_prefix ("/" CONFIG_FILE "/Appearance");

  for (i = 0; i < 3; i++) {
    if (GTK_TOGGLE_BUTTON (toolbar_style_radio_buttons[i])->active) {
      if (i != default_toolbar_style)
	config_set_int  ("toolbar style", default_toolbar_style = i);
      break;
    }
  }

  i = GTK_TOGGLE_BUTTON (toolbar_tips_check_button)->active;
  if (i != default_toolbar_tips)
    config_set_bool ("toolbar tips", default_toolbar_tips = i);

  i = GTK_TOGGLE_BUTTON (refresh_sorts_check_button)->active;
  if (i != default_refresh_sorts)
    config_set_bool ("sort on refresh", default_refresh_sorts = i);

  i = GTK_TOGGLE_BUTTON (refresh_on_update_check_button)->active;
  if (i != default_refresh_on_update)
    config_set_bool ("refresh on update", default_refresh_on_update = i);

  config_pop_prefix ();

  /* General */

  config_push_prefix ("/" CONFIG_FILE "/General");

  i = GTK_TOGGLE_BUTTON (terminate_check_button)->active;
  if (i != default_terminate)
    config_set_bool ("terminate", default_terminate = i);

  i = GTK_TOGGLE_BUTTON (iconify_check_button)->active;
  if (i != default_iconify)
    config_set_bool ("iconify", default_iconify = i);

  i = GTK_TOGGLE_BUTTON (launchinfo_check_button)->active;
  if (i != default_launchinfo)
    config_set_bool ("launchinfo", default_launchinfo = i);

  i = GTK_TOGGLE_BUTTON (prelaunchexec_check_button)->active;
  if (i != default_prelaunchexec)
    config_set_bool ("prelaunchexec", default_prelaunchexec = i);

  i = GTK_TOGGLE_BUTTON (save_lists_check_button)->active;
  if (i != default_save_lists)
    config_set_bool ("save lists", default_save_lists = i);

  i = GTK_TOGGLE_BUTTON (save_srvinfo_check_button)->active;
  if (i != default_save_srvinfo)
    config_set_bool ("save srvinfo", default_save_srvinfo = i);

  i = GTK_TOGGLE_BUTTON (save_plrinfo_check_button)->active;
  if (i != default_save_plrinfo)
    config_set_bool ("save players", default_save_plrinfo = i);

  i = GTK_TOGGLE_BUTTON (auto_favorites_check_button)->active;
  if (i != default_auto_favorites)
    config_set_bool ("refresh favorites", default_auto_favorites = i);

  config_pop_prefix ();

  /* QStat */

  config_push_prefix ("/" CONFIG_FILE "/QStat");

  i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (maxretries_spinner));
  if (i != maxretries)
    config_set_int ("maxretires", maxretries = i);

  i = gtk_spin_button_get_value_as_int (
                                   GTK_SPIN_BUTTON (maxsimultaneous_spinner));
  if (i != maxsimultaneous)
    config_set_int ("maxsimultaneous", maxsimultaneous = i);

  config_pop_prefix ();

  /* These are set from chained calls to "activate" callbacks */

  gtk_check_menu_item_set_active (
		  GTK_CHECK_MENU_ITEM (view_hostnames_menu_item),
		  GTK_TOGGLE_BUTTON (show_hostnames_check_button)->active);

  gtk_check_menu_item_set_active (
		  GTK_CHECK_MENU_ITEM (view_defport_menu_item),
                  GTK_TOGGLE_BUTTON (show_defport_check_button)->active);

  i = gtk_notebook_get_current_page (GTK_NOTEBOOK (profile_notebook));
  config_set_string ("/" CONFIG_FILE "/Player Profile/game", type2id (i));

  i = gtk_notebook_get_current_page (GTK_NOTEBOOK (games_notebook));
  config_set_string ("/" CONFIG_FILE "/Games Config/game", type2id (i));

  config_sync ();
  rc_save ();
}


static void set_pref_defaults (void) {
  int i;

  pref_q1_top_color    = default_q1_top_color;
  pref_q1_bottom_color = default_q1_bottom_color;

  pref_qw_top_color    = default_qw_top_color;
  pref_qw_bottom_color = default_qw_bottom_color;

  pref_b_switch     = default_b_switch;
  pref_w_switch     = default_w_switch;
  pref_noskins      = default_noskins;

  pref_qw_skin      = g_strdup (default_qw_skin);
  pref_q2_skin      = g_strdup (default_q2_skin);

  for (i = 0; i < GAMES_TOTAL; i++) {
    genprefs[i].pref_dir = g_strdup (games[i].dir);
    genprefs[i].real_dir = g_strdup (games[i].real_dir);
  }
}


static void update_q1_skin (void) {

  if (q1_skin_preview == NULL)
    return;			/* no configuration window */

  if (q1_skin_data) {
    g_free (q1_skin_data);
    q1_skin_data = NULL;
  }

  /* Use QuakeWorld 'base' skin */

  q1_skin_data = get_qw_skin ("base", genprefs[QW_SERVER].real_dir);

  if (q1_skin_data || q1_skin_is_valid) {
    draw_qw_skin (q1_skin_preview, q1_skin_data, 
                                     pref_q1_top_color, pref_q1_bottom_color);
    q1_skin_is_valid = (q1_skin_data)? TRUE : FALSE;
  }
}


static void update_qw_skins (char *initstr) {
  GList *list;
  char *str = NULL;

  if (qw_skin_preview == NULL)
    return;			/* no configuration window */

  if (qw_skin_data) {
    g_free (qw_skin_data);
    qw_skin_data = NULL;
  }

  list = get_qw_skin_list (genprefs[QW_SERVER].real_dir);

  if (initstr) {
    combo_set_vals (qw_skin_combo, list, initstr);
  }
  else {
    str = g_strdup (gtk_entry_get_text (
                               GTK_ENTRY (GTK_COMBO (qw_skin_combo)->entry)));
    combo_set_vals (qw_skin_combo, list, str);
  }

  if (list) {
    qw_skin_data = get_qw_skin (pref_qw_skin, genprefs[QW_SERVER].real_dir);
    g_list_foreach (list, (GFunc) g_free, NULL);
    g_list_free (list);
  }

  if (str) 
    g_free (str);

  if (qw_skin_data || qw_skin_is_valid) {
    draw_qw_skin (qw_skin_preview, qw_skin_data, 
                                     pref_qw_top_color, pref_qw_bottom_color);
    qw_skin_is_valid = (qw_skin_data)? TRUE : FALSE;
  }
}


static void update_q2_skins (char *initstr) {
  GList *list;
  char *str = NULL;

  if (q2_skin_preview == NULL)
    return;			/* no configuration window */

  if (q2_skin_data) {
    g_free (q2_skin_data);
    q2_skin_data = NULL;
  }

  list = get_q2_skin_list (genprefs[Q2_SERVER].real_dir);

  if (initstr) {
    combo_set_vals (q2_skin_combo, list, initstr);
  }
  else {
    str = g_strdup (gtk_entry_get_text (
                               GTK_ENTRY (GTK_COMBO (q2_skin_combo)->entry)));
    combo_set_vals (q2_skin_combo, list, str);
  }

  if (list) {
    q2_skin_data = get_q2_skin (pref_q2_skin, genprefs[Q2_SERVER].real_dir);
    g_list_foreach (list, (GFunc) g_free, NULL);
    g_list_free (list);
  }

  if (str) 
    g_free (str);

  if (q2_skin_data || q2_skin_is_valid) {
    draw_q2_skin (q2_skin_preview, q2_skin_data, Q2_SKIN_SCALE);
    q2_skin_is_valid = (q2_skin_data)? TRUE : FALSE;
  }
}


static void update_cfgs (enum server_type type, char *dir, char *initstr) {
  struct generic_prefs *prefs = &genprefs[type];

  GList *cfgs;
  char *str = NULL;

  cfgs = (*games[type].custom_cfgs) (dir, NULL);

  if (initstr) {
    combo_set_vals (prefs->cfg_combo, cfgs, initstr);
  }
  else {
    str = g_strdup (gtk_entry_get_text (
                            GTK_ENTRY (GTK_COMBO (prefs->cfg_combo)->entry)));
    combo_set_vals (prefs->cfg_combo, cfgs, str);
  }

  if (cfgs) {
    g_list_foreach (cfgs, (GFunc) g_free, NULL);
    g_list_free (cfgs);
  }

  if (str)
    g_free (str);
}


static void dir_entry_activate_callback (GtkWidget *widget, gpointer data) {
  struct generic_prefs *prefs;
  enum server_type type;

  type = (int) gtk_object_get_user_data (GTK_OBJECT (widget));
  prefs = &genprefs[type];

  if (prefs->pref_dir) g_free (prefs->pref_dir);
  prefs->pref_dir = strdup_strip (gtk_entry_get_text (
                                               GTK_ENTRY (prefs->dir_entry)));

  if (prefs->real_dir) g_free (prefs->real_dir);
  prefs->real_dir = expand_tilde (prefs->pref_dir);

  switch (type) {

  case Q1_SERVER:
    update_q1_skin ();
    break;

  case QW_SERVER:
    update_qw_skins (NULL);
    break;

  case Q2_SERVER:
    update_q2_skins (NULL);
    break;

  default:
    break;
  }

  update_cfgs (type, prefs->real_dir, NULL);
}


static void qw_skin_combo_changed_callback (GtkWidget *widget, gpointer data) {
  char *new_skin;

  new_skin = strdup_strip (
           gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (qw_skin_combo)->entry)));

  if (!pref_qw_skin && !new_skin)
    return;

  if (pref_qw_skin && new_skin) {
    if (strcmp (pref_qw_skin, new_skin) == 0) {
      g_free (new_skin);
      return;
    }
  }

  if (pref_qw_skin) g_free (pref_qw_skin);
  pref_qw_skin = new_skin;

  if (qw_skin_data) g_free (qw_skin_data);
  qw_skin_data = get_qw_skin (pref_qw_skin, genprefs[QW_SERVER].real_dir);

  if (qw_skin_data || qw_skin_is_valid) {
    draw_qw_skin (qw_skin_preview, qw_skin_data, 
                                     pref_qw_top_color, pref_qw_bottom_color);
    qw_skin_is_valid = (qw_skin_data)? TRUE : FALSE;
  }
}


static GtkWidget *color_button_event_widget = NULL;


static void set_player_color (GtkWidget *widget, int i) {

  if (color_button_event_widget == qw_top_color_button) {
    if (pref_qw_top_color != i) {
      pref_qw_top_color = i;
      set_bg_color (qw_top_color_button, pref_qw_top_color);
      if (qw_skin_is_valid) {
	draw_qw_skin (qw_skin_preview, qw_skin_data, 
                                     pref_qw_top_color, pref_qw_bottom_color);
      }
    }
    return;
  }

  if (color_button_event_widget == qw_bottom_color_button) {
    if (pref_qw_bottom_color != i) {
      pref_qw_bottom_color = i;
      set_bg_color (qw_bottom_color_button, pref_qw_bottom_color);
      if (qw_skin_is_valid) {
	draw_qw_skin (qw_skin_preview, qw_skin_data,
                                     pref_qw_top_color, pref_qw_bottom_color);
      }
    }
    return;
  }

  if (color_button_event_widget == q1_top_color_button) {
    if (pref_q1_top_color != i) {
      pref_q1_top_color = i;
      set_bg_color (q1_top_color_button, pref_q1_top_color);
      if (q1_skin_is_valid) {
	draw_qw_skin (q1_skin_preview, q1_skin_data, 
                                     pref_q1_top_color, pref_q1_bottom_color);
      }
    }
    return;
  }

  if (color_button_event_widget == q1_bottom_color_button) {
    if (pref_q1_bottom_color != i) {
      pref_q1_bottom_color = i;
      set_bg_color (q1_bottom_color_button, pref_q1_bottom_color);
      if (q1_skin_is_valid) {
	draw_qw_skin (q1_skin_preview, q1_skin_data,
                                     pref_q1_top_color, pref_q1_bottom_color);
      }
    }
    return;
  }
}


static int color_button_event_callback (GtkWidget *widget, GdkEvent *event) {
  GdkEventButton *bevent; 

  if (event->type == GDK_BUTTON_PRESS) {
    bevent = (GdkEventButton *) event; 
    color_button_event_widget = widget;

    if (color_menu == NULL)
      color_menu = create_color_menu (set_player_color);

    gtk_menu_popup (GTK_MENU (color_menu), NULL, NULL, NULL, NULL,
	                                         bevent->button, bevent->time);
    return TRUE;
  }
  return FALSE;
}


static GtkWidget *q1_skin_box_create (void) {
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *alignment;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* Top and Bottom Colors */

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_end (GTK_BOX (hbox), table, FALSE, FALSE, 2);

  /* Top (Shirt) Color */

  label = gtk_label_new (_("Top"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  q1_top_color_button = gtk_button_new_with_label (" ");
  gtk_widget_set_usize (q1_top_color_button, 40, -1);
  gtk_signal_connect (GTK_OBJECT (q1_top_color_button), "event",
                         GTK_SIGNAL_FUNC (color_button_event_callback), NULL);
  gtk_table_attach_defaults (GTK_TABLE (table), q1_top_color_button, 
                                                                  1, 2, 0, 1);
  set_bg_color (q1_top_color_button, fix_qw_player_color (pref_q1_top_color));
  gtk_widget_show (q1_top_color_button);

  /* Bottom (Pants) Color */

  label = gtk_label_new (_("Bottom"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  q1_bottom_color_button = gtk_button_new_with_label (" ");
  gtk_widget_set_usize (q1_bottom_color_button, 40, -1);
  gtk_signal_connect (GTK_OBJECT (q1_bottom_color_button), "event",
                         GTK_SIGNAL_FUNC (color_button_event_callback), NULL);
  gtk_table_attach_defaults (GTK_TABLE (table), q1_bottom_color_button, 
                                                                  1, 2, 1, 2);
  set_bg_color (q1_bottom_color_button, 
                                  fix_qw_player_color (pref_q1_bottom_color));
  gtk_widget_show (q1_bottom_color_button);

  gtk_widget_show (table);

  gtk_widget_show (hbox);

  /* Skin Preview  */

  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (alignment), frame);

  q1_skin_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (q1_skin_preview), 320, 200);
  gtk_container_add (GTK_CONTAINER (frame), q1_skin_preview);
  gtk_widget_show (q1_skin_preview);

  gtk_widget_show (frame);

  gtk_widget_show (alignment);

  gtk_widget_show (vbox);

  return vbox;
}


static GtkWidget *qw_skin_box_create (void) {
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *alignment;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* QW Skin ComboBox */

  alignment = gtk_alignment_new (0, 0, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);

  qw_skin_combo = gtk_combo_new ();
  gtk_entry_set_max_length (GTK_ENTRY (GTK_COMBO (qw_skin_combo)->entry), 256);
  gtk_widget_set_usize (GTK_COMBO (qw_skin_combo)->entry, 112, -1);
  gtk_combo_set_use_arrows_always (GTK_COMBO (qw_skin_combo), TRUE);
  gtk_combo_set_case_sensitive (GTK_COMBO (qw_skin_combo), TRUE);
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (qw_skin_combo)->entry), 
           "changed", GTK_SIGNAL_FUNC (qw_skin_combo_changed_callback), NULL);
  gtk_container_add (GTK_CONTAINER (alignment), qw_skin_combo);
  gtk_widget_show (qw_skin_combo);

  gtk_widget_show (alignment);

  /* Top and Bottom Colors */

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_end (GTK_BOX (hbox), table, FALSE, FALSE, 2);

  /* Top (Shirt) Color */

  label = gtk_label_new (_("Top"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  qw_top_color_button = gtk_button_new_with_label (" ");
  gtk_widget_set_usize (qw_top_color_button, 40, -1);
  gtk_signal_connect (GTK_OBJECT (qw_top_color_button), "event",
                         GTK_SIGNAL_FUNC (color_button_event_callback), NULL);
  gtk_table_attach_defaults (GTK_TABLE (table), qw_top_color_button, 
                                                                  1, 2, 0, 1);
  set_bg_color (qw_top_color_button, fix_qw_player_color (pref_qw_top_color));
  gtk_widget_show (qw_top_color_button);

  /* Bottom (Pants) Color */

  label = gtk_label_new (_("Bottom"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  qw_bottom_color_button = gtk_button_new_with_label (" ");
  gtk_widget_set_usize (qw_bottom_color_button, 40, -1);
  gtk_signal_connect (GTK_OBJECT (qw_bottom_color_button), "event",
                         GTK_SIGNAL_FUNC (color_button_event_callback), NULL);
  gtk_table_attach_defaults (GTK_TABLE (table), qw_bottom_color_button, 
                                                                  1, 2, 1, 2);
  set_bg_color (qw_bottom_color_button, 
                                  fix_qw_player_color (pref_qw_bottom_color));
  gtk_widget_show (qw_bottom_color_button);

  gtk_widget_show (table);

  gtk_widget_show (hbox);

  /* Skin Preview  */

  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (alignment), frame);

  qw_skin_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (qw_skin_preview), 320, 200);
  gtk_container_add (GTK_CONTAINER (frame), qw_skin_preview);
  gtk_widget_show (qw_skin_preview);

  gtk_widget_show (frame);

  gtk_widget_show (alignment);

  gtk_widget_show (vbox);

  return vbox;
}


static void q2_skin_combo_changed_callback (GtkWidget *widget, gpointer data) {
  char *new_skin;

  new_skin = strdup_strip (
           gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (q2_skin_combo)->entry)));

  if (!pref_q2_skin && !new_skin)
    return;

  if (pref_q2_skin && new_skin) {
    if (strcmp (pref_q2_skin, new_skin) == 0) {
      g_free (new_skin);
      return;
    }
  }

  if (pref_q2_skin) g_free (pref_q2_skin);
  pref_q2_skin = new_skin;

  if (q2_skin_data) g_free (q2_skin_data);
  q2_skin_data = get_q2_skin (pref_q2_skin, genprefs[Q2_SERVER].real_dir);

  if (q2_skin_data || q2_skin_is_valid) {
    draw_q2_skin (q2_skin_preview, q2_skin_data, Q2_SKIN_SCALE);
    q2_skin_is_valid = (q2_skin_data)? TRUE : FALSE;
  }
}


static GtkWidget *q2_skin_box_create (void) {
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *alignment;
  GtkWidget *frame;

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* Skin Preview  */

  alignment = gtk_alignment_new (0, 0.5, 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (alignment), frame);

  q2_skin_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
  gtk_preview_size (GTK_PREVIEW (q2_skin_preview), 
                                      32 * Q2_SKIN_SCALE, 32 * Q2_SKIN_SCALE);
  gtk_container_add (GTK_CONTAINER (frame), q2_skin_preview);
  gtk_widget_show (q2_skin_preview);

  gtk_widget_show (frame);
  gtk_widget_show (alignment);

  /* Q2 Skin ComboBox */

  alignment = gtk_alignment_new (1.0, 0, 0, 0);
  gtk_box_pack_end (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);

  q2_skin_combo = gtk_combo_new ();
  gtk_entry_set_max_length (GTK_ENTRY (GTK_COMBO (q2_skin_combo)->entry), 256);
  gtk_widget_set_usize (GTK_COMBO (q2_skin_combo)->entry, 144, -1);
  gtk_combo_set_use_arrows_always (GTK_COMBO (q2_skin_combo), TRUE);
  gtk_combo_set_case_sensitive (GTK_COMBO (q2_skin_combo), TRUE);
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (q2_skin_combo)->entry),
           "changed", GTK_SIGNAL_FUNC (q2_skin_combo_changed_callback), NULL);
  gtk_container_add (GTK_CONTAINER (alignment), q2_skin_combo);
  gtk_widget_show (q2_skin_combo);

  gtk_widget_show (alignment);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  return vbox;
}


static GtkWidget *player_profile_q1_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *alignment;
  GtkWidget *frame;
  GtkWidget *q1_skin;

  page_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 6);

  /* Q1 Colors */

  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_box_pack_start (GTK_BOX (page_vbox), alignment, FALSE, FALSE, 0);

  frame = gtk_frame_new (_("Colors"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (alignment), frame);

  q1_skin = q1_skin_box_create ();
  gtk_container_add (GTK_CONTAINER (frame), q1_skin);

  gtk_widget_show (frame);
  gtk_widget_show (alignment);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


static GtkWidget *player_profile_qw_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *alignment;
  GtkWidget *frame;
  GtkWidget *qw_skin;
  GtkWidget *hbox;
  GtkWidget *label;

  page_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 6);

  /* QW Skin */

  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_box_pack_start (GTK_BOX (page_vbox), alignment, FALSE, FALSE, 0);

  frame = gtk_frame_new (_("Skin/Colors"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (alignment), frame);

  qw_skin = qw_skin_box_create ();
  gtk_container_add (GTK_CONTAINER (frame), qw_skin);

  gtk_widget_show (frame);
  gtk_widget_show (alignment);

  /* QW Team */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 4);

  label = gtk_label_new (_("Team"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  team_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (team_entry, 96, -1);
  if (default_team) {
    gtk_entry_set_text (GTK_ENTRY (team_entry), default_team);
    gtk_entry_set_position (GTK_ENTRY (team_entry), 0);
  }
  gtk_box_pack_start (GTK_BOX (hbox), team_entry, FALSE, FALSE, 0);
  gtk_widget_show (team_entry);

  gtk_widget_show (hbox);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


static GtkWidget *player_profile_q2_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *alignment;
  GtkWidget *frame;
  GtkWidget *q2_skin;

  page_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_box_pack_start (GTK_BOX (page_vbox), alignment, FALSE, FALSE, 0);

  frame = gtk_frame_new (_("Model/Skin"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_add (GTK_CONTAINER (alignment), frame);

  q2_skin = q2_skin_box_create ();
  gtk_container_add (GTK_CONTAINER (frame), q2_skin);

  gtk_widget_show (frame);
  gtk_widget_show (alignment);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


static GtkWidget *player_profile_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *page;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *game_label;
  char *typestr;
  enum server_type type = QW_SERVER;

  page_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 8);

  /* Player Name */

  label = gtk_label_new (_("Name"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  name_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (name_entry, 96, -1);
  if (default_name) {
    gtk_entry_set_text (GTK_ENTRY (name_entry), default_name);
    gtk_entry_set_position (GTK_ENTRY (name_entry), 0);
  }
  gtk_box_pack_start (GTK_BOX (hbox), name_entry, FALSE, FALSE, 0);
  gtk_widget_show (name_entry);

  gtk_widget_show (hbox);

  profile_notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (profile_notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (profile_notebook), 4);
  gtk_box_pack_start (GTK_BOX (page_vbox), profile_notebook, FALSE, FALSE, 0);

  game_label = game_pixmap_with_label (Q1_SERVER);

  page = player_profile_q1_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (profile_notebook), page, game_label);

  game_label = game_pixmap_with_label (QW_SERVER);

  page = player_profile_qw_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (profile_notebook), page, game_label);

  game_label = game_pixmap_with_label (Q2_SERVER);

  page = player_profile_q2_page ();
  gtk_notebook_append_page (GTK_NOTEBOOK (profile_notebook), page, game_label);

  typestr = config_get_string ("/" CONFIG_FILE "/Player Profile/game");
  if (typestr) {
    type = id2type (typestr);
    g_free (typestr);
  }

  gtk_notebook_set_page (GTK_NOTEBOOK (profile_notebook), 
                         (type == Q2_SERVER)? 2 : (type == Q1_SERVER)? 0 : 1);

  gtk_widget_show (profile_notebook);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


static char *wb_switch_labels[9] = {
  N_("--- Anything ---"),
  N_("Axe"),
  N_("Shotgun"),
  N_("Super Shotgun"),
  N_("Nailgun"),
  N_("Super Nailgun"),
  N_("Grenade Launcher"),
  N_("Rocket Launcher"),
  N_("ThunderBolt")
};


static void set_w_switch_callback (GtkWidget *widget, int i) {
  pref_w_switch = i;
}


static void set_b_switch_callback (GtkWidget *widget, int i) {
  pref_b_switch = i;
}


static GtkWidget *create_wb_switch_menu (void (*callback) (GtkWidget *, int)) {
  GtkWidget *menu;
  GtkWidget *menu_item;
  int i;

  menu = gtk_menu_new ();

  for (i = 0; i < 9; i++) {
    menu_item = gtk_menu_item_new_with_label (_(wb_switch_labels[i]));
    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			GTK_SIGNAL_FUNC (callback), (gpointer) i);
    gtk_menu_append (GTK_MENU (menu), menu_item);
    gtk_widget_show (menu_item);
  }

  return menu;
}


static void noskins_option_menu_callback (GtkWidget *widget, int i) {
  pref_noskins = i;
}


static GtkWidget *create_noskins_menu (void) {
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();

  menu_item = gtk_menu_item_new_with_label (_("Use skins"));
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
                 GTK_SIGNAL_FUNC (noskins_option_menu_callback), (gpointer) 0);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label (_("Don\'t use skins"));
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
                 GTK_SIGNAL_FUNC (noskins_option_menu_callback), (gpointer) 1);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label (_("Don\'t download new skins"));
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
                 GTK_SIGNAL_FUNC (noskins_option_menu_callback), (gpointer) 2);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_show (menu_item);

  return menu;
}


static GtkWidget *generic_game_frame (enum server_type type) {
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *label;
  struct generic_prefs *prefs = &genprefs[type];

  frame = gtk_frame_new (games[type].name);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  if ((games[type].flags & GAME_CONNECT) == 0) {
    label = gtk_label_new ("*** Not Implemented ***");
    gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
    gtk_widget_show (label);

    gtk_widget_show (vbox);
    gtk_widget_show (frame);
    return frame;
  }

  table = gtk_table_new ((games[type].custom_cfgs)? 3 : 2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Command Line"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 
                                                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  genprefs[type].cmd_entry = gtk_entry_new ();
  if (games[type].cmd) {
    gtk_entry_set_text (GTK_ENTRY (genprefs[type].cmd_entry), games[type].cmd);
    gtk_entry_set_position (GTK_ENTRY (genprefs[type].cmd_entry), 0);
  }
  gtk_table_attach_defaults (GTK_TABLE (table), genprefs[type].cmd_entry,
                                                                  1, 2, 0, 1);
  gtk_widget_show (genprefs[type].cmd_entry);

  label = gtk_label_new (_("Working Directory"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, 
                                                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);

  genprefs[type].dir_entry = gtk_entry_new ();
  if (genprefs[type].pref_dir) {
    gtk_entry_set_text (GTK_ENTRY (genprefs[type].dir_entry), 
                                                     genprefs[type].pref_dir);
    gtk_entry_set_position (GTK_ENTRY (genprefs[type].dir_entry), 0);
  }
  gtk_table_attach_defaults (GTK_TABLE (table), genprefs[type].dir_entry,
                                                                  1, 2, 1, 2);
  if (games[type].custom_cfgs) {
    gtk_object_set_user_data (GTK_OBJECT (genprefs[type].dir_entry),
			      (gpointer) type);
    gtk_signal_connect (GTK_OBJECT (genprefs[type].dir_entry), 
			"activate",
			GTK_SIGNAL_FUNC (dir_entry_activate_callback),
			NULL);
    gtk_signal_connect (GTK_OBJECT (genprefs[type].dir_entry), 
			"focus_out_event",
			GTK_SIGNAL_FUNC (dir_entry_activate_callback), 
			NULL);
  }
  gtk_widget_show (genprefs[type].dir_entry);

  if (games[type].custom_cfgs) {
    label = gtk_label_new (_("Custom CFG"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3, 
                                                    GTK_FILL, GTK_FILL, 0, 0);
    gtk_widget_show (label);

    prefs->cfg_combo = gtk_combo_new ();
    gtk_entry_set_max_length (
                        GTK_ENTRY (GTK_COMBO (prefs->cfg_combo)->entry), 256);
    gtk_combo_set_case_sensitive (GTK_COMBO (prefs->cfg_combo), TRUE);
    gtk_table_attach_defaults (GTK_TABLE (table), prefs->cfg_combo, 1, 2, 2, 3);
    gtk_widget_show (prefs->cfg_combo);
  }

  gtk_widget_show (table);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  return frame;
}


static void game_radio_butto_toggled_callback (GtkWidget *widget, 
					              enum server_type type) {
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    gtk_notebook_set_page (GTK_NOTEBOOK (games_notebook), type);
}


#define	GAMES_COLS	3
#define GAMES_ROWS	((GAMES_TOTAL + GAMES_COLS - 1) / GAMES_COLS)


static GtkWidget *games_config_page (int defgame) {
  GtkWidget *page_vbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *page;
  GtkWidget *label;
  GtkWidget *game_label;
  GSList *group = NULL;
  char *typestr;
  int i;

  page_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  table = gtk_table_new (GAMES_COLS, GAMES_ROWS, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (page_vbox), table, FALSE, FALSE, 20);

  for (i = 0; i < GAMES_TOTAL; i++) {
    genprefs[i].game_button = gtk_radio_button_new (group);

    game_label = game_pixmap_with_label (i);
    gtk_container_add (GTK_CONTAINER (genprefs[i].game_button), game_label);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (genprefs[i].game_button));
    gtk_table_attach_defaults (GTK_TABLE (table), 
			       genprefs[i].game_button,
			       i % GAMES_COLS, i % GAMES_COLS + 1,
			       i / GAMES_COLS, i / GAMES_COLS + 1); 

    gtk_signal_connect (GTK_OBJECT (genprefs[i].game_button), "toggled",
           GTK_SIGNAL_FUNC (game_radio_butto_toggled_callback), (gpointer) i);

    gtk_widget_show (genprefs[i].game_button);
  }

  gtk_widget_show (table);

  games_notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (games_notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (games_notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (page_vbox), games_notebook, FALSE, FALSE, 0);

  for (i = 0; i < GAMES_TOTAL; i++) {
    page = generic_game_frame (i);

    label = gtk_label_new (games[i].name);
    gtk_widget_show (label);

    gtk_notebook_append_page (GTK_NOTEBOOK (games_notebook), page, label);
  }

  if (defgame == UNKNOWN_SERVER) {
    defgame = QW_SERVER;
    typestr = config_get_string ("/" CONFIG_FILE "/Games Config/game");
    if (typestr) {
      defgame = id2type (typestr);
      g_free (typestr);
    }
  }

  gtk_notebook_set_page (GTK_NOTEBOOK (games_notebook), defgame);

  gtk_toggle_button_set_active (
                     GTK_TOGGLE_BUTTON (genprefs[defgame].game_button), TRUE);

  gtk_widget_show (games_notebook);

  /* Common Options */

  frame = gtk_frame_new (_("Common Options"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 20);

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  /* Disable CD Audio */

  nocdaudio_check_button = 
                      gtk_check_button_new_with_label (_("Disable CD Audio"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nocdaudio_check_button), 
                                                           default_nocdaudio);
  gtk_box_pack_end (GTK_BOX (hbox), nocdaudio_check_button, TRUE, FALSE, 0);
  gtk_widget_show (nocdaudio_check_button);

  /* Disable Sound */

  nosound_check_button = gtk_check_button_new_with_label (_("Disable Sound"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nosound_check_button), 
                                                             default_nosound);
  gtk_box_pack_end (GTK_BOX (hbox), nosound_check_button, TRUE, FALSE, 0);
  gtk_widget_show (nosound_check_button);

  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


static void add_pushlatency_options (GtkWidget *vbox) {
  GtkWidget *hbox;
  GtkObject *adj;
  GSList *group = NULL;
  int i;

  static const char *pushlatency_modes[] = { 
    N_("Do not set (use game default)"), 
    N_("Automatically calculate from server ping time"), 
    N_("Fixed value")
  };

  for (i = 0; i < 3; i++) {
    hbox = gtk_hbox_new (FALSE, 4);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

    pushlatency_mode_radio_buttons[i] = 
             gtk_radio_button_new_with_label (group, _(pushlatency_modes[i]));
    group = gtk_radio_button_group (
                        GTK_RADIO_BUTTON (pushlatency_mode_radio_buttons[i]));
    gtk_box_pack_start (GTK_BOX (hbox), pushlatency_mode_radio_buttons[i], 
                                                             FALSE, FALSE, 0);
    gtk_widget_show (pushlatency_mode_radio_buttons[i]);

    gtk_widget_show (hbox);
  }

  gtk_toggle_button_set_active (
       GTK_TOGGLE_BUTTON (pushlatency_mode_radio_buttons[pushlatency_mode]),
       TRUE);

  adj = gtk_adjustment_new (pushlatency_value, -1000.0, -10.0, 10.0, 50.0, 0.0);

  pushlatency_value_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_spin_button_set_update_policy (
              GTK_SPIN_BUTTON (pushlatency_value_spinner), GTK_UPDATE_ALWAYS);
  gtk_widget_set_usize (pushlatency_value_spinner, 64, -1);
  gtk_box_pack_start (GTK_BOX (hbox), pushlatency_value_spinner, 
                                                             FALSE, FALSE, 0);
  gtk_widget_show (pushlatency_value_spinner);
}

static GtkWidget *q3_options_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

    frame = gtk_frame_new (games[Q3_SERVER].name);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

      vbox = gtk_vbox_new (FALSE, 4);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
      gtk_container_add (GTK_CONTAINER (frame), vbox);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	  label = gtk_label_new (_("Masterserver Protocol Version"));
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	  gtk_widget_show (label);

	  q3proto_entry = gtk_entry_new ();
	  gtk_entry_set_max_length(GTK_ENTRY (q3proto_entry),3);
	  if(default_q3proto)
	    gtk_entry_set_text (GTK_ENTRY (q3proto_entry), default_q3proto);
	  gtk_box_pack_start (GTK_BOX (hbox), q3proto_entry, FALSE, FALSE, 0);
	  gtk_widget_show (q3proto_entry);

	gtk_widget_show (hbox);

	vmfixbutton = gtk_check_button_new_with_label (_("vm_cgame fix"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (vmfixbutton), default_q3vmfix);
	gtk_box_pack_start (GTK_BOX (vbox), vmfixbutton, FALSE, FALSE, 0);
	gtk_widget_show (vmfixbutton);

	rafixbutton = gtk_check_button_new_with_label (_("Rocketarena fix"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (rafixbutton), default_q3rafix);
	gtk_box_pack_start (GTK_BOX (vbox), rafixbutton, FALSE, FALSE, 0);
	gtk_widget_show (rafixbutton);

	setfs_gamebutton = gtk_check_button_new_with_label (_("set fs_game on connect"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (setfs_gamebutton),
							default_q3setfs_game);
	gtk_box_pack_start (GTK_BOX (vbox), setfs_gamebutton, FALSE, FALSE, 0);
	gtk_widget_show (setfs_gamebutton);

      gtk_widget_show (vbox);
    gtk_widget_show (frame);
  gtk_widget_show (page_vbox);

  return page_vbox;
}

static GtkWidget *qw_q2_options_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *frame2;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *option_menu;
  GtkObject *adj;
  char buf[64];

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  /* QW Specific Features */

  frame = gtk_frame_new (games[QW_SERVER].name);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* 'w_switch' & 'b_switch' control */

  frame2 = gtk_frame_new (_("The highest weapon that Quake should "
			    "switch to..."));
  gtk_box_pack_start (GTK_BOX (vbox), frame2, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
  gtk_container_add (GTK_CONTAINER (frame2), vbox2);

  /* 'w_switch' */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("upon a weapon pickup"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), 
                               create_wb_switch_menu (set_w_switch_callback));
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), pref_w_switch);
  gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
  gtk_widget_show (option_menu);

  gtk_widget_show (hbox);

  /* 'b_switch' */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  label = gtk_label_new (_("upon a backpack pickup"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), 
                                create_wb_switch_menu (set_b_switch_callback));
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), pref_b_switch);
  gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
  gtk_widget_show (option_menu);

  gtk_widget_show (hbox);
  gtk_widget_show (vbox2);
  gtk_widget_show (frame2);

  /* 'pushlatency' */

  frame2 = gtk_frame_new (_("pushlatency"));
  gtk_box_pack_start (GTK_BOX (vbox), frame2, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
  gtk_container_add (GTK_CONTAINER (frame2), vbox2);

  add_pushlatency_options (vbox2);

  gtk_widget_show (vbox2);
  gtk_widget_show (frame2);

  /* 'noaim' */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  noaim_check_button = 
                   gtk_check_button_new_with_label (_("Disable auto-aiming"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (noaim_check_button), 
                                                               default_noaim);
  gtk_box_pack_start (GTK_BOX (hbox), noaim_check_button, FALSE, FALSE, 0);
  gtk_widget_show (noaim_check_button);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Network Options */

  g_snprintf (buf, 64, "%s/%s", games[QW_SERVER].name, games[Q2_SERVER].name);

  frame = gtk_frame_new (buf);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  hbox = gtk_hbox_new (FALSE, 16);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* Skins */

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  label = gtk_label_new (_("Skins"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  option_menu = gtk_option_menu_new ();
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), 
                                                       create_noskins_menu ());
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), pref_noskins);
  gtk_box_pack_end (GTK_BOX (hbox2), option_menu, FALSE, FALSE, 0);
  gtk_widget_show (option_menu);

  gtk_widget_show (hbox2);

  /* Rate */

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  label = gtk_label_new (_("Rate"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (default_rate, 0.0, 25000.0, 500.0, 1000.0, 0.0);

  rate_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (rate_spinner), 
                                                            GTK_UPDATE_ALWAYS);
  gtk_widget_set_usize (rate_spinner, 64, -1);
  gtk_box_pack_end (GTK_BOX (hbox2), rate_spinner, FALSE, FALSE, 0);
  gtk_widget_show (rate_spinner);

  gtk_widget_show (hbox2);

  gtk_widget_show (hbox);

  /* Troubleshooting */

  frame2 = gtk_frame_new (_("Troubleshooting"));
  gtk_box_pack_start (GTK_BOX (vbox), frame2, FALSE, FALSE, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
  gtk_container_add (GTK_CONTAINER (frame2), vbox2);

  /* 'cl_nodelta' */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  cl_nodelta_check_button = gtk_check_button_new_with_label (
                                 _("Disable delta-compression (cl_nodelta)"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cl_nodelta_check_button), 
                                                          default_cl_nodelta);
  gtk_box_pack_start (GTK_BOX (hbox), cl_nodelta_check_button, FALSE, FALSE, 0);
  gtk_widget_show (cl_nodelta_check_button);

  gtk_widget_show (hbox);

  /* 'cl_predict_players' ('cl_predict' in Q2) */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

  cl_predict_check_button = gtk_check_button_new_with_label (
                  _("Disable player/entity prediction (cl_predict_players)"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cl_predict_check_button), 
                                                      1 - default_cl_predict);
  gtk_box_pack_start (GTK_BOX (hbox), cl_predict_check_button, FALSE, FALSE, 0);
  gtk_widget_show (cl_predict_check_button);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox2);
  gtk_widget_show (frame2);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


static void terminate_toggled_callback (GtkWidget *widget, gpointer data) {
  int val;

  val = GTK_TOGGLE_BUTTON (terminate_check_button)->active;
  gtk_widget_set_sensitive (iconify_check_button, TRUE - val);
}

static void launchinfo_toggled_callback (GtkWidget *widget, gpointer data) {
  int val;

  val = GTK_TOGGLE_BUTTON (launchinfo_check_button)->active;
}

static void prelaunchexec_toggled_callback (GtkWidget *widget, gpointer data) {
  int val;

  val = GTK_TOGGLE_BUTTON (prelaunchexec_check_button)->active;
}


static void save_srvinfo_toggled_callback (GtkWidget *widget, gpointer data) {
  int val;

  val = GTK_TOGGLE_BUTTON (save_srvinfo_check_button)->active;
  gtk_widget_set_sensitive (save_plrinfo_check_button, val);
}


static GtkWidget *appearance_options_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GSList *group = NULL;
  static const char *toolbar_styles[] = { N_("Icons"), N_("Text"), N_("Both") };
  int i;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  frame = gtk_frame_new (_("Server List"));
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Lookup host names */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  show_hostnames_check_button = 
                       gtk_check_button_new_with_label (_("Show host names"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_hostnames_check_button),
                                                              show_hostnames);
  gtk_box_pack_start (GTK_BOX (hbox), show_hostnames_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_widget_show (show_hostnames_check_button);

  gtk_widget_show (hbox);

  /* Show default port */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  show_defport_check_button = 
                     gtk_check_button_new_with_label (_("Show default port"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_defport_check_button),
                                                           show_default_port);
  gtk_box_pack_start (GTK_BOX (hbox), show_defport_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_widget_show (show_defport_check_button);

  gtk_widget_show (hbox);

  /* Sort servers real-time during refresh */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  refresh_sorts_check_button = gtk_check_button_new_with_label (
                                  _("Sort servers real-time during refresh"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (refresh_sorts_check_button),
                                                       default_refresh_sorts);
  gtk_box_pack_start (GTK_BOX (hbox), refresh_sorts_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_widget_show (refresh_sorts_check_button);

  gtk_widget_show (hbox);

  /* Refresh on update */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  refresh_on_update_check_button = 
                     gtk_check_button_new_with_label (_("Refresh on update"));
  gtk_toggle_button_set_active (
			   GTK_TOGGLE_BUTTON (refresh_on_update_check_button), 
			   default_refresh_on_update);
  gtk_box_pack_start (GTK_BOX (hbox), refresh_on_update_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_widget_show (refresh_on_update_check_button);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Toolbar */

  frame = gtk_frame_new (_("Toolbar"));
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  /* Toolbar Style */

  for (i = 0; i < 3; i++) {
    toolbar_style_radio_buttons[i] = 
                gtk_radio_button_new_with_label (group, _(toolbar_styles[i]));
    group = gtk_radio_button_group (
                           GTK_RADIO_BUTTON (toolbar_style_radio_buttons[i]));
    gtk_box_pack_start (GTK_BOX (hbox), toolbar_style_radio_buttons[i], 
                                                             FALSE, FALSE, 0);
    gtk_widget_show (toolbar_style_radio_buttons[i]);
  }

  gtk_toggle_button_set_active (
       GTK_TOGGLE_BUTTON (toolbar_style_radio_buttons[default_toolbar_style]),
       TRUE);

  /* Toolbar Tips */

  toolbar_tips_check_button = gtk_check_button_new_with_label (_("Tooltips"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (toolbar_tips_check_button),
                                                        default_toolbar_tips);
  gtk_box_pack_end (GTK_BOX (hbox), toolbar_tips_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_widget_show (toolbar_tips_check_button);

  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  gtk_widget_show (page_vbox);

  return page_vbox;
}

static GtkWidget *general_options_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GSList *group = NULL;
  static const char *toolbar_styles[] = { N_("Icons"), N_("Text"), N_("Both") };
  int i;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* On Startup */

  frame = gtk_frame_new (_("On Startup"));
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  /* Refresh Favorites */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  auto_favorites_check_button = 
                     gtk_check_button_new_with_label (_("Refresh Favorites"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_favorites_check_button),
                                                       default_auto_favorites);
  gtk_box_pack_start (GTK_BOX (hbox), auto_favorites_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_widget_show (auto_favorites_check_button);

  gtk_widget_show (hbox);

  gtk_widget_show (frame);

  /* On Exit */

  frame = gtk_frame_new (_("On Exit"));
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  /* Save master lists */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  save_lists_check_button = 
                     gtk_check_button_new_with_label (_("Save server lists"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (save_lists_check_button), 
                                                          default_save_lists);
  gtk_box_pack_start (GTK_BOX (hbox), save_lists_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_widget_show (save_lists_check_button);

  gtk_widget_show (hbox);

  /* Save server information */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  save_srvinfo_check_button = 
               gtk_check_button_new_with_label (_("Save server information"));
  gtk_box_pack_start (GTK_BOX (hbox), save_srvinfo_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (save_srvinfo_check_button), 
                                                        default_save_srvinfo);
  gtk_signal_connect (GTK_OBJECT (save_srvinfo_check_button), "toggled",
                       GTK_SIGNAL_FUNC (save_srvinfo_toggled_callback), NULL);
  gtk_widget_show (save_srvinfo_check_button);

  gtk_widget_show (hbox);

  /* Save player information */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  save_plrinfo_check_button = 
               gtk_check_button_new_with_label (_("Save player information"));
  gtk_box_pack_start (GTK_BOX (hbox), save_plrinfo_check_button, 
                                                             FALSE, FALSE, 0);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (save_plrinfo_check_button), 
                                                        default_save_plrinfo);
  if (!default_save_srvinfo)
    gtk_widget_set_sensitive (save_plrinfo_check_button, FALSE);
  gtk_widget_show (save_plrinfo_check_button);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* On Quake Launch */

  frame = gtk_frame_new (_("When launching a game..."));
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 2); 
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox); 

  /* Terminate */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  terminate_check_button = gtk_check_button_new_with_label (
                           _("Terminate XQF"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (terminate_check_button),
                                                           default_terminate);
  gtk_signal_connect (GTK_OBJECT (terminate_check_button), "toggled",
                          GTK_SIGNAL_FUNC (terminate_toggled_callback), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), terminate_check_button, FALSE, FALSE, 0);
  gtk_widget_show (terminate_check_button);

  gtk_widget_show (hbox); 
   
  /* Iconify */

  iconify_check_button = 
                    gtk_check_button_new_with_label (_("Iconify XQF window"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (iconify_check_button), 
                                                             default_iconify);
  if (default_terminate)
    gtk_widget_set_sensitive (iconify_check_button, FALSE);
  gtk_widget_show (iconify_check_button);

  gtk_box_pack_end (GTK_BOX (hbox), iconify_check_button, FALSE, FALSE, 0);

  /* Launchinfo */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  launchinfo_check_button = gtk_check_button_new_with_label 
      (_("Create LaunchInfo.txt"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launchinfo_check_button),
                                                           default_launchinfo);
  gtk_signal_connect (GTK_OBJECT (launchinfo_check_button), "toggled",
                          GTK_SIGNAL_FUNC (launchinfo_toggled_callback), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), launchinfo_check_button, FALSE, FALSE, 0);
  gtk_widget_show (launchinfo_check_button);

//  gtk_widget_show (hbox);

  /* Prelaunchinfo */

  prelaunchexec_check_button = gtk_check_button_new_with_label 
      (_("Execute prelaunch"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prelaunchexec_check_button),
                                                           default_prelaunchexec);
  gtk_signal_connect (GTK_OBJECT (prelaunchexec_check_button), "toggled",
                          GTK_SIGNAL_FUNC (prelaunchexec_toggled_callback), NULL);
  gtk_widget_show (prelaunchexec_check_button);
  gtk_box_pack_end (GTK_BOX (hbox), prelaunchexec_check_button, FALSE, FALSE, 0);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox); 
  gtk_widget_show (frame);

  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


static GtkWidget *qstat_options_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkObject *adj;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  /* QStat preferences -- maxsimultaneous & maxretries */

  frame = gtk_frame_new (_("QStat Options"));
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* maxsimultaneous */

  label = gtk_label_new (_("Number of simultaneous servers to query"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (maxsimultaneous, 1.0, FD_SETSIZE, 1.0, 5.0, 0.0);

  maxsimultaneous_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (maxsimultaneous_spinner),
                                                           GTK_UPDATE_ALWAYS);
  gtk_widget_set_usize (maxsimultaneous_spinner, 48, -1);
  gtk_table_attach (GTK_TABLE (table), maxsimultaneous_spinner, 1, 2, 0, 1,
                                                                  0, 0, 0, 0);
  gtk_widget_show (maxsimultaneous_spinner);

  /* maxretries */

  label = gtk_label_new (_("Number of retries"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (maxretries, 1.0, MAX_RETRIES, 1.0, 1.0, 0.0);

  maxretries_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_widget_set_usize (maxretries_spinner, 48, -1);
  gtk_table_attach (GTK_TABLE (table), maxretries_spinner, 1, 2, 1, 2, 
                                                                  0, 0, 0, 0);
  gtk_widget_show (maxretries_spinner);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


void preferences_dialog (int page_num) {
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *notebook;
  GtkWidget *page;
  GtkWidget *button;
  GtkWidget *window;
  int game_num;

  game_num = page_num / 256;
  page_num = page_num % 256;

  if (!genprefs)
    genprefs = g_malloc0 (sizeof (struct generic_prefs) * GAMES_TOTAL);

  set_pref_defaults ();

  window = dialog_create_modal_transient_window (
                                    _("XQF: Preferences"), TRUE, FALSE, NULL);
  if (!GTK_WIDGET_REALIZED (window))
    gtk_widget_realize (window);

  allocate_quake_player_colors (window->window);

  vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);
         
  /*
   *  Notebook
   */

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (notebook), 4);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, FALSE, FALSE, 0);

  page = general_options_page ();
  label = gtk_label_new (_("General"));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  page = player_profile_page ();
  label = gtk_label_new (_("Player Profile"));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  page = games_config_page (game_num);
  label = gtk_label_new (_("Games"));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  page = appearance_options_page ();
  label = gtk_label_new (_("Appearance"));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  page = qstat_options_page ();
  label = gtk_label_new (_("QStat"));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  page = qw_q2_options_page ();
  label = gtk_label_new (_("QW/Q2"));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  page = q3_options_page ();
  label = gtk_label_new (_("Q3"));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), page_num);

  /* Initialize skins and custom cfgs */

  q1_skin_is_valid = TRUE;
  update_q1_skin ();
  update_cfgs (Q1_SERVER, genprefs[Q1_SERVER].real_dir,
                                                   games[Q1_SERVER].game_cfg);
  qw_skin_is_valid = TRUE;
  update_qw_skins (pref_qw_skin);
  update_cfgs (QW_SERVER, genprefs[QW_SERVER].real_dir, 
                                                   games[QW_SERVER].game_cfg);
  q2_skin_is_valid = TRUE;
  update_q2_skins (pref_q2_skin);
  update_cfgs (Q2_SERVER, genprefs[Q2_SERVER].real_dir, 
                                                   games[Q2_SERVER].game_cfg);
  gtk_widget_show (notebook);

  /* 
   *  Buttons at the bottom
   */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                    GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("OK"));
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		                     GTK_SIGNAL_FUNC (get_new_defaults), NULL);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                    GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  gtk_widget_show (window);

  gtk_main ();

  unregister_window (window);

  /* clean up */

  if (color_menu) {
    gtk_widget_destroy (color_menu);
    color_menu = NULL;
  }

  qw_skin_preview = NULL;
  q2_skin_preview = NULL;

  if (qw_skin_data) { 
    g_free (qw_skin_data);
    qw_skin_data = NULL; 
  }

  if (q2_skin_data) { 
    g_free (q2_skin_data);
    q2_skin_data = NULL; 
  }
}


static void user_fix_defaults (void) {
  config_set_string ("/" CONFIG_FILE "/Game: QS/cmd", "squake");
  config_set_string ("/" CONFIG_FILE "/Game: QWS/cmd", "qwcl.x11");
  config_set_string ("/" CONFIG_FILE "/Game: Q2S/cmd", "quake2");
#ifdef QSTAT23
  config_set_string ("/" CONFIG_FILE "/Game: Q3S/cmd", "linuxquake3");
#endif
  config_set_string ("/" CONFIG_FILE "/Games Config/player name", 
                                                          g_get_user_name ());
}


int fix_qw_player_color (int color) {
  color = color % 16;
  if (color > 13)
    color = 13;
  return color;
}


int init_user_info (void) {
  if (!g_get_user_name () || !g_get_home_dir () || !g_get_tmp_dir ()) {
    fprintf (stderr, _("Unable to get user name/home/tmpdir\n"));
    return FALSE;
  }
  user_rcdir  = file_in_dir (g_get_home_dir (), RC_DIR);
  return TRUE;
}


void free_user_info (void) {
  if (user_rcdir) {
    g_free (user_rcdir);
    user_rcdir = NULL; 
  }
}


int prefs_load (void) {
  char *oldversion;
  int i;
  int old_rc_loaded;
  int newversion = FALSE;

  oldversion = config_get_string ("/" CONFIG_FILE "/Program/version");

  old_rc_loaded = rc_parse ();
  if (old_rc_loaded == 0)
    rc_save ();

  if (oldversion) {
    newversion = g_strcasecmp (oldversion, XQF_VERSION);
    g_free (oldversion);
  }
  else {
    newversion = TRUE;
    if (old_rc_loaded == -1)
      user_fix_defaults ();
  }

  config_push_prefix ("/" CONFIG_FILE "/Game: QS");

  default_q1_top_color =      config_get_int ("top=0");
  default_q1_bottom_color =   config_get_int ("bottom=0");

  config_pop_prefix ();

  config_push_prefix ("/" CONFIG_FILE "/Game: QWS");

  default_team =              config_get_string ("team");
  default_qw_skin =           config_get_string ("skin");
  default_qw_top_color =      config_get_int ("top=0");
  default_qw_bottom_color =   config_get_int ("bottom=0");

  default_rate =              config_get_int ("rate=2500");
  default_cl_nodelta =        config_get_int ("cl_nodelta=0");
  default_cl_predict =        config_get_int ("cl_predict=1");
  default_noaim =             config_get_int ("noaim=0");
  default_b_switch =          config_get_int ("b_switch=0");
  default_w_switch =          config_get_int ("w_switch=0");
  default_noskins =           config_get_int ("noskins=0");
  pushlatency_mode =          config_get_int ("pushlatency mode=1");
  pushlatency_value =         config_get_int ("pushlatency value=-50");

  config_pop_prefix ();

  config_push_prefix ("/" CONFIG_FILE "/Game: Q2S");

  default_q2_skin =           config_get_string ("skin");

  config_pop_prefix ();

  /* Quake3 */
  config_push_prefix ("/" CONFIG_FILE "/Game: Q3S");

  default_q3proto =           config_get_string ("protocol=66");
  if(strlen(default_q3proto)==0) default_q3proto=NULL;
  default_q3vmfix =           config_get_bool ("vmfix=true");
  default_q3rafix =           config_get_bool ("rafix=true");
  default_q3setfs_game =           config_get_bool ("setfs_game=true");

  config_pop_prefix ();

  config_push_prefix ("/" CONFIG_FILE "/Games Config");

  default_nosound =           config_get_bool ("nosound=false");
  default_nocdaudio =         config_get_bool ("nocdaudio=false");
  default_name =              config_get_string ("player name");

  config_pop_prefix ();

  config_push_prefix ("/" CONFIG_FILE "/Appearance");

  show_hostnames =            config_get_bool ("show hostnames=true");
  show_default_port =         config_get_bool ("show default port=true");
  default_toolbar_style =     config_get_int  ("toolbar style=2");
  default_toolbar_tips =      config_get_bool ("toolbar tips=true");
  default_refresh_sorts =     config_get_bool ("sort on refresh=true");
  default_refresh_on_update = config_get_bool ("refresh on update=true");

  config_pop_prefix ();

  config_push_prefix ("/" CONFIG_FILE "/General");

  default_terminate =         config_get_bool ("terminate=false");
  default_iconify =           config_get_bool ("iconify=false");
  default_launchinfo =        config_get_bool ("launchinfo=false");
  default_prelaunchexec =     config_get_bool ("prelaunchexec=false");
  default_save_lists =        config_get_bool ("save lists=true");
  default_save_srvinfo =      config_get_bool ("save srvinfo=true");
  default_save_plrinfo =      config_get_bool ("save players=false");
  default_auto_favorites =    config_get_bool ("refresh favorites=false");

  config_pop_prefix ();

  config_push_prefix ("/" CONFIG_FILE "/QStat");

  maxretries =                config_get_int ("maxretires=3");
  maxsimultaneous =           config_get_int ("maxsimultaneous=20");

  config_pop_prefix ();

  for (i = 0; i < GAMES_TOTAL; i++)
    load_game_defaults (i);

  config_set_string ("/" CONFIG_FILE "/Program/version", XQF_VERSION);
  config_sync ();

#ifdef DEBUG
  fprintf (stderr, "prefs_load(): program version %s\n", 
                            (newversion)? "changed" : "not changed");
#endif

  /* Convert "dir" -> "real_dir" for all game types */

  for (i = 0; i < GAMES_TOTAL; i++) {
    if (games[i].real_dir) g_free (games[i].real_dir);
    games[i].real_dir = expand_tilde (games[i].dir);
  }

  return newversion;
}


/*
void prefs_save (void) {
  config_push_prefix ("/" CONFIG_FILE "/Preferences");
  config_set_bool ("show hostnames", show_hostnames);
  config_set_bool ("show default port", show_default_port);
  config_pop_prefix ();
  config_sync ();
}
*/

