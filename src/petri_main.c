#include <raylib.h>
#include <raymath.h>
#include <emscripten/emscripten.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"

#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#undef RAYGUI_IMPLEMENTATION

////////////////////////////////////////
// Generic Helper Functions
////////////////////////////////////////

static const uint64_t RNG_CONSTANT = 5111111111111119ull;

uint64_t bad_rng(uint64_t seed) {
    uint64_t tmp = seed * RNG_CONSTANT;
    return ((tmp << 62) | (tmp >> 2)) ^ (RNG_CONSTANT >> 1);
}

Vector2 Vector2Midpoint(Vector2 a, Vector2 b) {
    return (Vector2) { ( a.x + b.x ) / 2, (a.y + b.y) / 2 };
}

Vector2 Vector2Min(Vector2 a, Vector2 b) {
    return (Vector2) { JMIN(a.x, b.x), JMIN(a.y, b.y) };
}

Vector2 Vector2Max(Vector2 a, Vector2 b) {
    return (Vector2) { JMAX(a.x, b.x), JMAX(a.y, b.y) };
}

int consume_integer(char **input) {
    ASSERT(input != NULL);
    bool bad_input = true;
    int result = 0;
    while ('0' <= **input && **input <= '9') {
        bad_input = false;
        result = result * 10 + (**input - '0');
        ++(*input);
    }
    if (bad_input) {
        return -1;
    }
    return result;
}

Rectangle RectangleFromVectors(Vector2 a, Vector2 b) {
    Rectangle result = {
        .x = JMIN(a.x, b.x),
        .y = JMIN(a.y, b.y),
        .width = JMAX(a.x, b.x) - JMIN(a.x, b.x),
        .height = JMAX(a.y, b.y) - JMIN(a.y, b.y),
    };
    return result;
}


////////////////////////////////////////
// Level Data
////////////////////////////////////////

struct PetriLevel {
    char* sim_data;
    char* place_data;
    bool victory;
};

struct PetriLevel g_levels[] = {
    {
        .sim_data = "29,0;26>1;25>26,24;23,15>25;22,13>25;21,11>25;20,9>25;19,8>25;16,17>17,18;14,17>17,16;12,17>17,14;10,17>17,12;7,17>17,10;6>15,16;5>13,14;4>11,12;3>9,10;2>8,7;1>6;1>5;1>4;1>3;1>2",
        .place_data = "520,440;60,240,1;140,80;140,160;140,240;140,320;140,400;200,120;200,80;200,160;200,200;200,240;200,280;200,320;200,360;200,400;200,440;720,330;330,100;360,260;360,310;360,360;360,410;360,460;280,520;280,460;60,490;410,490;320,490,1;",
    },
    {
        .sim_data = "2,1;1>0",
        .place_data = "520,250;250,250;",
    },
    {
        .sim_data = "6,4;>3;5>2;3>1;1,2,4>0",
        .place_data = "550,300;450,350;450,300;350,350;450,250;350,300;",
    },
    {
        .sim_data = "14,10;4,1>5;5,1>6;6,1>8;8,1>7;7,1,2>0;9,12>3,1,10;3,11>9,13;2,11,12>",
        .place_data = "420,450;270,330;400,350,1;350,330,1;120,450,1;180,450;240,450;360,450;300,450;450,330;250,220;450,220;350,220;550,220;",
    },
    {
        .sim_data = "13,10;4,1>5;5,1>6;6,1>8;8,1>7;7,1,2>0;9,12>3,1,10;3,11>9,10;2,11,12>",
        .place_data = "420,450;270,330;400,350,1;350,330,1;120,450,1;180,450;240,450;360,450;300,450;450,330;250,220;450,220;350,220;550,220;",
    },
    {
        .sim_data = "38,0;24>31;35,28>0;32,37>33;31,36>27;32,34>35;32,33>34;31,30>32;31,27>30;28,25,19>;28,25,20>;28,25,21>;28,25,22>;28,25,23>;1>2;1>3;1>4;1>5;1>6;2>8,7;3>9,10;4>11,12;5>13,14;6>15,16;7,17>18,10;10,17>18,12;12,17>18,14;14,17>18,16;16,17>29;19,8>25;20,9>25;21,11>25;22,13>25;23,15>;25>26,24;26>1",
        .place_data = "560,490;60,240,1;140,80;140,160;140,240;140,320;140,400;200,120;200,80;200,160;200,200;200,240;200,280;200,320;200,360;200,400;200,440;350,110,1;350,50;370,260;370,310;370,360;370,410;370,460;290,520;290,460;60,490;670,460;330,490,1;350,170;710,460;630,520;750,460;710,400;670,400;630,400;630,460,3;750,400,1;",
        // One solution:
        // Simulation data:
        // 46,0;45>17;41,29>19,42;40,29>20,42;39,29>21,42;38,29>22,42;44,29>23,42;42>44,45;43,40>41,45;43,39>40,45;43,38>39,45;43,44>38,45;18>43;24>31;35,28>0;32,37>33;31,36>27;32,34>35;32,33>34;31,30>32;31,27>30;28,25,19>;28,25,20>;28,25,21>;28,25,22>;28,25,23>;1>2;1>3;1>4;1>5;1>6;2>8,7;3>9,10;4>11,12;5>13,14;6>15,16;7,17>18,10;10,17>18,12;12,17>18,14;14,17>18,16;16,17>29;19,8>25;20,9>25;21,11>25;22,13>25;23,15>25;25>26,24;26>1
        // Place data:
        // 560,490;60,240,1;140,80;140,160;140,240;140,320;140,400;200,120;200,80;200,160;200,200;200,240;200,280;200,320;200,360;200,400;200,440;350,110,1;350,50;370,210;370,260;370,310;370,360;370,410;290,520;290,460;60,490;670,460;330,490,1;400,150;710,460;630,520;750,460;710,400;670,400;630,400;630,460,3;750,400,1;500,340;500,290;500,240;500,190;450,430;730,130;500,390,1;460,120;
    },
    {
        .sim_data = "2,1;1>0",
        .place_data = "525,250;250,250;",
    },
    {
        .sim_data = "2,1;1>0",
        .place_data = "525,250;250,250;",
    },
    {
        .sim_data = "2,1;1>0",
        .place_data = "525,250;250,250;",
    },
    {
        .sim_data = "2,1;1>0",
        .place_data = "525,250;250,250;",
    },
    {
        .sim_data = "2,1;1>0",
        .place_data = "525,250;250,250;",
    },
    {
        .sim_data = "2,1;1>0",
        .place_data = "525,250;250,250;",
    },
    {
        .sim_data = "2,1;1>0",
        .place_data = "525,250;250,250;",
    },
};
#define NUM_LEVELS (sizeof(g_levels) / sizeof(struct PetriLevel))


////////////////////////////////////////
// Petri Simulation
////////////////////////////////////////

#define MAX_VALUES 10000
#define PS_MAX_PLACE_VALUE 15

struct PetriTransition;
struct PetriSimulation {
    uint8_t values[MAX_VALUES];
    int num_values;
    int num_locked_values;
    struct PetriTransition* transition_list;
    uint64_t last_rng;
};

struct PetriTransition {
    struct PetriTransition* next_transition;
    struct PetriTransition* next_valid_transition;
    struct PetriTransition* more_space;
    int source;
    int destination;
};

// Used as a placeholder on the last valid transition so that
// transition.next_valid_transition can be set to non-null for all valid
// transitions without creating a pointer loop.
struct PetriTransition g_last_valid_transition = {
    .source = -1,
    .destination = -1,
};

struct PetriTransition* ps_allocate_transition() {
    struct PetriTransition* result = calloc(1, sizeof(struct PetriTransition));
    ASSERT(result != NULL);
    result->next_transition = NULL;
    result->next_valid_transition = NULL;
    result->more_space = NULL;
    result->source = -1;
    result->destination = -1;
    return result;
}

void ps_free_transition(struct PetriTransition* transition) {
    struct PetriTransition *to_delete = transition;
    while (to_delete != NULL) {
        to_delete = transition->more_space;
        free(transition);
    }
}

void ps_destroy(struct PetriSimulation* simulation) {
    // todo
}

int ps_mark_valid_transitions(struct PetriSimulation* simulation, struct PetriTransition** out_first_valid) {
    ASSERT(simulation != NULL);
    struct PetriTransition* first_valid_transition = NULL;
    struct PetriTransition* last_valid_transition = NULL;
    int valid_count = 0;

    // Cleanup any transitions with no sources or destinations.
    for (struct PetriTransition** ptr = &(simulation->transition_list);
            *ptr != NULL;) {
        struct PetriTransition *transition = *ptr;

        while (transition->source == -1 && transition->destination == -1 && transition->more_space != NULL) {
            struct PetriTransition* to_delete = transition->more_space;
            transition->source = to_delete->source;
            transition->destination = to_delete->destination;
            transition->more_space = transition->more_space->more_space;
            to_delete->more_space = NULL;
            ps_free_transition(to_delete);
        }

        if (transition->source == -1 && transition->destination == -1) {
            *ptr = transition->next_transition;
            ps_free_transition(transition);
        } else {
            ptr = &(transition->next_transition);
        }
    }

    for (struct PetriTransition* transition = simulation->transition_list;
            transition != NULL;
            transition = transition->next_transition) {
        
        bool valid = true;
        for (struct PetriTransition* current_space = transition;
                valid && current_space != NULL;
                current_space = current_space->more_space) {
            ASSERT(current_space->source < simulation->num_values);
            valid = valid && ((current_space->source < 0) || (0 < simulation->values[current_space->source]));
        }
        transition->next_valid_transition = NULL;
        if (valid) {
            ++valid_count;
            transition->next_valid_transition = &g_last_valid_transition;
            if (first_valid_transition == NULL) {
                first_valid_transition = transition;
                last_valid_transition = transition;
            } else {
                last_valid_transition->next_valid_transition = transition;
                last_valid_transition = transition;
            }
        }
    }

    if (out_first_valid != NULL) {
        *out_first_valid = first_valid_transition;
    }
    return valid_count;
}

// Simulate one step. Return true if any change happened.
bool ps_step(struct PetriSimulation* simulation) {
    ASSERT(simulation != NULL);
    struct PetriTransition* first_valid_transition = NULL;
    int valid_count = 0;

    valid_count = ps_mark_valid_transitions(simulation, &first_valid_transition);

    simulation->last_rng = bad_rng(simulation->last_rng);
    if (valid_count == 0) {
        return false;
    }

    struct PetriTransition* to_fire = first_valid_transition;
    for (int i = simulation->last_rng % valid_count; 0 < i; --i) {
        to_fire = to_fire->next_valid_transition;
    }

    for (struct PetriTransition* current_space = to_fire;
            current_space != NULL;
            current_space = current_space->more_space) {
        ASSERT(current_space->destination < simulation->num_values);
        ASSERT(current_space->source < simulation->num_values);
        if (0 <= current_space->destination) {
            simulation->values[current_space->destination] = JMIN(PS_MAX_PLACE_VALUE, simulation->values[current_space->destination] + 1);
        }
        if (0 <= current_space->source) {
            ASSERT(0 < simulation->values[current_space->source]);
            --(simulation->values[current_space->source]);
        }
    }

    return true;
}

void ps_print(struct PetriSimulation* simulation) {
    printf("[ ");
    for (int i = 0; i < simulation->num_values; ++i) {
        printf("%d, ", (int)(simulation->values[i]));
    }
    printf("]\n");
}

enum LinkType {
    LINK_SOURCE = 0,
    LINK_DESTINATION = 1,
};
void ps_add_link(struct PetriSimulation* simulation, int place, struct PetriTransition* transition, enum LinkType link_type) {
    ASSERT(simulation != NULL);
    ASSERT(transition != NULL);
    ASSERT(0 <= place && place < simulation->num_values);

    for (;;) {
        if (link_type == LINK_SOURCE && transition->source == -1) {
            break;
        }
        if (link_type == LINK_SOURCE && transition->source == place) {
            return;
        }
        if (link_type == LINK_DESTINATION && transition->destination == -1) {
            break;
        }
        if (link_type == LINK_DESTINATION && transition->destination == place) {
            return;
        }
        if (transition->more_space == NULL) {
            transition->more_space = ps_allocate_transition();
        }
        transition = transition->more_space;
    }

    if (link_type == LINK_SOURCE) {
        transition->source = place;
    } else {
        transition->destination = place;
    }
}

#define PARSE_ERROR() goto err
struct PetriSimulation* ps_parse(char* puzzle) {
    struct PetriSimulation* result = malloc(sizeof(struct PetriSimulation));
    for (int i = 0; i < MAX_VALUES; ++i) {
        result->values[i] = 0;
    }
    result->num_values = -1;
    result->transition_list = NULL;
    result->last_rng = 0;

    result->num_values = consume_integer(&puzzle);
    if (result->num_values < 0) PARSE_ERROR();

    if (puzzle[0] != ',') PARSE_ERROR();
    ++puzzle;

    result->num_locked_values = consume_integer(&puzzle);
    if (result->num_locked_values < 0) PARSE_ERROR();

    for (;;) {
        if (puzzle[0] == '\0') {
            break;
        }
        if (puzzle[0] != ';') PARSE_ERROR();
        ++puzzle;
        struct PetriTransition* transition = ps_allocate_transition();
        transition->next_transition = result->transition_list;
        result->transition_list = transition;

        struct PetriTransition** next_space = &transition;
        while (puzzle[0] != '>') {
            int this_source = consume_integer(&puzzle);
            if (this_source < 0 || result->num_values <= this_source) PARSE_ERROR();

            if (*next_space == NULL) {
                *next_space = ps_allocate_transition();
            }
            (*next_space)->source = this_source;
            next_space = &((*next_space)->more_space);

            if (puzzle[0] == ',') {
                ++puzzle;
            } else if (puzzle[0] != '>') {
                PARSE_ERROR();
            }
        }
        ++puzzle;

        next_space = &transition;
        while (puzzle[0] != '\0' && puzzle[0] != ';') {
            int this_dest = consume_integer(&puzzle);
            if (this_dest < 0 || result->num_values <= this_dest) PARSE_ERROR();

            if (*next_space == NULL) {
                *next_space = ps_allocate_transition();
            }
            (*next_space)->destination = this_dest;
            next_space = &((*next_space)->more_space);

            if (puzzle[0] == ',') {
                ++puzzle;
            }
        }
    }

    return result;
err:
    ps_destroy(result);
    return NULL;
}

void ps_export(struct PetriSimulation* sim) {
    printf("%d,%d", sim->num_values, sim->num_locked_values);
    for (struct PetriTransition* transition = sim->transition_list;
            transition != NULL;
            transition = transition->next_transition) {
        printf(";");
        bool first_value = true;
        for (struct PetriTransition* current_space = transition;
                current_space != NULL;
                current_space = current_space->more_space) {
            if (0 <= current_space->source) {
                if (!first_value) {
                    printf(",");
                }
                first_value = false;
                printf("%d", current_space->source);
            }
        }
        printf(">");
        first_value = true;
        for (struct PetriTransition* current_space = transition;
                current_space != NULL;
                current_space = current_space->more_space) {
            if (0 <= current_space->destination) {
                if (!first_value) {
                    printf(",");
                }
                first_value = false;
                printf("%d", current_space->destination);
            }
        }
    }
    printf("\n");
}

bool ps_has_source(struct PetriTransition* transition, int place) {
    for (struct PetriTransition* current_space = transition;
            current_space != NULL;
            current_space = current_space->more_space) {
        if (current_space->source == place) {
            return true;
        }
    }
    return false;
}

bool ps_has_destination(struct PetriTransition* transition, int place) {
    for (struct PetriTransition* current_space = transition;
            current_space != NULL;
            current_space = current_space->more_space) {
        if (current_space->destination == place) {
            return true;
        }
    }
    return false;
}


////////////////////////////////////////
// Main Loop
////////////////////////////////////////

#define VALUE_SIZE 24
#define DEFAULT_SPACING 32
#define WIDE_SPACING (DEFAULT_SPACING * 3 / 2)
#define TRANSITION_WIDTH 8
#define TRANSITION_HEIGHT 32
#define MAX_MULTISELECT 100

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct ValueInterfaceData {
    int x;
    int y;
};

enum CursorActivity {
    CURSOR_CONSUMED = 0,
    CURSOR_DEFAULT = 1,
    CURSOR_DELETE = 2,
    CURSOR_MOVE_PLACE = 3,
    CURSOR_LINK_TO = 4,
    CURSOR_LINK_FROM = 5,
    CURSOR_LEVEL_SELECT = 6,
    CURSOR_DRAGGING = 7,
    CURSOR_HAS_MULTISELECT = 8,
};

struct GameContext {
    struct PetriSimulation* sim;
    int selected_place;
    enum CursorActivity activity;
    struct ValueInterfaceData value_interface_data[MAX_VALUES];
    uint8_t initial_values[MAX_VALUES];
    bool simulation_started;
    bool auto_run;
    int current_level;
    Vector2 drag_anchor;
    int multiselect_count;
    int multiselect_places[MAX_MULTISELECT];
} g_context;
bool g_context_init = false;

Rectangle place_location_r(int idx) {
    ASSERT(0 <= idx);
    ASSERT(idx < MAX_VALUES);
    ASSERT(g_context.sim != NULL);
    ASSERT(idx < g_context.sim->num_values);
    Rectangle result = {
        .x = g_context.value_interface_data[idx].x - VALUE_SIZE / 2,
        .y = g_context.value_interface_data[idx].y - VALUE_SIZE / 2,
        .width = VALUE_SIZE,
        .height= VALUE_SIZE,
    };
    return result;
}

Vector2 place_location_v(int idx) {
    ASSERT(0 <= idx);
    ASSERT(idx < MAX_VALUES);
    ASSERT(g_context.sim != NULL);
    ASSERT(idx < g_context.sim->num_values);
    Vector2 result = {
        .x = g_context.value_interface_data[idx].x,
        .y = g_context.value_interface_data[idx].y,
    };
    return result;
}

bool parse_positions(char* input) {
    int idx = 0;
    while (input[0] != '\0') {
        g_context.value_interface_data[idx].x = consume_integer(&input);
        if (g_context.value_interface_data[idx].x < 0 || WINDOW_WIDTH < g_context.value_interface_data[idx].x) PARSE_ERROR();

        if (input[0] != ',') PARSE_ERROR();
        ++input;

        g_context.value_interface_data[idx].y = consume_integer(&input);
        if (g_context.value_interface_data[idx].y < 0 || WINDOW_WIDTH < g_context.value_interface_data[idx].y) PARSE_ERROR();

        if (input[0] == ',') {
            ++input;
            int value = consume_integer(&input);
            if (value < 0 || PS_MAX_PLACE_VALUE < value) PARSE_ERROR();
            g_context.sim->values[idx] = value;
        }
        
        if (input[0] != ';') PARSE_ERROR();
        ++input;
        ++idx;
    }
    return true;
err:
    return false;
}

void load_level(int level) {
    ASSERT(0 <= level && level < NUM_LEVELS);
    g_context.sim = ps_parse(g_levels[level].sim_data);
    ASSERT(g_context.sim != NULL);
    ASSERT(parse_positions(g_levels[level].place_data));
    g_context.current_level = level;
    g_context.simulation_started = false;
}

void handle_cursor_activity(void) {
    Vector2 mouse_pos = GetMousePosition();
    if (g_context.activity == CURSOR_CONSUMED) {
        g_context.activity = CURSOR_DEFAULT;
    }
    if (g_context.activity == CURSOR_MOVE_PLACE) {
        static const int GRID_SIZE = 10;
        if (g_context.selected_place == -2) {
            // Moving a multiselection.
            int x_grid_steps = (mouse_pos.x - g_context.drag_anchor.x) / GRID_SIZE;
            int y_grid_steps = (mouse_pos.y - g_context.drag_anchor.y) / GRID_SIZE;
            if (x_grid_steps != 0 || y_grid_steps != 0) {
                for (int i = 0; i < g_context.multiselect_count; ++i) {
                    g_context.value_interface_data[g_context.multiselect_places[i]].x += x_grid_steps * GRID_SIZE;
                    g_context.value_interface_data[g_context.multiselect_places[i]].y += y_grid_steps * GRID_SIZE;
                }
                g_context.drag_anchor.x += x_grid_steps * GRID_SIZE;
                g_context.drag_anchor.y += y_grid_steps * GRID_SIZE;
            }
        } else {
            ASSERT(0 <= g_context.selected_place);
            g_context.value_interface_data[g_context.selected_place].x = (((int) mouse_pos.x + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE;
            g_context.value_interface_data[g_context.selected_place].y = (((int) mouse_pos.y + GRID_SIZE / 2) / GRID_SIZE) * GRID_SIZE;
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            g_context.activity = CURSOR_CONSUMED;
        }
    }
    // Right click to cancel.
    if (g_context.activity == CURSOR_LINK_FROM || g_context.activity == CURSOR_LINK_TO) {
        if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            g_context.activity = CURSOR_CONSUMED;
        }
    }
    if (g_context.activity == CURSOR_LINK_FROM) {
        ASSERT(g_context.sim != NULL);
        ASSERT(0 <= g_context.selected_place);
        Vector2 source = place_location_v(g_context.selected_place);
        DrawLineEx(source, mouse_pos, 2, RED);
        Rectangle new_transition_rect = {
            .x = source.x + VALUE_SIZE * 2 - TRANSITION_WIDTH / 2,
            .y = source.y - TRANSITION_HEIGHT / 2,
            .width = TRANSITION_WIDTH,
            .height = TRANSITION_HEIGHT,
        };
        DrawRectangleRec(new_transition_rect, GRAY);
        if (CheckCollisionPointRec(mouse_pos, new_transition_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            struct PetriTransition* new_transition = ps_allocate_transition();
            new_transition->next_transition = g_context.sim->transition_list;
            g_context.sim->transition_list = new_transition;
            ps_add_link(g_context.sim, g_context.selected_place, new_transition, LINK_SOURCE);
            g_context.activity = CURSOR_CONSUMED;
        }
    }
    if (g_context.activity == CURSOR_LINK_TO) {
        ASSERT(0 <= g_context.selected_place);
        Vector2 source = place_location_v(g_context.selected_place);
        DrawLineEx(source, mouse_pos, 2, BLUE);
        Rectangle new_transition_rect = {
            .x = source.x - VALUE_SIZE * 2 - TRANSITION_WIDTH / 2,
            .y = source.y - TRANSITION_HEIGHT / 2,
            .width = TRANSITION_WIDTH,
            .height = TRANSITION_HEIGHT,
        };
        DrawRectangleRec(new_transition_rect, GRAY);
        if (CheckCollisionPointRec(mouse_pos, new_transition_rect) && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            struct PetriTransition* new_transition = ps_allocate_transition();
            new_transition->next_transition = g_context.sim->transition_list;
            g_context.sim->transition_list = new_transition;
            ps_add_link(g_context.sim, g_context.selected_place, new_transition, LINK_DESTINATION);
            g_context.activity = CURSOR_CONSUMED;
        }
    }

    // Track mouse dragging
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && g_context.activity == CURSOR_DEFAULT) {
        g_context.drag_anchor = mouse_pos;
    }
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && g_context.activity != CURSOR_DRAGGING) {
        g_context.drag_anchor = (Vector2){0, 0};
    }
    static const int MIN_DRAG_DISTANCE = 5;
    if (g_context.drag_anchor.x != 0
            && IsMouseButtonDown(MOUSE_LEFT_BUTTON)
            && g_context.activity == CURSOR_DEFAULT
            && MIN_DRAG_DISTANCE < Vector2Distance(g_context.drag_anchor, mouse_pos)) {
        for (int place = 0; place < g_context.sim->num_values; ++place) {
            if (CheckCollisionPointRec(g_context.drag_anchor, place_location_r(place))) {
                if (g_context.selected_place == -2) {
                    bool in_multiselect = false;
                    for (int i = 0; i < g_context.multiselect_count; ++i) {
                        if (g_context.multiselect_places[i] == place) {
                            in_multiselect = true;
                            break;
                        }
                    }
                    if (in_multiselect) {
                        g_context.activity = CURSOR_MOVE_PLACE;
                        break;
                    }
                }
                g_context.selected_place = place;
                g_context.activity = CURSOR_MOVE_PLACE;
                break;
            }
        }
        if (g_context.activity == CURSOR_DEFAULT) {
            g_context.activity = CURSOR_DRAGGING;
        }
    }
    if (g_context.activity == CURSOR_DRAGGING) {
        Rectangle selection = RectangleFromVectors(g_context.drag_anchor, mouse_pos); 
        DrawRectangleLinesEx(selection, 2, GRAY);
        if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) {
            g_context.activity = CURSOR_CONSUMED;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            g_context.multiselect_count = 0;
            for (int place = 0; place < g_context.sim->num_values; ++place) {
                if (CheckCollisionRecs(selection, place_location_r(place))) {
                    g_context.multiselect_places[(g_context.multiselect_count)++] = place;
                }
            }
            if (g_context.multiselect_count == 0) {
                g_context.activity = CURSOR_CONSUMED;
                g_context.selected_place = -1;
            } else if (g_context.multiselect_count == 1) {
                g_context.activity = CURSOR_CONSUMED;
                g_context.selected_place = g_context.multiselect_places[0];
                g_context.multiselect_count = 0;
            } else {
                g_context.activity = CURSOR_CONSUMED;
                g_context.selected_place = -2;
            }
        }
    }
    if (g_context.selected_place != -2) {
        g_context.multiselect_count = 0;
    }
}

void export_level() {
    if (g_context.sim == NULL) {
        return;
    }

    printf("Simulation data:\n");
    ps_export(g_context.sim);

    printf("Place data:\n");
    for (int i = 0; i < g_context.sim->num_values; ++i) {
        printf("%d,%d", g_context.value_interface_data[i].x, g_context.value_interface_data[i].y);
        if (0 < g_context.sim->values[i]) {
            printf(",%d;", (int)(g_context.sim->values[i]));
        } else {
            printf(";");
        }
    }
    printf("\n");
}

void draw_level_select(void) {
    Rectangle level_select_window = { 50, 50, WINDOW_WIDTH - 100, WINDOW_HEIGHT - 200 };
    if (GuiWindowBox(level_select_window, "Level Select")) {
        g_context.activity = CURSOR_CONSUMED;
    }

    Rectangle level_box = { 100, 100, DEFAULT_SPACING * 2, VALUE_SIZE };
    for (int level = 0; level < NUM_LEVELS; ++level) {
        char text[20] = {0};
        if (level == 0) {
            snprintf(text, sizeof(text), "Editor");
        } else {
            snprintf(text, sizeof(text), "Level %d", level);
        }
        if (GuiButton(level_box, text) && g_context.activity == CURSOR_LEVEL_SELECT) {
            load_level(level);
            g_context.activity = CURSOR_CONSUMED;
        }
        level_box.x += DEFAULT_SPACING * 3;
        if (WINDOW_WIDTH - 100 - DEFAULT_SPACING * 3 < level_box.x) {
            level_box.x = 100;
            level_box.y += DEFAULT_SPACING;
        }
    }
}

void draw_help_text(void) {
    char level_label[20] = {0};
    if (g_context.current_level == 0) {
        snprintf(level_label, sizeof(level_label), "Editor");
    } else {
        snprintf(level_label, sizeof(level_label), "Level %d", g_context.current_level);
    }
    GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_LEFT);
    GuiLabel((Rectangle) {DEFAULT_SPACING, DEFAULT_SPACING, 0, 0}, level_label);
}

void draw_user_interface(void) {
    static const int BUTTON_SPACING = DEFAULT_SPACING * 4;
    Rectangle button_rect = {
        .x = WIDE_SPACING,
        .y = WINDOW_HEIGHT - WIDE_SPACING,
        .width = DEFAULT_SPACING * 3,
        .height = VALUE_SIZE,
    };

    draw_help_text();

    GuiSetState(g_context.simulation_started ? STATE_DISABLED : STATE_NORMAL);
    bool new_place = GuiButton(button_rect, "New Place");
    button_rect.x += BUTTON_SPACING;
    GuiSetState(STATE_NORMAL);

    GuiSetState(g_context.simulation_started ? STATE_NORMAL : STATE_DISABLED);
    bool reset = GuiButton(button_rect, "Reset");
    button_rect.x += BUTTON_SPACING;
    GuiSetState(STATE_NORMAL);

    bool step = GuiButton(button_rect, "Step");
    button_rect.x += BUTTON_SPACING;

    char* autorun_text = g_context.auto_run ? "Autorun Off" : "Autorun On";
    bool toggle_autorun = GuiButton(button_rect, autorun_text);
    button_rect.x += BUTTON_SPACING;

    bool export = GuiButton(button_rect, "Export");
    button_rect.x += BUTTON_SPACING;

    bool level_select = GuiButton(button_rect, "Level Select");
    button_rect.x += BUTTON_SPACING;

    if (toggle_autorun) {
        g_context.auto_run = !g_context.auto_run;
    }

    if (!g_context.simulation_started && (step || toggle_autorun)) {
        printf("Starting simulation\n");
        for (int i = 0; i < MAX_VALUES; ++i) {
            g_context.initial_values[i] = g_context.sim->values[i];
        }
        g_context.simulation_started = true;
    }

    if (reset && g_context.simulation_started) {
        printf("Stopping simulation\n");
        for (int i = 0; i < MAX_VALUES; ++i) {
            g_context.sim->values[i] = g_context.initial_values[i];
        }
        g_context.auto_run = false;
        g_context.simulation_started = false;
        g_context.sim->last_rng = 0;
    }

    if (step || g_context.auto_run) {
        ps_step(g_context.sim);
    }

    if (export) {
        export_level();
    }

    if (level_select) {
        g_context.activity = CURSOR_LEVEL_SELECT;
    }

    ps_mark_valid_transitions(g_context.sim, NULL);

    if (new_place) {
        ASSERT(!g_context.simulation_started);
        ASSERT(g_context.sim->num_values < MAX_VALUES);
        int place = g_context.sim->num_values++;
        g_context.sim->values[place] = 0;
        g_context.selected_place = place;
        g_context.activity = CURSOR_MOVE_PLACE;
    }

    for (struct PetriTransition* transition = g_context.sim->transition_list;
            transition != NULL;
            transition = transition->next_transition) {
        Vector2 summed_connected_positions = {0};
        int count_connected_positions = 0;
        for (struct PetriTransition* current_space = transition;
                current_space != NULL;
                current_space = current_space->more_space) {
            if (0 <= current_space->source) {
                summed_connected_positions = Vector2Add(summed_connected_positions, place_location_v(current_space->source));
                ++count_connected_positions;
            }
            if (0 <= current_space->destination) {
                summed_connected_positions = Vector2Add(summed_connected_positions, place_location_v(current_space->destination));
                ++count_connected_positions;
            }
        }
        ASSERT(0 < count_connected_positions);
        Vector2 transition_location = {
            .x = summed_connected_positions.x / count_connected_positions,
            .y = summed_connected_positions.y / count_connected_positions
        };
        if (count_connected_positions == 1 && 0 <= transition->source) {
            transition_location = Vector2Add((Vector2){VALUE_SIZE * 2, 0}, transition_location);
        }
        if (count_connected_positions == 1 && 0 <= transition->destination) {
            transition_location = Vector2Add((Vector2){-VALUE_SIZE * 2, 0}, transition_location);
        }

        // Check if connections to/from this transition can be deleted.
        bool can_delete = true;
        for (struct PetriTransition* current_space = transition;
                current_space != NULL;
                current_space = current_space->more_space) {
            if (0 <= current_space->source && current_space->source < g_context.sim->num_locked_values) {
                can_delete = false;
                break;
            }
            if (0 <= current_space->destination && current_space->destination < g_context.sim->num_locked_values) {
                can_delete = false;
                break;
            }
        }
        if (g_context.simulation_started || g_context.activity != CURSOR_DEFAULT || 0 <= g_context.selected_place) {
            can_delete = false;
        }

        // Draw lines from connections to transtion
        for (struct PetriTransition* current_space = transition;
                current_space != NULL;
                current_space = current_space->more_space) {
            if (0 <= current_space->source) {
                Vector2 source = place_location_v(current_space->source);
                Vector2 dest = transition_location;
                if (ps_has_destination(transition, current_space->source)) {
                    source.y += 4;
                    dest.y += 4;
                }
                DrawLineEx(source, dest, 2, RED);
                Vector2 midpoint = Vector2Midpoint(source, dest);
                Vector2 mouse = GetMousePosition();
                if (can_delete && Vector2Distance(midpoint, mouse) < DEFAULT_SPACING) {
                    bool delete = GuiButton((Rectangle){midpoint.x - 6, midpoint.y - 6, 12, 12}, "x");\
                    if (delete) {
                        g_context.activity = CURSOR_CONSUMED;
                        current_space->source = -1;
                        can_delete = false;
                    }
                }
            }
            if (0 <= current_space->destination) {
                Vector2 source = transition_location;
                Vector2 dest = place_location_v(current_space->destination);
                if (ps_has_source(transition, current_space->destination)) {
                    source.y -= 4;
                    dest.y -= 4;
                }
                DrawLineEx(source, dest, 2, BLUE);
                Vector2 midpoint = Vector2Midpoint(source, dest);
                Vector2 mouse = GetMousePosition();
                if (can_delete && Vector2Distance(midpoint, mouse) < DEFAULT_SPACING) {
                    bool delete = GuiButton((Rectangle){midpoint.x - 6, midpoint.y - 6, 12, 12}, "x");
                    if (delete) {
                        g_context.activity = CURSOR_CONSUMED;
                        current_space->destination = -1;
                        can_delete = false;
                    }
                }
            }
        }
        
        Rectangle transition_rect = {
            .x = transition_location.x - TRANSITION_WIDTH / 2,
            .y = transition_location.y - TRANSITION_HEIGHT / 2,
            .width = TRANSITION_WIDTH,
            .height = TRANSITION_HEIGHT,
        };
        Color transition_color = transition->next_valid_transition == NULL
                ? BLACK : GREEN;
        DrawRectangleRec(transition_rect, transition_color);
        
        // Check if the transition was clicked
        Vector2 mouse_pos = GetMousePosition();
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(mouse_pos, transition_rect)) {
            if (g_context.activity == CURSOR_LINK_FROM) {
                ps_add_link(g_context.sim, g_context.selected_place, transition, LINK_SOURCE);
                g_context.activity = CURSOR_CONSUMED;
            }
            if (g_context.activity == CURSOR_LINK_TO) {
                ps_add_link(g_context.sim, g_context.selected_place, transition, LINK_DESTINATION);
                g_context.activity = CURSOR_CONSUMED;
            }
        }
    }

    // Updating place selection is delayed so that we can canel the operation if
    // a button on top of it in the UI is clicked.
    int new_selected_place = -1;
    for (int i = 0; i < g_context.sim->num_values; i++) {
        int value = g_context.sim->values[i];
        char value_text[20];
        snprintf(value_text, sizeof(value_text), "%d", value);

        Color color = BLUE;
        if (i == 0) {
            color = GREEN;
        } else if (i < g_context.sim->num_locked_values) {
            color = BLACK;
        }
        DrawCircleV(place_location_v(i), VALUE_SIZE / 2 + 3, color);
        DrawCircleV(place_location_v(i), VALUE_SIZE / 2, WHITE);

        GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
        bool place_clicked = GuiLabelButton(place_location_r(i), value_text);
        if (place_clicked && g_context.activity == CURSOR_DEFAULT) {
            new_selected_place = i;
        }
        bool locked = i < g_context.sim->num_locked_values;
        if (place_clicked
                && g_context.activity == CURSOR_LINK_TO
                && !locked
                && i != g_context.selected_place) {
            struct PetriTransition* new_transition = ps_allocate_transition();
            new_transition->next_transition = g_context.sim->transition_list;
            g_context.sim->transition_list = new_transition;
            ps_add_link(g_context.sim, g_context.selected_place, new_transition, LINK_DESTINATION);
            ps_add_link(g_context.sim, i, new_transition, LINK_SOURCE);
            g_context.activity = CURSOR_CONSUMED;
        }
        if (place_clicked
                && g_context.activity == CURSOR_LINK_FROM
                && !locked
                && i != g_context.selected_place) {
            struct PetriTransition* new_transition = ps_allocate_transition();
            new_transition->next_transition = g_context.sim->transition_list;
            g_context.sim->transition_list = new_transition;
            ps_add_link(g_context.sim, g_context.selected_place, new_transition, LINK_SOURCE);
            ps_add_link(g_context.sim, i, new_transition, LINK_DESTINATION);
            g_context.activity = CURSOR_CONSUMED;
        }
        if (i == g_context.selected_place) {
            Rectangle label_position = place_location_r(i);
            label_position.y += DEFAULT_SPACING + VALUE_SIZE;
            if (i < g_context.sim->num_locked_values) {
                GuiLabel(label_position, "This place is LOCKED. You cannot modify it or any connected transitions.");
                label_position.y += VALUE_SIZE;
            }
            if (i == 0) {
                GuiLabel(label_position, "This place is the GOAL. Increment it to beat the level.");
                label_position.y += VALUE_SIZE;
            }
        }
        bool selected = i == g_context.selected_place;
        for (int multiselect_idx = 0; multiselect_idx < g_context.multiselect_count; ++multiselect_idx) {
            if (g_context.multiselect_places[multiselect_idx] == i) {
                selected = true;
            }
        }
        if (selected) {
            Rectangle outline = place_location_r(i);
            outline.x -= 5;
            outline.y -= 5;
            outline.width += 10;
            outline.height += 10;
            DrawRectangleLinesEx(outline, 1.0, GRAY);
        }
    }

    if (0 <= g_context.selected_place && g_context.activity == CURSOR_DEFAULT) {
        Rectangle rect = place_location_r(g_context.selected_place);
        bool increment = false;
        bool decrement = false;
        bool link_as_source = false;
        bool link_as_dest = false;
        bool move = false;
        bool cancel = false;
        if (g_context.simulation_started || g_context.selected_place < g_context.sim->num_locked_values) {
            move = GuiButton((Rectangle){ rect.x, rect.y + DEFAULT_SPACING, 24, 24 }, "M");
            cancel = GuiButton((Rectangle){ rect.x + DEFAULT_SPACING, rect.y + DEFAULT_SPACING, 24, 24 }, "x");
        } else {
            link_as_dest = GuiButton((Rectangle){ rect.x, rect.y + DEFAULT_SPACING, 24, 24}, ">");
            link_as_source = GuiButton((Rectangle){ rect.x + DEFAULT_SPACING, rect.y + DEFAULT_SPACING, 24, 24}, "<");
            increment = GuiButton((Rectangle){ rect.x + DEFAULT_SPACING * 2, rect.y + DEFAULT_SPACING, 24, 24 }, "+");
            decrement = GuiButton((Rectangle){ rect.x + DEFAULT_SPACING * 3, rect.y + DEFAULT_SPACING, 24, 24 }, "-");
            move = GuiButton((Rectangle){ rect.x + DEFAULT_SPACING * 4, rect.y + DEFAULT_SPACING, 24, 24 }, "M");
            cancel = GuiButton((Rectangle){ rect.x + DEFAULT_SPACING * 5, rect.y + DEFAULT_SPACING, 24, 24 }, "x");
        }
        if (increment) {
            g_context.sim->values[g_context.selected_place] = JMIN(g_context.sim->values[g_context.selected_place] + 1, PS_MAX_PLACE_VALUE);
            g_context.activity = CURSOR_CONSUMED;
        }
        if (decrement) {
            g_context.sim->values[g_context.selected_place] = JMAX(g_context.sim->values[g_context.selected_place] - 1, 0);
            g_context.activity = CURSOR_CONSUMED;
        }
        if (move) {
            g_context.activity = CURSOR_MOVE_PLACE;
        }
        if (link_as_source) {
            g_context.activity = CURSOR_LINK_TO;
        }
        if (link_as_dest) {
            g_context.activity = CURSOR_LINK_FROM;
        }
        if (cancel) {
            g_context.selected_place = -1;
            g_context.activity = CURSOR_CONSUMED;
        }
        if (increment || decrement || link_as_source || link_as_dest || move || cancel) {
            new_selected_place = -1;
        }
    }

    if (0 <= new_selected_place) {
        g_context.selected_place = new_selected_place;
        g_context.activity = CURSOR_CONSUMED;
    }
    
    if (g_context.activity == CURSOR_LEVEL_SELECT) {
        draw_level_select();
    }
}

void loop(void) {
    if (!g_context_init) {
        g_context.selected_place = -1;
        g_context.activity = CURSOR_DEFAULT;
        for (int i = 0; i < MAX_VALUES; i++) {
            g_context.value_interface_data[i].x = 0;
            g_context.value_interface_data[i].y = 0;
        }
        g_context.simulation_started = false;
        g_context.auto_run = false;
        g_context_init = true;
        load_level(1);
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    handle_cursor_activity();
    draw_user_interface();
    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) && g_context.activity == CURSOR_DEFAULT) {
        g_context.selected_place = -1;
        g_context.multiselect_count = 0;
    }
    EndDrawing();
}

int main(int argc, char** argv) {

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Petri Demo");
    SetTargetFPS(60);

    emscripten_set_main_loop(loop, 0, 1);
    CloseWindow();
    return 0;
}
