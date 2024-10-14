#include <stdio.h>
#include <stdint.h>
#include <ctype.h>

#define BOARD_SIZE 64

// Function for converting user input (e.g., 'A3') to a board index (0-63)
int ConvertToIndex(char column, int row) {
    column = toupper(column);

    // Validate input
    if (column < 'A' || column > 'H' || row < 1 || row > 8) {
        return -1;
    }
    int col = column - 'A';
    int r = row - 1;
    return r * 8 + col;
}

// Initializes the board with starting positions
void InitializeBoard(uint64_t *black_pieces, uint64_t *white_pieces, uint64_t *kings) {
    *black_pieces = 0;
    *white_pieces = 0;
    *kings = 0;

    // Place black pieces on rows 1-3
    for (int row = 0; row <= 2; row++) {
        for (int col = (row + 1) % 2; col < 8; col += 2) {
            int index = row * 8 + col;
            *black_pieces |= (uint64_t)1 << index;
        }
    }

    // Place white pieces on rows 6-8
    for (int row = 5; row <= 7; row++) {
        for (int col = (row + 1) % 2; col < 8; col += 2) {
            int index = row * 8 + col;
            *white_pieces |= (uint64_t)1 << index;
        }
    }
}

// Prints an instance of the current state of the board
void PrintBoard(uint64_t black_pieces, uint64_t white_pieces, uint64_t kings) {
    printf("  A B C D E F G H\n");
    for (int row = 7; row >= 0; row--) {
        printf("%d ", row + 1);
        for (int col = 0; col < 8; col++) {
            int index = row * 8 + col;
            uint64_t mask = (uint64_t)1 << index;

            if (black_pieces & mask) {
                // 'B' for black king, 'b' for black regular piece
                printf("%c ", (kings & mask) ? 'B' : 'b');
            } else if (white_pieces & mask) {
                // Same as above but for white kings and regular pieces
                printf("%c ", (kings & mask) ? 'W' : 'w');
            } else {
                printf(". "); // Empty square
            }
        }
        printf("%d\n", row + 1);
    }
    printf("  A B C D E F G H\n");
}

// Function for checking the legality of move
int IsLegalMove(uint64_t player_pieces, uint64_t opponent_pieces, uint64_t kings, int start, int end, int player, int *capture) {
    // Validate indices
    if (start < 0 || start >= BOARD_SIZE || end < 0 || end >= BOARD_SIZE)
        return 0;

    uint64_t occupied = player_pieces | opponent_pieces;
    uint64_t start_mask = (uint64_t)1 << start;
    uint64_t end_mask = (uint64_t)1 << end;

    // The starting square must be nonempty
    if (!(player_pieces & start_mask))
        return 0;

    // The ending square must be empty
    if (occupied & end_mask)
        return 0;

    int start_row = start / 8;
    int start_col = start % 8;
    int end_row = end / 8;
    int end_col = end % 8;

    int row_diff = end_row - start_row;
    int col_diff = end_col - start_col;

    // Move must be diagonal
    if (abs(row_diff) != abs(col_diff))
        return 0;

    int is_king = (kings & start_mask) != 0;

    // Determine move direction for regular pieces
    int forward = (player == 1) ? 1 : -1;

    // Regular move (no capture)
    if (abs(row_diff) == 1) {
        *capture = 0;
        if (is_king || row_diff == forward)
            return 1;
        else
            return 0; // Wrong direction
    }
    // Capture move
    else if (abs(row_diff) == 2) {
        int mid_row = (start_row + end_row) / 2;
        int mid_col = (start_col + end_col) / 2;
        int mid_index = mid_row * 8 + mid_col;
        uint64_t mid_mask = (uint64_t)1 << mid_index;

        // There must be an opponent's piece to capture
        if (!(opponent_pieces & mid_mask))
            return 0;

        *capture = 1;

        if (is_king || row_diff == 2 * forward)
            return 2;
        else
            return 0; // Wrong direction
    }

    // Invalid move distance
    return 0;
}

// Function for moving pieces on board
void MovePiece(uint64_t *pieces, int start, int end) {
    uint64_t start_mask = (uint64_t)1 << start;
    uint64_t end_mask = (uint64_t)1 << end;

    // Changes an instance of the piece from starting point to finish point
    *pieces &= ~start_mask;
    *pieces |= end_mask;
}

// Function for capturing pieces from opponent team
void CapturePiece(uint64_t *opponent_pieces, uint64_t *kings, int position) {
    uint64_t mask = (uint64_t)1 << position;

    // Remove opponent piece (whatever it was king/regular piece)
    *opponent_pieces &= ~mask;
    *kings &= ~mask;
}

// Function for checking king promotion
int CheckPromotion(int position, int player) {
    int row = position / 8;
    return (player == 1 && row == 7) || (player == 2 && row == 0);
}

// Function for checking options
int HasAvailableCapture(uint64_t player_pieces, uint64_t opponent_pieces,
                        uint64_t kings, int position, int player) {
    int directions[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    uint64_t start_mask = (uint64_t)1 << position;
    int is_king = (kings & start_mask) != 0;
    int forward = (player == 1) ? 1 : -1;

    for (int i = 0; i < 4; i++) {
        int dr = directions[i][0];
        int dc = directions[i][1];

        // Skips backward moves for non-kings
        if (!is_king && dr != forward)
            continue;

        int mid_row = (position / 8) + dr;
        int mid_col = (position % 8) + dc;
        int end_row = mid_row + dr;
        int end_col = mid_col + dc;

        // Checks which way I can go/are there any boundaries
        if (end_row < 0 || end_row >= 8 || end_col < 0 || end_col >= 8)
            continue;

        int mid_index = mid_row * 8 + mid_col;
        int end_index = end_row * 8 + end_col;

        uint64_t mid_mask = (uint64_t)1 << mid_index;
        uint64_t end_mask = (uint64_t)1 << end_index;

        // There must be an opponent's piece to capture and landing square must be empty
        if ((opponent_pieces & mid_mask) && !((player_pieces | opponent_pieces) & end_mask))
            return 1; // Capture is available
    }

    return 0; // No captures available
}

int utility() {
    uint64_t black_pieces, white_pieces, kings;
    int player = 1; // 1 for Black, 2 for White

    InitializeBoard(&black_pieces, &white_pieces, &kings);

    while (1) {
        PrintBoard(black_pieces, white_pieces, kings);

        // Check for win condition
        if (black_pieces == 0) {
            printf("White wins!\n");
            break;
        } else if (white_pieces == 0) {
            printf("Black wins!\n");
            break;
        }

        printf("%s's turn.\n", player == 1 ? "Black" : "White");

        int valid_move = 0;
        while (!valid_move) {
            char start_col, end_col;
            int start_row, end_row;

            printf("Enter the starting position (e.g., B6): ");
            if (scanf(" %c%d", &start_col, &start_row) != 2) {
                printf("Invalid input format. Please try again.\n");
                continue;
            }

            printf("Enter the ending position (e.g., C5): ");
            if (scanf(" %c%d", &end_col, &end_row) != 2) {
                printf("Invalid input format. Please try again.\n");
                continue;
            }

            int start = ConvertToIndex(start_col, start_row);
            int end = ConvertToIndex(end_col, end_row);

            if (start == -1 || end == -1) {
                printf("Invalid board position. Please try again.\n");
                continue;
            }

            int capture = 0;
            uint64_t *player_pieces = (player == 1) ? &black_pieces : &white_pieces;
            uint64_t *opponent_pieces = (player == 1) ? &white_pieces : &black_pieces;

            int move_result = IsLegalMove(*player_pieces, *opponent_pieces, kings, start, end, player, &capture);

            if (move_result == 0) {
                printf("Invalid move. Try again.\n");
                continue;
            }

            // Move the piece
            MovePiece(player_pieces, start, end);

            // Handle king movement
            uint64_t start_mask = (uint64_t)1 << start;
            uint64_t end_mask = (uint64_t)1 << end;

            if (kings & start_mask) {
                kings &= ~start_mask;
                kings |= end_mask;
            }

            // Handle capture
            if (capture) {
                int mid_row = (start / 8 + end / 8) / 2;
                int mid_col = (start % 8 + end % 8) / 2;
                int mid_index = mid_row * 8 + mid_col;
                CapturePiece(opponent_pieces, &kings, mid_index);
            }

            // Check for promotion
            if (CheckPromotion(end, player)) {
                printf("%s piece promoted to king!\n", player == 1 ? "Black" : "White");
                kings |= end_mask;
            }

            valid_move = 1;

            // Handle multiple jumps
            if (capture) {
                while (HasAvailableCapture(*player_pieces, *opponent_pieces, kings, end, player)) {
                    PrintBoard(black_pieces, white_pieces, kings);
                    printf("%s can make another jump from %c%d.\n",
                           player == 1 ? "Black" : "White", 'A' + (end % 8), (end / 8) + 1);

                    printf("Enter the next ending position: ");
                    start = end;
                    if (scanf(" %c%d", &end_col, &end_row) != 2) {
                        printf("Invalid input format. Please try again.\n");
                        break;
                    }

                    end = ConvertToIndex(end_col, end_row);
                    if (end == -1) {
                        printf("Invalid board position. Please try again.\n");
                        continue;
                    }

                    capture = 0;
                    move_result = IsLegalMove(*player_pieces, *opponent_pieces, kings, start, end, player, &capture);

                    if (move_result != 2) {
                        printf("Invalid jump. Try again.\n");
                        continue;
                    }

                    // Move the piece
                    MovePiece(player_pieces, start, end);

                    // Handle king movement
                    start_mask = (uint64_t)1 << start;
                    end_mask = (uint64_t)1 << end;

                    if (kings & start_mask) {
                        kings &= ~start_mask;
                        kings |= end_mask;
                    }

                    // Handle capture
                    int mid_row = (start / 8 + end / 8) / 2;
                    int mid_col = (start % 8 + end % 8) / 2;
                    int mid_index = mid_row * 8 + mid_col;
                    CapturePiece(opponent_pieces, &kings, mid_index);

                    // Check for promotion
                    if (CheckPromotion(end, player)) {
                        printf("%s piece promoted to king!\n", player == 1 ? "Black" : "White");
                        kings |= end_mask;
                        break;
                    }
                }
            }
        }

        // Switch players for input
        player = 3 - player; //  1 (Black) and 2 (White)
    }

    return 0;
}
