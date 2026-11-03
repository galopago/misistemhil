#include "display_layout_templates.h"

#include <string.h>

static void set_text_line(display_text_line_t *line, const char *text)
{
    if (line == NULL) {
        return;
    }
    if (text == NULL) {
        line->text[0] = '\0';
    } else {
        strncpy(line->text, text, sizeof(line->text) - 1);
        line->text[sizeof(line->text) - 1] = '\0';
    }
    line->has_align = false;
    line->has_emphasis = false;
}

static void init_text_region(display_text_region_t *text, int max_lines,
                             display_font_policy_t font_policy)
{
    memset(text, 0, sizeof(*text));
    text->max_lines = max_lines;
    text->font_policy = font_policy;
    text->horizontal_align = DISPLAY_H_ALIGN_LEFT;
    text->vertical_align = DISPLAY_V_ALIGN_CENTER;
    text->emphasis = DISPLAY_EMPHASIS_NORMAL;
}

static void fill_text_lines(display_text_region_t *text, const char *const *lines,
                            size_t line_count, int max_lines)
{
    text->line_count = 0;
    const size_t count = line_count < (size_t)max_lines ? line_count : (size_t)max_lines;
    for (size_t i = 0; i < count; ++i) {
        set_text_line(&text->lines[i], lines[i]);
        text->line_count++;
    }
}

void display_layout_template_full_four_lines(display_layout_t *layout, const char *const *lines,
                                             size_t line_count)
{
    if (layout == NULL) {
        return;
    }

    memset(layout, 0, sizeof(*layout));
    layout->region_count = 1;

    display_region_t *region = &layout->regions[0];
    strncpy(region->id, "main", sizeof(region->id) - 1);
    region->rect = (display_rect_t){.x = 0, .y = 0, .width = 128, .height = 64};
    region->content.type = DISPLAY_CONTENT_TEXT;

    init_text_region(&region->content.text, 4, DISPLAY_FONT_AUTO_FIT);
    fill_text_lines(&region->content.text, lines, line_count, 4);
}

void display_layout_template_full_two_lines(display_layout_t *layout, const char *const *lines,
                                            size_t line_count)
{
    if (layout == NULL) {
        return;
    }

    memset(layout, 0, sizeof(*layout));
    layout->region_count = 1;

    display_region_t *region = &layout->regions[0];
    strncpy(region->id, "main", sizeof(region->id) - 1);
    region->rect = (display_rect_t){.x = 0, .y = 0, .width = 128, .height = 64};
    region->content.type = DISPLAY_CONTENT_TEXT;

    init_text_region(&region->content.text, 2, DISPLAY_FONT_AUTO_FIT);
    region->content.text.horizontal_align = DISPLAY_H_ALIGN_CENTER;
    fill_text_lines(&region->content.text, lines, line_count, 2);
}

void display_layout_template_top_two_bottom_one(display_layout_t *layout,
                                                const char *const *lines, size_t line_count)
{
    if (layout == NULL) {
        return;
    }

    memset(layout, 0, sizeof(*layout));
    layout->region_count = 2;

    display_region_t *top = &layout->regions[0];
    strncpy(top->id, "top", sizeof(top->id) - 1);
    top->rect = (display_rect_t){.x = 0, .y = 0, .width = 128, .height = 32};
    top->content.type = DISPLAY_CONTENT_TEXT;
    init_text_region(&top->content.text, 2, DISPLAY_FONT_SMALL);
    fill_text_lines(&top->content.text, lines, line_count, 2);

    display_region_t *bottom = &layout->regions[1];
    strncpy(bottom->id, "bottom", sizeof(bottom->id) - 1);
    bottom->rect = (display_rect_t){.x = 0, .y = 32, .width = 128, .height = 32};
    bottom->content.type = DISPLAY_CONTENT_TEXT;
    init_text_region(&bottom->content.text, 1, DISPLAY_FONT_LARGE);
    bottom->content.text.horizontal_align = DISPLAY_H_ALIGN_CENTER;
    if (line_count > 2) {
        set_text_line(&bottom->content.text.lines[0], lines[2]);
        bottom->content.text.line_count = 1;
    }
}

void display_layout_template_qr_left_text_right(display_layout_t *layout, const char *payload,
                                                const char *const *text_lines,
                                                size_t text_line_count)
{
    if (layout == NULL) {
        return;
    }

    memset(layout, 0, sizeof(*layout));
    layout->region_count = 2;

    display_region_t *qr = &layout->regions[0];
    strncpy(qr->id, "qr", sizeof(qr->id) - 1);
    qr->rect = (display_rect_t){.x = 0, .y = 0, .width = 64, .height = 64};
    qr->content.type = DISPLAY_CONTENT_QR;
    if (payload != NULL) {
        strncpy(qr->content.qr.payload, payload, sizeof(qr->content.qr.payload) - 1);
    }
    qr->content.qr.error_correction = DISPLAY_QR_EC_LOW;
    qr->content.qr.scale = DISPLAY_QR_SCALE_AUTO;
    qr->content.qr.quiet_zone_modules = DISPLAY_QR_QUIET_ZONE_MODULES;

    display_region_t *text = &layout->regions[1];
    strncpy(text->id, "text", sizeof(text->id) - 1);
    text->rect = (display_rect_t){.x = 64, .y = 0, .width = 64, .height = 64};
    text->content.type = DISPLAY_CONTENT_TEXT;
    init_text_region(&text->content.text, 4, DISPLAY_FONT_SMALL);
    fill_text_lines(&text->content.text, text_lines, text_line_count, 4);
}
