#include "text_renderer.hpp"

#include "../datastructures/string.hpp"
#include "../rendering/opengl_state.hpp"
#include "../utility/utils.hpp"
#include "shader_program.hpp"
#include "texture.hpp"
#include "mesh_gpu_data.hpp"

/*
    TODO:
        * Think about how to use the text renderer api
        * Add color to text rendering
        * Maybe have a background shadow rendering
        * Check how good distance field looks on smol text sizes (Pretty good actually lol)
        * Maybe have a minimum text size (Think about text renderer api (Placement, color...))
        * If SDF text-rendering doesn't do to well on small scales, try having a fixed small size texture atlas
*/

struct FontVertex
{
    vec2 position;
    vec2 uv_coords;
    vec3 color;
    float pixel_size;
    FontVertex(const vec2& pos, const vec2& uv_coords, vec3 color, float pixel_size)
        : position(pos), uv_coords(uv_coords), pixel_size(pixel_size), color(color) {};
    FontVertex() {};
};

struct TextRenderer
{
    // Atlas data
    GlyphAtlas glyph_atlas;
    Texture atlas_bitmap_texture;
    Texture atlas_sdf_texture;

    // Shaders
    ShaderProgram* bitmap_shader;
    ShaderProgram* sdf_shader;

    // Gpu data
    Mesh_GPU_Data font_mesh;
    DynamicArray<FontVertex> text_vertices;
    DynamicArray<GLuint> text_indices;

    // Text positioning cache
    TextLayout text_layout;
    vec3 default_color;

    // Window data
    int screen_width;
    int screen_height;
};


TextLayout text_layout_create()
{
    TextLayout info;
    info.character_positions = dynamic_array_create_empty<Character_Position>(512);
    return info;
}

void text_layout_destroy(TextLayout* info) {
    dynamic_array_destroy(&info->character_positions);
}

TextRenderer* text_renderer_create_from_font_atlas_file(OpenGLState* state, FileListener* listener, const char* font_filepath, int screen_width, int screen_height)
{
    TextRenderer* text_renderer = new TextRenderer();
    text_renderer->text_layout = text_layout_create();
    text_renderer->screen_width = screen_width;
    text_renderer->screen_height = screen_height;
    text_renderer->glyph_atlas = optional_unwrap(glyph_atlas_create_from_atlas_file(font_filepath));
    text_renderer->default_color = vec3(1.0f);

    /*
    // Create Font File
    text_renderer->glyph_atlas = optional_unwrap(glyph_atlas_create_from_font_file("resources/consola.ttf", 128, 1600, 16, 8, true));
    glyph_atlas_save_as_file(&text_renderer->glyph_atlas, "resources/glyph_atlas.atlas");
    //text_renderer->glyph_atlas = optional_unwrap(glyph_atlas_create_from_font_file("resources/cour.ttf", 128, 1600, 16, 8, true));
    //glyph_atlas_save_as_file(&text_renderer->glyph_atlas, "resources/glyph_atlas_cour.atlas");
    //glyph_atlas_print_glyph_information(&text_renderer->glyph_atlas);
    */

    // Initialize shaders
    text_renderer->bitmap_shader = optional_unwrap(shader_program_create(listener, { "resources/shaders/font_bitmap.frag", "resources/shaders/font_bitmap.vert" }));
    text_renderer->sdf_shader = optional_unwrap(shader_program_create(listener, { "resources/shaders/font_sdf.frag", "resources/shaders/font_sdf.vert" }));
    // Initialize textures
    text_renderer->atlas_bitmap_texture = texture_create_from_texture_bitmap(
        &text_renderer->glyph_atlas.atlas_bitmap,
        texture_filtermode_make_nearest(),
        state
    );
    text_renderer->atlas_sdf_texture = texture_create_from_bytes(
        (byte*)text_renderer->glyph_atlas.atlas_distance_field.data,
        text_renderer->glyph_atlas.atlas_bitmap.width,
        text_renderer->glyph_atlas.atlas_bitmap.height,
        1,
        GL_FLOAT,
        texture_filtermode_make_linear(),
        state
    );
    // Initialize GPU data
    //Vertex_GPU_Buffer vertex_gpu_buffer = vertex_gpu_buffer_create_empty(sizeof(FontVertex)*1024, GL_DYNAMIC_DRAW);
    VertexAttributeInformation attribute_informations[] = {
        vertex_attribute_information_make(GL_FLOAT, 2, DefaultVertexAttributeLocation::POSITION_2D, 0, sizeof(FontVertex)),
        vertex_attribute_information_make(GL_FLOAT, 2, DefaultVertexAttributeLocation::TEXTURE_COORDINATE_2D, sizeof(vec2), sizeof(FontVertex)),
        vertex_attribute_information_make(GL_FLOAT, 3, 4, sizeof(vec2) + sizeof(vec2), sizeof(FontVertex)),
        vertex_attribute_information_make(GL_FLOAT, 1, 3, sizeof(vec2)*2 + sizeof(vec3), sizeof(FontVertex)),
    };
    text_renderer->font_mesh = mesh_gpu_data_create(
        state,
        gpu_buffer_create_empty(sizeof(FontVertex)*1024, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW),
        array_create_static(attribute_informations, 4),
        gpu_buffer_create_empty(sizeof(GLuint) * 1024 * 4, GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW),
        GL_TRIANGLES,
        0
    );

    text_renderer->text_vertices = dynamic_array_create_empty<FontVertex>(1024);
    text_renderer->text_indices = dynamic_array_create_empty<GLuint>(1024);

    return text_renderer;
}

void text_renderer_destroy(TextRenderer* renderer)
{
    text_layout_destroy(&renderer->text_layout);
    shader_program_destroy(renderer->bitmap_shader);
    shader_program_destroy(renderer->sdf_shader);
    mesh_gpu_data_destroy(&renderer->font_mesh);
    glyph_atlas_destroy(&renderer->glyph_atlas);
    dynamic_array_destroy(&renderer->text_vertices);
    dynamic_array_destroy(&renderer->text_indices);
    delete renderer;
}

vec2 text_renderer_get_scaling_factor(TextRenderer* renderer, float relative_height)
{
    // Glpyh information sizes (in 23.3 format) to normalized screen coordinates scaling factor
    GlyphAtlas* atlas = &renderer->glyph_atlas;
    float CHARACTER_HEIGHT_NORMALIZED = relative_height;
    const float scaling_factor_x = CHARACTER_HEIGHT_NORMALIZED / (atlas->ascender - atlas->descender) *
        ((float)renderer->screen_height / renderer->screen_width);
    const float scaling_factor_y = CHARACTER_HEIGHT_NORMALIZED / (atlas->ascender - atlas->descender);

    return vec2(scaling_factor_x, scaling_factor_y);
}

void text_renderer_add_text_from_layout(
    TextRenderer* renderer,
    TextLayout* layout,
    vec2 position
)
{
    GlyphAtlas* atlas = &renderer->glyph_atlas;
    vec2 scaling_factor = text_renderer_get_scaling_factor(renderer, layout->relative_height);
    float descender = atlas->descender * scaling_factor.y;
    float distance_field_scaling;
    {
        float line_pixel_size_in_atlas = ((atlas->ascender - atlas->descender) / 64.0f); // In pixel per line
        float line_size_on_screen = layout->relative_height * renderer->screen_height; // In pixel per line
        distance_field_scaling = line_size_on_screen / line_pixel_size_in_atlas;
    }

    for (int i = 0; i < layout->character_positions.size; i++)
    {
        Character_Position* char_pos = &renderer->text_layout.character_positions[i];
        GlyphInformation* glyph_info = char_pos->glyph_info;

        BoundingBox2 char_box;
        char_box.min.x =
            char_pos->bounding_box.min.x + position.x +
            glyph_info->bearing_x * scaling_factor.x;
        char_box.min.y =
            char_pos->bounding_box.min.y + position.y - descender +
            (glyph_info->bearing_y - glyph_info->glyph_height) * scaling_factor.y;
        char_box.max.x = char_box.min.x +
            glyph_info->glyph_width * scaling_factor.x;
        char_box.max.y = char_box.min.y +
            glyph_info->glyph_height * scaling_factor.y;

        // Push back 4 vertices for each glyph
        FontVertex bb_bottom_left(
            vec2(char_box.min.x, char_box.min.y),
            vec2(glyph_info->atlas_fragcoords_left, glyph_info->atlas_fragcoords_bottom),
            char_pos->color,
            distance_field_scaling
        );
        FontVertex bb_bottom_right(
            vec2(char_box.max.x, char_box.min.y),
            vec2(glyph_info->atlas_fragcoords_right, glyph_info->atlas_fragcoords_bottom),
            char_pos->color,
            distance_field_scaling
        );
        FontVertex bb_top_left(
            vec2(char_box.min.x, char_box.max.y),
            vec2(glyph_info->atlas_fragcoords_left, glyph_info->atlas_fragcoords_top),
            char_pos->color,
            distance_field_scaling
        );
        FontVertex bb_top_right(
            vec2(char_box.max.x, char_box.max.y),
            vec2(glyph_info->atlas_fragcoords_right, glyph_info->atlas_fragcoords_top),
            char_pos->color,
            distance_field_scaling
        );
        int quad_start_index = renderer->text_vertices.size;
        dynamic_array_push_back(&renderer->text_vertices, bb_bottom_left);
        dynamic_array_push_back(&renderer->text_vertices, bb_bottom_right);
        dynamic_array_push_back(&renderer->text_vertices, bb_top_right);
        dynamic_array_push_back(&renderer->text_vertices, bb_top_left);
        // Push back 6 indices for each glyph
        dynamic_array_push_back(&renderer->text_indices, (GLuint)quad_start_index + 0);
        dynamic_array_push_back(&renderer->text_indices, (GLuint)quad_start_index + 1);
        dynamic_array_push_back(&renderer->text_indices, (GLuint)quad_start_index + 2);
        dynamic_array_push_back(&renderer->text_indices, (GLuint)quad_start_index + 0);
        dynamic_array_push_back(&renderer->text_indices, (GLuint)quad_start_index + 2);
        dynamic_array_push_back(&renderer->text_indices, (GLuint)quad_start_index + 3);
    }
}

void text_renderer_add_text(
    TextRenderer* renderer,
    String* text,
    vec2 position,
    float relative_height,
    float line_gap_percent)
{
    // Loop over all glyphs
    TextLayout* layout = text_renderer_calculate_text_layout(renderer, text, relative_height, line_gap_percent);
    text_renderer_add_text_from_layout(renderer, layout, position);
}

TextLayout* text_renderer_calculate_text_layout(
    TextRenderer* renderer,
    String* text,
    float relative_height,
    float line_gap_percent)
{
    GlyphAtlas* atlas = &renderer->glyph_atlas;
    vec2 scaling_factor = text_renderer_get_scaling_factor(renderer, relative_height);
    renderer->text_layout.relative_height = relative_height;

    float max_cursor_x = 0.0f;
    float cursor_x = 0.0f;
    //float cursor_y = -atlas->descender * scaling_factor.y;
    float cursor_y = 0.0f;

    dynamic_array_reset(&renderer->text_layout.character_positions);
    for (int i = 0; i < text->size; i++)
    {
        byte current_character = (*text)[i];
        if (current_character == '\n') {
            cursor_y -= relative_height * line_gap_percent;
            if (cursor_x > max_cursor_x) {
                max_cursor_x = cursor_x;
            }
            cursor_x = 0.0f;
            continue;
        }

        // Get Glyph info
        GlyphInformation* info = &atlas->glyph_informations[atlas->character_to_glyph_map[current_character]];;

        // Add character information
        {
            Character_Position pos;
            pos.glyph_info = info;
            pos.bounding_box.min = vec2(cursor_x, cursor_y);
            pos.bounding_box.max = vec2(cursor_x + info->advance_x * scaling_factor.x,
                cursor_y + (atlas->ascender - atlas->descender) * scaling_factor.y);
            pos.color = renderer->default_color;
            dynamic_array_push_back(&renderer->text_layout.character_positions, pos);
        }

        // Advance to next character
        cursor_x += info->advance_x * scaling_factor.x;
    }
    if (cursor_x > max_cursor_x) {
        max_cursor_x = cursor_x;
    }

    // Push up all character, so that all y coordinates are > 0
    for (int i = 0; i < renderer->text_layout.character_positions.size; i++) {
        renderer->text_layout.character_positions[i].bounding_box.min.y += cursor_y;
        renderer->text_layout.character_positions[i].bounding_box.max.y += cursor_y;
    }

    renderer->text_layout.size = vec2(max_cursor_x, -cursor_y);
    return &renderer->text_layout;
}

void text_renderer_update_window_size(TextRenderer* renderer, int new_width, int new_height)
{
    renderer->screen_width = new_width;
    renderer->screen_height = new_height;
}

void text_renderer_render(TextRenderer* renderer, OpenGLState* state)
{
    // Update font_mesh
    gpu_buffer_update(
        &renderer->font_mesh.vertex_buffers[0].vertex_buffer,
        dynamic_array_to_bytes(&renderer->text_vertices)
    );
    mesh_gpu_data_update_index_buffer(
        &renderer->font_mesh,
        dynamic_array_to_array(&renderer->text_indices),
        state
    );
    renderer->font_mesh.index_count = renderer->text_indices.size;

    // Reset buffers
    dynamic_array_reset(&renderer->text_vertices);
    dynamic_array_reset(&renderer->text_indices);

    opengl_state_set_blending_state(state, true, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_FUNC_ADD);
    //if (render_with_sdf) {
    shader_program_set_uniform(renderer->sdf_shader, state, "sampler", texture_bind_to_unit(&renderer->atlas_sdf_texture, state));
    mesh_gpu_data_draw_with_shader_program(&renderer->font_mesh, renderer->sdf_shader, state);
    /*
    }
    else {
        shader_program_set_uniform(renderer->bitmap_shader, state, "sampler", texture_bind_to_unit(&renderer->atlas_bitmap_texture, state));
        mesh_gpu_data_draw_with_shader_program(&renderer->font_mesh, renderer->bitmap_shader, state);
    }
    */

    /*
    shader_program_set_uniform(renderer->sdf_shader, state, "sampler", texture_bind_to_unit(&renderer->atlas_sdf_texture, state));
    mesh_gpu_data_draw_with_shader_program(&renderer->quad_mesh, renderer->sdf_shader, state);
    */
}

float text_renderer_get_cursor_advance(TextRenderer* renderer, float relative_height)
{
    vec2 scaling_factor = text_renderer_get_scaling_factor(renderer, relative_height);
    return renderer->glyph_atlas.cursor_advance * scaling_factor.x;
}

Texture* text_renderer_get_texture(TextRenderer* renderer)
{
    return &renderer->atlas_sdf_texture;
}

void text_renderer_set_color(TextRenderer* renderer, vec3 color) {
    renderer->default_color = color;
}

float text_renderer_calculate_text_width(TextRenderer* renderer, int char_count, float relative_height) {
    return (float)renderer->glyph_atlas.cursor_advance / (renderer->glyph_atlas.ascender - renderer->glyph_atlas.descender) * relative_height * char_count;
}









/*
    How to implement: either
        - Use a tool to create distance fields onto images, load those
        - Use a library to load TrueType font types, and render those
           |--> Create distance field from rasterized images/create distance fields directly from those images
        - Load TrueType/OpenType font data manually, do bezier stuff, would probably be lots of fun :)
*/

/*
    Typography information/vocabulary:
        Font        ... Set of character images used to print/display text
        Font Family ... Contains multiple font faces of the same template
        Font Face   ... Contains an image for each character
            Example: Font family: "Arial", Font Face: "Arial Bold", "Arial italic"...

    Files:
        Digital font    ... File containing one or more font faces
        Font collection ... same as above

    Character and mapping:
        Glyph       ... A image of a single character
                        One character can have multiple glyphs (multiple styles, different faces) and multiple characters can share the same glyph
        Charmap     ... Convert character codes into glyph indices inside a font file
                        Depends on encoding, files may contain ASCII, Unicode or other encodings

    Sizing, dimensions in printing:
        Text sizes are not given in pixels, since pixels are not PHYSICAL units
        (They depend on monitor size --> specified by DPI/PPI)
        This is why they are given in points (1/72 of an inch).
        Points are called points because printers methods used points (Point array printers)

        BUT when designing a text, designers do not think about logical sizes, only the proportions of each glyph to another.
        This dimensionless space is the EM-SQUARE, and glyph information is usually stored in this em square
        To convert the em units to inches, metrics are provided in the font file.

        So, to go from the file to pixels, you do the following
            EM-Space        -->   Physical Size in inches     (Font metrics in file)
            Physical Size   -->   Pixel size                  (dpi)

    Vector representation:
        A glphy is represented by quadratic or cubic bezier curves in the em-space, and
        the coordinates are on a fixed grid, typically given by signed 16-bit integers.

    Hinting: To get from the vector representation to a bitmap (Grid-fitting)

    Text positioning, and size metrics:
    Per Font Face information:
        Baseline        ...     Horizontal or vertical line on which characters are placed
        Pen/Origin      ...     Current position on the baseline (Is always on baseline)
        Ascent          ...     Per character information on height of the character from the baseline (Always positive)
        Descent         ...     Per character information on the lowest part from the baseline (Always negative)
        Linespace       ...     How much the baseline should shift in a new line, should be linespace = ascent - descent + linegap
        Linegap         ...     Per font information, additional space that should exist between lines
        BoundingBox     ...     Bounding box of all glyphs
    Per Glyph informations
        Advance width   ...     How much the pen should advance horizontally after drawing
        Advance height  ...     How much the pen should advance verticaly after drawing (0 for horizontal languages)
        Bearing left    ...     Distance from pen to left side of the bounding box
        Bearing right   ...     Distance from pen to top side of the bounding box
        Glyph width     ...     Width  (of bounding box)
        Glyph height    ...     Height (of bounding box)

    Kerning
        Because each single glyph contains an ascent, and because of a glpyhs form, sometimes unwanted whitespace may appear.
        As an example, AV usually has a lot of whitespace, and when determining the glyphs position, we want to avoid this.
        This is what kerning does. (Usually contained in the GPOS and kern tables of the font)
*/

/*
    What are my requirements:
        - ANSI-Code (8 bit instead of 7 bit ASCII code, unicode is not compatible)
        - Transforms/Scale invariance -> Distance field rendering
        - Kerning would also be cool, but i generally only need unispaced fonts

    Interesting stuff to implement:
        - Distance field form raster image
        - Rectangle packing in texture atlas
        - Font rasterization (When loading fonts myself) from bezier curves
        - Kerning and text placement with other metrics
*/

/*
    RESEARCH:
    ---------
    Libraries for text rendering:
        - DirectWrite (DirectX/Microsoft)
        - Some Apple stuff
        - FreeType (Font rasterizer)
        - Slug (Does rendering directly in pixel shader with bezier curves, interesting paper)
        - Some other

    Font file formats:
        - TrueType (Old, made by apple)
        - OpenType (Based on TrueType, trademark Microsoft)
*/

