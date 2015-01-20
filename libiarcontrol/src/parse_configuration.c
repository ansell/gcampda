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
#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include "my_stdio.h"
#include "parse_tools.h"
#include "locks.h"
#ifdef CONFIG_ENVIROMENT_SUPPORT
#  include <wordexp.h>
#endif

/**
 *  struct configuration tools:
 **/

struct configuration *make_conf(void)
{
	struct configuration *fc;

	fc = (struct configuration *) malloc(sizeof(struct configuration));
	if (fc == NULL) {
		SYSLOG(LOG_ERR,
		       "struct configuration *make_conf: malloc fail.");
		/* FIXME: */
		return NULL;
	}
	fc->line = NULL;
	fc->flag = FCF_INITIAL;
	fc->type = FCT_EOF;
	fc->next = NULL;
	fc->pte_data = NULL;
	fc->expand = NULL;

	return fc;	
}


static void __free_conf(struct configuration *conf)
{
	if (conf->line != NULL)
		free(conf->line);
#ifdef CONFIG_ENVIROMENT_SUPPORT
	if (conf->flag & FCF_ENVIRON)
		putenv (conf->tag);
#endif
#ifdef CONFIG_BRACKET_SUPPORT
	if (conf->flag & FCF_EXPAND)
		free (conf->expand);
#endif
	free (conf);
}


int free_conf(struct configuration *conf)
{
	struct configuration *cf = conf;
	struct configuration *cfp;

	while(cf != NULL){
		cfp = cf;
		cf = cf->next;
		__free_conf(cfp);
	}

	return 0;
}


struct configuration *find_conf_tag(struct configuration *conf, char *tag)
{
	struct configuration *cfp = conf;

	while(cfp != NULL) {
		if (cfp->type == FCT_TAG && !(cfp->flag & FCF_DELETED)) {
			if (strcmp(cfp->tag, tag) == 0 ) {
				return cfp;
			}
		}
		cfp = cfp->next;
	}

	return (struct configuration *) NULL;
}


int find_conf_tag_int(struct configuration *conf, char *tag)
{
	int val = -1;
	struct configuration *cfp = NULL;
	
	if ((cfp = find_conf_tag(conf, tag)) != NULL) {
		val = atoi(cfp->data);
	}

	return val;
}


static int write_conf_line(char **line, char *tag, char *data, char *comment)
{
	int size = 80;
	int len;

	if (*line != NULL) free(*line);
	
	if ((*line = (char *) malloc(size)) == NULL)  return -1;
	
	if ( comment != NULL ) {
		len = snprintf(*line, size,"%s = %s # %s", tag, data, comment);
	} else {
		len = snprintf(*line, size,"%s = %s", tag, data);
	}
	if(len >= size) { /* Try again. */
		size = len + 1;
		*line = (char *) realloc (*line, size);
		if (*line != NULL) {
			if ( comment != NULL ) {
				len = snprintf(*line, size,"%s = %s # %s", 
					       tag, data, comment);
			} else {
				len = snprintf(*line, size,"%s = %s", 
					       tag, data);
			}
		}
		else {
			len = -1;
		}
		
	}

	return len;
}


int mod_conf_data(struct configuration *cf, char *data)
{
	int ret = -1;
	char *line=NULL;
	
	if (cf != NULL && cf->type == FCT_TAG && !(cf->flag & FCF_DELETED)) {

		cf->len = write_conf_line(&line, cf->tag, data, cf->comment);
		if (cf->len > 0) {
			free(cf->line);
			cf->line = line;
			cf->sline = cf->line;
			if (parse_line(cf->line, &cf->tag, 
				       &cf->data, &cf->comment) == 0){
				ret = 0;
			}
		}
	}

	return ret;
}


int change_conf_tag(struct configuration *conf, char *tag, char *data)
{
	int ret = -1;
	struct configuration *t = find_conf_tag(conf, tag);
	
	if (t != NULL) {
		ret = mod_conf_data(t, data);
	}

	return ret;
} 


/* return pointer to new element */
struct configuration *add_conf_tag(struct configuration *conf, 
			  char *tag, char *data, char *comment)
{
	struct configuration *p = conf;

	while(p->next != NULL) p = p->next;
	if ( p->type != FCT_EOF )
		return NULL;

	p->next = make_conf();
	p->type = FCT_TAG;
	p->len = write_conf_line(&p->line, tag, data, comment);
	p->sline = p->line;
	parse_line(p->line, &p->tag, &p->data, &p->comment);

	return p;
}

/* return pointer to element */
struct configuration *add_mod_conf_tag(struct configuration *conf, 
			  char *tag, char *data, char *comment)
{
	struct configuration *p = find_conf_tag(conf, tag);

	if( p == NULL) {
		p = add_conf_tag(conf, tag, data, comment);
	}else {
		mod_conf_data(p, data);
	}
	
	return p;
}


/* Only mark which FCF_DELETED, don't delete this */
int del_conf_tag(struct configuration *conf, char *tag)
{
	int ret = -1;
	struct configuration *t = find_conf_tag(conf, tag);
	
	if (t != NULL) {
		t->flag |= FCF_DELETED;
		ret = 0;
	}

	return ret;
}


int purge_conf_tag(struct configuration *conf)
{
	struct configuration *cf = conf;
	struct configuration *cfp = NULL;
	struct configuration *cfd;

	while (conf->flag & FCF_DELETED) { /* los primeros */
		cfd = conf;
		conf = conf->next;
		__free_conf(cfd);
	}
	if(conf != NULL) {
		cfp = conf;
		cf = conf->next;
		while(cf != NULL){ /* el el medio */
			if (cf->flag & FCF_DELETED) {
				cfd = cf;
				cfp->next = cf->next;
				cf = cf->next;
				__free_conf(cfd);
			} else {
				cfp = cf;
				cf = cf->next;
			}
		}
	}

	return 0;
}


/* Adds definitions from the environment, and expand it. */
#ifdef CONFIG_ENVIROMENT_SUPPORT
int put_env_conf(struct configuration *conf)
{
	struct configuration *cfp = conf;
	wordexp_t result;
	int i, ret;
	int len;

	while(cfp != NULL) {
		if (cfp->type == FCT_TAG && cfp->expand == NULL  &&
		    !(cfp->flag & (FCF_DELETED
#ifdef CONFIG_BRACKET_SUPPORT
				   | FCF_ARRAY | FCF_ARR_PARSED
#endif
			    ))) {
			if(cfp->flag & FCF_ENVIRON) {
				cfp->flag &= ~FCF_ENVIRON;
				putenv (cfp->tag);
			}
			if(cfp->flag & FCF_EXPAND) {
				cfp->flag &= ~FCF_EXPAND;
				free(cfp->expand);
			}
			len = strlen(cfp->tag) + 1;
			if( (ret = wordexp (cfp->data, &result, 0)) == 0) {
				for(i=0;i<result.we_wordc;i++)
					len += strlen(result.we_wordv[i])+1;
				/* len tiene 1 mas por el \0 del
                                   sprintf que sigue */
				cfp->expand = (char *) 
					malloc(sizeof(char) * len);
				cfp->flag |= FCF_EXPAND;

				len = sprintf(cfp->expand,"%s=", cfp->tag);
				cfp->pte_data = cfp->expand+len;
				len += sprintf(cfp->expand+len, "%s", 
					       result.we_wordv[0]);
				for(i=1;i<result.we_wordc;i++){
					len += sprintf(cfp->expand+len, " %s", 
						       result.we_wordv[i]);
				}
				wordfree (&result);
				if (putenv(cfp->expand) == 0) {
					cfp->flag |= FCF_ENVIRON;
				}
			}else if (ret == WRDE_NOSPACE ) {
				wordfree (&result);
			}else {
				SYSLOG(LOG_ERR,"Error expanding words: %s",
				       cfp->data);

			}
		}
		cfp = cfp->next;
	}

	return 0;
}
#endif

#ifdef CONFIG_ENVIROMENT_SUPPORT
int unset_env_conf(struct configuration *conf)
{
	struct configuration *cfp = conf;

	while(cfp != NULL) {
		if (cfp->flag & FCF_ENVIRON) {
			cfp->flag &= ~FCF_ENVIRON;
			putenv (cfp->tag);
		}
		cfp = cfp->next;
	}

	return 0;


}
#endif

/**
 * struct configuration tools END.
 **/


/**
 * struct configuration: stream file tools:
 **/

#ifdef CONFIG_BRACKET_SUPPORT
/* DELETE_SPACES no anda aun: elimina los espacios de todo, no solo {} */
//#define DELETE_SPACES
static int delete_c_comments_and_spaces(char *line, int *length)
{
	int i;
	int offset = 0;
	int start_comment = 0;
#ifdef DELETE_SPACES
	int start_sh_comment = 0;
	int start_com = 0;
	int bra_count = 0;
#endif
	char *t;

	for(i=0, t = line; i< *length;i++,t++) {
		if( start_comment == 1) {
			offset++;
		}
#ifdef DELETE_SPACES
		if (!start_sh_comment && !start_comment) {
			if (*t == '{') {
				bra_count++;
				t++;
				i++;
			}
			if (*t == '}') {
				bra_count--;
				t++;
				i++;
			}
			if (bra_count > 0) {
				while (WHITESPACE(*t) && i< *length &&
				       !start_com) {
					offset++;
					t++;
					i++;
				} 
		
				if(*t == '"') {
					offset++;
					t++;
					i++;
					start_com ^= 1;
				}
			}else if (bra_count < 0) {
				printf("%s\n", line);
			}
			      
		}
		if (*t == '#' && ! start_comment)
			start_sh_comment = 1;
#endif
		if (*t == '/' && *(t+1) == '*' ) {
			start_comment = 1;
			offset += 2;
			i++;
			t++;
		}else if (*t == '*' && *(t+1) == '/' ) {
			start_comment = 0;
			offset += 1;
			i++;
			t++;
		}else if (start_comment == 0 && offset != 0)
			*(t-offset) = *t;
	}


	if ( offset ) {
		*length -= offset;
		*(line+*length) = 0;
	}

	return start_comment;
}
#endif


#ifdef CONFIG_BRACKET_SUPPORT
static int match_bracket(char *line, int *length)
{
	int bra_count = 0;
	int start_comment = 0;
	int i;
	char *t;
	
	for( t = line,i=0; i < *length; t++, i++){
		if (*t == '{' && !start_comment) bra_count++;
		else if (*t == '}' && !start_comment) bra_count--;
		else if (*t == '#') {
			start_comment = 1;
			if (bra_count > 0) {
				*length = i + 1;
				*t = 0;
				break;
			}
		}
		else if (*t == '/' && *(t+1) == '*' ) 
			start_comment = 2;
		else if (*t == '*' && *(t+1) == '/' )
			start_comment = 0;

	}
	if (start_comment == 2) return 1;
		
	//printf("match_bracket [%s]: %d\n", line,bra_count);
	return bra_count ;

}
#endif


static int __lgetline(char **line, size_t *size, FILE *stream)
{
	int len;
	int length = 0;
	int done = 0;
	size_t n = LINE_BASE_LEN_CONF;
	char *lineptr = (char *) malloc(n);

	while (!done) {
		done = 1;
		len = getdelim (&lineptr, &n, '\n', stream);
		if(len > 0) {
			if (*line == NULL) 
				*line =  (char *) malloc(len + 1);
			else 
				*line =  (char *) 
					realloc(*line, length + len + 1);
			*size = length + len + 1;
			strncpy(*line+length, lineptr, len);
			length += len;
			*(*line+length-1) = 0;
			if (*(*line+length-2) == '\\') {
				done = 0;
				length -= 2;
			} 
#ifdef CONFIG_BRACKET_SUPPORT
			else if (match_bracket(*line, &length) > 0) {
				done = 0;
				length--;
			}
#endif
		}
		
	}
#ifdef CONFIG_BRACKET_SUPPORT
	/* Saco los comentarios tipo c */
	if (len > 0) {
		if( delete_c_comments_and_spaces(*line, &length) != 0) {
			SYSLOG(LOG_ERR,
			       "read_conf_file: comments not match.");
		}
	}else
#endif
		if (len < 0) length = -1;

	if (lineptr != NULL) free(lineptr);

	return length;
}


int read_conf_file(struct configuration **conf, char *filename)
{
	struct configuration *fc;
	FILE *fp;
	int count = 0;

	fp = fopen(filename, "r");
	if( fp == NULL ){
                SYSLOG(LOG_ERR,
                       "read_conf_file: Error opening file: %s",
		       filename);
                return -1;
        }

	if (*conf == NULL) {
		fc = *conf = make_conf();
	} else {
		for(fc = *conf; fc->next != NULL; fc = fc->next);
	}

	while((fc->len = __lgetline(&fc->line, &fc->size, fp)) > 0) {
		count++;
		fc->line_num = count;
		fc->sline = stripwhite(fc->line);
		if (parse_line(fc->sline, &fc->tag, 
			       &fc->data, &fc->comment) == 0){
			fc->type = FCT_TAG;
#ifdef CONFIG_BRACKET_SUPPORT
			if ( *(fc->data) == '{' )
				fc->flag |= FCF_ARRAY;
#endif
		} else if(fc->comment != NULL) {
			fc->type = FCT_COMMENT;
		} else if (*fc->sline == 0) {
			fc->type = FCT_NULL;
		} else {
			fc->type = FCT_UNKNOW;
			SYSLOG(LOG_ERR,"%s: ignoring line %d \"%s\"",
			      filename, fc->line_num, fc->line);
		}
		fc->next = make_conf();
		fc = fc->next;
	}
	fclose(fp);

	return 0;
}


int write_conf_file_flag(struct configuration *conf, char *filename, int flag)
{
	struct configuration *cf = conf;
	FILE *fp = NULL;
	char fmt[40];
	int c, i, t;
	int lock;

	if ( (lock = if_lock_file(filename)) ) {
		SYSLOG(LOG_ERR,"File %s locked by pid: %d.", filename, lock);
		return -1;
	}

	//fp = stdout;
	fp = fopen(filename, "w");
	if( fp == NULL ){
                SYSLOG(LOG_ERR,
                       "write_conf_file: Error opening file: %s",
		       filename);
                return -1;
        }

	while (cf != NULL) {
		if ( (cf->flag & flag) || ( 
		     !(cf->flag & (FCF_DELETED
#ifdef CONFIG_BRACKET_SUPPORT
				   |FCF_ARR_PARSED
#endif
			     )) && (flag == 0))) {
			if (cf->type == FCT_TAG) {
				if ( cf->comment != NULL ) {
					c = sprintf(fmt,"%%s = %%s");
					i = 40-strlen(cf->tag)-
						strlen(cf->data)-3;
					t = i >> 3;
					if ( i & 0x07 ) t++;
					for(i=0; i < t; i++)
						c += sprintf(fmt+c,"\t");
					sprintf(fmt+c,"# %%s\n");
					fprintf(fp, fmt, cf->tag, cf->data, 
						cf->comment);
				} else {
					fprintf(fp,"%s = %s\n", 
						cf->tag, cf->data);
				}
			} else if (cf->type == FCT_COMMENT) {
				fprintf(fp,"#%s\n", cf->comment);
			} else if (cf->type == FCT_NULL) {
				fprintf(fp,"\n");
			} else if (!(cf->flag & FCF_DELETED) && 
				   cf->type != FCT_EOF) {
				fprintf(fp,"# [ignored: %s]\n", cf->sline);
			}
		}
		cf = cf->next;
	}
	fclose(fp);
	unlock_file(filename);

	return 0;
}


int write_conf_file(struct configuration *conf, char *filename)
{
	return write_conf_file_flag(conf, filename, 0);
}



/**
 * struct configuration: stream file tools: END.
 **/
