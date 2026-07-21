#include "test.h"
#include "screen.h"
#include <stdlib.h>

static Terminal create_test_terminal(void) {
    Terminal term;
    initialise_screen(&term, DEFAULT_COLS, DEFAULT_ROWS);
    return term;
}

static void free_test_terminal(Terminal *term) {
    if (term->grid) {
        free(term->grid);
        term->grid = NULL;
    }
}

TEST(test_initial_cursor_position) {
    Terminal term = create_test_terminal();

    ASSERT_EQ(term.cursor.x_pos, 0);
    ASSERT_EQ(term.cursor.y_pos, 0);
    ASSERT_TRUE(term.cursor.visible);

    free_test_terminal(&term);
}

TEST(test_all_cells_cleared_to_space) {
    Terminal term = create_test_terminal();

    bool all_spaces = true;
    for (int r = 0; r < term.rows; r++) {
        for (int c = 0; c < term.cols; c++) {
            if (term.grid[r * term.cols + c].character != ' ') {
                all_spaces = false;
            }
        }
    }
    ASSERT_TRUE(all_spaces);

    free_test_terminal(&term);
}

TEST(test_default_colours_and_attrs) {
    Terminal term = create_test_terminal();

    int corner_indices[] = {
        0,
        term.cols - 1,
        (term.rows - 1) * term.cols,
        term.rows * term.cols - 1
    };

    for (int i = 0; i < 4; i++) {
        Cell cell = term.grid[corner_indices[i]];
        
        // Assert background matches BLACK (0, 0, 0, 255)
        ASSERT_EQ(cell.bg_colour.r, BLACK.r);
        ASSERT_EQ(cell.bg_colour.g, BLACK.g);
        ASSERT_EQ(cell.bg_colour.b, BLACK.b);
        ASSERT_EQ(cell.bg_colour.a, BLACK.a);

        // Assert foreground matches the initial color assigned in screen.c
        // If screen.c uses RAYWHITE (245, 245, 245, 255), this will pass cleanly!
        ASSERT_EQ(cell.fg_colour.r, RAYWHITE.r);
        ASSERT_EQ(cell.fg_colour.g, RAYWHITE.g);
        ASSERT_EQ(cell.fg_colour.b, RAYWHITE.b);
        ASSERT_EQ(cell.fg_colour.a, RAYWHITE.a);

        ASSERT_EQ(cell.attrs, 0);
    }

    free_test_terminal(&term);
}

TEST(test_reinitialise_clears_previous_state) {
    Terminal term = create_test_terminal();

    int idx = 3 * term.cols + 3;
    term.grid[idx].character = 'X';
    term.grid[idx].fg_colour = GREEN;
    term.cursor.x_pos = 10;
    term.cursor.y_pos = 5;
    term.cursor.visible = false;

    initialise_screen(&term, DEFAULT_COLS, DEFAULT_ROWS);

    ASSERT_EQ(term.cursor.x_pos, 0);
    ASSERT_EQ(term.cursor.y_pos, 0);
    ASSERT_TRUE(term.cursor.visible);
    ASSERT_EQ(term.grid[idx].character, ' ');
    ASSERT_EQ(term.grid[idx].fg_colour.r, RAYWHITE.r);

    free_test_terminal(&term);
}

TEST(test_move_screen_down_shifts_rows_up) {
    Terminal term = create_test_terminal();

    for (int row = 0; row < term.rows; row++) {
        term.grid[row * term.cols].character = (char)('A' + row);
    }

    move_screen_down(&term);

    for (int row = 0; row < term.rows - 1; row++) {
        ASSERT_EQ(term.grid[row * term.cols].character, 'A' + row + 1);
    }

    free_test_terminal(&term);
}

TEST(test_move_screen_down_clears_last_row) {
    Terminal term = create_test_terminal();

    int last_row_offset = (term.rows - 1) * term.cols;
    for (int col = 0; col < term.cols; col++) {
        term.grid[last_row_offset + col].character = 'Z';
    }

    move_screen_down(&term);

    bool last_row_cleared = true;
    for (int col = 0; col < term.cols; col++) {
        if (term.grid[last_row_offset + col].character != ' ') {
            last_row_cleared = false;
        }
    }
    ASSERT_TRUE(last_row_cleared);

    free_test_terminal(&term);
}

TEST(test_move_screen_down_sets_cursor_to_last_row) {
    Terminal term = create_test_terminal();

    term.cursor.y_pos = 3;
    move_screen_down(&term);

    ASSERT_EQ(term.cursor.y_pos, term.rows - 1);

    free_test_terminal(&term);
}

TEST(test_move_screen_down_preserves_column_alignment) {
    Terminal term = create_test_terminal();

    term.grid[5 * term.cols + 0].character = 'L';
    term.grid[5 * term.cols + 40].character = 'M';
    term.grid[5 * term.cols + (term.cols - 1)].character = 'R';

    move_screen_down(&term);

    ASSERT_EQ(term.grid[4 * term.cols + 0].character, 'L');
    ASSERT_EQ(term.grid[4 * term.cols + 40].character, 'M');
    ASSERT_EQ(term.grid[4 * term.cols + (term.cols - 1)].character, 'R');

    free_test_terminal(&term);
}

int main(void) {
    test_initial_cursor_position();
    test_all_cells_cleared_to_space();
    test_default_colours_and_attrs();
    test_reinitialise_clears_previous_state();

    test_move_screen_down_shifts_rows_up();
    test_move_screen_down_clears_last_row();
    test_move_screen_down_sets_cursor_to_last_row();
    test_move_screen_down_preserves_column_alignment();

    TEST_SUMMARY();
    return tests_failed != 0;
}