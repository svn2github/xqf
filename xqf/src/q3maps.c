/* XQF - Quake server browser and launcher
 * Functions for finding installed q1-q3&clones maps
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

#include "gnuconfig.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <glib.h>

#include "debug.h"
#include "zip/unzip.h"

#include "q3maps.h"


/** pak file code ripped from q2 */

typedef unsigned char 		byte;

#define IDPAKHEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')

typedef struct
{
    char	name[56];
    int		filepos, filelen;
} dpackfile_t;

typedef struct
{
    int		ident;		// == IDPAKHEADER
    int		dirofs;
    int		dirlen;
} dpackheader_t;

static inline int    LittleLong (int l)
{
#if BYTE_ORDER == BIG_ENDIAN
    byte    b1,b2,b3,b4;

    b1 = l&255;
    b2 = (l>>8)&255;
    b3 = (l>>16)&255;
    b4 = (l>>24)&255;

    return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
#else
    return l;
#endif
}

static void findmaps_pak(const char* packfile, GHashTable* maphash)
{
    dpackheader_t header;
    int numpackfiles;
    int fd;
    dpackfile_t info;

    fd = open(packfile, O_RDONLY);
    if (fd < 0)
    {
	perror(packfile);
	return;
    }

    if(read (fd, &header, sizeof(header))<0)
    {
	perror(packfile);
	return;
    }
    if (LittleLong(header.ident) != IDPAKHEADER)
    {
	debug(0,"%s is not a packfile\n", packfile);
	return;
    }
    header.dirofs = LittleLong (header.dirofs);
    header.dirlen = LittleLong (header.dirlen);

    numpackfiles = header.dirlen / sizeof(dpackfile_t);

//    printf ("packfile %s (%i files)\n", packfile, numpackfiles);

    if(lseek (fd, header.dirofs, SEEK_SET)==-1)
    {
	perror(packfile);
	return;
    }

    while(read (fd,&info, sizeof(dpackfile_t))>0)
    {
	if(!strncmp(info.name,"maps/",5) &&
	    !strcmp(info.name+strlen(info.name)-4,".bsp"))
	{
	    // s#maps/(.*)\.bsp#\1#
	    char* mapname=g_strndup(info.name+5,strlen(info.name)-4-5);
	    if(g_hash_table_lookup(maphash,mapname))
	    {
		g_free(mapname);
	    }
	    else
	    {
		g_hash_table_insert(maphash,mapname,GUINT_TO_POINTER(1));
	    }

	}
    }

    close(fd);

    return;
}

/** open zip file and insert all contained .bsp into maphash */
static void findq3maps_zip(const char* path, GHashTable* maphash)
{
    unzFile f;
    int ret;

    if(!path || !maphash)
	return;

    f=unzOpen(path);
    if(!f)
    {
	debug(0,"error opening zip file %s",path);
	return;
    }

    for (ret = unzGoToFirstFile(f); ret == UNZ_OK; ret = unzGoToNextFile(f))
    {
	enum { bufsize = 256 };
	char buf[bufsize] = {0};
	if(unzGetCurrentFileInfo(f,NULL,buf,bufsize,NULL,0,NULL,0) == UNZ_OK)
	{
	    if(!g_strncasecmp(buf,"maps/",5)
		&& !g_strcasecmp(buf+strlen(buf)-4,".bsp"))
	    {
		// s#maps/(.*)\.bsp#\1#
		char* mapname=g_strndup(buf+5,strlen(buf)-4-5);
		if(g_hash_table_lookup(maphash,mapname))
		{
		    g_free(mapname);
		}
		else
		{
		    g_hash_table_insert(maphash,mapname,GUINT_TO_POINTER(1));
		}
	    }
	}
    }

    if(unzClose(f) != UNZ_OK)
    {
	debug(0,"couldn't close file %s",path);
    }
}

gboolean quake_contains_dir(const char* name, int level, GHashTable* maphash)
{
//   printf("directory %s at level %d\n",name,level);
    if(level == 0)
	return TRUE;
    if(level == 1 && !g_strcasecmp(name+strlen(name)-4,"maps"))
	return TRUE;

    return FALSE;
}

void quake_contains_file( const char* name, int level, GHashTable* maphash)
{
//    printf("%s at level %d\n",name,level);
    if(strlen(name)>4
	&& !g_strcasecmp(name+strlen(name)-4,".pak")
	&& level == 1)
    {
	findmaps_pak(name,maphash);
    }
    if(strlen(name)>4
	&& !g_strcasecmp(name+strlen(name)-4,".bsp")
	&& level == 2)
    {
	char* basename = g_basename(name);
	char* mapname=g_strndup(basename,strlen(basename)-4);
	if(g_hash_table_lookup(maphash,mapname))
	{
	    g_free(mapname);
	}
	else
	{
	    g_hash_table_insert(maphash,mapname,GUINT_TO_POINTER(1));
	}
    }
}


void q3_contains_file(const char* name, int level, GHashTable* maphash)
{
//    printf("%s at level %d\n",name,level);
    if(strlen(name)>4
	&& !g_strcasecmp(name+strlen(name)-4,".pk3")
	&& level == 1)
    {
	findq3maps_zip(name,maphash);
    }
    if(strlen(name)>4
	&& !g_strcasecmp(name+strlen(name)-4,".bsp")
	&& level == 2)
    {
	char* basename = g_basename(name);
	char* mapname=g_strndup(basename,strlen(basename)-4);
	if(g_hash_table_lookup(maphash,mapname))
	{
	    g_free(mapname);
	}
	else
	{
	    g_hash_table_insert(maphash,mapname,GUINT_TO_POINTER(1));
	}
    }
}

/** 
 * traverse directory tree starting at startdir. Calls found_file for each file
 * and found_dir for each directory (non-recoursive). Note: There is no loop
 * detection so make sure found_dir returns false at some level.
**/
void traverse_dir(const char* startdir, FoundFileFunction found_file, FoundDirFunction found_dir, gpointer data)
{
    DIR* dir = NULL;
    struct dirent* dire = NULL;
    char* curdir = NULL;
    GSList* dirstack = NULL;
    typedef struct
    {
	char* name;
	int level;
    } DirStackEntry;

    DirStackEntry* dse;
    
    if(!startdir || !found_dir || !found_file)
	return;

    dse = g_new0(DirStackEntry,1);
    dse->name=g_strdup(startdir);

    dirstack = g_slist_prepend(dirstack,dse);

    while(dirstack)
    {
	GSList* current = dirstack;
	dirstack = g_slist_remove_link(dirstack,dirstack);

	dse = current->data;
	curdir = dse->name;

	dir = opendir(curdir);
	if(!dir)
	{
	    perror(curdir);
	    g_free(curdir);
	    g_free(dse);
	    continue;
	}

	while((dire = readdir(dir)))
	{
	    char* name = dire->d_name;
	    struct stat statbuf;

	    if(!strcmp(name,".") || !strcmp(name,".."))
		continue;

	    name = g_strconcat(curdir,"/",name,NULL);
	    if(stat(name,&statbuf))
	    {
		perror(name);
		g_free(name);
		continue;
	    }
	    if(S_ISDIR(statbuf.st_mode))
	    {
		if(found_dir(name, dse->level, data))
		{
		    DirStackEntry* tmpdse = g_new0(DirStackEntry,1);
		    tmpdse->name=name;
		    tmpdse->level=dse->level+1;
		    dirstack = g_slist_prepend(dirstack,tmpdse);
		}
		else
		{
		    g_free(name);
		}
	    }
	    else
	    {
		found_file(name, dse->level, data);
		g_free(name);
	    }
	}

	closedir(dir);
	g_free(curdir);
	g_free(dse);
    }
}

// case insensitive compare for hash
static gint maphashmapsequal(gconstpointer m1, gconstpointer m2)
{
    return (g_strcasecmp(m1,m2)==0);
}

static void maphashforeachfunc(char* key, gpointer value, gpointer user_data)
{
    printf("%s ",key);
}

static gboolean maphashforeachremovefunc(gpointer key, gpointer value, gpointer user_data)
{
    g_free(key);
    return TRUE;
}

/** free all keys and destroy maphash */
void q3_clear_maps(GHashTable* maphash)
{
    if(!maphash)
	return;
    g_hash_table_foreach_remove(maphash, maphashforeachremovefunc, NULL);
    g_hash_table_destroy(maphash);
}

/** create map hash */
GHashTable* q3_init_maphash()
{
    return g_hash_table_new(g_str_hash,maphashmapsequal);
}

/** return true if mapname is contained in maphash, false otherwise */
gboolean q3_lookup_map(GHashTable* maphash, const char* mapname)
{
    if(g_hash_table_lookup(maphash,mapname))
	return TRUE;
    return FALSE;
}

void findq3maps(GHashTable* maphash, const char* startdir)
{
    traverse_dir(startdir, (FoundFileFunction)q3_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
}

void findquakemaps(GHashTable* maphash, const char* startdir)
{
    traverse_dir(startdir, (FoundFileFunction)quake_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
}



#if 0

// gcc -g -Wall -O0 `glib-config --cflags` `glib-config --libs` -lz -o listq3maps q3maps.c zip/*.c debug.c

static void q3_print_maps(GHashTable* maphash)
{
    g_hash_table_foreach(maphash, (GHFunc) maphashforeachfunc, NULL);
    printf("\n");
}

int main (void)
{
    GHashTable* maphash = NULL;

    maphash = q3_init_maphash();

//    traverse_dir("/usr/local/games/quake3", (FoundFileFunction)q3_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
//    traverse_dir("/home/madmax/.q3a", (FoundFileFunction)q3_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
//    traverse_dir("/usr/share/games/quakeforge", (FoundFileFunction)quake_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
//    traverse_dir("/usr/share/games/quake2", (FoundFileFunction)quake_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
    traverse_dir("/usr/local/games/hl", (FoundFileFunction)quake_contains_file, (FoundDirFunction)quake_contains_dir, (gpointer)maphash);

    q3_print_maps(maphash);

    printf("%d\n",q3_lookup_map(maphash,"blub"));
    printf("%d\n",q3_lookup_map(maphash,"q3dm1"));

    q3_clear_maps(maphash);

    return 0;
}
#endif
