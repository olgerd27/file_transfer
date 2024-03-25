/*
 * This example was provided by ChatGPT.
 * Execution of this program looks strange, so I'm not sure if this program works correctly 
 * on my Fedora Linux host.
 */
#include <stdio.h>
#include <unistd.h>

// Function to move the cursor to a specific position
void gotoXY(int x, int y) {
  printf("\033[%d;%dH", y, x);
}

// Function to update multiple lines of output
void updateOutput(int val) {
  // Update line 1
  gotoXY(1, 1);
  printf("Updated content for line 1: %i", val);

  // Update line 2
  gotoXY(1, 2);
  printf("Updated content for line 2: %i", val + 10);

  // Update line 3
  gotoXY(1, 3);
  printf("Updated content for line 3");
}

int main() {
  int i;
  for (i = 0; i <= 100; i++) {
    // Clear the screen
    printf("\033[2J");
    // Move cursor to top-left corner
    printf("\033[H");

    // Update output
    updateOutput(i);
    // Sleep for 100ms (0.1 second)
    usleep(100000);
  }
  return 0;
}

