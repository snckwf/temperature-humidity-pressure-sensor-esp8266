// Parametric case for WEMOS D1 Mini V4 + AHT20 + BMP280 module
// Designed for FDM 3D printing
// - separate body + lid
// - good ventilation
// - open window above the sensor
// - side USB-C opening for the Wemos
//
// Note: the AHT20+BMP280 sensor module comes in slightly different variants.
// The dimensions below are parametric: if your module is different,
// only change the values in the PARAMETERS section.

$fn = 48;

// =========================
// VIEW / PRINT
// =========================
show_print_layout = true;   // true = body and lid laid out for export/printing
show_assembly     = false;  // true = show the closed assembly

// =========================
// MAIN PARAMETERS
// =========================
// WEMOS D1 Mini V4
wemos_l = 34.3;
wemos_w = 25.4;
wemos_h = 10.5;   // conservative height with low components/headers

// AHT20 + BMP280 module (typically 16 x 16 / 16.5 mm)
sensor_l = 16.5;
sensor_w = 16.0;
sensor_h = 5.0;

// Case
wall        = 2.2;
floor_t     = 2.2;
lid_t       = 2.0;
lip_h       = 3.2;
lip_t       = 1.3;
fit         = 0.25;   // lid-to-base clearance
corner_r    = 4.0;
board_clear = 0.8;
inner_h     = 12.5;

// Internal layout
front_space = 5.0;    // space on the USB side
channel_gap = 6.0;    // wiring space between Wemos and sensor
rear_space  = 3.5;

// Board supports
standoff_h  = 2.0;
standoff_xy = 2.8;
ret_h       = 1.2;
ret_t       = 1.2;
ret_gap     = 0.35;

// Sensor window
sensor_window_margin = 1.4;

// Vents
side_slot_w  = 2.2;
side_slot_h  = 7.0;
side_slot_gap = 2.0;
side_rows_z  = [4.2, 8.5];

top_slot_w   = 2.0;
top_slot_l   = 11.0;
top_slot_gap = 2.0;

// USB-C opening (generous for tolerances)
usb_open_w = 12.0;
usb_open_h = 6.0;
usb_open_z = floor_t + standoff_h + 1.2;

// =========================
// DERIVED DIMENSIONS
// =========================
inner_l = front_space + wemos_l + channel_gap + sensor_l + rear_space;
inner_w = max(wemos_w, sensor_w) + 2*board_clear;

outer_l = inner_l + 2*wall;
outer_w = inner_w + 2*wall;
base_h  = floor_t + inner_h;

wemos_x = wall + front_space;
wemos_y = wall + (inner_w - wemos_w)/2;

sensor_x = wall + front_space + wemos_l + channel_gap;
sensor_y = wall + (inner_w - sensor_w)/2;

// =========================
// UTILITY
// =========================
module rounded_box(size=[10,10,10], r=2) {
    rr = min(r, min(size[0], size[1]) / 2 - 0.01);
    hull() {
        for (x = [rr, size[0]-rr])
            for (y = [rr, size[1]-rr])
                translate([x, y, 0]) cylinder(h=size[2], r=rr);
    }
}

module rounded_hole(size=[10,10,5], r=1.2) {
    rounded_box(size=size, r=r);
}

module board_posts(x, y, l, w, z0=0) {
    for (px = [x + ret_gap, x + l - standoff_xy - ret_gap])
        for (py = [y + ret_gap, y + w - standoff_xy - ret_gap])
            translate([px, py, z0]) cube([standoff_xy, standoff_xy, standoff_h]);
}

module board_retainers(x, y, l, w, z0=standoff_h) {
    // low lips that stop the PCB from sliding sideways
    // left and right edges
    translate([x - ret_t + ret_gap, y + ret_gap, z0])
        cube([ret_t, w - 2*ret_gap, ret_h]);
    translate([x + l - ret_gap, y + ret_gap, z0])
        cube([ret_t, w - 2*ret_gap, ret_h]);
    // rear edge
    translate([x + ret_gap, y + w - ret_t + ret_gap, z0])
        cube([l - 2*ret_gap, ret_t, ret_h]);
}

module side_vents(x_start, x_end, left_side=true) {
    len = x_end - x_start;
    count = max(1, floor((len - side_slot_gap) / (side_slot_w + side_slot_gap)));
    for (row_z = side_rows_z)
        for (i = [0:count-1]) {
            xx = x_start + side_slot_gap + i*(side_slot_w + side_slot_gap);
            if (left_side)
                translate([xx, -0.2, row_z]) cube([side_slot_w, wall + 0.6, side_slot_h]);
            else
                translate([xx, outer_w - wall - 0.4, row_z]) cube([side_slot_w, wall + 0.6, side_slot_h]);
        }
}

module bottom_vents(x_start, x_end, y_margin=4.5, slot_w=2.2, slot_l=10) {
    len = x_end - x_start;
    count = max(1, floor((len - 2) / (slot_w + 1.8)));
    y1 = wall + y_margin;
    y2 = outer_w - wall - y_margin - slot_l;
    for (yy = [y1, y2])
        for (i = [0:count-1]) {
            xx = x_start + 1 + i*(slot_w + 1.8);
            translate([xx, yy, -0.1]) cube([slot_w, slot_l, floor_t + 0.3]);
        }
}

module top_vents_wemos() {
    vent_area_x0 = wemos_x + 5.0;
    vent_area_x1 = wemos_x + wemos_l - 5.0;
    vent_area_y0 = wall + 4.5;
    vent_area_y1 = outer_w - wall - 4.5;
    count = max(1, floor((vent_area_x1 - vent_area_x0) / (top_slot_w + top_slot_gap)));
    for (yy = [vent_area_y0, vent_area_y1 - top_slot_l])
        for (i = [0:count-1]) {
            xx = vent_area_x0 + i*(top_slot_w + top_slot_gap);
            translate([xx, yy, -0.1]) cube([top_slot_w, top_slot_l, lid_t + 0.3]);
        }
}

module sensor_window() {
    wx = sensor_x + sensor_window_margin;
    wy = sensor_y + sensor_window_margin;
    wl = sensor_l - 2*sensor_window_margin;
    ww = sensor_w - 2*sensor_window_margin;
    translate([wx, wy, -0.1]) rounded_hole([wl, ww, lid_t + 0.3], r=1.0);
}

module usb_cutout() {
    translate([-0.2,
               outer_w/2 - usb_open_w/2,
               usb_open_z])
        rounded_hole([wall + 0.6, usb_open_w, usb_open_h], r=1.0);
}

// =========================
// BODY
// =========================
module case_base() {
    union() {
        difference() {
            rounded_box([outer_l, outer_w, base_h], corner_r);

            // main cavity
            translate([wall, wall, floor_t])
                rounded_box([inner_l, inner_w, inner_h + 0.2], max(0.8, corner_r - wall));

            // USB-C opening
            usb_cutout();

            // side vents
            side_vents(wall + 4, wemos_x + wemos_l + 1.0, true);
            side_vents(wall + 4, wemos_x + wemos_l + 1.0, false);
            side_vents(sensor_x - 1.0, sensor_x + sensor_l + 0.5, true);
            side_vents(sensor_x - 1.0, sensor_x + sensor_l + 0.5, false);

            // bottom vents
            bottom_vents(wemos_x + 3.0, wemos_x + wemos_l - 4.0);
            bottom_vents(sensor_x + 1.5, sensor_x + sensor_l - 1.5, y_margin=6.0, slot_w=1.8, slot_l=7.0);
        }

        // Wemos supports
        board_posts(wemos_x, wemos_y, wemos_l, wemos_w, floor_t);
        board_retainers(wemos_x, wemos_y, wemos_l, wemos_w, floor_t + standoff_h);

        // lower front lead-in on the USB side to make insertion easier
        translate([wemos_x + ret_gap, wemos_y + ret_gap, floor_t + standoff_h])
            cube([3.0, ret_t, ret_h]);
        translate([wemos_x + ret_gap, wemos_y + wemos_w - ret_gap, floor_t + standoff_h])
            cube([3.0, ret_t, ret_h]);

        // sensor supports
        board_posts(sensor_x, sensor_y, sensor_l, sensor_w, floor_t);
        board_retainers(sensor_x, sensor_y, sensor_l, sensor_w, floor_t + standoff_h);

        // small front sensor stops
        translate([sensor_x + ret_gap, sensor_y - ret_t + ret_gap, floor_t + standoff_h])
            cube([sensor_l - 2*ret_gap, ret_t, ret_h]);

        // open channel for the I2C wires between Wemos and sensor
        translate([wemos_x + wemos_l + 0.8,
                   outer_w/2 - 4,
                   floor_t])
            cube([channel_gap - 1.6, 8, 1.4]);
    }
}

// =========================
// LID (oriented for printing)
// =========================
module case_lid_print() {
    difference() {
        union() {
            rounded_box([outer_l, outer_w, lid_t], corner_r);

            // inner press-fit lip
            translate([wall + fit, wall + fit, lid_t])
                difference() {
                    rounded_box([inner_l - 2*fit, inner_w - 2*fit, lip_h], max(0.8, corner_r - wall - fit));
                    translate([lip_t, lip_t, -0.1])
                        rounded_box([inner_l - 2*fit - 2*lip_t,
                                     inner_w - 2*fit - 2*lip_t,
                                     lip_h + 0.2],
                                     max(0.4, corner_r - wall - fit - lip_t));
                }
        }

        // large window above the sensor
        sensor_window();

        // top vents above the Wemos
        top_vents_wemos();

        // finger notch to open the lid
        translate([4.0, outer_w/2 - 6, -0.1])
            rounded_hole([5.0, 12.0, lid_t + 0.3], r=1.5);
    }
}

// =========================
// ASSEMBLED LID
// =========================
module case_lid_assembled() {
    translate([0, outer_w, base_h + lid_t + lip_h])
        rotate([180, 0, 0])
            case_lid_print();
}

// =========================
// OUTPUT
// =========================
if (show_assembly) {
    case_base();
    case_lid_assembled();
}

if (show_print_layout) {
    case_base();
    translate([outer_l + 12, 0, 0]) case_lid_print();
}
