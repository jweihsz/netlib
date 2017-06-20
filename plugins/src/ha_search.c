#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ha_debug.h"

#include "ha_search.h"

#define ts_conf_s_priv(_conf) ((u8 *)(_conf+1) + _conf->len)

struct ts_ops_s
{
	const char *name;
	uint (*priv_size)(uint);
	bool (*priv_init)(const struct ts_conf_s *);
	uint (*search)(const u8 *,uint,const struct ts_conf_s *);
};

struct ts_conf_s
{
	struct ts_ops_s *ops;
	uint len;
	bool ign;
	u8 pattern[0];
};


/*HORSPOOL*/
/*串的模式匹配算法*/
struct horspool_priv
{
	int shift[ASIZE];
};

static uint horspool_priv_size(uint patlen)
{
	return sizeof(struct horspool_priv);
}
static bool horspool_priv_init(const struct ts_conf_s *conf)
{
	struct horspool_priv *priv = (struct horspool_priv *)ts_conf_s_priv(conf);
	uint i;
	for (i = 0; i < sizeof(priv->shift)/sizeof(priv->shift[0]);i++)
		priv->shift[i] = conf->len;

	for (i = 0 ; i < conf->len  - 1; i++)  {
			priv->shift[conf->pattern[i]] = conf->len - i - 1; 
			if (conf->ign) /*这里是忽略大小写*/
				priv->shift[tolower(conf->pattern[i])] = conf->len - i - 1; 
	}
	return true;	
}

static uint horspool_search(const u8 *text,uint textlen,const struct ts_conf_s *conf)
{
	struct horspool_priv *priv = (struct horspool_priv *)ts_conf_s_priv(conf);
	uint pos,cnt;
	for (pos = conf->len,cnt = 0; pos <= textlen ; cnt = 0) {
		while (cnt < conf->len) {
			u8 p = conf->pattern[conf->len - 1 - cnt];
			u8 c = text[pos - 1 - cnt];
			if (conf->ign)
				c = tolower(c);
			if (c == p){
				cnt++;
			}else {
				pos += priv->shift[text[pos - 1]];
				break;
			}	
		}

		if (cnt  == conf->len)
			return pos - conf->len;
	}
	return UINT32_MAX;
}

/*SUNDAY*/
struct sunday_priv
{
	int shift[ASIZE];
};

static uint sunday_priv_size(uint patlen)
{
	return sizeof(struct sunday_priv);
}

static bool sunday_priv_init(const struct ts_conf_s *conf)
{
	struct sunday_priv *priv = (struct sunday_priv *)ts_conf_s_priv(conf);
	uint i;
	for (i = 0 ; i < ASIZE ; i++)
		priv->shift[i] = conf->len;
	
	for (i = 0 ; i < conf->len ; i++)  {
		priv->shift[conf->pattern[i]] = conf->len - i; 
		if (conf->ign)
			priv->shift[tolower(conf->pattern[i])] = conf->len - i;
	}
	return true;
}

static uint sunday_search(const u8 *text,uint textlen,const struct ts_conf_s *tc)
{
	struct sunday_priv *priv = (struct sunday_priv *)ts_conf_s_priv(tc);
	
	uint pos,cnt;
	for (pos = tc->len ,cnt = 0; pos < textlen ;cnt = 0) {
		while (cnt < tc->len) {
			u8 c = text[pos - 1 - cnt];
			u8 p = tc->pattern[tc->len - 1 - cnt];

			if (tc->ign)
				p = tolower(p);
	
			if (c != p) {
				pos += priv->shift[text[pos]];
				break;		
			} else {
				cnt++;
			}
		}
		if (cnt == tc->len)
			return pos - tc->len;
	}
	return UINT32_MAX;
}


struct kmp_priv
{
	int shift[0];
};

static uint kmp_priv_size(uint patlen)
{
	return patlen * sizeof(int);
}

static bool kmp_priv_init(const struct ts_conf_s *conf)
{
	struct kmp_priv *priv = (struct kmp_priv *)ts_conf_s_priv(conf);
	uint i,prefix;
	priv->shift[0] = 0;
	for (i = 1,prefix = 0; i < conf->len ; i++) {
		while (prefix > 0 && conf->pattern[prefix] != conf->pattern[i])
			prefix = priv->shift[prefix - 1];
		if (conf->pattern[prefix] == conf->pattern[i])
			prefix++;
		priv->shift[i] = prefix;
	}
	return true;
}

static uint kmp_search(const u8 *text,uint textlen,const struct ts_conf_s *conf)
{
	struct kmp_priv *priv = (struct kmp_priv *)ts_conf_s_priv(conf);
	uint pass,pos;
	for (pos = 0 ,pass = 0; pos < textlen ; pos++) {
		while (pass > 0 && conf->pattern[pass] != (conf->ign ? tolower(text[pos]) : text[pos]))
			pass = priv->shift[pass - 1];
		if (conf->pattern[pass] == (conf->ign ? tolower(text[pos]) :text[pos]))
			pass++;
		if (pass == conf->len)
			return pos - conf->len + 1;
	}
	return UINT32_MAX;
}



/*RAW SUFFIX*/
static uint raw_priv_size(uint unuse)
{
	return 0;
} 
static bool raw_priv_init(const struct ts_conf_s *tc)
{
	return true;
}
static uint raw_suffix_search(const u8 *text,uint textlen,const struct ts_conf_s *tc)
{
	unsigned pos;
	for (pos = tc->len; pos < textlen ; pos++) {
		unsigned cnt;
		for (cnt = 0 ; cnt < tc->len ; cnt++) {
			u8 p = tc->pattern[tc->len - 1 - cnt];
			u8 c = text[pos - 1 - cnt];
			if (tc->ign)
				c = tolower(c);
			
			if ( p != c)
				break;
		}
		if (cnt == tc->len)
			return pos - tc->len;
	}
	return UINT32_MAX;	
}

static struct ts_ops_s ts_ops [] = 
{
	{
		.name = "kmp",
		.priv_size = kmp_priv_size,
		.priv_init = kmp_priv_init,
		.search = kmp_search,
	},
 
	{
		.name = "sunday",
		.priv_size = sunday_priv_size,
		.priv_init = sunday_priv_init,
		.search = sunday_search,	
	},

	{
		.name = "horspool",
		.priv_size = horspool_priv_size,
		.priv_init = horspool_priv_init,
		.search = horspool_search,
	},
	
	{
		.name = "raw",
		.priv_size = raw_priv_size,
		.priv_init = raw_priv_init,
		.search = raw_suffix_search,
	},

	{
		.name = NULL,
	},
};


/*寻找一种匹配算法 */
struct ts_conf_s *ts_conf_new_s(const const u8 *pat,uint patlen,bool ign,
							const char *name)
{
	struct ts_conf_s *res = NULL;

	struct ts_ops_s *ops ;
	
	for (ops = ts_ops ; ops->name ;ops++)
		if (!strcmp(name,ops->name)) 
			break;

	if (!ops->name)
		return res;
	
	uint ext = 	ops->priv_size(patlen);
	uint size = sizeof(*res) + patlen * sizeof(u8) + ext;
	res = MALLOC(size);
	if (!res)
		return res;
	
	res->len = patlen;
	res->ign = ign; 
	res->ops = ops;
	if (ign)
		while (patlen--) 
			res->pattern[patlen] = tolower(pat[patlen]);
	else
		memcpy(res->pattern,pat,patlen);

	ops->priv_init(res);	
	return res;
}
void ts_conf_destory_s(struct ts_conf_s *tc)
{
	FREE(tc);
}
uint ts_search_s(const struct ts_conf_s *tc,const u8 *text,uint textlen)
{
	return tc->ops->search(text,textlen,tc);
}

#if 1

/*这是采用原始模式进行匹配strstr()*/
uint raw_search(const u8 *pat,unsigned int patlen,const u8 *text,unsigned int textlen,bool ign)
{
	 uint pos;
	 for (pos = patlen ; pos <= textlen ; pos++) {
		 uint cur;
		 for (cur = 0 ; cur < patlen ; cur++) {
			int t,p;
			t = text[pos - 1 - cur];
			p = pat[patlen - cur - 1];
			if (ign) {
				t = tolower(t);
				p = tolower(p);
			}
			if (t != p)
				break;
		}
		if (cur == patlen)
			return pos - patlen;
	 }
	 return UINT32_MAX;
}

#endif


