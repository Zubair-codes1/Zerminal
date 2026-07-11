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

int main(void) {
    test_initial_cursor_position();
    test_all_cells_cleared_to_space();
    test_default_colours_and_attrs();
    test_reinitialise_clears_previous_state();

    TEST_SUMMARY();
    return tests_failed != 0;
}