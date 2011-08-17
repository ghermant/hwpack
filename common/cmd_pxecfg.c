/*
 * Copyright 2010-2011 Calxeda, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <errno.h>
#include <linux/list.h>

#include "menu.h"

#define MAX_TFTP_PATH_LEN 127

static char *from_env(char *envvar)
{
	char *ret;

	ret = getenv(envvar);

	if (!ret)
		printf("missing environment variable: %s\n", envvar);

	return ret;
}

/*
 * Returns the ethaddr environment variable formated with -'s instead of :'s
 */
static void format_mac_pxecfg(char **outbuf)
{
	char *p, *ethaddr;

	*outbuf = NULL;

	ethaddr = from_env("ethaddr");

	if (!ethaddr)
		ethaddr = from_env("usbethaddr");

	if (!ethaddr)
		return;

	/* "01-" MAC type prefix, dash-separated MAC address, zero */
	*outbuf = malloc(3 + strlen(ethaddr) + 1);

	if (*outbuf == NULL)
		return;

	/* MAC type 1; ideally should be read from the DHCP/BOOTP reply packet,
	 * but in practice always 1 for Ethernet */
	sprintf(*outbuf, "01-%s", ethaddr);

	for (p = *outbuf + 3; *p; p++) {
		if (*p == ':')
			*p = '-';
	}
}

/*
 * Returns the directory the file specified in the bootfile env variable is
 * in.
 */
static char *get_bootfile_path(void)
{
	char *bootfile, *bootfile_path, *last_slash;
	size_t path_len;

	bootfile = from_env("bootfile");

	if (!bootfile)
		return NULL;

	last_slash = strrchr(bootfile, '/');

	if (last_slash == NULL)
		return NULL;

	path_len = (last_slash - bootfile) + 1;

	bootfile_path = malloc(path_len + 1);

	if (bootfile_path == NULL) {
		printf("out of memory\n");
		return NULL;
	}

	strncpy(bootfile_path, bootfile, path_len);

	bootfile_path[path_len] = '\0';

	return bootfile_path;
}

/*
 * As in pxelinux, paths to files in pxecfg files are relative to the location
 * of bootfile. get_relfile takes a path from a pxecfg file and joins it with
 * the bootfile path to get the full path to the target file.
 */
static int get_relfile(char *file_path, void *file_addr)
{
	char *bootfile_path;
	size_t path_len;
	char relfile[MAX_TFTP_PATH_LEN+1];
	char addr_buf[10];
	char *tftp_argv[] = {"tftp", NULL, NULL, NULL};

	bootfile_path = get_bootfile_path();

	path_len = strlen(file_path);

	if (bootfile_path)
		path_len += strlen(bootfile_path);

	if (path_len > MAX_TFTP_PATH_LEN) {
		printf("Base path too long (%s%s)\n",
					bootfile_path ? bootfile_path : "",
					file_path);

		if (bootfile_path)
			free(bootfile_path);

		return -ENAMETOOLONG;
	}

	sprintf(relfile, "%s%s",
			bootfile_path ? bootfile_path : "",
			file_path);

	if (bootfile_path)
		free(bootfile_path);

	printf("Retreiving file: %s\n", relfile);

	sprintf(addr_buf, "%p", file_addr);

	tftp_argv[1] = addr_buf;
	tftp_argv[2] = relfile;

	if (do_tftpb(NULL, 0, 3, tftp_argv)) {
		printf("File not found\n");
		return -ENOENT;
	}

	return 1;
}

static int get_pxecfg_file(char *file_path, void *file_addr)
{
	unsigned long config_file_size;
	int err;

	err = get_relfile(file_path, file_addr);

	if (err < 0)
		return err;

	config_file_size = simple_strtoul(getenv("filesize"), NULL, 16);
	*(char *)(file_addr + config_file_size) = '\0';

	return 1;
}

static int get_pxelinux_path(char *file, void *pxecfg_addr)
{
	size_t base_len = strlen("pxelinux.cfg/");
	char path[MAX_TFTP_PATH_LEN+1];

	if (base_len + strlen(file) > MAX_TFTP_PATH_LEN) {
		printf("path too long, skipping\n");
		return -ENAMETOOLONG;
	}

	sprintf(path, "pxelinux.cfg/%s", file);

	return get_pxecfg_file(path, pxecfg_addr);
}

static int pxecfg_uuid_path(void *pxecfg_addr)
{
	char *uuid_str;

	uuid_str = from_env("pxeuuid");

	if (!uuid_str)
		return -ENOENT;

	return get_pxelinux_path(uuid_str, pxecfg_addr);
}

static int pxecfg_mac_path(void *pxecfg_addr)
{
	char *mac_str = NULL;

	format_mac_pxecfg(&mac_str);

	if (!mac_str)
		return -ENOENT;

	return get_pxelinux_path(mac_str, pxecfg_addr);
}

static int pxecfg_ipaddr_paths(void *pxecfg_addr)
{
	char ip_addr[9];
	int mask_pos, err;

	sprintf(ip_addr, "%08X", ntohl(NetOurIP));

	for (mask_pos = 7; mask_pos >= 0;  mask_pos--) {
		err = get_pxelinux_path(ip_addr, pxecfg_addr);

		if (err > 0)
			return err;

		ip_addr[mask_pos] = '\0';
	}

	return -ENOENT;
}

/*
 * Follows pxelinux's rules to download a pxecfg file from a tftp server.  The
 * file is stored at the location given by the pxecfg_addr environment
 * variable, which must be set.
 *
 * UUID comes from pxeuuid env variable, if defined
 * MAC addr comes from ethaddr env variable, if defined
 * IP
 *
 * see http://syslinux.zytor.com/wiki/index.php/PXELINUX
 */
static int get_pxecfg(int argc, char * const argv[])
{
	char *pxecfg_ram;
	void *pxecfg_addr;

	pxecfg_ram = from_env("pxecfg_ram");

	if (!pxecfg_ram)
		return 1;

	pxecfg_addr = (void *)simple_strtoul(pxecfg_ram, NULL, 16);

	if (pxecfg_uuid_path(pxecfg_addr) > 0
		|| pxecfg_mac_path(pxecfg_addr) > 0
		|| pxecfg_ipaddr_paths(pxecfg_addr) > 0
		|| get_pxelinux_path("default", pxecfg_addr) > 0) {

		printf("Config file found\n");

		return 1;
	}

	printf("Config file not found\n");

	return 0;
}

static int get_relfile_envaddr(char *file_path, char *envaddr_name)
{
	void *file_addr;
	char *envaddr;

	envaddr = from_env(envaddr_name);

	if (!envaddr)
		return -ENOENT;

	file_addr = (void *)simple_strtoul(envaddr, NULL, 16);

	return get_relfile(file_path, file_addr);
}

struct pxecfg_label {
	char *name;
	char *kernel;
	char *append;
	char *initrd;
	int attempted;
	int localboot;
	struct list_head list;
};

struct pxecfg {
	char *title;
	char *default_label;
	int timeout;
	int prompt;
	struct list_head labels;
};

struct pxecfg_label *label_create(void)
{
	struct pxecfg_label *label;

	label = malloc(sizeof *label);

	if (!label)
		return NULL;

	label->name = NULL;
	label->kernel = NULL;
	label->append = NULL;
	label->initrd = NULL;
	label->localboot = 0;
	label->attempted = 0;

	return label;
}

static void label_destroy(struct pxecfg_label *label)
{
	if (label->name)
		free(label->name);

	if (label->kernel)
		free(label->kernel);

	if (label->append)
		free(label->append);

	if (label->initrd)
		free(label->initrd);

	free(label);
}

static void label_print(void *data)
{
	struct pxecfg_label *label = data;

	printf("Label: %s\n", label->name);

	if (label->kernel)
		printf("\tkernel: %s\n", label->kernel);

	if (label->append)
		printf("\tappend: %s\n", label->append);

	if (label->initrd)
		printf("\tinitrd: %s\n", label->initrd);
}

static int label_localboot(struct pxecfg_label *label)
{
	char *localcmd, *dupcmd;
	int ret;

	localcmd = from_env("localcmd");

	if (!localcmd)
		return -ENOENT;

	/*
	 * dup the command to avoid any issues with the version of it existing
	 * in the environment changing during the execution of the command.
	 */
	dupcmd = strdup(localcmd);

	if (!dupcmd) {
		printf("out of memory\n");
		return -ENOMEM;
	}

	if (label->append)
		setenv("bootargs", label->append);

	printf("running: %s\n", dupcmd);

	ret = run_command2(dupcmd, 0);

	free(dupcmd);

	return ret;
}

/*
 * Do what it takes to boot a chosen label.
 *
 * Retreive the kernel and initrd, and prepare bootargs.
 */
static void label_boot(struct pxecfg_label *label)
{
	char *bootm_argv[] = { "bootm", NULL, NULL, NULL, NULL };

	label_print(label);

	label->attempted = 1;

	if (label->localboot) {
		label_localboot(label);
		return;
	}

	if (label->kernel == NULL) {
		printf("No kernel given, skipping label\n");
		return;
	}

	if (label->initrd) {
		if (get_relfile_envaddr(label->initrd, "initrd_ram") < 0) {
			printf("Skipping label\n");
			return;
		}

		bootm_argv[2] = getenv("initrd_ram");
	} else {
		bootm_argv[2] = "-";
	}

	if (get_relfile_envaddr(label->kernel, "kernel_ram") < 0) {
		printf("Skipping label\n");
		return;
	}

	if (label->append)
		setenv("bootargs", label->append);

	bootm_argv[1] = getenv("kernel_ram");

	/*
	 * fdt usage is optional - if unset, this stays NULL.
	 */
	bootm_argv[3] = getenv("fdtaddr");

	if (bootm_argv[3])
		do_bootm(NULL, 0, 4, bootm_argv);
	else
		do_bootm(NULL, 0, 3, bootm_argv);
}

enum token_type {
	T_EOL,
	T_STRING,
	T_EOF,
	T_MENU,
	T_TITLE,
	T_TIMEOUT,
	T_LABEL,
	T_KERNEL,
	T_APPEND,
	T_INITRD,
	T_LOCALBOOT,
	T_DEFAULT,
	T_PROMPT,
	T_INCLUDE,
	T_INVALID
};

struct token {
	char *val;
	enum token_type type;
};

enum lex_state {
	L_NORMAL = 0,
	L_KEYWORD,
	L_SLITERAL
};

static const struct token keywords[] = {
	{"menu", T_MENU},
	{"title", T_TITLE},
	{"timeout", T_TIMEOUT},
	{"default", T_DEFAULT},
	{"prompt", T_PROMPT},
	{"label", T_LABEL},
	{"kernel", T_KERNEL},
	{"localboot", T_LOCALBOOT},
	{"append", T_APPEND},
	{"initrd", T_INITRD},
	{"include", T_INCLUDE},
	{NULL, T_INVALID}
};

static char *get_string(char **p, struct token *t, char delim, int lower)
{
	char *b, *e;
	size_t len, i;

	b = e = *p;

	while (*e) {
		if ((delim == ' ' && isspace(*e)) || delim == *e)
			break;
		e++;
	}

	len = e - b;

	t->val = malloc(len + 1);
	if (!t->val) {
		printf("out of memory\n");
		return NULL;
	}

	for (i = 0; i < len; i++, b++) {
		if (lower)
			t->val[i] = tolower(*b);
		else
			t->val[i] = *b;
	}

	t->val[len] = '\0';

	*p = e;

	t->type = T_STRING;

	return t->val;
}

static void get_keyword(struct token *t)
{
	int i;

	for (i = 0; keywords[i].val; i++) {
		if (!strcmp(t->val, keywords[i].val)) {
			t->type = keywords[i].type;
			break;
		}
	}
}

static void get_token(char **p, struct token *t, enum lex_state state)
{
	char *c = *p;

	t->type = T_INVALID;

	/* eat non EOL whitespace */
	while (*c == ' ' || *c == '\t')
		c++;

	/* eat comments */
	if (*c == '#') {
		while (*c && *c != '\n')
			c++;
	}

	if (*c == '\n') {
		t->type = T_EOL;
		c++;
	} else if (*c == '\0') {
		t->type = T_EOF;
		c++;
	} else if (state == L_SLITERAL) {
		get_string(&c, t, '\n', 0);
	} else if (state == L_KEYWORD) {
		get_string(&c, t, ' ', 1);
		get_keyword(t);
	}

	*p = c;
}

static void eol_or_eof(char **c)
{
	while (**c && **c != '\n')
		(*c)++;
}

static int parse_sliteral(char **c, char **dst)
{
	struct token t;
	char *s = *c;

	get_token(c, &t, L_SLITERAL);

	if (t.type != T_STRING) {
		printf("Expected string literal: %.*s\n", (int)(*c - s), s);
		return -EINVAL;
	}

	*dst = t.val;

	return 1;
}

static int parse_integer(char **c, int *dst)
{
	struct token t;
	char *s = *c;

	get_token(c, &t, L_SLITERAL);

	if (t.type != T_STRING) {
		printf("Expected string: %.*s\n", (int)(*c - s), s);
		return -EINVAL;
	}

	*dst = (int)simple_strtoul(t.val, NULL, 10);

	free(t.val);

	return 1;
}

static int parse_pxecfg_top(char *p, struct pxecfg *cfg, int nest_level);

static int handle_include(char **c, char *base,
		struct pxecfg *cfg, int nest_level)
{
	char *include_path;
	int err;

	err = parse_sliteral(c, &include_path);

	if (err < 0) {
		printf("Expected include path\n");
		return err;
	}

	err = get_pxecfg_file(include_path, base);

	if (err < 0) {
		printf("Couldn't get %s\n", include_path);
		return err;
	}

	return parse_pxecfg_top(base, cfg, nest_level);
}

static int parse_menu(char **c, struct pxecfg *cfg, char *b, int nest_level)
{
	struct token t;
	char *s = *c;
	int err;

	get_token(c, &t, L_KEYWORD);

	switch (t.type) {
	case T_TITLE:
		err = parse_sliteral(c, &cfg->title);

		break;

	case T_INCLUDE:
		err = handle_include(c, b + strlen(b) + 1, cfg,
						nest_level + 1);
		break;

	default:
		printf("Ignoring malformed menu command: %.*s\n",
				(int)(*c - s), s);
	}

	if (err < 0)
		return err;

	eol_or_eof(c);

	return 1;
}

static int parse_label_menu(char **c, struct pxecfg *cfg,
				struct pxecfg_label *label)
{
	struct token t;
	char *s;

	s = *c;

	get_token(c, &t, L_KEYWORD);

	switch (t.type) {
	case T_DEFAULT:
		if (cfg->default_label)
			free(cfg->default_label);

		cfg->default_label = strdup(label->name);

		if (!cfg->default_label)
			return -ENOMEM;

		break;
	default:
		printf("Ignoring malformed menu command: %.*s\n",
				(int)(*c - s), s);
	}

	eol_or_eof(c);

	return 0;
}

static int parse_label(char **c, struct pxecfg *cfg)
{
	struct token t;
	char *s;
	struct pxecfg_label *label;
	int err;

	label = label_create();
	if (!label)
		return -ENOMEM;

	err = parse_sliteral(c, &label->name);
	if (err < 0) {
		printf("Expected label name\n");
		label_destroy(label);
		return -EINVAL;
	}

	list_add_tail(&label->list, &cfg->labels);

	while (1) {
		s = *c;
		get_token(c, &t, L_KEYWORD);

		err = 0;
		switch (t.type) {
		case T_MENU:
			err = parse_label_menu(c, cfg, label);
			break;

		case T_KERNEL:
			err = parse_sliteral(c, &label->kernel);
			break;

		case T_APPEND:
			err = parse_sliteral(c, &label->append);
			break;

		case T_INITRD:
			err = parse_sliteral(c, &label->initrd);
			break;

		case T_LOCALBOOT:
			err = parse_integer(c, &label->localboot);
			break;

		case T_EOL:
			break;

		/*
		 * A label ends when we either get to the end of a file, or
		 * get some input we otherwise don't have a handler defined
		 * for.
		 */
		default:
			/* put it back */
			*c = s;
			return 1;
		}

		if (err < 0)
			return err;
	}
}

#define MAX_NEST_LEVEL 16

static int parse_pxecfg_top(char *p, struct pxecfg *cfg, int nest_level)
{
	struct token t;
	char *s, *b, *label_name;
	int err;

	b = p;

	if (nest_level > MAX_NEST_LEVEL) {
		printf("Maximum nesting exceeded\n");
		return -EMLINK;
	}

	while (1) {
		s = p;

		get_token(&p, &t, L_KEYWORD);

		err = 0;
		switch (t.type) {
		case T_MENU:
			err = parse_menu(&p, cfg, b, nest_level);
			break;

		case T_TIMEOUT:
			err = parse_integer(&p, &cfg->timeout);
			break;

		case T_LABEL:
			err = parse_label(&p, cfg);
			break;

		case T_DEFAULT:
			err = parse_sliteral(&p, &label_name);

			if (label_name) {
				if (cfg->default_label)
					free(cfg->default_label);

				cfg->default_label = label_name;
			}

			break;

		case T_PROMPT:
			err = parse_integer(&p, &cfg->prompt);
			break;

		case T_EOL:
			break;

		case T_EOF:
			return 1;

		default:
			printf("Ignoring unknown command: %.*s\n",
							(int)(p - s), s);
			eol_or_eof(&p);
		}

		if (err < 0)
			return err;
	}
}

static void destroy_pxecfg(struct pxecfg *cfg)
{
	struct list_head *pos, *n;
	struct pxecfg_label *label;

	if (cfg->title)
		free(cfg->title);

	if (cfg->default_label)
		free(cfg->default_label);

	list_for_each_safe(pos, n, &cfg->labels) {
		label = list_entry(pos, struct pxecfg_label, list);

		label_destroy(label);
	}

	free(cfg);
}

static struct pxecfg *parse_pxecfg(char *menucfg)
{
	struct pxecfg *cfg;

	cfg = malloc(sizeof(*cfg));

	if (!cfg) {
		printf("Out of memory\n");
		return NULL;
	}

	cfg->timeout = 0;
	cfg->prompt = 0;
	cfg->default_label = NULL;
	cfg->title = NULL;

	INIT_LIST_HEAD(&cfg->labels);

	if (parse_pxecfg_top(menucfg, cfg, 1) < 0) {
		destroy_pxecfg(cfg);
		return NULL;
	}

	return cfg;
}

static struct menu *pxecfg_to_menu(struct pxecfg *cfg)
{
	struct pxecfg_label *label;
	struct list_head *pos;
	struct menu *m;
	int err;

	m = menu_create(cfg->title, cfg->timeout, cfg->prompt, label_print);

	if (!m)
		return NULL;

	list_for_each(pos, &cfg->labels) {
		label = list_entry(pos, struct pxecfg_label, list);

		if (menu_item_add(m, label->name, label) != 1) {
			menu_destroy(m);
			return NULL;
		}
	}

	if (cfg->default_label) {
		err = menu_default_set(m, cfg->default_label);
		if (err != 1) {
			if (err != -ENOENT) {
				menu_destroy(m);
				return NULL;
			}

			printf("Missing default: %s\n", cfg->default_label);
		}
	}

	return m;
}

static void boot_unattempted_labels(struct pxecfg *cfg)
{
	struct list_head *pos;
	struct pxecfg_label *label;

	list_for_each(pos, &cfg->labels) {
		label = list_entry(pos, struct pxecfg_label, list);

		if (!label->attempted)
			label_boot(label);
	}
}

static void handle_pxecfg(struct pxecfg *cfg)
{
	void *choice;
	struct menu *m;

	m = pxecfg_to_menu(cfg);
	if (!m)
		return;

	if (menu_get_choice(m, &choice) == 1)
		label_boot(choice);

	menu_destroy(m);

	boot_unattempted_labels(cfg);
}

static int boot_pxecfg(int argc, char * const argv[])
{
	unsigned long pxecfg_addr;
	struct pxecfg *cfg;
	char *pxecfg_ram;

	if (argc == 2) {
		pxecfg_ram = from_env("pxecfg_ram");
		if (!pxecfg_ram)
			return 1;

		pxecfg_addr = simple_strtoul(pxecfg_ram, NULL, 16);
	} else if (argc == 3) {
		pxecfg_addr = simple_strtoul(argv[2], NULL, 16);
	} else {
		printf("Invalid number of arguments\n");
		return 1;
	}

	cfg = parse_pxecfg((char *)(pxecfg_addr));

	if (cfg == NULL) {
		printf("Error parsing config file\n");
		return 1;
	}

	handle_pxecfg(cfg);

	destroy_pxecfg(cfg);

	return 0;
}

int do_pxecfg(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc < 2) {
		printf("pxecfg requires at least one argument\n");
		return EINVAL;
	}

	if (!strcmp(argv[1], "get"))
		return get_pxecfg(argc, argv);

	if (!strcmp(argv[1], "boot"))
		return boot_pxecfg(argc, argv);

	printf("Invalid pxecfg command: %s\n", argv[1]);

	return EINVAL;
}

U_BOOT_CMD(
	pxecfg, 2, 1, do_pxecfg,
	"commands to get and boot from pxecfg files",
	"get - try to retrieve a pxecfg file using tftp\npxecfg "
	"boot [pxecfg_addr] - boot from the pxecfg file at pxecfg_addr\n"
);
