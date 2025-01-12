/*
 * uqWordLadder.c
 * Word Ladder
 * Created by: Adnaan Buksh
 * Student number: 47435568
 */

// includes
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <csse2310a1.h>

// constants
#define MAX_LENGTH 9
#define DEF_LENGTH 4
#define MIN_LENGTH 2
#define MAX_STEP 40
#define DEF_STEP 13
#define GAVE_UP_CODE 19
#define MAX_WORD 50
#define STEP_ERROR_CODE 13
#define FILE_ERROR_CODE 1
#define WORD_LEN_ERROR_CODE 15
#define LENGTH_CONFLICT_CODE 2
#define USAGE_ERROR_CODE 11
#define WORD_ERROR_CODE 10
#define WORD_SAME_CODE 4
#define DEF_DICT "/usr/share/dict/words"
#define LENGTH_CONFLICT "uqwordladder: Word length conflict - lengths must be\
 consistent"
#define WORD_LEN_ERROR "uqwordladder: Word length should be between 2 and 9\
 (inclusive)"
#define USAGE_ERROR "Usage: uqwordladder [--init initialWord]\
 [--target toWord] [--max stepLimit] [--length len] [--dict dictfilename]"
#define WORD_ERROR "uqwordladder: Words must not contain non-letters"
#define WORD_SAME "uqwordladder: Start and end words must not be the same"
#define STEP_ERROR "uqwordladder: Limit on steps must be word length to 40\
 (inclusive)"
#define FILE_NOT_OPENING "uqwordladder: File \"%s\" cannot be opened\n"
#define WELCOME "Welcome to UQWordLadder!\n\
Your goal is to turn '%s' into '%s' in at most %d steps\n"
#define ENTER_WORD "Enter word %i (or ? for help):\n"
#define GAVE_UP "Game over - you gave up."
#define ONLY_LETTERS "Word should contain only letters - try again."
#define DIFFER_ONE "Word must differ by only one letter - try again."
#define NO_PREVIOUS "You can't return to a previous word - try again."
#define NOT_IN_DICT "Word not found in dictionary - try again."
#define GAME_OVER "Game over - no more steps remaining."

/*Infomation need for the game*/
typedef struct {
    char* initWord; //initial word
    char* toWord; //target word
    unsigned int stepLim; //step limit
    unsigned int wordLen; //word length
    char* dict; //dictionary file location
    bool dictSet; //if dictionary file location is set
    bool wordLenSet; //if word length is set
    bool stepLimSet; //if step limit is set
    bool initWordSet; //if initial word is set
    bool toWordSet; //if target word is set
    int wordsInDict; //number of words in dictionary
    char** filteredDict; //stored array of words from dictionary
    char** givenWords; //stored array of words given by user
    int attempt; //number of attempts user has made
    int sugWordsLen; //number of words user can make
    char** sugWords; //array of suggested words user can make
} Data;

// functions

/* my_exit()
* −−−−−−−−−−−−−−−
* Frees all memory, that was allocated memory, and exits the program.
*
* exitCode: Specified exit code for the program to exit with.
* data: Struct containing all the data for the game.
*
* Returns: Nothing.
*/
void my_exit(int exitCode, Data data){
    free(data.initWord);
    free(data.toWord);
    //freeing all memory to avoid memory leaks
    if (data.wordsInDict > 0) {
        for (int i = 0; i < data.wordsInDict; i++) {
            free(data.filteredDict[i]);
        }
    }
    if (data.attempt > 1) {
        for (int i = 0; i < data.attempt - 1; i++) {
            free(data.givenWords[i]);
        }
    }
    if (data.sugWordsLen > 0) { 
        for (int i = 0; i < data.sugWordsLen; i++) {
            free(data.sugWords[i]);
        }
        
    }
    free(data.sugWords);
    free(data.filteredDict);
    free(data.givenWords);
    exit(exitCode);
}

/* error_exit()
* −−−−−−−−−−−−−−−
* Exits program with given error message and exit code 
*
* message: Error message to be printed to stderr.
* exitCode: Specified exit code for the program to exit with.
* data: Struct containing all the data for the game.
*
* Returns: Nothing.
*/
void error_exit(char* message, int exitCode, Data data) {
    fprintf(stderr, "%s\n", message);
    my_exit(exitCode, data);
}

/* check_digits()
* −−−−−−−−−−−−−−−
* Checks to see if all characters are digits 
*
* number: The given pointer to the string.
*
* Returns: boolean
*/
bool check_digits(const char* number) {
    while (*number) {
        if (*number < '0' || *number > '9') { 
            return false;
        }
        number++; //points to next char
    }
    return true;
}

/* check_chars()
* −−−−−−−−−−−−−−−
* Checks to see if all characters are characters 
*
* word: The given pointer to the string.
*
* Returns: boolean
*/
bool check_chars(const char* word) {
    while (*word) {
        if (!isalpha(*word)) {
            return false;
        }
        word++;
    }
    return true;
}

/* make_caps()
* −−−−−−−−−−−−−−−
* Iterates through the string and makes uppercase 
*
* word: The given pointer to the string.
*
* Returns: boolean
*/
void make_caps(char* word) {
    for (int i = 0; word[i]; i++) {
        word[i] = toupper(word[i]);
    }
}

/* check_input_length()
* −−−−−−−−−−−−−−−
* Checks if user inputs a length 
*
* data: Struct containing all the data for the game.
* argc: number of arguments given
* argv[]: arguments given
*
* Returns: Updated data
* Errors: Error message prints and exits if invalid length given
*/
Data check_input_length(Data data, int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        //compares to see is --length is a given argument
        if (strcmp(argv[i], "--length") == 0) {
            // (i + 1 < argc) checks if next arg exists
            if (i + 1 < argc && data.wordLenSet == false 
                    && atol(argv[i + 1]) > 0 && check_digits(argv[i + 1])) {
                data.wordLen = atol(argv[i + 1]);
                data.wordLenSet = true;
                i++;
            } else {
                fprintf(stderr, "%s\n", USAGE_ERROR);
                exit(USAGE_ERROR_CODE);
            }
        }
    }
    if (data.wordLenSet == false) {
        data.wordLen = DEF_LENGTH;
    }
    return data;
}

/* malloc_set()
* −−−−−−−−−−−−−−−
* −−−−−−−−−−−−−−−
* Allocates memory and sets variables
*
* data: Struct containing all the data for the game.
*
* Returns: Updated data
*/
Data malloc_set(Data data){
    //Cast incase of warning
    data.givenWords = (char** )malloc(sizeof(char*)); 
    data.filteredDict = (char** )malloc(sizeof(char*));
    data.wordsInDict = 0;
    data.attempt = 1;
    data.sugWords = (char** )malloc(sizeof(char*));
    data.sugWordsLen = 0;
    data.initWord = (char* )malloc(sizeof(char) * (data.wordLen + 1));
    data.toWord = (char* )malloc(sizeof(char) * (data.wordLen + 1));
    return data;
}

/* check_command_line()
* −−−−−−−−−−−−−−−
* Checks to see if command line inputs given are valid
*
* data: Struct containing all the data for the game.
* argc: number of arguments given
* argv[]: arguments given
*
* Returns: Updated data
* Errors: Error message prints and exits if invalid commands given
*/
Data check_command_line(Data data, int argc, char* argv[]) {
    data = check_input_length(data, argc, argv);
    //mallocs memory now so easy to free later
    data = malloc_set(data);
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--init") == 0 && i + 1 < argc 
                && data.initWordSet == false) {
            if (strlen(argv[i + 1]) != data.wordLen) {
                data.initWord = realloc(data.initWord,
                        sizeof(char) * (strlen(argv[i + 1]) + 1));
            }
            strcpy(data.initWord, argv[i + 1]);
            make_caps(data.initWord);
            data.initWordSet = true;
            i++;
        } else if (strcmp(argv[i], "--target") == 0 && i + 1 < argc 
                && data.toWordSet == false) {
            if (strlen(argv[i + 1]) != data.wordLen) {
                data.toWord = realloc(data.toWord, 
                        sizeof(char) * (strlen(argv[i + 1]) + 1));
            }
            strcpy(data.toWord, argv[i + 1]);
            //makes all letters uppercase
            make_caps(data.toWord);
            data.toWordSet = true;
            i++;
        } else if (strcmp(argv[i], "--max") == 0 && i + 1 < argc 
                && data.stepLimSet == false && atol(argv[i + 1]) > 0 
                && check_digits(argv[i + 1])) {
            data.stepLim = atol(argv[i + 1]);
            data.stepLimSet = true;
            i++;
        } else if (strcmp(argv[i], "--dict") == 0 && i + 1 < argc 
                && data.dictSet == false) {
            data.dict = argv[i + 1];
            data.dictSet = true;
            i++;
        } else if (strcmp(argv[i], "--length") == 0) {
            //Do nothing it was checked before
            i++;
        } else {
            error_exit(USAGE_ERROR, USAGE_ERROR_CODE, data);
        }
    }
    return data;
}

/* length_check()
* −−−−−−−−−−−−−−−
* Sets word lenght if not given then,
* Checks if the length of the words are the same as the word length
*
* data: Struct containing all the data for the game.
*
* Returns: Updated data
* Errors: Error message prints and exits if legnths are inconsistent or not in 
* valid range
*/
Data length_check(Data data) {
    //sets lenght of the word if not specified
    if (data.wordLenSet == false && data.initWordSet == false 
            && data.toWordSet == true) {
        data.wordLen = strlen(data.toWord);
    } else if (data.wordLenSet == false && data.initWordSet == true 
            && data.toWordSet == false) {
        data.wordLen = strlen(data.initWord);
    }
    //checks if the length of the words are the same
    if (data.initWordSet == true){
        if (strlen(data.initWord) != data.wordLen) {
            error_exit(LENGTH_CONFLICT, LENGTH_CONFLICT_CODE, data);
        } else if (data.toWordSet == true && strlen(data.toWord) 
                != strlen(data.initWord)) {
            error_exit(LENGTH_CONFLICT, LENGTH_CONFLICT_CODE, data);
        }
    } 
    if (data.toWordSet == true) {
        if (strlen(data.toWord) != data.wordLen) {
            error_exit(LENGTH_CONFLICT, LENGTH_CONFLICT_CODE, data);
        }
    } 
    //checks if the word length is in valid range
    if (data.wordLen < MIN_LENGTH || data.wordLen > MAX_LENGTH) {
        error_exit(WORD_LEN_ERROR, WORD_LEN_ERROR_CODE, data);
    }
    return data;
}

/* word_check()
* −−−−−−−−−−−−−−−
* Checks if all the character in word are valid 
*
* data: Struct containing all the data for the game.
*
* Returns: Updated data
* Errors: Error message prints and exits if initial and target words are same
*/
Data word_check(Data data) {
    if (data.initWordSet == true) {
        if (check_chars(data.initWord) == false) {
            error_exit(WORD_ERROR, WORD_ERROR_CODE, data);
        }
    } else {
        // stored in a temp variable to avoid same word being pointed to
        const char* temp = get_uqwordladder_word(data.wordLen);
        strcpy(data.initWord, temp);
    }
    if (data.toWordSet == true) {
        if (check_chars(data.toWord) == false) {
            error_exit(WORD_ERROR, WORD_ERROR_CODE, data);
        }
    } else {
        const char* temp2 = get_uqwordladder_word(data.wordLen);
        strcpy(data.toWord, temp2);
    }
    if (strcasecmp(data.initWord, data.toWord) == 0) {
        error_exit(WORD_SAME, WORD_SAME_CODE, data);
    }    
    return data;
}

/* step_check()
* −−−−−−−−−−−−−−−
* Checks if the step limit is valid 
*
* data: Struct containing all the data for the game.
*
* Returns: Updated data
* Errors: Error message prints and exits if stemp limit out of set range
*/
Data step_check(Data data) {
    if (data.stepLimSet == false) {
        data.stepLim = DEF_STEP;
    } else if (data.stepLim < data.wordLen || data.stepLim > MAX_STEP) {
        error_exit(STEP_ERROR, STEP_ERROR_CODE, data);
    }
    return data;
}

/* set_false()
* −−−−−−−−−−−−−−−
* Sets all the user changable variable bools to false
*
* data: Struct containing all the data for the game.
*
* Returns: Updated data
*/
Data set_false(Data data) {
    data.dictSet = false;
    data.wordLenSet = false;
    data.stepLimSet = false;
    data.initWordSet = false;
    data.toWordSet = false;
    return data;
}

/* read_dictionary()
* −−−−−−−−−−−−−−−
* Reads the dictionary file and stores all valid length words in an array
*
* data: Struct containing all the data for the game.
*
* Returns: Updated data
* Errors: Error message prints and exits if unreadable or non-existent
*/
Data read_dictionary(Data data) {
    if (data.dictSet == false) {
        data.dict = DEF_DICT;
    }
    FILE* file = fopen(data.dict, "r");
    if (file == NULL){
        fprintf(stderr, FILE_NOT_OPENING, data.dict);
        my_exit(FILE_ERROR_CODE, data);
    }
    char word[MAX_WORD + 1]; // +1 for null terminator

    while (fgets(word, sizeof(char*) * MAX_WORD, file) != NULL) {
        if (strlen(word) > 0 && word[strlen(word) - 1] == '\n') {
            word[strlen(word) - 1] = '\0';
        }
        if (strlen(word) == data.wordLen && check_chars(word)) {  
            data.filteredDict = (char**)realloc(data.filteredDict,
                sizeof(char*) * (data.wordsInDict + 1));
            make_caps(word);
            data.filteredDict[data.wordsInDict] = strdup(word);
            data.wordsInDict++;
        }
    }
    //closes file
    fclose(file);
    return data;
}

/* print_stdout()
* −−−−−−−−−−−−−−−
* Prints message to stdout
*
* message: Output message
*
* Returns: Nothing
*/
void print_stdout(char* message) {
    fprintf(stdout, "%s\n", message);
}

/* in_dict()
* −−−−−−−−−−−−−−−
* Check if the given word is in the dictionary
*
* word: given word to check
* data: Struct containing all the data for the game.
*
* Returns: boolean if word in dictionary
*/
bool in_dict(char* word, Data data) {
    for (int i = 0; i < data.wordsInDict; i++) {
        if (strcmp(word, data.filteredDict[i]) == 0) {
            return true;
        }
    }
    return false;
}

/* previous_word()
* −−−−−−−−−−−−−−−
* Check if the given word was a previous word entered
*
* word: given word to check
* data: Struct containing all the data for the game.
*
* Returns: boolean if word was previous word
*/
bool previous_word(Data data, char* word) {
    for (int i = 0; i < data.attempt - 1; i++) {
        if (strcmp(word, data.givenWords[i]) == 0) {
            return true;
        }
    }
    return false;
}

/* one_letter_diff()
* −−−−−−−−−−−−−−−
* Check if the given word was a previous word entered
*
* word1: given word to check
* word2: given word to check against
*
* Returns: boolean the 2 words are only 1 letter different
*/
bool one_letter_diff(char* word1, char* word2) {
    int diff = 0;
    for (int i = 0; i < strlen(word1); i++) {
        if (word1[i] != word2[i]) {
            diff++;
        }
    }
    if (diff == 1) {
        return true;
    }
    return false;
}

/* print_suggestions()
* −−−−−−−−−−−−−−−
* Prints all vaild attempts user can make or no suggestions available
*
* data: Struct containing all the data for the game.
* previous: given word to check against
*
* Returns: Nothing
*/
void print_suggestions(Data data, char* previous) {    
    //Check if given word is final word
    if (one_letter_diff(previous, data.toWord) == true) {
        data.sugWords[data.sugWordsLen] = strdup(data.toWord);
        data.sugWordsLen++;
    } 
    //adds all valid words to array
    for (int i = 0; i < data.wordsInDict; i++) {
        if (one_letter_diff(previous, data.filteredDict[i]) == true
                && previous_word(data, data.filteredDict[i]) == false
                && strcmp(data.filteredDict[i], data.toWord) != 0
                && strcmp(data.filteredDict[i], data.initWord) != 0) {
            data.sugWords = realloc(data.sugWords, 
                    sizeof(char*) * (data.sugWordsLen + 1));
            data.sugWords[data.sugWordsLen] = strdup(data.filteredDict[i]);
            data.sugWordsLen++;
        }
    }
    if (data.sugWordsLen == 0) {
        print_stdout("No suggestions available.");
    } else {
        print_stdout("Suggestions:-----------");
        for (int i = 0; i < data.sugWordsLen; i++) {
            fprintf(stdout, " %s\n", data.sugWords[i]);
        }
        print_stdout("-----End of Suggestions");
    }
}

/* check_input()
* −−−−−−−−−−−−−−−
* Gets user input and checks if it is valid
*
* data: Struct containing all the data for the game.
* input: users input
*
* Returns: Updated data
* Errors: Message prints and exits if game won or step limit reached 
*/
Data check_input(char input[MAX_LENGTH + 2], Data data){
    if (strcmp(input, "?") == 0) {
        if (data.attempt == 1) {
            print_suggestions(data, data.initWord);
        } else {
            print_suggestions(data, data.givenWords[data.attempt - 2]);
        }
    } else if (strlen(input) != data.wordLen) {
        fprintf(stdout, "Word should have %d characters - try again.\n",
                data.wordLen);
    } else if (check_chars(input) == false) {
        print_stdout(ONLY_LETTERS);
    } else if (( data.attempt == 1 && one_letter_diff(input, data.initWord) 
            == false) || (data.attempt > 1 && one_letter_diff(input,
            data.givenWords[data.attempt - 2]) == false)) { 
            //-2 cause attempt starts at 1
        print_stdout(DIFFER_ONE);
    } else if (strcmp(input, data.initWord) == 0 || 
            (data.attempt > 1 && previous_word(data, input) == true)) {
        print_stdout(NO_PREVIOUS);
    } else if (in_dict(input, data) == false) {
        print_stdout(NOT_IN_DICT);
    } else if (strcmp(input, data.toWord) == 0) {
        fprintf(stdout, "Well done - you solved the ladder in %d steps.\n",
                data.attempt);
        my_exit(0, data);
    } else if (data.attempt == data.stepLim) {
        print_stdout(GAME_OVER);
        my_exit(12, data);
    } else {
        //increases size of array and adds given valid word to it
        data.givenWords = (char**)realloc(data.givenWords, 
                sizeof(char*) * (data.attempt));
        data.givenWords[data.attempt - 1] = strdup(input);
        data.attempt++;
    }
    return data;
}

/* game_loop()
* −−−−−−−−−−−−−−−
* Gets user input and checks if it is valid
*
* data: Struct containing all the data for the game.
*
* Returns: Nothing
* Errors: Message prints and exits if exited 
* REF: Used Chat-GPT to convered my function with fgets, to something with
* REF: dynamically allocated memory. Which it gave getline.
* REF: Which was changed to fit styleguide and comments added to show my 
* REF: understanding of the function.
*/
void game_loop(Data data) {
    char* input = NULL; // Initialize input pointer to NULL
    size_t inputSize = 0; // Initialize inputSize to 0

    while (1) {
        fprintf(stdout, ENTER_WORD, data.attempt);
        // getline() reads the whole line from stdin.
        // since input is NULL and inputSize 0
        // getline will allocate sufficent memory for input
        if (getline(&input, &inputSize, stdin) == -1) {
            free(input); // Free the allocated memory
            print_stdout(GAVE_UP);
            my_exit(GAVE_UP_CODE, data);
        }
        // Remove trailing newline character
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }
        make_caps(input);
        data = check_input(input, data);
    }
    // Free dynamically allocated memory for input
    free(input);
}

int main(int argc, char* argv[]) {
    Data data;
    data = set_false(data);
    data = check_command_line(data, argc, argv);
    data = length_check(data);
    data = word_check(data);
    data = step_check(data);
    data = read_dictionary(data);
    //Welcome message
    fprintf(stdout, WELCOME, data.initWord, data.toWord, data.stepLim);
    //starts loop
    game_loop(data);
}