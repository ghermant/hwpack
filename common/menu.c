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
#include <malloc.h>
#include <errno.h>
#include <linux/list.h>

#include "menu.h"

struct menu_item {
	char *key;
	void *data;
	struct list_head list;
};

struct menu {
	struct menu_item *default_item;
	int timeout;
	char *title;
	int prompt;
	void (*item_data_print)(void *);
	struct list_head items;
};

static inline void *menu_items_iter(struct menu *m,
		void *(*callback)(struct menu *, struct menu_item *, void *),
		void *extra)
{
	struct list_head *pos, *n;
	struct menu_item *item;
	void *ret;

	list_for_each_safe(pos, n, &m->items) {
		item = list_entry(pos, struct menu_item, list);

		ret = callback(m, item, extra);

		if (ret)
			return ret;
	}

	return NULL;
}

static inline void *menu_item_print(struct menu *m,
				struct menu_item *item,
				void *extra)
{
	if (!m->item_data_print)
		printf("%s\n", item->key);
	else
		m->item_data_print(item->data);

	return NULL;
}

static inline void *menu_item_destroy(struct menu *m,
				struct menu_item *item,
				void *extra)
{
	if (item->key)
		free(item->key);

	free(item);

	return NULL;
}

static inline void menu_display(struct menu *m)
{
	if (m->title)
		printf("%s:\n", m->title);

	menu_items_iter(m, menu_item_print, NULL);
}

static inline void *menu_item_key_match(struct menu *m,
				struct menu_item *item,
				void *extra)
{
	char *item_key = extra;

	if (!item_key || !item->key) {
		if (item_key == item->key)
			return item;

		return NULL;
	}

	if (strcmp(item->key, item_key) == 0)
		return item;

	return NULL;
}

static inline struct menu_item *menu_item_by_key(struct menu *m,
							char *item_key)
{
	return menu_items_iter(m, menu_item_key_match, item_key);
}

static inline int menu_interrupted(struct menu *m)
{
	if (!m->timeout)
		return 0;

	if (abortboot(m->timeout/10))
		return 1;

	return 0;
}

static inline int menu_use_default(struct menu *m)
{
	return !m->prompt && !menu_interrupted(m);
}

static inline int menu_default_choice(struct menu *m, void **choice)
{
	if (m->default_item) {
		*choice = m->default_item->data;
		return 1;
	}

	return -ENOENT;
}

static inline int menu_interactive_choice(struct menu *m, void **choice)
{
	char cbuf[CONFIG_SYS_CBSIZE];
	struct menu_item *choice_item = NULL;
	int readret;

	while (!choice_item) {
		cbuf[0] = '\0';

		menu_display(m);

		readret = readline_into_buffer("Enter choice: ", cbuf);

		if (readret >= 0) {
			choice_item = menu_item_by_key(m, cbuf);

			if (!choice_item)
				printf("%s not found\n", cbuf);
		} else {
			printf("^C\n");
			return -EINTR;
		}
	}

	*choice = choice_item->data;

	return 1;
}

int menu_default_set(struct menu *m, char *item_key)
{
	struct menu_item *item;

	if (!m)
		return -EINVAL;

	item = menu_item_by_key(m, item_key);

	if (!item)
		return -ENOENT;

	m->default_item = item;

	return 1;
}

int menu_get_choice(struct menu *m, void **choice)
{
	if (!m || !choice)
		return -EINVAL;

	if (menu_use_default(m))
		return menu_default_choice(m, choice);

	return menu_interactive_choice(m, choice);
}

/*
 * note that this replaces the data of an item if it already exists, but
 * doesn't change the order of the item.
 */
int menu_item_add(struct menu *m, char *item_key, void *item_data)
{
	struct menu_item *item;

	if (!m)
		return -EINVAL;

	item = menu_item_by_key(m, item_key);

	if (item) {
		item->data = item_data;
		return 1;
	}

	item = malloc(sizeof *item);
	if (!item)
		return -ENOMEM;

	item->key = strdup(item_key);

	if (!item->key) {
		free(item);
		return -ENOMEM;
	}

	item->data = item_data;

	list_add_tail(&item->list, &m->items);

	return 1;
}

struct menu *menu_create(char *title, int timeout, int prompt,
				void (*item_data_print)(void *))
{
	struct menu *m;

	m = malloc(sizeof *m);

	if (!m)
		return NULL;

	m->default_item = NULL;
	m->prompt = prompt;
	m->timeout = timeout;
	m->item_data_print = item_data_print;

	if (title) {
		m->title = strdup(title);
		if (!m->title) {
			free(m);
			return NULL;
		}
	} else
		m->title = NULL;


	INIT_LIST_HEAD(&m->items);

	return m;
}

int menu_destroy(struct menu *m)
{
	if (!m)
		return -EINVAL;

	menu_items_iter(m, menu_item_destroy, NULL);

	if (m->title)
		free(m->title);

	free(m);

	return 1;
}
