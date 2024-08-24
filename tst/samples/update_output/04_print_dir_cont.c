/*
 * This example was provided by ChatGPT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

// Function to clear the screen
void clearScreen() {
    printf("\033[2J");
    printf("\033[H");
}

// Function to print directory contents and wait for user input
void printDirectoryContents(const char *dirname) {
    DIR *dir;
    struct dirent *entry;

    // Open directory
    dir = opendir(dirname);
    if (dir == NULL) {
        fprintf(stderr, "Error: Failed to open directory %s\n", dirname);
        return;
    }

    // Print directory contents
    printf("Contents of directory '%s':\n", dirname);
    while ((entry = readdir(dir)) != NULL) {
        printf("%s\n", entry->d_name);
    }

    // Close directory
    closedir(dir);

    // Wait for user input
    printf("\nPress ENTER to continue...");
    fflush(stdout); // Flush stdout to ensure prompt is displayed
    getchar(); // Wait for user input

    // Clear screen for the next directory
    clearScreen();
}

int main() {
    // List of directories to print contents
    const char *directories[] = {
        ".",
        "..",
        "../.."
        // Add more directories as needed
    };

    // Clear screen for the next directory
    clearScreen();

    // Iterate over directories
    for (int i = 0; i < sizeof(directories) / sizeof(directories[0]); i++) {
        printDirectoryContents(directories[i]);
    }

    return 0;
}

