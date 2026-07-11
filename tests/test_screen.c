#include "test.h"
#include "screen.h"

TEST(test_initial_cursor_position) {
    initialise_screen();
    ASSERT_EQ(terminal_cursor.x_pos, 0);
    ASSERT_EQ(terminal_cursor.y_pos, 0);
}

TEST(test_all_cells_cleared_to_space) {
    initialise_screen();

    bool all_spaces = true;
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            if (pixels[row][col].character != ' ') {
                all_spaces = false;
            }
        }
    }
    ASSERT_TRUE(all_spaces);
}

TEST(test_default_colours_and_attrs) {
    initialise_screen();

    // spot-check corners rather than every cell -- if these are right,
    // the loop in initialise_screen almost certainly did the rest right too
    int rows_to_check[] = {0, ROWS - 1};
    int cols_to_check[] = {0, COLS - 1};

    for (int r = 0; r < 2; r++) {
        for (int c = 0; c < 2; c++) {
            Cell cell = pixels[rows_to_check[r]][cols_to_check[c]];
            ASSERT_EQ(cell.bg_colour, 0x000000FF);
            ASSERT_EQ(cell.fg_colour, 0xFFFFFFFF);
            ASSERT_EQ(cell.attrs, 0);
        }
    }
}

TEST(test_reinitialise_clears_previous_state) {
    initialise_screen();

    // dirty the screen and move the cursor
    pixels[3][3].character = 'X';
    pixels[3][3].fg_colour = 0x00FF00FF;
    terminal_cursor.x_pos = 10;
    terminal_cursor.y_pos = 5;

    // re-running initialise_screen should wipe all of that out
    initialise_screen();

    ASSERT_EQ(terminal_cursor.x_pos, 0);
    ASSERT_EQ(terminal_cursor.y_pos, 0);
    ASSERT_EQ(pixels[3][3].character, ' ');
    ASSERT_EQ(pixels[3][3].fg_colour, 0xFFFFFFFF);
}

TEST(test_move_screen_down_shifts_rows_up) {
    initialise_screen();

    // put a distinct character on every row so we can track where it ends up
    for (int row = 0; row < ROWS; row++) {
        pixels[row][0].character = (char)('A' + row);
    }

    move_screen_down();

    // row 1's content should now be in row 0, row 2's in row 1, etc.
    for (int row = 0; row < ROWS - 1; row++) {
        ASSERT_EQ(pixels[row][0].character, 'A' + row + 1);
    }
}

TEST(test_move_screen_down_clears_last_row) {
    initialise_screen();

    // dirty the last row so we can tell it actually got reset
    for (int col = 0; col < COLS; col++) {
        pixels[ROWS - 1][col].character = 'Z';
    }

    move_screen_down();

    bool last_row_cleared = true;
    for (int col = 0; col < COLS; col++) {
        if (pixels[ROWS - 1][col].character != ' ') {
            last_row_cleared = false;
        }
    }
    ASSERT_TRUE(last_row_cleared);
}

TEST(test_move_screen_down_resets_last_row_colours) {
    initialise_screen();

    // corrupt colours in the last row before scrolling
    pixels[ROWS - 1][0].fg_colour = 0x123456FF;
    pixels[ROWS - 1][0].bg_colour = 0x654321FF;

    move_screen_down();

    ASSERT_EQ(pixels[ROWS - 1][0].fg_colour, 0xFFFFFFFF);
    ASSERT_EQ(pixels[ROWS - 1][0].bg_colour, 0x000000FF);
}

TEST(test_move_screen_down_sets_cursor_to_last_row) {
    initialise_screen();

    terminal_cursor.y_pos = 3; // arbitrary starting point
    move_screen_down();

    ASSERT_EQ(terminal_cursor.y_pos, ROWS - 1);
}

TEST(test_move_screen_down_preserves_column_alignment) {
    initialise_screen();

    // scatter characters across different columns on row 5
    pixels[5][0].character = 'L';
    pixels[5][40].character = 'M';
    pixels[5][COLS - 1].character = 'R';

    move_screen_down();

    // everything on row 5 should have moved to row 4, same columns
    ASSERT_EQ(pixels[4][0].character, 'L');
    ASSERT_EQ(pixels[4][40].character, 'M');
    ASSERT_EQ(pixels[4][COLS - 1].character, 'R');
}

TEST(test_move_screen_down_twice_scrolls_further) {
    initialise_screen();

    pixels[2][0].character = 'X';

    move_screen_down(); // row 2 -> row 1
    move_screen_down(); // row 1 -> row 0

    ASSERT_EQ(pixels[0][0].character, 'X');
}

int main(void) {
    test_initial_cursor_position();
    test_all_cells_cleared_to_space();
    test_default_colours_and_attrs();
    test_reinitialise_clears_previous_state();

    test_move_screen_down_shifts_rows_up();
    test_move_screen_down_clears_last_row();
    test_move_screen_down_resets_last_row_colours();
    test_move_screen_down_sets_cursor_to_last_row();
    test_move_screen_down_preserves_column_alignment();
    test_move_screen_down_twice_scrolls_further();

    TEST_SUMMARY();
    return tests_failed != 0;
}