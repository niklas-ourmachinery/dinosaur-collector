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
#include <plugins/ui/ui_renderer.h>

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

struct tm_simulate_state_o {
    tm_allocator_i* allocator;
    uint32_t money;
    uint32_t background_image;
};

static tm_simulate_state_o* start(tm_simulate_start_args_t* args)
{
    TM_STATIC_ASSERT(sizeof(tm_simulate_state_o) < RESERVE_STATE_BYTES);

    tm_simulate_state_o* state = tm_alloc(args->allocator, RESERVE_STATE_BYTES);
    memset(state, 0, RESERVE_STATE_BYTES);
    *state = (tm_simulate_state_o){
        .allocator = args->allocator,
        .money = 112,
    };

    const tm_tt_id_t background_asset = tm_the_truth_assets_api->asset_from_path(args->tt, args->asset_root, "art/backgrounds/background.creation");
    if (TM_ASSERT(background_asset.u64, tm_error_api->def, "Image not found")) {
        const tm_tt_id_t background = tm_the_truth_api->get_subobject(args->tt, tm_tt_read(args->tt, background_asset), TM_TT_PROP__ASSET__OBJECT);
        // TODO: Pipe a proper device_affinity_mask?
        tm_creation_graph_context_t ctx = (tm_creation_graph_context_t){ .rb = args->render_backend, .device_affinity_mask = TM_RENDERER_DEVICE_AFFINITY_MASK_ALL, .tt = args->tt };
        tm_creation_graph_instance_t inst = tm_creation_graph_api->create_instance(background, &ctx);
        tm_creation_graph_output_t output = tm_creation_graph_api->output(&inst, TM_CREATION_GRAPH__IMAGE__OUTPUT_NODE_HASH, &ctx, 0);
        const tm_creation_graph_image_data_t* cg_image = (tm_creation_graph_image_data_t*)output.output;
        tm_renderer_handle_t image = cg_image->handle;
        state->background_image = tm_ui_renderer_api->allocate_image_slot(args->ui_renderer);
        tm_ui_renderer_api->set_image(args->ui_renderer, state->background_image, image);
    }

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

    // Placeholder sky and ground.
    const tm_rect_t sky_r = tm_rect_divide_y(args->rect, 0, 2, 0);
    const tm_rect_t ground_r = tm_rect_divide_y(args->rect, 0, 2, 1);
    style->color = (tm_color_srgb_t){ .r = 200, .g = 200, .b = 255, .a = 255 };
    tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, style, sky_r);
    style->color = (tm_color_srgb_t){ .r = 50, .g = 100, .b = 50, .a = 255 };
    tm_draw2d_api->fill_rect(uib.vbuffer, *uib.ibuffers, style, ground_r);

    style->color = (tm_color_srgb_t){ 255, 255, 255, 255 };
    tm_draw2d_api->textured_rect(uib.vbuffer, *uib.ibuffers, style, args->rect, state->background_image, (tm_rect_t){ 0, 0, 1, 1 });
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
    char money_str[64];
    sprintf(money_str, "Score: %d", state->money);
    tm_ui_api->text(args->ui, args->uistyle, &(tm_ui_text_t){ .rect = money_amount_r, .text = money_str, .color = &(tm_color_srgb_t){ .r = 255, .a = 255 } });
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
    tm_the_truth_assets_api = reg->get(TM_THE_TRUTH_ASSETS_API_NAME);
    tm_creation_graph_api = reg->get(TM_CREATION_GRAPH_API_NAME);
    tm_ui_renderer_api = reg->get(TM_UI_RENDERER_API_NAME);
    tm_error_api = reg->get(TM_ERROR_API_NAME);
    tm_the_truth_api = reg->get(TM_THE_TRUTH_API_NAME);
}
