#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>


#define WIDTH 20
#define HEIGHT 20

typedef struct Position { // it will be contains the coordinates of snake node or apple
    int x;
    int y;
} Position;

typedef struct SnakeNode {
    Position pos;
    struct SnakeNode *next; // self-referencing struct
} SnakeNode;

typedef struct Snake {
    SnakeNode *head;
    SnakeNode *tail;
    int length;
    int direction;  
} Snake;

void clearScreen() // clear the entire screen
{
    printf("\033[2J"); 
    printf("\033[0;0f");
}


void saveScore(int score) {
    int bestScore = 0;
    FILE *Recordfile = fopen("hall_of_fame.txt", "r+");
    if (!Recordfile) return; // if file didn't found then just quit from the function
    else {
        fscanf(Recordfile, "%d", &bestScore); // Read the current record from file
        // if new score bigger than the previous one, then update
        if (score > bestScore) {
            rewind(Recordfile); // clear the file
            fprintf(Recordfile, "%d\n", score);
        }
    }
    fclose(Recordfile);
}

void ReadScore()
{
    FILE *Recordfile = fopen("hall_of_fame.txt", "r");
    if(!Recordfile) return; // if file didn't found then just quit from the function
    int number;
    while (fscanf(Recordfile, "%d", &number) == 1) { 
        printf("%d\n", number);            
    }

    fclose(Recordfile);
}

void disableBufferedInput() { //Disables canonical and echo mode for real-time input handling.
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag &= ~ICANON;
    term.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &term);
}

void enableBufferedInput() {
    struct termios term;
    tcgetattr(0, &term);
    term.c_lflag |= ICANON;
    term.c_lflag |= ECHO;
    tcsetattr(0, TCSANOW, &term);
}

int kbhit() { //check if the any key pressed or not
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

Snake* createSnake() {
    Snake *snake = (Snake*)malloc(sizeof(Snake)); 
    snake->head = (SnakeNode*)malloc(sizeof(SnakeNode));
    snake->tail = snake->head;
    snake->head->pos.x = WIDTH / 2; // coordinates of snake spawn
    snake->head->pos.y = HEIGHT / 2; // coordinates of snake spawn
    snake->head->next = NULL;
    snake->length = 1; // set the length of the snake
    snake->direction = 1;  //the origin direction is right
    return snake;
}

void placeApple(Position *apple) { //Randomly spawn the apple on the game field
    apple->x = rand() % (WIDTH - 2) + 1;
    apple->y = rand() % (HEIGHT - 2) + 1;
}

void display(Snake *snake, Position apple) {
    clearScreen();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (y == 0 || y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {
                printf("#");
            } else {
                int isSnake = 0;
                SnakeNode *current = snake->head;
                while (current) {
                    if (current->pos.x == x && current->pos.y == y) {
                        printf("O"); //snake
                        isSnake = 1;
                        break;
                    }
                    current = current->next;
                }
                if (!isSnake) {
                    if (apple.x == x && apple.y == y)
                        printf("A"); //apple
                    else
                        printf(" "); // free space
                }
            }
        }
        printf("\n");
    }
}

void displayTwoPlayers(Snake *snake1, Snake *snake2, Position apple) {
    clearScreen();
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (y == 0 || y == HEIGHT - 1 || x == 0 || x == WIDTH - 1) {
                printf("#");
            } else {
                int isSnake1 = 0, isSnake2 = 0;

                // check if head or body of first snake is in this chunk or not
                SnakeNode *current = snake1->head;
                while (current) {
                    if (current->pos.x == x && current->pos.y == y) {
                        isSnake1 = 1;
                        break;
                    }
                    current = current->next;
                }

                // check if head or body of second snake is in this chunk or not
                current = snake2->head;
                while (current) {
                    if (current->pos.x == x && current->pos.y == y) {
                        isSnake2 = 1;
                        break;
                    }
                    current = current->next;
                }

                // display symbols for apples and snakes
                if (isSnake1) {
                    printf("O"); // snake 1
                } else if (isSnake2) {
                    printf("X"); // Snake 2
                } else if (apple.x == x && apple.y == y) {
                    printf("A"); // apple
                } else {
                    printf(" "); // free space
                }
            }
        }
        printf("\n");
    }
}

int checkSelfCollision(Snake *snake) {
    Position headPos = snake->head->pos;
    SnakeNode *current = snake->head->next;
    while (current) {
        if (current->pos.x == headPos.x && current->pos.y == headPos.y) { // check if coordinates of the snake's head is the same with the any snake's body node or not
            return 1;
        }
        current = current->next;
    }
    return 0;
}

int checkCollisionWithSnake(Snake *snake1, Snake *snake2) {
    SnakeNode *current = snake2->head;
    while (current) {
        if (current->pos.x == snake1->head->pos.x && current->pos.y == snake1->head->pos.y) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

int updateSnake(Snake *snake, Position *apple, int *score) {
    Position newHeadPos = snake->head->pos;

    switch (snake->direction) {
        case 0: newHeadPos.y -= 1; break; // up
        case 1: newHeadPos.x += 1; break; // right
        case 2: newHeadPos.y += 1; break; // down
        case 3: newHeadPos.x -= 1; break; // left
    }

    //collision with boarder
    if (newHeadPos.x == 0 || newHeadPos.x == WIDTH - 1 || newHeadPos.y == 0 || newHeadPos.y == HEIGHT - 1) {
        return 1;
    }
    // check collision with itself
    if (checkSelfCollision(snake)) return 1;


    // new head
    SnakeNode *newHead = (SnakeNode*)malloc(sizeof(SnakeNode));
    newHead->pos = newHeadPos;
    newHead->next = snake->head;
    snake->head = newHead;
    snake->length++;

    // check if apple is eaten
    if (newHeadPos.x == apple->x && newHeadPos.y == apple->y) {
        (*score)++;
        placeApple(apple);
    } else {
        SnakeNode *temp = snake->head;
        while (temp->next != snake->tail) {
            temp = temp->next;
        }
        free(snake->tail);
        snake->tail = temp;
        snake->tail->next = NULL;
        snake->length--;
    }
    return 0;
}


void menu() {
    printf("=== Snake Game ===\n");
    printf("1. Play Game (1 Player)\n");
    printf("2. Play Game (2 Players)\n");
    printf("3. Hall of Fame\n");
    printf("4. Exit\n");
    printf("Choose an option: ");
}

void gameLoop() {
    Snake *snake = createSnake();
    Position apple;
    placeApple(&apple);
    int score = 0;

    while (1) {
        if (kbhit()) {
            char ch = getchar();
            int newDirection = snake->direction;
            switch (ch) {
                case 'w': newDirection = 0; break;
                case 'd': newDirection = 1; break;
                case 's': newDirection = 2; break;
                case 'a': newDirection = 3; break;
                case 'q': return;
            }
            // forbid the turn on 180 degrees
            if ((snake->direction % 2) != (newDirection % 2)) {
                snake->direction = newDirection;
            } 
        }
        if (updateSnake(snake, &apple, &score)) { //check if the game ended or not
            clearScreen();
            SnakeNode *current = snake->head;
            SnakeNode *temp;
            while (current!=NULL) {
            temp=current->next;
            free(current);
            current = temp;
        }
            printf("Game Over! Your score: %d\n", score);
            saveScore(score);
            printf("Press any key to return to the menu...\n");
            getchar(); 
            return;
        }
        display(snake, apple);
        usleep(200000); // game speed for this mode
    }
}

void gameLoopTwoPlayers() {
    Snake *snake1 = createSnake();
    Snake *snake2 = createSnake();
    snake2->direction=3;

    snake2->head->pos.x = WIDTH / 2 -1; // the initial position of the second snake
    snake2->head->pos.y = HEIGHT / 2; // the initial position of the second snake

    Position apple;
    placeApple(&apple);
    int score1 = 0, score2 = 0;

    while (1) {
        if (kbhit()) {
            char ch = getchar();
            int newDirection1 = snake1->direction;
            int newDirection2 = snake2->direction;

            switch (ch) {
                case 'w': newDirection1 = 0; break;
                case 'd': newDirection1 = 1; break;
                case 's': newDirection1 = 2; break;
                case 'a': newDirection1 = 3; break;
                case 'i': newDirection2 = 0; break;
                case 'l': newDirection2 = 1; break;
                case 'k': newDirection2 = 2; break;
                case 'j': newDirection2 = 3; break;
                case 'q': return; // exit to menu
            }

            // forbid the turn on 180 degrees
            if ((snake1->direction % 2) != (newDirection1 % 2)) {
                snake1->direction = newDirection1;
            }
            if ((snake2->direction % 2) != (newDirection2 % 2)) {
                snake2->direction = newDirection2;
            }
        }

        // update snakes
        if (updateSnake(snake1, &apple, &score1) ||
            updateSnake(snake2, &apple, &score2) ||
            checkCollisionWithSnake(snake1, snake2) ||
            checkCollisionWithSnake(snake2, snake1)) {

            clearScreen();
            printf("Game Over!\n");
            printf("Player 1 score: %d\n", score1);
            printf("Player 2 score: %d\n", score2);
            printf("Press any key to return to the menu...\n");
            getchar();
            return;
        }

        // display the field
        displayTwoPlayers(snake1, snake2, apple);

        usleep(500000); // game speed for this mode
    }
}

int main() {
    srand(time(0));
    disableBufferedInput();

    while (1) { // the main cycle
        clearScreen(); 
        menu(); // dispaly menu
        char choice = getchar();
        //getchar(); //if we want to confirm the input (extra feature)

        switch (choice) {
            case '1': // game for 1 player
                gameLoop();
                break;
            case '2': // game for 2 players
                gameLoopTwoPlayers();
                break;
            case '3': // to watch the records
                clearScreen();
                printf("\n");
                printf("The best reuslt is: ");
                ReadScore();
                printf("\npress any key to return in menu!\n");
                getchar();
                break;
            case '4': // exit
                enableBufferedInput();
                printf("Exiting...\n");
                return 0;

            default:
                printf("Invalid choice. Try again.\n");
                usleep(1000000); // a small pause
        }
    }

    enableBufferedInput(); // In case switch off the input
    return 0;
}
