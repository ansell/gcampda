/** 
 *   Camera Control for STAR I (originally).
 *
 *   Included in libiarcontrol v 0.1.5
 *
 *   (c) Copyright 2005 Federico Bareilles <fede@iar.unlp.edu.ar>,
 *   Instituto Argentino de Radio Astronomia (IAR).
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *     
 *   The author does NOT admit liability nor provide warranty for any
 *   of this software. This material is provided "AS-IS" in the hope
 *   that it may be useful for others.
 *
 **/

#ifndef _PARSE_TOOLS_H
#define _PARSE_TOOLS_H

/* Configurations: */
#define CONFIG_BRACKET_SUPPORT
#define CONFIG_ENVIROMENT_SUPPORT
/* Configurations: End */


#include "line_conf.h"

/* Implemented in parse_configuration.c: */
/* File Conf. Type definitions: */
#define FCT_UNKNOW  0
#define FCT_COMMENT 1
#define FCT_TAG     2
#define FCT_EOF     3
#define FCT_NULL    4

/* File Conf. Flag definitions: */
#define FCF_INITIAL 0x01 /* tendria que ser 0 y agregar un FCF_NORMAL 0x01 */
#define FCF_DELETED 0x02
#define FCF_EXPAND  0x04
#define FCF_ENVIRON 0x08
#ifdef CONFIG_BRACKET_SUPPORT
#  define FCF_ARRAY      0x10
#  define FCF_ARR_PARSED 0x20
#endif

#ifdef CONFIG_BRACKET_SUPPORT
/* Configuration element separator: */
#  define CONFIG_ELMT_SEP ','
/* Configuration vector separator: */
#  define CONFIG_VEC_SEP ','
#endif

struct configuration {
	char type;
	char flag;
	int len;
	int line_num;
	char *line;
	size_t size; /* size of buffer *line */
	char *sline;
	char *tag;
	char *data;
	char *comment;
	char *pte_data; /* pointer to expand data */ 
	char *expand; /* Value expanded in enviroment */
	struct configuration *next;
};

int read_conf_file(struct configuration **conf, char *filename);
struct configuration *make_conf(void);
int free_conf(struct configuration *conf);
int write_conf_file(struct configuration *conf, char *filename);
int write_conf_file_flag(struct configuration *conf, char *filename, int flag);
struct configuration *find_conf_tag(struct configuration *conf, char *tag);
/* find_conf_tag_int: ret -1 if fail */
int find_conf_tag_int(struct configuration *conf, char *tag);
int mod_conf_data(struct configuration *cf, char *data);
int change_conf_tag(struct configuration *conf, char *tag, char *data);
/* return pointer to new element */
struct configuration *add_conf_tag(struct configuration *conf, char *tag, 
				   char *data, char *comment);
/* return pointer to element */
struct configuration *add_mod_conf_tag(struct configuration *conf, 
				       char *tag, char *data, char *comment);

int del_conf_tag(struct configuration *conf, char *tag);
int purge_conf_tag(struct configuration *conf);

#ifdef CONFIG_ENVIROMENT_SUPPORT
int put_env_conf(struct configuration *conf);
int unset_env_conf(struct configuration *conf);
#endif
#ifdef CONFIG_BRACKET_SUPPORT
int expand_conf_braket_array(struct configuration *conf);
#endif


#endif /* _PARSE_TOOLS_H */
