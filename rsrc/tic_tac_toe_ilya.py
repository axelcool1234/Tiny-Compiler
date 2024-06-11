def initialize_board():
    return [[0, 0, 0], [0, 0, 0], [0, 0, 0]]

def print_board(board):
    for row in board:
        print(' '.join(map(str, row)))
    print()

def make_move(board, player):
    print(f"Player {player}'s turn")
    while True:
        try:
            row = int(input("Enter row (0-2): "))
            col = int(input("Enter column (0-2): "))
            if 0 <= row <= 2 and 0 <= col <= 2 and board[row][col] == 0:
                board[row][col] = player
                break
            else:
                print("Invalid move, try again.")
        except ValueError:
            print("Invalid input, try again.")

def check_winner(board):
    # Check rows
    for row in board:
        if row[0] == row[1] == row[2] != 0:
            return row[0]
    # Check columns
    for col in range(3):
        if board[0][col] == board[1][col] == board[2][col] != 0:
            return board[0][col]
    # Check diagonals
    if board[0][0] == board[1][1] == board[2][2] != 0:
        return board[0][0]
    if board[0][2] == board[1][1] == board[2][0] != 0:
        return board[0][2]
    return 0

def main():
    board = initialize_board()
    player = 1
    moves = 0
    winner = 0

    while moves < 9 and winner == 0:
        print_board(board)
        make_move(board, player)
        winner = check_winner(board)
        if winner == 0:
            player = 3 - player  # Switch player
            moves += 1

    print_board(board)
    if winner != 0:
        print(f"Player {winner} wins!")
    else:
        print("It's a draw!")

if __name__ == "__main__":
    # Add these variables
    # var b00, b01, b02, b10, b11, b12, b20, b21, b22;
    # var player, row, col, moves, winner;
    
    
    
    main()