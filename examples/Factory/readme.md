# Factory Example

This example is a factory test/demo firmware for T-Glass V3. It uses an LVGL
tileview to switch between hardware test pages.

## T8 Screen Move Page

The T8 page is `PAGE_SCREEN_MOVE`. It is used to adjust the position of the
whole factory UI tileview on the display.

### Enter The T8 Page

Use the touch button to switch pages until the T8 page is shown.

When T8 is first displayed, the title is:

```text
Screen Move
```

At this time, the page is only selected. Touch input will not change
`tileview_x` or `tileview_y`.

### Enter Adjustment Mode

On the T8 page, double-click the BOOT button to enter adjustment mode.

After entering adjustment mode, the title changes to:

```text
Adjusting
```

Only in this mode can the touch button adjust the overall tileview position.

### Select Direction

In adjustment mode, single-click the BOOT button to switch the move direction.
The direction text cycles in this order:

```text
UP -> DOWN -> LEFT -> RIGHT -> UP
```

The default direction is `UP`.

### Move The Tileview

In adjustment mode, press the touch button to move the whole tileview by one
step. Each step changes the offset by `5`.

Current code behavior:

| Direction | Offset Change |
| --- | --- |
| `UP` | `tileview_y -= 5` |
| `DOWN` | `tileview_y += 5` |
| `LEFT` | `tileview_x -= 5` |
| `RIGHT` | `tileview_x += 5` |

After each move, the tileview is re-aligned with:

```cpp
lv_obj_align(tileview, LV_ALIGN_BOTTOM_MID, tileview_x, tileview_y);
```

### Offset Limits

The current code limits the offsets to this range:

```text
tileview_x: -63 ~ 63
tileview_y: -63 ~ 63
```

If a move would exceed the range, the value is clamped to the nearest limit.

### Save And Exit

While in adjustment mode, double-click the BOOT button again to:

1. exit adjustment mode;
2. save `tileview_x` and `tileview_y` to `Preferences`;
3. switch to the next tileview page.

The saved values use the `glass_config` Preferences namespace:

```text
tileview_x
tileview_y
```

On the next boot, the firmware reads these values and restores the saved layout.
