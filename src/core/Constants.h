#ifndef CONSTANTS_H
#define CONSTANTS_H

#define DOCK_SLIDE_ANIM_MS 300.f
#define DOCK_MARGIN_L 16.f
#define DOCK_MARGIN_R DOCK_MARGIN_L

// Visible when the dock is hidden (to allow focus)
#define DOCK_MARGIN_T 4.f

// Hidden when the dock is visible (to prevent unfocus)
#define DOCK_MARGIN_B_EXTRA 4.f

#define DOCK_MARGIN_B 4.f
#define ITEM_MAX_SIZE 96.f

/* % respect to items size */

#define SEPARATOR_SCALE 0.16f
#define ITEM_SPACING 0.12f
#define ITEM_DOT_SIZE 0.05f
#define ITEM_DOT_MARGIN_TOP 0.004f
#define ITEM_DOT_MARGIN_BOTTOM 0.04f
#define DOCK_PADDING_TOP 0.18f
#define DOCK_PADDING_LEFT 0.12f
#define DOCK_PADDING_RIGHT 0.12f
#define DOCK_BORDER_RADIUS 0.38f
#define DOCK_SHADOW_RADIUS 0.76f
#define DOCK_SHADOW_OFFSET_Y 0.0f

/* Fixed */

#define ICON_SIZE ITEM_MAX_SIZE * 2.f
#define ICON_SCALE_HIG 1.f
#define ICON_SCALE_MED 0.5f
#define ICON_SCALE_LOW 0.25f

#define DOCK_MAX_HEIGHT ITEM_MAX_SIZE + ITEM_MAX_SIZE * (ITEM_DOT_SIZE + ITEM_DOT_MARGIN_TOP + ITEM_DOT_MARGIN_BOTTOM + DOCK_PADDING_TOP)
#define DOCK_SURFACE_HEIGHT DOCK_MAX_HEIGHT + DOCK_MARGIN_T + DOCK_MARGIN_B + ITEM_MAX_SIZE * (DOCK_SHADOW_RADIUS - DOCK_SHADOW_OFFSET_Y)

/* Fonts */

#define DEFAULT_FONT_FAMILY "Inter"
#define TOOLTIP_FONT_SIZE 13.f
#define MENU_FONT_SIZE 14.f

/* Tooltip dimensions */
#define TOOLTIP_BORDER_RADIUS 4.f
#define TOOLTIP_BLUR_RADIUS 4.f
#define TOOLTIP_PADDING 8.f
#define TOOLTIP_CARET_SIZE 8.f

/* Menu */
#define MENU_PADDING 10.f
#define MENU_BORDER_RADIUS 6.f
#define MENU_BLUR_RADIUS 6.f
#define MENU_CARET_SIZE 8.f

#endif // CONSTANTS_H
