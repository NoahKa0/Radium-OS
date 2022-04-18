#include <cli/snake.h>
#include <memorymanagement/memorymanagement.h>

using namespace sys;
using namespace sys::cli;
using namespace sys::common;

void setChar(uint8_t x, uint8_t y, char c);
void printf(const char*);
void printNum(uint32_t);

CmdSnake::CmdSnake() {
  this->snake = (uint32_t*) MemoryManager::activeMemoryManager->malloc((WIDTH * HEIGHT + 1) * sizeof(uint32_t));
  this->length = 2;
  this->foodX = 10;
  this->foodY = 10;
  this->direction = 0;
}
CmdSnake::~CmdSnake() {
  MemoryManager::activeMemoryManager->free(this->snake);
}

void setChar(uint8_t x, uint8_t y, char c);

void CmdSnake::run(common::String** args, common::uint32_t argsLength) {
  this->inputEnabled = false; // Disable printing characters, and disable string inputs.

  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      setChar(x, y, ' ');
    }
  }

  this->snake[0] = this->toCoord(40, 12);
  this->snake[1] = this->toCoord(40, 13);

  uint32_t r = this->lcg(SystemTimer::getTimeInInterrupts());
  foodX = r % WIDTH;
  foodY = (r >> 8) % HEIGHT;

  bool isAlive = true;
  while (isAlive) {
    setChar(foodX, foodY, '%');

    int8_t x = this->toX(this->snake[0]);
    int8_t y = this->toY(this->snake[0]);

    if (direction == UP) {
      y--;
    }
    if (direction == DOWN) {
      y++;
    }
    if (direction == RIGHT) {
      x++;
    }
    if (direction == LEFT) {
      x--;
    }
    x = (x + WIDTH) % WIDTH; // X and Y should be bounded to with and height.
    y = (y + HEIGHT) % HEIGHT;

    uint32_t sc = this->toCoord(x, y);
    for (int i = 0; i < this->length; i++) {
      if (this->snake[i] == sc) {
        isAlive = false;
      }
    }

    if (x == foodX && y == foodY) {
      this->length++;
      r = this->lcg(SystemTimer::getTimeInInterrupts());
      foodX = r % WIDTH;
      foodY = (r >> 8) % HEIGHT;
    }

    if (this->length >= WIDTH * HEIGHT) {
      break;
    }

    setChar(this->toX(this->snake[this->length - 1]), this->toY(this->snake[this->length - 1]), ' ');
    setChar(x, y, '#');

    for (int i = 0; i < this->length; i++) {
      this->snake[this->length - i] = this->snake[this->length - i - 1];
    }
    this->snake[0] = this->toCoord(x, y);
    SystemTimer::sleep(this->length > 5 ? 100 : 200);
  }

  for (uint8_t x = 0; x < WIDTH; x++) {
    for (uint8_t y = 0; y < HEIGHT; y++) {
      setChar(x, y, ' ');
    }
  }

  if (this->length >= WIDTH * HEIGHT) {
    printf("You won the game!\n");
  }
  printf("Score: ");
  printNum(this->length);
  printf("\n");
}

void CmdSnake::onKeyDown(char c) {
  if (c == 'w' && this->direction != DOWN) {
    this->direction = UP;
  }
  if (c == 's' && this->direction != UP) {
    this->direction = DOWN;
  }
  if (c == 'a' && this->direction != RIGHT) {
    this->direction = LEFT;
  }
  if (c == 'd' && this->direction != LEFT) {
    this->direction = RIGHT;
  }
}

uint8_t CmdSnake::toX(uint32_t c) {
  return c % 80;
}

uint8_t CmdSnake::toY(uint32_t c) {
  return c / 80;
}

uint32_t CmdSnake::toCoord(uint8_t x, uint8_t y) {
  return y * 80 + x;
}

uint32_t CmdSnake::lcg(uint32_t n) {
  return ((n * 7621) + 1) % 32768;
}
