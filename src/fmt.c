/* Pure presentation formatters (no terminal I/O). */

#include "config.h"
#include "fmt.h"
#include "game.h"
#include "items.h"

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
