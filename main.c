/* main.c */

#define _GNU_SOURCE
#include <printf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/random.h>

#include "banner.h"

#define UNUSED __attribute__((unused))

/* Force a compilation error if condition is true, but also produce a
   result (of value 0 and type size_t), so the expression can be used
   e.g. in a structure initializer (or where-ever else comma expressions
   aren't permitted). */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
/* &a[0] degrades to a pointer: a different type from an array */
#define MUST_BE_ARRAY(a) BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + MUST_BE_ARRAY(arr))

char oom_saver[] = "OOM";

char* strdup_no_oom(const char *str)
{
    char *dup = strdup(str);
    if (dup == NULL)
        dup = oom_saver;
    return dup;
}

void strfree_no_oom(char *str)
{
    if (str != oom_saver)
        free(str);
}

static char *RED = "\e[1;31m";
static char *GREEN = "\e[1;32m";
static char *BLUE = "\e[1;34m";
static char *BLACK = "\e[0m";

#ifdef VERBOSE
 #define VERBOSE_PRINT(fmt, ...) \
    printf("\e[2;34;40m(VERBOSE) [%s] \e[0;30;46m" fmt "\e[0m\n", __PRETTY_FUNCTION__, ## __VA_ARGS__)
#else
 #define VERBOSE_PRINT(...)
#endif

/* this fprintf is vulnerable because it's passed an attacker-controlled format string */
int vulnerable_fprintf(FILE *stream, size_t nargs, const char *vulnerable_fmt, ...)
{
    /* foil attempts at leaking too much info: look for $ and too many % */
    size_t percent = 0;
    for (const char *c = vulnerable_fmt; *c != '\0'; c++) {
        if (*c == '$')
            return fprintf(stream, "<PrintGuard™: $ deflected>");
        if (*c == '%') {
            percent++;
            if (percent > nargs)
                return fprintf(stream, "<PrintGuard™: suspicous amount of %% deflected>");
        }
    }

    /* print */
    va_list args;
    va_start(args, vulnerable_fmt);
    int written = vfprintf(stream, vulnerable_fmt, args);
    va_end(args);
    return written;
}

void reply(const char *type, const char *fmt, ...)
{
    printf("%s: ", type);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

#define reply_msg(fmt, ...) \
    reply("msg", fmt, ## __VA_ARGS__)

#define reply_error(fmt, ...) \
    reply("error", fmt, ## __VA_ARGS__)

union object_data {
    struct baggage {
        char *description;
        unsigned cart;
        unsigned weight;
        const char *contents;
    } baggage;
    struct cart {
        char *designation;
        unsigned flight;
        unsigned capacity;
    } cart;
    struct flight {
        char *destination;
    } flight;
};

struct object {
    enum type {
        BAGGAGE,
        CART,
        FLIGHT,
    } type;
    union object_data d;
};

static const char *dummy_contents[8] = {"clothes", "dirty clothes", "electronics", "wine", "beach items", "books", "souvenirs", "a sedated cat"};

#define FLAG_ENV "FLAG"
#define FLAG_FMT "ECW{%s}"

static struct object flag_baggage;
static struct object heavy_cart;
static struct object secure_flight;

#define FLAG_BAGGAGE_DESC "top secret suitcase"
#define FLAG_BAGGAGE_WEIGHT 42
#define HEAVY_CART_DESIG "secure heavy-duty cart"
#define HEAVY_CART_CAPACITY 50
#define SECURE_FLIGHT_DEST "top secret destination"

static struct object *airport[1024] = {&flag_baggage, &heavy_cart, &secure_flight, NULL};

/* hard-coded indexes */
#define INVALID_ID 0
#define MIN_ID 1
#define SECURE_FLIGHT_ID 3
#define MAX_ID (ARRAY_SIZE(airport))

unsigned reserve_object(struct object ***slotptr)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(airport) && airport[i] != NULL; i++);
    if (i == ARRAY_SIZE(airport)) {
        reply_error("airport capacity reached");
        return 0;
    }
    *slotptr = &airport[i];
    unsigned id = i+1u;
    return id;
}

struct object* get_object(unsigned id, bool send_errors)
{
    size_t i = id-1u; /* overflow is defined, and handled */
    if (i >= ARRAY_SIZE(airport) || airport[i] == NULL) {
        if (send_errors)
            reply_error("bad id: %u", id);
        return NULL;
    }
    return airport[i];
}

struct object* get_typed_object(unsigned id, enum type type, bool send_errors)
{
    struct object *object = get_object(id, send_errors);
    if (object == NULL)
        return NULL;
    if (object->type != type) {
        if (send_errors)
            reply_error("bad object: %u", id);
        return NULL;
    }
    return object;
}

bool is_static_object(unsigned id)
{
    struct object *obj = get_object(id, false);
    return obj == &flag_baggage || obj == &heavy_cart || obj == &secure_flight;
}

struct object* take_object(unsigned id)
{
    size_t i = id-1u; /* overflow is defined, and handled */
    if (i >= ARRAY_SIZE(airport))
        return NULL;
    struct object *obj = airport[i];
    airport[i] = NULL;
    return obj;
}

bool parse_id(enum type type, char **args, unsigned *id)
{
    int read = 0;
    if (sscanf(*args, "%u%n", id, &read) != 1) {
        reply_error("bad id");
        return false;
    }

    struct object *object = get_typed_object(*id, type, true);
    if (object == NULL)
        return false;

    *args = &(*args)[read];

    return true;
}

#define DEFAULT_BAGGAGE_WEIGHT 1

void cmd_baggage(char *desc)
{
    struct object **slot;
    unsigned i = reserve_object(&slot);
    if (i == 0)
        return;
    struct object *b = calloc(1, sizeof(*b));
    if (b == NULL) {
        reply_error("out of memory");
        return;
    }

    desc = strdup_no_oom(desc);

    /* random contents */
    unsigned r;
    if (getrandom(&r, sizeof(r), 0) == sizeof(r))
        r %= ARRAY_SIZE(dummy_contents);
    else
        r = 0;

    b->type = BAGGAGE;
    b->d.baggage.description = desc;
    b->d.baggage.cart = INVALID_ID;
    b->d.baggage.weight = DEFAULT_BAGGAGE_WEIGHT;
    b->d.baggage.contents = dummy_contents[r];
    *slot = b;

    reply_msg("baggage checkin: %s%s%s", BLUE, desc, BLACK);
    reply("id", "%zu", i);
}

#define DEFAULT_CART_CAPACITY 10

void cmd_cart(char *desig)
{
    struct object **slot;
    unsigned i = reserve_object(&slot);
    if (i == 0)
        return;
    struct object *c = calloc(1, sizeof(*c));
    if (c == NULL) {
        reply_error("out of memory");
        return;
    }

    desig = strdup_no_oom(desig);

    c->type = CART;
    c->d.cart.designation = desig;
    c->d.cart.flight = INVALID_ID;
    c->d.cart.capacity = DEFAULT_CART_CAPACITY;
    *slot = c;

    reply_msg("cart ready: %s%s%s", BLUE, desig, BLACK);
    reply("id", "%zu", i);
}

void cmd_flight(char *dest)
{
    struct object **slot;
    unsigned i = reserve_object(&slot);
    if (i == 0)
        return;
    struct object *f = calloc(1, sizeof(*f));
    if (f == NULL) {
        reply_error("out of memory");
        return;
    }

    dest = strdup_no_oom(dest);

    f->type = FLIGHT;
    f->d.flight.destination = dest;
    *slot = f;

    reply_msg("flight announced: %s%s%s", BLUE, f->d.flight.destination, BLACK);
    reply("id", "%zu", i);
}

void cmd_put(char *args)
{
    unsigned bid, cid;
    if (!parse_id(BAGGAGE, &args, &bid))
        return;
    if (!parse_id(CART, &args, &cid))
        return;

    reply_msg("%P", bid, cid);
}

int printf_put(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    /* don't check the object types, we want confusions */
    unsigned bid = *(unsigned*)args[0];
    struct object *obj = get_object(bid, false);
    if (obj == NULL)
        return fprintf(stream, "missing baggage ID");
    struct baggage *b = &obj->d.baggage;
    unsigned cid = *(unsigned*)args[1];
    obj = get_object(cid, false);
    if (obj == NULL)
        return fprintf(stream, "missing cart ID");
    struct cart *c = &obj->d.cart;

    if (b->weight > c->capacity) {
        return fprintf(stream, "baggage exceeds remaining cart capacity!");
    }

    b->cart = cid;
    c->capacity -= b->weight;

    return fprintf(stream, "baggage %s%s%s successfully put onto cart %s%s%s", BLUE, b->description, BLACK, BLUE, c->designation, BLACK);
}

int printf_arginfo_put(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 2) {
        argtypes[0] = PA_INT;
        sizes[0] = sizeof(unsigned);
        argtypes[1] = PA_INT;
        sizes[1] = sizeof(unsigned);
    }
    return 2;
}

void cmd_route(char *args)
{
    unsigned cid, fid;
    if (!parse_id(CART, &args, &cid))
        return;
    if (!parse_id(FLIGHT, &args, &fid))
        return;

    reply_msg("%R", cid, fid);
}

int printf_route(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    /* don't check the object types, we want confusions */
    unsigned cid = *(unsigned*)args[0];
    struct object *obj = get_object(cid, false);
    if (obj == NULL)
        return fprintf(stream, "missing cart ID");
    struct cart *c = &obj->d.cart;
    unsigned fid = *(unsigned*)args[1];
    obj = get_object(fid, false);
    if (obj == NULL)
        return fprintf(stream, "missing flight ID");
    struct flight *f = &obj->d.flight;

    /* prevent re-routing the secure cart */
    if (c == &heavy_cart.d.cart)
        return fprintf(stream, "unable to re-route the secure cart!");

    c->flight = fid;

    return fprintf(stream, "cart %s%s%s routed to flight for %s%s%s", BLUE, c->designation, BLACK, BLUE, f->destination, BLACK);
}

int printf_arginfo_route(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 2) {
        argtypes[0] = PA_INT;
        sizes[0] = sizeof(unsigned);
        argtypes[1] = PA_INT;
        sizes[1] = sizeof(unsigned);
    }
    return 2;
}

void cmd_takeoff(char *args)
{
    unsigned fid;
    if (!parse_id(FLIGHT, &args, &fid))
        return;

    reply_msg("%T", fid);
}

int printf_takeoff(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    /* ID has already been validated, just get the object */
    unsigned fid = *(unsigned*)args[0];
    struct object *obj = get_object(fid, false);
    if (obj == NULL)
        return fprintf(stream, "missing flight ID");
    struct flight *f = &obj->d.flight;
    bool secure = f == &secure_flight.d.flight;
    int written = fprintf(stream, "loading baggages onto %sflight to %s%s%s", secure ? "secure " : "", BLUE, f->destination, BLACK);

    /* loop over all objects to find baggages for this flight */
    size_t count = 0;
    for (unsigned id = MIN_ID; id <= MAX_ID; id++) {
        obj = get_typed_object(id, BAGGAGE, false);
        if (obj == NULL)
            continue;
        struct baggage *b = &obj->d.baggage;
        if (b->cart == INVALID_ID)
            continue;
        /* by design b->cart *should* always be a cart ID, so we use the untyped getter
           (though through the vuln, the user may have set b->cart to something that's not a cart ID) */
        obj = get_object(b->cart, false);
        if (obj == NULL)
            continue;
        struct cart *c = &obj->d.cart;
        if (c->flight != fid)
            continue;

        /* found a baggage put on a cart routed to our flight */
        written += fprintf(stream, "\n screening baggage %s%s%s contents: ", BLUE, b->description, BLACK);
        if (secure)
            written += fprintf(stream, "%sREDACTED%s", RED, BLACK);
        else
            written += fprintf(stream, "%s%s%s", RED, b->contents, BLACK);

        c->capacity += b->weight;

        if (id == fid) {
            /* special case where the vuln was used and a baggage (possibly the flag baggage) was routed to itself, then made to take off
             * found through fuzzing with AFL
             * e.g.:
             *      -> baggage bag
             *         ID 5: BAGGAGE cart=INVALID
             *      -> cart %R
             *         ID 6: CART
             *      -> status 6 5 5                 # triggers the vuln to route 5 (as a CART) to 5 (as a FLIGHT)
             *         ID 5: BAGGAGE cart=5
             *      -> cart %T
             *         ID 7: CART
             *      -> status 7 5                   # triggers the vuln to make 5 (as a FLIGHT) take off
             *                                      # works since 5 (as a BAGGAGE) has cart=5
             *                                      # and 5 (as a CART) has flight=5
             *                                      # which is the "flight" being made to take off
             *
             * if this happens we can't take/free the baggage here, else it will UAF when trying to free the flight
             */
            // nothing to do, just don't free the baggage
            VERBOSE_PRINT("not freeing self-referencing baggage/flight %u\n", id);
        } else if (is_static_object(id)) {
            /* it's a static object (normally the flag baggage), remove it without freeing it */
            take_object(id);
        } else {
            strfree_no_oom(b->description);
            free(take_object(id));
        }

        count += 1;
    }
    if (count == 0)
        written += fprintf(stream, "\n no baggages to load!");

    //TODO: clear flight id of carts which loaded their baggages onto this flight?
    //      or leave it as a vuln / vuln helper?

    written += fprintf(stream, "\n flight to %s%s%s took off!", BLUE, f->destination, BLACK);
    if (is_static_object(fid)) {
        /* it's a static object (normally the secure flight), remove it without freeing it */
        take_object(fid);
        /* and de-route the secure cart */
        heavy_cart.d.cart.flight = INVALID_ID;
    } else {
        strfree_no_oom(f->destination);
        free(take_object(fid));
    }

    return written;
}

int printf_arginfo_takeoff(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 1) {
        argtypes[0] = PA_INT;
        sizes[0] = sizeof(unsigned);
    }
    return 1;
}

#define MAX_STATUS_IDS 10

void cmd_status(char *args)
{
    /* parse ids */
    unsigned ids[MAX_STATUS_IDS] = {0};
    int read = 0;
    size_t n;
    char *argctx;
    char *curarg = strtok_r(args, " ", &argctx);
    for(n = 0; n < ARRAY_SIZE(ids) && curarg; n++) {
        VERBOSE_PRINT("considering id: %s", curarg);
        if (sscanf(curarg, "%u%n", &ids[n], &read) != 1 || (size_t) read != strlen(curarg)) {
            reply_error("bad id: %s", curarg);
            return;
        }
        curarg = strtok_r(NULL, " ", &argctx);
    }
    VERBOSE_PRINT("parsed %zu ids", n);

    /* print objects */
    reply_msg("%*S", n, ids);
}

int printf_status(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    unsigned *ids = *(unsigned**)args[0];
    int written = 0;
    for (int i = 0; i < info->width; i++) {
        if (i > 0)
            written += fprintf(stream, "\n");

        struct object *obj = get_object(ids[i], false);
        VERBOSE_PRINT("[o=%p d=%p]", obj, &obj->d);
        if (obj == NULL) {
            written += fprintf(stream, "\n %u: [n/a]\n  ", ids[i]);
            continue;
        }

        switch(obj->type) {
            case BAGGAGE: {
                const struct baggage *b = &obj->d.baggage;
                written += fprintf(stream, "\n %u: [baggage]\n  ", ids[i]);
                written += vulnerable_fprintf(stream, MAX_STATUS_IDS - i - 1, b->description,
                    ids[i+1], ids[i+2], ids[i+3], ids[i+4], ids[i+5], ids[i+6], ids[i+7], ids[i+8], ids[i+9]); /* artificial :-/ */
                if (b->cart == INVALID_ID)
                    written += fprintf(stream, "\n  cart: N/A");
                else
                    written += fprintf(stream, "\n  cart: %s%d%s", BLUE, b->cart, BLACK);
                written += fprintf(stream, "\n  weight: %s%u%s", BLUE, b->weight, BLACK);
                VERBOSE_PRINT("\n  [contents %p: <<<%s>>>]", b->contents, b->contents);
                break;
            }

            case CART: {
                const struct cart *c = &obj->d.cart;
                written += fprintf(stream, "\n %u: [cart]\n  ", ids[i]);
                written += vulnerable_fprintf(stream, MAX_STATUS_IDS - i - 1, c->designation,
                    ids[i+1], ids[i+2], ids[i+3], ids[i+4], ids[i+5], ids[i+6], ids[i+7], ids[i+8], ids[i+9]); /* artificial :-/ */
                if (c->flight == INVALID_ID)
                    written += fprintf(stream, "\n  flight: N/A");
                else
                    written += fprintf(stream, "\n  flight: %s%u%s", BLUE, c->flight, BLACK);
                written += fprintf(stream, "\n  capacity: %s%u%s", BLUE, c->capacity, BLACK);
                break;
            }

            case FLIGHT: {
                bool secure = obj == &secure_flight;
                const struct flight *f = &obj->d.flight;
                written += fprintf(stream, "\n %u: [flight]\n  ", ids[i]);
                written += fprintf(stream, "flight to %s%s%s", BLUE, f->destination, BLACK);
                written += fprintf(stream, "\n  secure: %s%s%s", BLUE, secure ? "yes" : "no", BLACK);
                break;
            }

            default:
                written += fprintf(stream, "invalid object type, should never happen!");
                break;
        }
    }
    return written;
}

int printf_arginfo_status(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 1) {
        argtypes[0] = PA_POINTER;
        sizes[0] = sizeof(unsigned*);
    }
    return 1;
}

static const struct {
    const char *name;
    void (*fn)(char *args);
    bool need_args;
} commands[] = {
    {"baggage", cmd_baggage, true},
    {"cart", cmd_cart, true},
    {"flight", cmd_flight, true},
    {"put", cmd_put, true}, // %P
    {"route", cmd_route, true}, // %R
    {"takeoff", cmd_takeoff, true}, // %T
    {"status", cmd_status, true}, // %S
};

int printguard_n_specifier(FILE *stream, UNUSED const struct printf_info *info, UNUSED const void *const *args)
{
    return fprintf(stream, "<PrintGuard™: %%n deflected>");
}

int printguard_n_specifier_info(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *size)
{
    return 1; /* TODO: consume 0 args? */
}

int main(UNUSED int argc, UNUSED char *argv[])
{
    /* disable colors if not printing to tty */
    if (!isatty(1))
        RED = BLUE = GREEN = BLACK = "";

    if (register_printf_specifier('P', printf_put, printf_arginfo_put) ||
        register_printf_specifier('R', printf_route, printf_arginfo_route) ||
        register_printf_specifier('T', printf_takeoff, printf_arginfo_takeoff) ||
        register_printf_specifier('S', printf_status, printf_arginfo_status) ||
        register_printf_specifier('n', printguard_n_specifier, printguard_n_specifier_info)) {
        fprintf(stderr, "init failed\n");
        return EXIT_FAILURE;
    }

    const char *flag_env = getenv(FLAG_ENV);
    if (flag_env == NULL)
        flag_env = "flag";
    char *flag_contents;
    if (asprintf(&flag_contents, FLAG_FMT, flag_env) == -1) {
        perror("internal error");
        return 1;
    }

    flag_baggage.type = BAGGAGE;
    flag_baggage.d.baggage.description = FLAG_BAGGAGE_DESC;
    flag_baggage.d.baggage.cart = INVALID_ID;
    flag_baggage.d.baggage.weight = FLAG_BAGGAGE_WEIGHT;
    flag_baggage.d.baggage.contents = flag_contents;

    heavy_cart.type = CART;
    heavy_cart.d.cart.designation = HEAVY_CART_DESIG;
    heavy_cart.d.cart.flight = SECURE_FLIGHT_ID;
    heavy_cart.d.cart.capacity = HEAVY_CART_CAPACITY;

    secure_flight.type = FLIGHT;
    secure_flight.d.flight.destination = SECURE_FLIGHT_DEST;

    if (isatty(1))
        printf("%s", banner_color);
    else
        printf("%s", banner_bw);
    printf("                                        +------------------------+\n"
           "                                        | secured by PrintGuard™ |\n"
           "                                        +------------------------+\n\n\n"
           "Welcome to this computerized Airport Routing System Endpoint!\n\n\n");

    char *line = NULL;
    size_t linelen = 0;
    while (!feof(stdin) && !ferror(stdin)) {
        printf("> ");
        ssize_t len = getline(&line, &linelen, stdin);
        if (len < 0)
            continue;
        /* there may not be a newline at eof */
        if (line[len-1] == '\n')
            line[len-1] = '\0';

        VERBOSE_PRINT("line=<<<%s>>>", line);
        char *cmdctx;
        char *cmd = strtok_r(line, " ", &cmdctx);
        if (cmd == NULL) /* TODO: error? */
            continue;
        if (cmd[0] == '#')
            continue;
        char *arg = strtok_r(NULL, "", &cmdctx);

        size_t i;
        for (i = 0; i < ARRAY_SIZE(commands) && strcmp(cmd, commands[i].name) != 0; i++);
        if (i == ARRAY_SIZE(commands)) {
            reply_error("unknown command: %s", cmd);
            continue;
        }

        if (commands[i].need_args && (arg == NULL || strlen(arg) < 1)) {
            reply_error("missing arguments");
            continue;
        }

        commands[i].fn(arg);
    }
    /* free getline buffer */
    free(line);

    /* free the flag */
    free(flag_contents);

    return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
}
