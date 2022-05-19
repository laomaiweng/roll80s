/* main.c */

#include <printf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED __attribute__((unused))

/* Force a compilation error if condition is true, but also produce a
   result (of value 0 and type size_t), so the expression can be used
   e.g. in a structure initializer (or where-ever else comma expressions
   aren't permitted). */
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int:-!!(e); }))
/* &a[0] degrades to a pointer: a different type from an array */
#define MUST_BE_ARRAY(a) BUILD_BUG_ON_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + MUST_BE_ARRAY(arr))

#define RED "\e[1;31m"
#define GREEN "\e[1;32m"
#define BLUE "\e[1;34m"
#define BLACK "\e[0m"

#ifdef VERBOSE
 #define VERBOSE_PRINT(fmt, ...) \
    printf("(VERBOSE) [%s] " fmt "\n", __PRETTY_FUNCTION__, ## __VA_ARGS__)
#else
 #define VERBOSE_PRINT(...)
#endif

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

union thing_data {
    struct player {
        char *name;
        int hitpoints;
        unsigned level;
        unsigned experience;
    } player;
    struct monster {
        char *name;
        int hitpoints;
        unsigned strength;
    } monster;
    struct chest {
        const char *name;
        unsigned lock;
        char *contents;
    } chest;
};

struct thing {
    enum type {
        PLAYER,
        MONSTER,
        CHEST
    } type;
    union thing_data d;
};

#define ECW_FLAG "ECW{flag}"
static struct thing flag;

static struct thing *universe[1024] = {&flag, NULL};

unsigned reserve_thing()
{
    /* slot 0 is the flag, start searching at 1 */
    size_t i;
    for (i = 1; i < ARRAY_SIZE(universe) && universe[i] != NULL; i++);
    if (i == ARRAY_SIZE(universe)) {
        reply_error("universe limit reached");
        return 0;
    }
    return i;
}

struct thing* get_thing(unsigned id)
{
    if (id >= ARRAY_SIZE(universe) || universe[id] == NULL) {
        reply_error("bad id: %u", id);
        return NULL;
    }
    return universe[id];
}

struct thing* get_typed_thing(unsigned id, enum type type)
{
    struct thing *thing = get_thing(id);
    if (thing == NULL)
        return NULL;
    if (thing->type != type) {
        reply_error("bad thing: %u", id);
        return NULL;
    }
    return thing;
}

void delete_thing(void *data)
{
    size_t i;
    for (i = 0; i < ARRAY_SIZE(universe); i++)
        if (universe[i] != NULL && &universe[i]->d == data)
            break;
    if (i == ARRAY_SIZE(universe)) {
        reply_error("bad id");
        return;
    }

    free(universe[i]);
    universe[i] = NULL;
}

#define LEVEL_CAP 99

bool gain_experience(struct player *p, unsigned xp)
{
    if (p->level >= LEVEL_CAP)
        return false;

    p->experience += xp;
    if (p->experience <= p->level)
        return false;

    p->experience = 0;
    p->level += 1;
    return true;
}

bool parse_player(char **args, struct player **player)
{
    unsigned id;
    int read = 0;
    if (sscanf(*args, "%u %n", &id, &read) != 1) {
        reply_error("bad id");
        return false;
    }

    struct thing *thing = get_typed_thing(id, PLAYER);
    if (thing == NULL)
        return false;

    *player = &thing->d.player;
    *args = &(*args)[read];

    return true;
}

bool parse_player_with_args(char **args, struct player **player, const char *fmt, ...)
{
    if (!parse_player(args, player))
        return false;

    va_list subargs;
    va_start(subargs, fmt);
    int scanned = vsscanf(*args, fmt, subargs);
    va_end(subargs);
    scanned = scanned < 0 ? 0 : scanned; /* can it be < 0 with sscanf? */

    size_t nargs = parse_printf_format(fmt, 0, NULL);
    if ((size_t) scanned != nargs) {
        reply_error("bad args");
        return false;
    }

    return true;
}

void cmd_player(char *name)
{
    if (name == NULL || strlen(name) < 1) {
        reply_error("bad name");
        return;
    }

    unsigned i = reserve_thing();
    if (i == 0)
        return;
    struct thing *p = calloc(1, sizeof(*p));
    if (p == NULL) {
        reply_error("out of memory");
        return;
    }

    p->type = PLAYER;
    p->d.player.name = strdup(name);
    p->d.player.hitpoints = 1;
    p->d.player.level = 1;
    universe[i] = p;

    reply_msg("hello " BLUE "%s" BLACK "!", name);
    reply("id", "%zu", i);
}

void cmd_monster(char *args)
{
    unsigned strength, hp;
    int read = 0;
    if (sscanf(args, "%u/%u %n", &strength, &hp, &read) != 2) {
        if (read > 1)
            reply_error("bad attributes: %*s", read-1, args);
        else
            reply_error("bad attributes");
        return;
    }
    char *name = &args[read];
    if (strlen(name) < 1) {
        reply_error("bad name");
        return;
    }

    unsigned i = reserve_thing();
    if (i == 0)
        return;
    struct thing *m = calloc(1, sizeof(*m));
    if (m == NULL) {
        reply_error("out of memory");
        return;
    }

    m->type = MONSTER;
    m->d.monster.name = strdup(name);
    m->d.monster.hitpoints = hp;
    m->d.monster.strength = strength;
    universe[i] = m;

    reply_msg("a wild " BLUE "%s" BLACK " appears!", name);
    reply("id", "%zu", i);
}

void cmd_chest(char *args)
{
    unsigned lock;
    int read = 0;
    if (sscanf(args, "%u %n", &lock, &read) != 1) {
        reply_error("bad lock strength");
        return;
    }
    char *contents = &args[read];

    unsigned i = reserve_thing();
    if (i == 0)
        return;
    struct thing *c = calloc(1, sizeof(*c));
    if (c == NULL) {
        reply_error("out of memory");
        return;
    }

    c->type = CHEST;
    c->d.chest.name = "chest";
    c->d.chest.lock = lock;
    c->d.chest.contents = strdup(contents);
    universe[i] = c;

    reply_msg("there's a %s in the room", c->d.chest.name);
    reply("id", "%zu", i);
}

void cmd_attack(char *args)
{
    struct player *p;
    unsigned monster_id;
    if (!parse_player_with_args(&args, &p, "%u", &monster_id))
        return;
    struct thing *m = get_typed_thing(monster_id, MONSTER);
    if (m == NULL)
        return;

    reply_msg("%A", p, &m->d.monster);
}

int printf_attack(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    struct player *p = *((struct player**)(args[0]));
    struct monster *m = *((struct monster**)(args[1]));

    int attack = m->strength - p->level;
    if (attack < 0)
        attack = 0;
    p->hitpoints -= attack;
    m->hitpoints -= p->level;

    int written = 0;
    written += fprintf(stream, "%s hits %s for " BLUE "%d" BLACK " damage", p->name, m->name, p->level);
    written += fprintf(stream, "\n %s hits %s for " GREEN "%d" BLACK " damage", m->name, p->name, attack);
    if (m->hitpoints <= 0) {
        written += fprintf(stream, "\n %s dies!", m->name);
        if (gain_experience(p, m->strength))
            written += fprintf(stream, "\n %s levels up", p->name);
        free(m->name);
        delete_thing(m);
    }
    if (p->hitpoints == 0) {
        written += fprintf(stream, "\n %s is in critical condition", p->name);
    } else if (p->hitpoints < 0) {
        if (p->hitpoints < -10)
            written += fprintf(stream, "\n %s is torn to bits!", p->name);
        else
            written += fprintf(stream, "\n %s dies :(", p->name);
        free(p->name);
        delete_thing(p);
    }
    return written;
}

int printf_arginfo_attack(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 2) {
        argtypes[0] = PA_POINTER;
        sizes[0] = sizeof(struct player*);
        argtypes[1] = PA_POINTER;
        sizes[1] = sizeof(struct monster*);
    }
    return 2;
}

void cmd_heal(char *args)
{
    struct player *p;
    int hp;
    if (!parse_player_with_args(&args, &p, "%d", &hp))
        return;

    reply_msg("%H", p, hp);
}

int printf_heal(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    struct player *p = *((struct player**)(args[0]));
    int hp = *((int*)(args[1]));

    p->hitpoints += hp;

    int written = 0;
    written += fprintf(stream, "heal %s for " BLUE "%+d" BLACK " hitpoints", p->name, hp);
    return written;
}

int printf_arginfo_heal(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 2) {
        argtypes[0] = PA_POINTER;
        sizes[0] = sizeof(struct player*);
        argtypes[1] = PA_INT;
        sizes[1] = sizeof(int);
    }
    return 2;
}

void cmd_open(char *args)
{
    struct player *p;
    unsigned chest_id;
    if (!parse_player_with_args(&args, &p, "%u", &chest_id))
        return;

    struct thing *c = get_typed_thing(chest_id, CHEST);
    if (c == NULL)
        return;

    reply_msg("%O", p, &c->d.chest);
}

int printf_open(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    struct player *p = *((struct player**)(args[0]));
    struct chest *c = *((struct chest**)(args[1]));

    int written = 0;
    written += fprintf(stream, "%s attempts to open a %s", p->name, c->name);
    if ((int) p->level >= (int) c->lock) {
        written += fprintf(stream, "\n lock picked successfully!");
        if (p->level == c->lock && gain_experience(p, 1))
            written += fprintf(stream, "\n %s levels up", p->name);
        if (strlen(c->contents) > 0)
            written += fprintf(stream, "\n %s finds %s", p->name, c->contents);
        else
            written += fprintf(stream, "\n it's empty :(");
        if (c != &flag.d.chest) {
            free(c->contents);
            delete_thing(c);
        }
    } else
        written += fprintf(stream, "\n this lock is too tough");
    return written;
}

int printf_arginfo_open(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 2) {
        argtypes[0] = PA_POINTER;
        sizes[0] = sizeof(struct player*);
        argtypes[1] = PA_POINTER;
        sizes[1] = sizeof(struct chest*);
    }
    return 2;
}

void cmd_talk(char *args)
{
    struct player *p;
    if (!parse_player(&args, &p))
        return;

    reply_msg("%T", p, args);
}

int printf_talk(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    const struct player *p = *((const struct player**)(args[0]));
    char *msg = *((char**)(args[1]));
    size_t len = strlen(msg);
    int written = 0;

    if (len > 2 && msg[0] == '*' && msg[len-1] == '*') {
        msg[len-1] = '\0';
        written += fprintf(stream, "%s " GREEN "%s" BLACK, p->name, &msg[1]);
    } else
        written += fprintf(stream, "%s says: " GREEN "%s" BLACK, p->name, msg);
    return written;
}

int printf_arginfo_talk(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 2) {
        argtypes[0] = PA_POINTER;
        sizes[0] = sizeof(struct player*);
        argtypes[1] = PA_POINTER;
        sizes[1] = sizeof(char*);
    }
    return 2;
}

void cmd_status(char *args)
{
    /* parse ids */
    unsigned ids[10];
    int read = 0;
    char *curarg = args;
    size_t n;
    for(n = 0; n < ARRAY_SIZE(ids) && sscanf(curarg, "%u%n", &ids[n], &read) == 1; n++, curarg += read);

    /* find things */
    struct thing *things[10];
    for (size_t i = 0; i < n; i++) {
        things[i] = get_thing(ids[i]);
        if (things[i] == NULL)
            return;
    }

    /* print things */
    reply_msg("%*S", n, things);
}

int printf_status(FILE *stream, UNUSED const struct printf_info *info, const void *const *args)
{
    const struct thing **things = *((const struct thing***)(args[0]));
    int n = info->width;
    int written = 0;
    for (int i = 0; i < n; i++) {
        if (i > 0)
            written += fprintf(stream, "\n\n ");
        const struct thing *t = things[i];
        VERBOSE_PRINT("[t=%p d=%p]", t, &t->d);
        switch(t->type) {
            case PLAYER: {
                const struct player *p = &t->d.player;
                written += fprintf(stream, p->name, &things[i+1]->d, &things[i+2]->d, &things[i+3]->d, &things[i+4]->d); /* artificial but heh */
                written += fprintf(stream, "\n  hitpoints: " BLUE "%d" BLACK, p->hitpoints);
                written += fprintf(stream, "\n  level: " BLUE "%u" BLACK, p->level);
                written += fprintf(stream, "\n  experience: " BLUE "%d" BLACK, p->experience);
                break;
            }

            case MONSTER: {
                const struct monster *m = &t->d.monster;
                written += fprintf(stream, m->name, &things[i+1]->d, &things[i+2]->d, &things[i+3]->d, &things[i+4]->d); /* artificial but heh */
                written += fprintf(stream, "\n  hitpoints: " BLUE "%d" BLACK, m->hitpoints);
                written += fprintf(stream, "\n  strength: " BLUE "%u" BLACK, m->strength);
                break;
            }

            case CHEST: {
                const struct chest *c = &t->d.chest;
                written += fprintf(stream, "a %s", c->name);
                written += fprintf(stream, "\n  lock: " BLUE "%d" BLACK, c->lock);
                VERBOSE_PRINT("\n  [contents %p: <<<%s>>>]", c->contents, c->contents);
                break;
            }

            default:
                written += fprintf(stream, "invalid thing type, should never happen!");
                break;
        }
    }
    return written;
}

int printf_arginfo_status(UNUSED const struct printf_info *info, UNUSED size_t n, UNUSED int *argtypes, UNUSED int *sizes)
{
    if (n >= 1) {
        argtypes[0] = PA_POINTER;
        sizes[0] = sizeof(struct thing*);
    }
    return 1;
}

void cmd_gm(char *msg)
{
    if (msg == NULL || strlen(msg) < 1) {
        reply_error("bad message");
        return;
    }

    reply("gm", RED "%s" BLACK, msg);
}

static const struct {
    const char *name;
    void (*fn)(char *args);
} commands[] = {
    {"player", cmd_player},
    {"monster", cmd_monster},
    {"chest", cmd_chest},
    {"attack", cmd_attack}, // %A
    {"heal", cmd_heal}, // %H
    {"open", cmd_open}, // %O
    {"talk", cmd_talk}, // %T
    {"status", cmd_status}, // %S
    {"gm", cmd_gm},
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
    if (register_printf_specifier('A', printf_attack, printf_arginfo_attack) ||
        register_printf_specifier('H', printf_heal, printf_arginfo_heal) ||
        register_printf_specifier('O', printf_open, printf_arginfo_open) ||
        register_printf_specifier('T', printf_talk, printf_arginfo_talk) ||
        register_printf_specifier('S', printf_status, printf_arginfo_status) ||
        register_printf_specifier('n', printguard_n_specifier, printguard_n_specifier_info)) {
        fprintf(stderr, "init failed\n");
        return EXIT_FAILURE;
    }

    flag.type = CHEST;
    flag.d.chest.name = "golden chest";
    flag.d.chest.lock = LEVEL_CAP + 1; /* out of reach for fair lock picking */
    flag.d.chest.contents = "the flag: " ECW_FLAG;

    while (!feof(stdin) && !ferror(stdin)) {
        char *line = NULL;
        size_t linelen = 0;
        ssize_t len = getline(&line, &linelen, stdin);
        if (len < 0) {
            // read error, must still free buffer
            free(line);
            continue;
        }
        // there may not be a newline at eof
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

        commands[i].fn(arg);

        // free getline buffer
        free(line);
    }
    return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
}
