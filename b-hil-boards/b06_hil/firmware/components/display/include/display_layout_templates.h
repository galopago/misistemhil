#pragma once

#include "display_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void display_layout_template_full_four_lines(display_layout_t *layout,
                                             const char *const *lines,
                                             size_t line_count);
void display_layout_template_full_two_lines(display_layout_t *layout,
                                            const char *const *lines,
                                            size_t line_count);
void display_layout_template_top_two_bottom_one(display_layout_t *layout,
                                                const char *const *lines,
                                                size_t line_count);
void display_layout_template_qr_left_text_right(display_layout_t *layout,
                                                  const char *payload,
                                                  const char *const *text_lines,
                                                  size_t text_line_count);

#ifdef __cplusplus
}
#endif
