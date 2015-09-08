#include "font.h"
#include "list.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define HASH_SIZE 4096
#define TINY_FONT 12

struct hash_rect {
	struct hash_rect *next;
	struct font_rect rect;
	struct list_head next_char;
	struct list_head time;
	int version;
	int c;
	int line;
	int font;
	int edge;
};

struct font_line {
	int start_line;
	int height;
	int space;
	struct list_head head;
};

struct font {
	int width;
	int height;
	int max_line;
	int version;
	struct list_head time;
	struct hash_rect *freelist;
	struct font_line *line;
	struct hash_rect *hash[HASH_SIZE];
};

struct font *font_new(int width, int height) {
	struct font *f;
	int max_line = height / TINY_FONT;
	int max_char = max_line*width / TINY_FONT;
	int ssize = max_char * sizeof(struct hash_rect);
	int lsize = max_line * sizeof(struct font_line);
	int size = sizeof(*f) + ssize + lsize;
	int i;

	f = malloc(size);
	f->width = width;
	f->height = height;
	f->max_line = 0;
	f->version = 0;
	INIT_LIST_HEAD(&f->time);
	f->freelist = (struct hash_rect *)(f + 1);
	f->line = (struct font_line *)((intptr_t)f->freelist + ssize);

	for (i = 0; i < max_char; i++) {
		struct hash_rect *hr = &f->freelist[i];
		hr->next = &f->freelist[i + 1];
	}
	f->freelist[max_char - 1].next = 0;
	memset(f->hash, 0, sizeof(f->hash));
	return f;
}

void font_free(struct font *f) {
	free(f);
}

static inline int hash(int c, int font, int edge) {
	return ((unsigned)(c ^ (font * 97))) % HASH_SIZE;
}

const struct font_rect *font_lookup(struct font *f, int c, int font, int edge) {
	int h = hash(c, font, edge);
	struct hash_rect *hr = f->hash[h];
	while (hr) {
		if (hr->c == c && hr->font == font && hr->edge == edge) {
			list_move_tail(&hr->time, &f->time);
			hr->version = f->version;
			return &hr->rect;
		}
		hr = hr->next;
	}
	return 0;
}

static struct font_line *new_line(struct font *f, int height) {
	int start = 0;
	int max;
	struct font_line *line;
	if (f->max_line > 0) {
		line = &f->line[f->max_line - 1];
		start = line->start_line + line->height;
	}
	if (start + height > f->height) {
		return 0;
	}
	max = f->height / TINY_FONT;
	if (f->max_line >= max) {
		return 0;
	}
	line = &f->line[f->max_line++];
	line->start_line = start;
	line->height = height;
	line->space = f->width;
	INIT_LIST_HEAD(&line->head);
	return line;
}

static struct font_line *find_line(struct font *f, int width, int height) {
	int i;
	for (i = 0; i < f->max_line; i++) {
		struct font_line *line = &f->line[i];
		if (height == line->height && width <= line->space) {
			return line;
		}
	}
	return new_line(f, height);
}

static struct hash_rect *new_node(struct font *f) {
	struct hash_rect *ret;
	if (!f->freelist) {
		return 0;
	}
	ret = f->freelist;
	f->freelist = ret->next;
	return ret;
}

static struct hash_rect *find_space(struct font *f, struct font_line *line, int width) {
	int start = 0;
	struct hash_rect *hr, *n;
	int max_space = 0;
	int space;
	list_for_each_entry(hr, struct hash_rect, &line->head, next_char) {
		space = hr->rect.x - start;
		if (space >= width) {
			n = new_node(f);
			if (!n) {
				return 0;
			}
			n->line = line - f->line;
			n->rect.x = start;
			n->rect.y = line->start_line;
			n->rect.w = width;
			n->rect.h = line->height;
			list_add_tail(&n->next_char, &hr->next_char);
			return n;
		}
		if (space > max_space) {
			max_space = space;
		}
		start = hr->rect.x + hr->rect.w;
	}
	space = f->width - start;
	if (space < width) {
		if (space > max_space) {
			line->space = space;
		} else {
			line->space = max_space;
		}
		return 0;
	}
	n = new_node(f);
	if (!n) {
		return 0;
	}
	n->line = line - f->line;
	n->rect.x = start;
	n->rect.y = line->start_line;
	n->rect.w = width;
	n->rect.h = line->height;
	list_add_tail(&n->next_char, &line->head);
	return n;
}

static void adjust_space(struct font *f, struct hash_rect *hr) {
	struct font_line *line = &f->line[hr->line];
	if (hr->next_char.next == &line->head) {
		hr->rect.w = f->width - hr->rect.x;
	} else {
		struct hash_rect *next = list_entry(hr->next_char.next, struct hash_rect, next_char);
		hr->rect.w = next->rect.x - hr->rect.x;
	}

	if (hr->next_char.prev == &line->head) {
		hr->rect.w += hr->rect.x;
		hr->rect.x = 0;
	} else {
		struct hash_rect *prev = list_entry(hr->next_char.prev, struct hash_rect, next_char);
		int x = prev->rect.x + prev->rect.w;
		hr->rect.w += hr->rect.x - x;
		hr->rect.x = x;
	}

	if (hr->rect.w > line->space) {
		line->space = hr->rect.w;
	}
}

static struct hash_rect *rem_char(struct font *f, int c, int font, int edge) {
	struct hash_rect *lst;
	int h = hash(c, font, edge);
	struct hash_rect *hr = f->hash[h];
	if (hr->c == c && hr->font == font && hr->edge == edge) {
		f->hash[h] = hr->next;
		list_del(&hr->time);
		adjust_space(f, hr);
		return hr;
	}
	lst = hr;
	hr = hr->next;
	while (hr) {
		if (hr->c == c && hr->font == font && hr->edge == edge) {
			lst->next = hr->next;
			list_del(&hr->time);
			adjust_space(f, hr);
			return hr;
		}
		lst = hr;
		hr = hr->next;
	}
	assert(0);
	return 0;
}

static struct hash_rect *rem_space(struct font *f, int width, int height) {
	struct hash_rect *hr, *tmp;
	list_for_each_entry_safe(hr, struct hash_rect, tmp, &f->time, time) {
		struct hash_rect *ret;
		if (hr->version == f->version) {
			continue;
		}
		if (hr->rect.h != height) {
			continue;
		}
		ret = rem_char(f, hr->c, hr->font, hr->edge);
		if (hr->rect.w >= width) {
			ret->rect.w = width;
			return ret;
		} else {
			list_del(&ret->next_char);
			ret->next = f->freelist;
			f->freelist = ret;
		}
	}
	return 0;
}

static struct font_rect *insert_char(struct font *f, int c, int font, struct hash_rect *hr, int edge) {
	int h;
	hr->c = c;
	hr->font = font;
	hr->edge = edge;
	hr->version = f->version;
	list_add_tail(&hr->time, &f->time);
	h = hash(c, font, edge);
	hr->next = f->hash[h];
	f->hash[h] = hr;
	return &hr->rect;
}

const struct font_rect *font_insert(struct font *f, int c, int font, int width, int height, int edge) {
	struct hash_rect *hr;

	if (width > f->width) {
		return 0;
	}
	if (font_lookup(f, c, font, edge)) {
		return 0;
	}
	for (;;) {
		struct font_line *line = find_line(f, width, height);
		if (!line) {
			break;
		}
		hr = find_space(f, line, width);
		if (hr) {
			return insert_char(f, c, font, hr, edge);
		}
	}
	hr = rem_space(f, width, height);
	if (hr) {
		return insert_char(f, c, font, hr, edge);
	}
	return 0;
}

void font_remove(struct font *f, int c, int font, int edge) {
	int h = hash(c, font, edge);
	struct hash_rect *hr = f->hash[h];
	while (hr) {
		if (hr->c == c && hr->font == font && hr->edge == edge) {
			list_move(&hr->time, &f->time);
			hr->version = f->version - 1;
			return;
		}
		hr = hr->next;
	}
}

void font_flush(struct font *f) {
	++f->version;
}
