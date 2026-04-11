#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include <stdio.h>
#include <stdbool.h>

typedef struct {
    char    *data;
    size_t  len;
} StringView;

#define SV_ARG(s)       (int)s.len, s.data
#define SV_FMT          ".*s"

#define SV(d, l)        ((StringView) { .data = d, .len = l })
#define SV_NULL         (SV(NULL, 0))

// Constructors
StringView sv_init(char *data, size_t len);
StringView sv_from_cstr(const char *cstr);

// Methods
StringView sv_trim_left(StringView sv);
StringView sv_trim_right(StringView sv);
StringView sv_trim(StringView sv);
bool       sv_equal(StringView a, StringView b);
StringView sv_split(StringView *sv, char c);
bool       sv_contains(StringView sv, StringView candidate);
StringView sv_substring(StringView sv, size_t from, size_t to);

// Transformers
char      *sv_to_str(StringView sv, char *buffer, size_t len);


#ifdef STRING_VIEW_IMPLEMENTATION

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Constructors
StringView sv_init(char *data, size_t len) {
    return (StringView) {
        .data = data,
        .len = len
    };
}

StringView sv_from_cstr(const char *cstr) {
    return sv_init(cstr, strlen(cstr));
}

// Methods
StringView sv_trim_left(StringView sv) {
    while(sv.len != 0 && isspace(sv.data[0])) {
        sv.data += 1;
        sv.len -= 1;
    }
    return sv;
}

StringView sv_trim_right(StringView sv) {
    while(sv.len != 0 && isspace(sv.data[sv.len - 1])) {
        sv.len -= 1;
    }
    return sv;
}

StringView sv_trim(StringView sv) {
    return sv_trim_right(sv_trim_left(sv));
}

bool sv_equal(StringView a, StringView b) {
    if(a.len != b.len) return false;
    return memcmp(a.data, b.data, a.len) == 0;
}

StringView sv_split(StringView *sv, char c) {
    StringView result = sv_init(sv->data, 0);
    while(sv->len != 0 && sv->data[0] != c) {
        sv->len -= 1;
        sv->data += 1;
        result.len += 1;
    }
    return result;
}

bool sv_contains(StringView sv, StringView candidate) {
    if(candidate.len > sv.len) return false;
    for(size_t i = 0; i <= sv.len - candidate.len; ++i) {
        StringView sub_sv = sv_substring(sv, i, candidate.len);
        if(sv_equal(sub_sv, candidate)) return true;
    }
    return false;
}

StringView sv_substring(StringView sv, size_t from, size_t to) {
    if(to > sv.len) {
        fprintf(stderr, "to index out of bound");
        abort();
    };

    if(from >= sv.len) {
        fprintf(stderr, "from index out of bound");
        abort();
    }

    return sv_init(sv.data + from, to - from);
}

// Transformers
char *sv_to_str(StringView sv, char *buffer, size_t len) {
    if(buffer == NULL) {
        fprintf(stderr, "buffer pointer must not be NULL");
        abort();
    }

    if(len > sv.len) len = sv.len;
    memcpy(buffer, sv.data, len);

    return buffer;
}


#endif // STRING_VIEW_IMPLEMENTATION

#endif // STRING_VIEW_H