#include <foundation/allocator.h>
#include <foundation/api_registry.h>
#include <foundation/rect.inl>

#include <plugins/simulate/simulate_entry.h>
#include <plugins/ui/draw2d.h>
#include <plugins/ui/ui.h>

static struct tm_ui_api* tm_ui_api;
static struct tm_draw2d_api* tm_draw2d_api;

struct tm_simulate_state_o {
    // Store state here.

    tm_allocator_i* allocator;
} tm_gameplay_state_o;

static tm_simulate_state_o* start(tm_simulate_start_args_t* args)
{
    tm_simulate_state_o* state = tm_alloc(args->allocator, sizeof(*state));
    *state = (tm_simulate_state_o){
        .allocator = args->allocator,
    };

    // Setup stuff at beginning of simulation.

    return state;
}

static void stop(tm_simulate_state_o* state)
{
    // Clean up when simulation ends.

    tm_allocator_i a = *state->allocator;
    tm_free(&a, state, sizeof(*state));
}

static void background(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);

    // Placeholder sky and ground.
    const tm_rect_t sky_r = tm_rect_divide_y(args->rect, 0, 2, 0);
    const tm_rect_t ground_r = tm_rect_divide_y(args->rect, 0, 2, 1);
    style->color = (tm_color_srgb_t){ .r = 200, .g = 200, .b = 255, .a = 255 };
    tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, style, sky_r);
    style->color = (tm_color_srgb_t){ .r = 50, .g = 100, .b = 50, .a = 255 };
    tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, style, ground_r);
}

static void money(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);

    const tm_rect_t inset_r = tm_rect_inset(args->rect, 5, 5);
    const tm_rect_t money_symbol_r = tm_rect_split_bottom(tm_rect_split_left(inset_r, 30, 0, 0), 30, 0, 1);
    style->color = (tm_color_srgb_t){ .r = 255, .a = 255 };
    tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, style, money_symbol_r);

    const tm_rect_t money_amount_r = { .x = tm_rect_right(money_symbol_r) + 10, .y = money_symbol_r.y - 2, .w = args->rect.w, .h = 30 };
    args->uistyle->font_scale = 2.9f;
    tm_ui_api->text(args->ui, args->uistyle, &(tm_ui_text_t){ .rect = money_amount_r, .text = "299", .color = &(tm_color_srgb_t){ .r = 255, .a = 255 } });
}

static void main_screen(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    background(state, args);
    money(state, args);
}

// Called once a frame.
static void tick(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    main_screen(state, args);
    money(state, args);
}

static tm_simulate_entry_i simulate_entry_i = {
    // Change this and re-run hash.exe if you wish to change the unique identifier
    .id = TM_STATIC_HASH("tm_dinosaur_simulate", 0xcc5e7b0d0f04fed9ULL),
    .display_name = "Dinosaur Simulate",
    .start = start,
    .stop = stop,
    .tick = tick,
};

TM_DLL_EXPORT void tm_load_plugin(struct tm_api_registry_api* reg, bool load)
{
    tm_add_or_remove_implementation(reg, load, TM_SIMULATE_ENTRY_INTERFACE_NAME, &simulate_entry_i);

    tm_ui_api = reg->get(TM_UI_API_NAME);
    tm_draw2d_api = reg->get(TM_DRAW2D_API_NAME);
}
