/*
 * Pure presentation formatters: build player-facing strings into caller
 * buffers, but never print or mutate game state.
 */

#include "config.h"
#include "fmt.h"
#include "game.h"
#include "items.h"
#include "txtres.h"

static int fmt_buf_append_char(char *buf, int bufsize, int pos, char ch)
{
    if (pos < 0 || pos >= bufsize - 1) {
        return -1;
    }
    buf[pos] = ch;
    buf[pos + 1] = '\0';
    return pos + 1;
}

static int fmt_buf_append_str(char *buf, int bufsize, int pos, const char *s)
{
    if (s == 0) {
        return pos;
    }
    while (*s) {
        pos = fmt_buf_append_char(buf, bufsize, pos, *s);
        if (pos < 0) {
            return -1;
        }
        s++;
    }
    return pos;
}

static int fmt_buf_append_uint(char *buf, int bufsize, int pos, int value)
{
    char digits[12];
    int count;
    int i;

    if (value < 0) {
        return -1;
    }
    count = 0;
    if (value == 0) {
        digits[count++] = '0';
    } else {
        while (value > 0 && count < (int)sizeof(digits)) {
            digits[count++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }
    for (i = count - 1; i >= 0; --i) {
        pos = fmt_buf_append_char(buf, bufsize, pos, digits[i]);
        if (pos < 0) {
            return -1;
        }
    }
    return pos;
}

static int fmt_buf_append_stack(char *buf, int bufsize, int pos, int item_id, int count)
{
    const char *name;

    name = item_name(item_id);
    pos = fmt_buf_append_char(buf, bufsize, pos, ' ');
    if (pos < 0) {
        return -1;
    }
    pos = fmt_buf_append_str(buf, bufsize, pos, name);
    if (pos < 0) {
        return -1;
    }
    if (count > 1) {
        pos = fmt_buf_append_char(buf, bufsize, pos, ' ');
        if (pos < 0) {
            return -1;
        }
        pos = fmt_buf_append_char(buf, bufsize, pos, '[');
        if (pos < 0) {
            return -1;
        }
        pos = fmt_buf_append_uint(buf, bufsize, pos, count);
        if (pos < 0) {
            return -1;
        }
        pos = fmt_buf_append_char(buf, bufsize, pos, ']');
        if (pos < 0) {
            return -1;
        }
    }
    return pos;
}

static int fmt_buf_append_two_s_fmt(char *buf, int bufsize, int pos,
    const char *fmt, const char *name)
{
    if (fmt == 0) {
        return pos;
    }
    while (*fmt) {
        if (fmt[0] == '%' && fmt[1] == 's') {
            pos = fmt_buf_append_str(buf, bufsize, pos, name);
            if (pos < 0) {
                return -1;
            }
            fmt += 2;
        } else {
            pos = fmt_buf_append_char(buf, bufsize, pos, *fmt);
            if (pos < 0) {
                return -1;
            }
            fmt++;
        }
    }
    return pos;
}

int fmt_room_ground_items(const struct GameState *game, int room_id,
    char *buf, int bufsize)
{
    int s;
    int ground_count;
    int first_ground;
    int pos;

    if (buf == 0 || bufsize <= 0 || game == 0) {
        return -1;
    }
    if (room_id < 0 || room_id >= CFG_ROOM_MAX) {
        return -1;
    }
    buf[0] = '\0';
    ground_count = 0;
    first_ground = ITEM_NONE;
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        if (game->room_item[room_id][s] != ITEM_NONE) {
            if (ground_count == 0) {
                first_ground = game->room_item[room_id][s];
            }
            ground_count += 1;
        }
    }
    if (ground_count == 0) {
        return 0;
    }
    pos = 0;
    if (ground_count == 1) {
        pos = fmt_buf_append_two_s_fmt(buf, bufsize, pos,
            TXT_UI_GROUND_ITEM_FMT, item_name(first_ground));
        if (pos < 0) {
            return -1;
        }
        return pos;
    }
    pos = fmt_buf_append_str(buf, bufsize, pos, TXT_UI_GROUND_ITEMS_HEADER);
    if (pos < 0) {
        return -1;
    }
    for (s = 0; s < CFG_AREA_ITEM_SLOTS; ++s) {
        if (game->room_item[room_id][s] != ITEM_NONE) {
            pos = fmt_buf_append_two_s_fmt(buf, bufsize, pos,
                TXT_UI_GROUND_ITEM_LINE_FMT,
                item_name(game->room_item[room_id][s]));
            if (pos < 0) {
                return -1;
            }
        }
    }
    return pos;
}

int fmt_exploration_map(const struct GameState *game, char *buf, int bufsize)
{
    int min_x;
    int max_x;
    int min_y;
    int max_y;
    int i;
    int any;
    int px;
    int py;
    int pos;

    if (buf == 0 || bufsize <= 0 || game == 0) {
        return -1;
    }
    buf[0] = '\0';
    any = 0;
    min_x = 0;
    max_x = 0;
    min_y = 0;
    max_y = 0;
    for (i = 0; i < game->world.room_count; ++i) {
        if (!game->room_explored[i]) {
            continue;
        }
        if (!game->world.map_ready[i]) {
            continue;
        }
        if (!any) {
            min_x = game->world.map_x[i];
            max_x = game->world.map_x[i];
            min_y = game->world.map_y[i];
            max_y = game->world.map_y[i];
            any = 1;
        } else {
            if (game->world.map_x[i] < min_x) {
                min_x = game->world.map_x[i];
            }
            if (game->world.map_x[i] > max_x) {
                max_x = game->world.map_x[i];
            }
            if (game->world.map_y[i] < min_y) {
                min_y = game->world.map_y[i];
            }
            if (game->world.map_y[i] > max_y) {
                max_y = game->world.map_y[i];
            }
        }
    }
    if (!any) {
        pos = fmt_buf_append_str(buf, bufsize, 0, TXT_MAP_NONE_EXPLORED);
        if (pos < 0) {
            return -1;
        }
        return pos;
    }
    pos = fmt_buf_append_str(buf, bufsize, 0, TXT_MAP_HEADER);
    if (pos < 0) {
        return -1;
    }
    for (py = min_y; py <= max_y; ++py) {
        int first_cell;

        first_cell = 1;
        for (px = min_x; px <= max_x; ++px) {
            int rid;
            int k;
            char ch;

            rid = -1;
            for (k = 0; k < game->world.room_count; ++k) {
                if (!game->room_explored[k]) {
                    continue;
                }
                if (!game->world.map_ready[k]) {
                    continue;
                }
                if (game->world.map_x[k] != px || game->world.map_y[k] != py) {
                    continue;
                }
                if (rid < 0 || k < rid) {
                    rid = k;
                }
            }
            if (rid < 0) {
                ch = ' ';
            } else if (rid == game->player.room_id) {
                ch = '@';
            } else {
                ch = g_room_names[rid][0];
            }
            if (!first_cell) {
                pos = fmt_buf_append_char(buf, bufsize, pos, ' ');
                if (pos < 0) {
                    return -1;
                }
            }
            first_cell = 0;
            pos = fmt_buf_append_char(buf, bufsize, pos, ch);
            if (pos < 0) {
                return -1;
            }
        }
        pos = fmt_buf_append_char(buf, bufsize, pos, '\n');
        if (pos < 0) {
            return -1;
        }
    }
    pos = fmt_buf_append_str(buf, bufsize, pos, TXT_MAP_LEGEND);
    if (pos < 0) {
        return -1;
    }
    return pos;
}

int fmt_inv_bag_items(const struct GameState *game, char *buf, int bufsize)
{
    int i;
    int j;
    int seen[CFG_BAG_MAX];
    int seen_count;
    int item_id;
    int count;
    int first;
    int pos;

    if (buf == 0 || bufsize <= 0) {
        return -1;
    }
    buf[0] = '\0';
    if (game->bag_count <= 0) {
        return 0;
    }
    seen_count = 0;
    first = 1;
    pos = 0;
    for (i = 0; i < game->bag_count; ++i) {
        item_id = game->bag[i];
        for (j = 0; j < seen_count; ++j) {
            if (seen[j] == item_id) {
                break;
            }
        }
        if (j < seen_count) {
            continue;
        }
        seen[seen_count] = item_id;
        seen_count += 1;
        count = 0;
        for (j = 0; j < game->bag_count; ++j) {
            if (game->bag[j] == item_id) {
                count += 1;
            }
        }
        if (!first) {
            pos = fmt_buf_append_char(buf, bufsize, pos, ',');
            if (pos < 0) {
                return -1;
            }
        }
        first = 0;
        pos = fmt_buf_append_stack(buf, bufsize, pos, item_id, count);
        if (pos < 0) {
            return -1;
        }
    }
    return pos;
}
