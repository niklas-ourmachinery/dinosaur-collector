#include <foundation/allocator.h>
#include <foundation/api_registry.h>
#include <foundation/error.h>
#include <foundation/rect.inl>
#include <foundation/the_truth.h>
#include <foundation/the_truth_assets.h>

#include <plugins/creation_graph/creation_graph.h>
#include <plugins/creation_graph/creation_graph_output.inl>
#include <plugins/creation_graph/image_nodes.h>
#include <plugins/renderer/render_backend.h>
#include <plugins/renderer/renderer_api_types.h>
#include <plugins/simulate/simulate_entry.h>
#include <plugins/ui/draw2d.h>
#include <plugins/ui/ui.h>
#include <plugins/ui/ui_custom.h>
#include <plugins/ui/ui_renderer.h>

#include <math.h>
#include <memory.h>
#include <stdio.h>

static struct tm_ui_api* tm_ui_api;
static struct tm_draw2d_api* tm_draw2d_api;
static struct tm_the_truth_assets_api* tm_the_truth_assets_api;
static struct tm_creation_graph_api* tm_creation_graph_api;
static struct tm_ui_renderer_api* tm_ui_renderer_api;
static struct tm_error_api* tm_error_api;
static struct tm_the_truth_api* tm_the_truth_api;

// Reserve this many bytes for the state, so that we can grow it while hot-reloading.
enum { RESERVE_STATE_BYTES = 32 * 1024 };

enum IMAGES {
    MISSING,

    BACKGROUND,

    ANKYLOSAURUS,

    ALBUM,
    BACK,
    BONE,
    CLOSE,
    INVENTORY,
    MENU,
    MENU_BACKGROUND,
    SHOP,
    SQUARE,

    NUM_IMAGES,
};

const char* image_paths[NUM_IMAGES] = {
    [MISSING] = "art/icons/missing.creation",

    [BACKGROUND] = "art/backgrounds/background.creation",

    [ANKYLOSAURUS] = "art/dinosaurs/ankylosaurus.creation",

    [ALBUM] = "art/icons/album.creation",
    [BACK] = "art/icons/back.creation",
    [BONE] = "art/icons/bone.creation",
    [CLOSE] = "art/icons/close.creation",
    [INVENTORY] = "art/icons/inventory.creation",
    [MENU] = "art/icons/menu.creation",
    [MENU_BACKGROUND] = "art/icons/menu_background.creation",
    [SHOP] = "art/icons/shop.creation",
    [SQUARE] = "art/icons/square.creation",
};

enum STATE {
    STATE__MAIN,
    STATE__MENU,
    STATE__INVENTORY,
    STATE__SHOP,
    STATE__ALBUM,
};

struct tm_simulate_state_o {
    tm_allocator_i* allocator;
    uint32_t money;
    uint32_t images[NUM_IMAGES];
    uint32_t state;
};

static uint32_t load_image(tm_simulate_start_args_t* args, const char* asset_path)
{
    const tm_tt_id_t asset = tm_the_truth_assets_api->asset_from_path(args->tt, args->asset_root, asset_path);
    if (!TM_ASSERT(asset.u64, tm_error_api->def, "Image not found `%s`", asset_path))
        return 0;

    const tm_tt_id_t object = tm_the_truth_api->get_subobject(args->tt, tm_tt_read(args->tt, asset), TM_TT_PROP__ASSET__OBJECT);
    // TODO: Pipe a proper device_affinity_mask?
    tm_creation_graph_context_t ctx = (tm_creation_graph_context_t){ .rb = args->render_backend, .device_affinity_mask = TM_RENDERER_DEVICE_AFFINITY_MASK_ALL, .tt = args->tt };
    tm_creation_graph_instance_t inst = tm_creation_graph_api->create_instance(object, &ctx);
    tm_creation_graph_output_t output = tm_creation_graph_api->output(&inst, TM_CREATION_GRAPH__IMAGE__OUTPUT_NODE_HASH, &ctx, 0);
    const tm_creation_graph_image_data_t* cg_image = (tm_creation_graph_image_data_t*)output.output;
    const uint32_t image = tm_ui_renderer_api->allocate_image_slot(args->ui_renderer);
    tm_ui_renderer_api->set_image(args->ui_renderer, image, cg_image->handle);
    return image;
}

static tm_simulate_state_o* start(tm_simulate_start_args_t* args)
{
    TM_STATIC_ASSERT(sizeof(tm_simulate_state_o) < RESERVE_STATE_BYTES);

    tm_simulate_state_o* state = tm_alloc(args->allocator, RESERVE_STATE_BYTES);
    memset(state, 0, RESERVE_STATE_BYTES);
    *state = (tm_simulate_state_o){
        .allocator = args->allocator,
        .money = 112,
    };

    for (uint32_t i = 0; i < NUM_IMAGES; ++i)
        state->images[i] = load_image(args, image_paths[i]);

    return state;
}

static void stop(tm_simulate_state_o* state)
{
    tm_allocator_i a = *state->allocator;
    tm_free(&a, state, RESERVE_STATE_BYTES);
}

static void background(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->include_alpha = true;
    style->color = (tm_color_srgb_t){ 255, 255, 255, 255 };

    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, args->rect, state->images[BACKGROUND], (tm_rect_t){ 0, 0, 1, 1 });
}

static void dinosaurs(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->include_alpha = true;
    style->color = (tm_color_srgb_t){ 255, 255, 255, 255 };

    const float x = (float)fmod(args->time * 200, args->rect.w + 300) - 300;
    const tm_rect_t ankylo_r = { x, tm_rect_bottom(args->rect) - 200, 200, 100 };
    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, ankylo_r, state->images[ANKYLOSAURUS], (tm_rect_t){ 0, 0, 1, 1 });
}

static void money(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->color = (tm_color_srgb_t){ .r = 255, .g = 255, .b = 255, .a = 255 };
    style->include_alpha = true;
    const float unit = tm_min(args->rect.w, args->rect.h);

    const float icon_size = 0.08f * unit;
    const float font_scale = icon_size / 18.0f;

    const tm_rect_t inset_r = tm_rect_inset(args->rect, 5, 5);
    const tm_rect_t money_symbol_r = tm_rect_split_bottom(tm_rect_split_left(inset_r, icon_size, 0, 0), icon_size, 0, 1);
    const tm_rect_t money_amount_r = { .x = tm_rect_right(money_symbol_r) + 10, .y = money_symbol_r.y - 2, .w = args->rect.w, .h = icon_size };
    args->uistyle->font_scale = font_scale;
    char money_str[64];
    sprintf(money_str, "%d", state->money);
    const tm_rect_t metrics_r = tm_ui_api->text_metrics(args->ui, args->uistyle, money_str);
    const tm_rect_t draw_r = { .y = money_symbol_r.y, .w = money_amount_r.x + metrics_r.w, .h = args->rect.h - money_symbol_r.y };
    const tm_rect_t background_r = tm_rect_inset(draw_r, -5, -5);

    tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, style, background_r);
    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, money_symbol_r, state->images[BONE], (tm_rect_t){ 0, 0, 1, 1 });
    style->color = (tm_color_srgb_t){ .a = 255 };
    tm_ui_api->text(args->ui, args->uistyle, &(tm_ui_text_t){ .rect = money_amount_r, .text = money_str, .color = &style->color });
}

#define HEXCOLOR(c) ((tm_color_srgb_t){ .a = 255, .r = 0xff & (c >> 16), .g = 0xff & (c >> 8), .b = 0xff & (c >> 0) })

static bool button(tm_simulate_state_o* state, tm_simulate_frame_args_t* args, tm_rect_t r, const uint32_t image_idx)
{
    const uint64_t id = tm_ui_api->make_id(args->ui);
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->color = (tm_color_srgb_t){ .r = 255, .g = 255, .b = 255, .a = 255 };
    style->include_alpha = true;

    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, r, state->images[image_idx], (tm_rect_t){ 0, 0, 1, 1 });

    if (tm_ui_api->is_hovering(args->ui, r, 0))
        uib.activation->next_hover = id;

    return (uib.activation->hover == id && uib.input->left_mouse_pressed);
}

static void menu(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    tm_ui_buffers_t uib = tm_ui_api->buffers(args->ui);
    tm_draw2d_style_t style[1] = { 0 };
    tm_ui_api->to_draw_style(args->ui, style, args->uistyle);
    style->color = (tm_color_srgb_t){ .r = 255, .g = 255, .b = 255, .a = 255 };
    style->include_alpha = true;
    const float unit = tm_min(args->rect.w, args->rect.h);

    const float icon_size = 0.15f * unit;
    const tm_rect_t inset_r = tm_rect_inset(args->rect, 5, 5);
    const tm_rect_t menu_icon_r = tm_rect_split_top(tm_rect_split_left(inset_r, icon_size, 0, 0), icon_size, 0, 0);

    tm_rect_t rect = args->rect;
    if (state->state == STATE__MAIN) {
        if (button(state, args, menu_icon_r, MENU))
            state->state = STATE__MENU;
        return;
    }

    if (button(state, args, menu_icon_r, CLOSE))
        state->state = STATE__MAIN;

    const tm_rect_t menu_r = tm_rect_center_in(0.8f * unit, 0.8f * unit, args->rect);
    const tm_rect_t close_r = tm_rect_center_in(0.1f * unit, 0.1f * unit, (tm_rect_t){ menu_r.x + 0.02f * unit, menu_r.y + 0.02f * unit });

    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, menu_r, state->images[MENU_BACKGROUND], (tm_rect_t){ 0, 0, 1, 1 });

    if (button(state, args, close_r, state->state == STATE__MENU ? CLOSE : BACK))
        state->state = state->state == STATE__MENU ? STATE__MAIN : STATE__MENU;
    const tm_rect_t menu_inset_r = tm_rect_inset(menu_r, 0.05f * unit, 0.05f * unit);
    rect = menu_inset_r;

    if (state->state == STATE__MENU) {
        for (uint32_t i = 0; i < 3; ++i) {
            const tm_rect_t row_r = tm_rect_divide_y(rect, 0.04f * unit, 3, i);
            for (uint32_t j = 0; j < 3; ++j) {
                const tm_rect_t icon_r = tm_rect_divide_x(row_r, 0.04f * unit, 3, j);
                const uint32_t idx = i * 3 + j;
                switch (idx) {
                case 0:
                    if (button(state, args, icon_r, INVENTORY))
                        state->state = STATE__INVENTORY;
                    break;
                case 1:
                    if (button(state, args, icon_r, SHOP))
                        state->state = STATE__SHOP;
                    break;
                case 2:
                    if (button(state, args, icon_r, ALBUM))
                        state->state = STATE__ALBUM;
                    break;
                }
            }
        }
    }
}

static void main_screen(tm_simulate_state_o* state, tm_simulate_frame_args_t* args)
{
    background(state, args);
    dinosaurs(state, args);
    money(state, args);
    menu(state, args);
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
    tm_the_truth_assets_api = reg->get(TM_THE_TRUTH_ASSETS_API_NAME);
    tm_creation_graph_api = reg->get(TM_CREATION_GRAPH_API_NAME);
    tm_ui_renderer_api = reg->get(TM_UI_RENDERER_API_NAME);
    tm_error_api = reg->get(TM_ERROR_API_NAME);
    tm_the_truth_api = reg->get(TM_THE_TRUTH_API_NAME);
}
