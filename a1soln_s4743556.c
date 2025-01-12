/*
 * uqwordladder.c
 *	CSSE2310/7231 - Assignment One - 2023 - Semester Two
 *
 *	Written by Peter Sutton, p.sutton@uq.edu.au
 *	This version personalised for s4743556 Adnaan BUKSH
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <csse2310a1.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

// The maximum length of any dictionary word can be assumed to be 50 chars
#define MAX_DICTIONARY_WORD_LENGTH 50

// When reading dictionary words into a buffer, we need to allow space for
// the word + a newline + a terminating null
#define WORD_BUFFER_SIZE (MAX_DICTIONARY_WORD_LENGTH + 2)

// Default dictionary that we search
#define DEFAULT_DICTIONARY "/usr/share/dict/words"

// Limits on length of word
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 9
#define DEFAULT_WORD_LENGTH 4

// Command line option arguments
#define START_ARG_TEXT "--init"
#define END_ARG_TEXT "--target"
#define MAX_ARG_TEXT "--max"
#define LEN_ARG_TEXT "--length"
#define DICTIONARY_ARG_TEXT "--dict"

// Limit on the number of steps that will be permitted by default
// (i.e. if not specified on the command line)
#define DEFAULT_STEP_LIMIT 13

// Max value that will be accepted on the step limit (min value is the
// length of the word.)
#define MAX_STEP_LIMIT 40

// Usage error message
#define USAGE_MESSAGE "Usage: uqwordladder [--init initialWord] " \
	"[--target toWord] [--max stepLimit] " \
	"[--length len] [--dict dictfilename]\n"

// Error messages output when command line arguments are invalid
#define ERROR_MSG_INCONSISTENT \
	"uqwordladder: Word length conflict - lengths must be consistent\n"
#define ERROR_MSG_BAD_LENGTH \
	"uqwordladder: Word length should be between 2 and 9 (inclusive)\n"
#define ERROR_MSG_INVALID_WORD \
	"uqwordladder: Words must not contain non-letters\n"
#define ERROR_MSG_SAME_WORDS \
	"uqwordladder: Start and end words must not be the same\n"
#define ERROR_MSG_INVALID_STEP_LIMIT \
	"uqwordladder: Limit on steps must be word length to %d (inclusive)\n"
#define ERROR_MSG_BAD_DICTIONARY "uqwordladder: File \"%s\" cannot be opened\n"

// Messages output during game play and on game over
#define MSG_INVALID_WORD_LENGTH "Word should have %d characters - try again.\n"
#define MSG_INVALID_WORD_CHARACTERS \
	"Word should contain only letters - try again.\n"
#define MSG_INVALID_WORD_DIFFERENCE \
	"Word must differ by only one letter - try again.\n"
#define MSG_WORD_REPEATED "You can't return to a previous word - try again.\n"
#define MSG_WORD_NOT_IN_DICTIONARY \
	"Word not found in dictionary - try again.\n"
#define MSG_GAME_OVER_SUCCESS \
	"Well done - you solved the ladder in %d steps.\n"
#define MSG_GAME_OVER_RAN_OUT_OF_STEPS "Game over - no more steps remaining.\n"
#define MSG_GAME_OVER_EOF "Game over - you gave up.\n"

// Messages related to printing suggestions
#define MSG_SUGGESTIONS_HEADER "Suggestions:-----------\n"
#define MSG_SUGGESTIONS_FOOTER "-----End of Suggestions\n"
#define MSG_SUGGESTIONS_NONE_FOUND "No suggestions available.\n"

// Enumerated type with our exit statuses
typedef enum {
    OK = 0,
    DICTIONARY_ERROR = 1,
    GAME_OVER = 12,
    QUIT = 19,
    STEP_LIMIT_ERROR = 13,
    USAGE_ERROR = 11,
    WORD_ERROR = 10,
    WORD_LEN_CONSISTENCY_ERROR = 2,
    WORD_LEN_ERROR = 15,
    WORD_SAME_ERROR = 4
} ExitStatus;

// Enumerated type with our argument types - used for the getopt() version
// of command line argument parsing
typedef enum {
    FROM_ARG = 1,
    TO_ARG = 2,
    MAX_ARG = 3,
    LEN_ARG = 4,
    DICTIONARY_ARG = 5
} ArgType;

// Structure type to hold our game parameters - obtained from the command line
typedef struct {
    char* startWord;
    char* endWord;
    char* wordLenStr;
    int wordLen;
    char* stepLimitStr;
    int stepLimit;
    char* dictionaryFileName;
} GameParameters;

// Structure type to hold a list of words - used for the dictionary, as well
// as the list of entered words
typedef struct {
    int numWords;
    char** wordArray;
} WordList;

/* Function prototypes - see descriptions with the functions themselves */
GameParameters process_command_line(int argc, char* argv[]);
GameParameters process_command_line_getopt(int argc, char* argv[]);
int validate_word_lengths(const GameParameters param);
int validate_step_limit(const GameParameters param);
void validate_word_arguments(const GameParameters param);
void usage_error(void);
void word_length_error(void);
void word_length_consistency_error(void);
void same_word_error(void);
void step_limit_error(void);
void dictionary_error(const char* fileName);
bool is_string_positive_decimal(char* str);
void check_word_is_valid(const char* word);
WordList read_dictionary(const GameParameters param);
bool words_differ_by_one_char(const char* word1, const char* word2);
WordList add_word_to_list(WordList words, const char* word);
void free_word_list(WordList words);
char* convert_word_to_upper_case(char* word);
bool word_contains_only_letters(const char* word);
bool is_word_in_list(const char* word, WordList words);
char* read_line(void);
ExitStatus play_game(GameParameters param, WordList words);
bool check_attempt(const char* attempt, int wordLen, WordList validWords,
	WordList previousSteps, const char* targetWord);
void print_suggestions(WordList previousWords, WordList validWords, 
	const char* targetWord);

/*****************************************************************************/
int main(int argc, char* argv[]) {
    GameParameters gameDetails;
    WordList validWords;

    // Process the command line arguments - and get supplied game parameters.
    // Will not return if arguments are invalid - prints message and exits. 
#ifdef USE_GETOPT
    gameDetails = process_command_line_getopt(argc, argv);
#else
    gameDetails = process_command_line(argc, argv);
#endif
    // Check the word lengths are valid/consistent and determine the length
    gameDetails.wordLen = validate_word_lengths(gameDetails);

    // Check the words are valid and different.
    validate_word_arguments(gameDetails);

    // Make sure the step limit is valid if specified
    gameDetails.stepLimit = validate_step_limit(gameDetails);

    // Read the dictionary into memory
    validWords = read_dictionary(gameDetails);

    // If one or other word is not specified, get and copy a random word from
    // the dictionary. If a word is not coming from the dictionary we also 
    // strdup() so that we know these strings are always in dynamically 
    // allocated memory (makes freeing easier).
    if (!gameDetails.startWord) {
	gameDetails.startWord = 
		strdup(get_uqwordladder_word(gameDetails.wordLen));
    } else {
	gameDetails.startWord = strdup(gameDetails.startWord);
    }
    if (!gameDetails.endWord) {
	gameDetails.endWord = 
		strdup(get_uqwordladder_word(gameDetails.wordLen));
    } else {
	gameDetails.endWord = strdup(gameDetails.endWord);
    }

    // Play the game and output the result
    ExitStatus status = play_game(gameDetails, validWords);

    // Tidy up and exit
    free_word_list(validWords);
    free(gameDetails.startWord);
    free(gameDetails.endWord);
    return status;
}

/* 
 * process_command_line()
 * 	Go over the supplied command line arguments, check their validity, and
 *	if OK return the game parameters. (The start and end words, if given,
 *	are converted to upper case.) If the command line is invalid, then 
 *	we print a usage error message and exit. 
 */
GameParameters process_command_line(int argc, char* argv[]) {
    // No parameters to start with (these values will be updated with values
    // from the command line, if specified)
    GameParameters param = { .startWord = NULL, .endWord = NULL,
	    .wordLenStr = NULL, .stepLimitStr = NULL, 
	    .dictionaryFileName = NULL };

    // Skip over the program name argument
    argc--;
    argv++;

    // Check for option arguments - we know they come in pairs with a value
    // argument so only keep processing if there are at least 2 args left.
    while (argc >= 2 && argv[0][0] == '-') {
	if (strcmp(argv[0], START_ARG_TEXT) == 0 && !param.startWord) {
	    param.startWord = convert_word_to_upper_case(argv[1]);
	} else if (strcmp(argv[0], END_ARG_TEXT) == 0 && !param.endWord) {
	    param.endWord = convert_word_to_upper_case(argv[1]);
	} else if (strcmp(argv[0], LEN_ARG_TEXT) == 0 && !param.wordLenStr &&
		is_string_positive_decimal(argv[1])) {
	    param.wordLenStr = argv[1];
	} else if (strcmp(argv[0], MAX_ARG_TEXT) == 0 && !param.stepLimitStr &&
		is_string_positive_decimal(argv[1])) {
	    param.stepLimitStr = argv[1];
	} else if (strcmp(argv[0], DICTIONARY_ARG_TEXT) == 0 
		&& !param.dictionaryFileName) {
	    param.dictionaryFileName = argv[1];
	} else {
	    // Unexpected argument (could be a repeated argument also)
	    usage_error();
	}
	// If we got here, we processed an option argument and value - skip
	// over those, then return to the top of the loop to check for more
	argc -= 2;
	argv += 2;
    }

    // If any arguments now remain then this is a usage error
    if (argc) {
	usage_error();
    }

    return param;
}

/* 
 * process_command_line_getopt()
 * 	Go over the supplied command line arguments, check their validity, and
 *	if OK return the game parameters. (The start and end words, if given,
 *	are converted to upper case.) If the command line is invalid, then 
 *	we print a usage error message and exit. 
 *	This is a getopt version of the command line processing.
 */
GameParameters process_command_line_getopt(int argc, char* argv[]) {
    // No parameters to start with (these values will be updated with values
    // from the command line, if specified)
    GameParameters param = { .startWord = NULL, .endWord = NULL,
	    .wordLenStr = NULL, .stepLimitStr = NULL, 
	    .dictionaryFileName = NULL };

    // REF: Code based on the example in the getopt(3) man page
    // Note the "+ 2"s here are to skip over the "--" part of the argument
    // string because those constants are defined for the arg checking function
    // above. If you were only using getopt() then you would define the
    // constants differently, i.e. without the "--" prefix.
    struct option longOptions[] = {
	    { START_ARG_TEXT + 2, required_argument, 0, FROM_ARG},
	    { END_ARG_TEXT + 2, required_argument, 0, TO_ARG},
	    { MAX_ARG_TEXT + 2, required_argument, 0, MAX_ARG},
	    { LEN_ARG_TEXT + 2, required_argument, 0, LEN_ARG},
	    { DICTIONARY_ARG_TEXT + 2, required_argument, 0, DICTIONARY_ARG},
	    { 0, 0, 0, 0}};
    int optionIndex = 0;

    while (true) {
	// Get the next option argument. (":" prevents error message printing)
	int opt = getopt_long(argc, argv, ":", longOptions, &optionIndex);
	fflush(stdout);
	if (opt == -1) { // Ran out of option arguments
	    break;
	} else if (opt == FROM_ARG && !param.startWord) {
	    param.startWord = convert_word_to_upper_case(optarg);
	} else if (opt == TO_ARG && !param.endWord) {
	    param.endWord = convert_word_to_upper_case(optarg);
	} else if (opt == LEN_ARG && !param.wordLenStr &&
		is_string_positive_decimal(optarg)) {
	    param.wordLenStr = optarg;
	} else if (opt == MAX_ARG && !param.stepLimitStr &&
		is_string_positive_decimal(optarg)) {
	    param.stepLimitStr = optarg;
	} else if (opt == DICTIONARY_ARG && !param.dictionaryFileName) {
	    param.dictionaryFileName = optarg;
	} else {
	    usage_error();
	}
    }

    // If any arguments now remain, then this is a usage error.
    if (optind < argc) {
	usage_error();
    }

    return param;
}

/*
 * validate_word_lengths()
 * 	Makes sure the word lengths given in the game parameters are valid and
 * 	consistent, otherwise we exit with an appropriate message. If the 
 * 	lengths specified are correct and consistent then we return the word
 * 	length. If no lengths are specified in the game parameters then we 
 * 	return the default length.
 */
int validate_word_lengths(const GameParameters param) {
    int len = DEFAULT_WORD_LENGTH;
    // Get the wordLength from command line, if given. We already know the
    // wordLenStr element is a positive decimal integer.
    if (param.wordLenStr) {
	len = atoi(param.wordLenStr);
	if (strlen(param.wordLenStr) >= (sizeof(int) * 2)) {
	    // The word length string is way too long and could have overflowed
	    // so we just set the length to INT_MAX. sizeof(int)*2 is the 
	    // number of hex digits in an integer so we can be sure if the num
	    // of decimal digits is this or bigger then we have a problem.
	    len = INT_MAX;
	}
    }

    // Check for word length consistency if more than one of the starter word,
    // target word and word length are specified.
    if (param.startWord && param.endWord && 
	    strlen(param.startWord) != strlen(param.endWord)) {
	word_length_consistency_error();
    }
    if (param.startWord && param.wordLenStr && 
	    strlen(param.startWord) != len) {
	word_length_consistency_error();
    }
    if (param.endWord && param.wordLenStr && strlen(param.endWord) != len) {
	word_length_consistency_error();
    }

    // Lengths are consistent. Make sure we have the value from whatever
    // field has been specified.
    if (!param.wordLenStr) {
	if (param.startWord) {
	    len = strlen(param.startWord);
	} else if (param.endWord) {
	    len = strlen(param.endWord);
	}
    } // else have already set the 'len' variable to this length

    // Check the length is valid
    if (len < MIN_WORD_LENGTH || len > MAX_WORD_LENGTH) {
	word_length_error();
    }

    // Lengths consistent and valid - return the word length for this game
    return len;
}

/*
 * validate_step_limit()
 * 	Makes sure the step limit in the game parameters (param)
 * 	is valid if specified - it must be between the word length 
 * 	and MAX_STEP_LIMIT. If not, we will print an appropriate 
 * 	error message and exit. We return the step limit (which
 * 	will be default limit if it wasn't specified on the command
 * 	line).
 */
int validate_step_limit(const GameParameters param) {
    if (param.stepLimitStr) {
	// We already know the string must represent a positive decimal integer
	// so we need to check the range. There is a possibility that atoi()
	// overflows, so we also check that the length of the string is as
	// expected.
	int limit = atoi(param.stepLimitStr);
	if (strlen(param.stepLimitStr) > 2 ||
		limit < param.wordLen || limit > MAX_STEP_LIMIT) {
	    step_limit_error();	// will not return
	}
	return limit;
    } else {
	return DEFAULT_STEP_LIMIT;
    }
}

/*
 * validate_word_arguments()
 * 	Check the initial and target words provided on the command line
 * 	(from param) to make sure the words are valid and (if both are 
 * 	given) different. Exit with an error message if not.
 */
void validate_word_arguments(const GameParameters param) {
    // If a start or end word has been given on the command line then
    // check they are valid 
    if (param.startWord) {
	check_word_is_valid(param.startWord); // may not return
    }
    if (param.endWord) {
	check_word_is_valid(param.endWord); // may not return
    }
    if (param.startWord && param.endWord &&
	    strcmp(param.startWord, param.endWord) == 0) {
	same_word_error();	// does not return
    }
    // Provided words (if any) OK if we get here
}

/*  
 * usage_error()
 *	Print the usage error message, then exit with a non-zero exit status.
 */
void usage_error(void) {
    fprintf(stderr, USAGE_MESSAGE);
    exit(USAGE_ERROR);
}

/*
 * word_length_error()
 * 	Print an error message about incorrect word lengths, then
 * 	exit with a non-zero exit status.
 */
void word_length_error(void) {
    fprintf(stderr, ERROR_MSG_BAD_LENGTH);
    exit(WORD_LEN_ERROR);
}

/*
 * word_length_consistency_error()
 * 	Print an error message about inconsistent word lengths, then
 * 	exit with a non-zero exit status.
 */
void word_length_consistency_error(void) {
    fprintf(stderr, ERROR_MSG_INCONSISTENT);
    exit(WORD_LEN_CONSISTENCY_ERROR);
}

/*
 * same_word_error()
 * 	Print an error message about words being the same, then exit
 * 	with a non-zero exit status.
 */
void same_word_error(void) {
    fprintf(stderr, ERROR_MSG_SAME_WORDS);
    exit(WORD_SAME_ERROR);
}

/*
 * step_limit_error()
 * 	Print an error message about invalid step limit, then exit with a 
 * 	non-zero exit status.
 */
void step_limit_error(void) {
    fprintf(stderr, ERROR_MSG_INVALID_STEP_LIMIT, MAX_STEP_LIMIT);
    exit(STEP_LIMIT_ERROR);
}

/*  
 * dictionary_error()
 *	Print the error message about being unable to open dictionary
 *	(including the supplied fileName in the message). Exit
 *	with appropriate exit code.
 */
void dictionary_error(const char* fileName) {
    fprintf(stderr, ERROR_MSG_BAD_DICTIONARY, fileName);
    exit(DICTIONARY_ERROR);
}

/*
 * is_string_postive_decimal()
 * 	Returns true if the given string (str) is the string representation of
 * 	a positive decimal integer.
 */
bool is_string_positive_decimal(char* str) {
    // First digit must be 1 to 9
    if (str[0] < '1' || str[0] > '9') {
	return false;
    }
    // Other digits (if present) must be 0 to 9
    str++;
    while (*str && isdigit(*str)) {
	str++;
    }

    // If we exited the loop at the end of the string then every other
    // character found was a digit. 
    return (*str == '\0');
}

/*
 * check_word_is_valid()
 * 	Checks if the given word is a valid word - contains only letters
 * 	If not, an error message is printed and the program exits with an 
 * 	appropriate code.
 */
void check_word_is_valid(const char* word) {
    if (!word_contains_only_letters(word)) {
	fprintf(stderr, ERROR_MSG_INVALID_WORD);
	exit(WORD_ERROR);
    }
}

/*
 * read_dictionary()
 *	Read all words from the dictionary file name specified in the game
 *	parameters from the command line (param) (or the default
 *	dictionary file if this is null). We exit with an error message
 *	if the dictionary can't be opened, otherwise we return the set of
 *	words from the dictionary that contain only letters and of the 
 *	correct length (also specified in the game parameters).
 */
WordList read_dictionary(const GameParameters param) {
    const char* fileName;
    if (param.dictionaryFileName) {
	fileName = param.dictionaryFileName;
    } else {
	// No dictionary given on command line, use default
	fileName = DEFAULT_DICTIONARY;
    }
    FILE* fileStream = fopen(fileName, "r");
    if (!fileStream) {
	// Can't open file for reading - print error message and exit
	dictionary_error(fileName);
    }

    WordList validWords;
    char currentWord[WORD_BUFFER_SIZE]; 	// Buffer to hold word.

    // Initialise our list of matches - nothing in it initially.
    validWords.numWords = 0;
    validWords.wordArray = 0;

    // Read lines of file one by one 
    while (fgets(currentWord, WORD_BUFFER_SIZE, fileStream)) {
	// Word has been read - remove any newline at the end if present
	int len = strlen(currentWord);
	if (len > 0 && currentWord[len - 1] == '\n') {
	    currentWord[len - 1] = '\0';
	    len--;
	}
	// If the word is the right length and contains only letters then
	// convert it to uppercase and add it to the dictionary
	if (len == param.wordLen && word_contains_only_letters(currentWord)) {
	    convert_word_to_upper_case(currentWord);
	    validWords = add_word_to_list(validWords, currentWord);
	}
    }
    fclose(fileStream);
    return validWords;
}

/* 
 * words_differ_by_one_char()
 * 	Returns true if the two words are the same length and differ
 * 	only by one character. The words are guaranteed to be uppercase.
 */
bool words_differ_by_one_char(const char* word1, const char* word2) {
    int wordLen = strlen(word1);
    if (strlen(word2) != wordLen) {
	return false;
    }
    int numCharsSame = 0;
    for (int i = 0; i < wordLen; i++) {
	if (word1[i] == word2[i]) {
	    numCharsSame++;
	}
    }
    return (numCharsSame == wordLen - 1);
}

/* 
 * add_word_to_list()
 *	Copy the given word and add it to the given list of matches (words).
 */
WordList add_word_to_list(WordList words, const char* word) {
    char* wordCopy;

    // Make a copy of the word into newly allocated memory
    wordCopy = strdup(word);

    // Make sure we have enough space to store our list (array) of
    // words.
    words.wordArray = realloc(words.wordArray,
	    sizeof(char*) * (words.numWords + 1));
    words.wordArray[words.numWords] = wordCopy;	// Add word to list
    words.numWords++;	// Update count of words
    return words;
}

/*
 * free_word_list()
 * 	Deallocates all memory associated with the given list of words.
 */
void free_word_list(WordList words) {
    for (int i = 0; i < words.numWords; i++) {
	free(words.wordArray[i]);
    }
    free(words.wordArray);
}

/*
 * convert_word_to_upper_case()
 * 	Traverses the supplied word and converts each lower case letter
 * 	into the upper case equivalent. We return a pointer to the string
 * 	we're given.
 */
char* convert_word_to_upper_case(char* word) {
    char* cursor = word;
    while (*cursor) {
	*cursor = toupper(*cursor);
	cursor++;
    }
    return word;
}

/*
 * word_contains_only_letters()
 * 	Traverses the word and returns true if the string contains only
 * 	letters (upper or lower case), false otherwise.
 */
bool word_contains_only_letters(const char* word) {
    while (*word) {
	if (!isalpha(*word)) {
	    return false;
	} 
	word++;
    }
    return true;
}

/*
 * is_word_in_list()
 * 	Returns true if the given word is in the given list of words (words),
 * 	false otherwise. The word and all words in the list are known to be 
 * 	upper case.
 */
bool is_word_in_list(const char* word, WordList words) {
    for (int i = 0; i < words.numWords; i++) {
	if (strcmp(words.wordArray[i], word) == 0) {
	    return true;
	}
    }
    // Word not found
    return false;
}

/*
 * read_line()
 *	Read a line of indeterminate length from stdin (i.e. we read
 * 	characters until we reach a newline or EOF). If we hit EOF at 
 *	the start of the line then we return NULL, otherwise we 
 *	return the line of text (without any newline) in dynamically 
 *	allocated memory.
 */
char* read_line(void) {
    char* buffer = NULL;
    int bufSize = 0;
    int character;

    while (character = fgetc(stdin), (character != EOF && character != '\n')) {
	// Got a character - grow the buffer and put it in the buffer
	buffer = realloc(buffer, bufSize + 1);
	buffer[bufSize] = character;
	bufSize++;
    }
    // Got EOF or newline
    if (character == '\n' || bufSize > 0) {
	// Have a line to return - grow the buffer and null terminate it
	buffer = realloc(buffer, bufSize + 1);
	buffer[bufSize] = '\0';
    } // else got EOF at start of line - we'll be returning NULL
    return buffer;
}

/*
 * play_game()
 *	Play the uqwordladder game with the given parameters and list of
 *	valid words from the dictionary.
 *	We return an exit status suitable for returning from main.
 */
ExitStatus play_game(GameParameters param, WordList dictionary) {
    int numStepsMade = 0;
    bool success = false;
    WordList previousSteps = {0, NULL};
    previousSteps = add_word_to_list(previousSteps, param.startWord);

    printf("Welcome to UQWordLadder!\n");
    printf("Your goal is to turn '%s' into '%s' in at most %d steps\n",
	    param.startWord, param.endWord, param.stepLimit);

    while (numStepsMade < param.stepLimit) {
	// Prompt for word
	printf("Enter word %d (or ? for help):\n", numStepsMade + 1);
	// Read line of text from user. Abort if EOF.
	char* attempt = read_line();
	if (!attempt) {
	    break; // EOF
	}
	if (strcmp(attempt, "?") == 0) { // User has asked for help
	    print_suggestions(previousSteps, dictionary, param.endWord);
	} else {
	    // Convert the word to upper case, and make sure it is valid.
	    // If so, check if we've solved the game, otherwise add it to 
	    // our list of attempts.
	    convert_word_to_upper_case(attempt);
	    if (check_attempt(attempt, param.wordLen, dictionary, 
		    previousSteps, param.endWord)) {
		numStepsMade++;
		if (strcmp(attempt, param.endWord) == 0) { // Got it!
		    success = true;
		    free(attempt);
		    break;
		}
		previousSteps = add_word_to_list(previousSteps, attempt);
	    }
	}
	free(attempt);
    }
    // Tidy up, print the appropriate message to the user and return
    free_word_list(previousSteps);
    if (success) {
	printf(MSG_GAME_OVER_SUCCESS, numStepsMade);
	return OK;
    } else if (numStepsMade == param.stepLimit) {
	printf(MSG_GAME_OVER_RAN_OUT_OF_STEPS);
	return GAME_OVER;
    } else { // must have hit EOF
	printf(MSG_GAME_OVER_EOF);
	return QUIT;
    }
}

/*
 * check_attempt()
 *	Check that the given attempt is valid, i.e. that it has the right
 *	length (matches wordLen), that it contains only letters, 
 *	differs by one letter from the previous attempt (last element in 
 *	previousSteps), that it isn't one of the previous attempts, and
 *	that either it is the target word or it's in teh dictionary 
 *	(validWords).
 *	Return true if OK, false otherwise (with a suitable message printed).
 */
bool check_attempt(const char* attempt, int wordLen,
	WordList validWords, WordList previousSteps, const char* targetWord) {
    if (strlen(attempt) != wordLen) {
	printf(MSG_INVALID_WORD_LENGTH, wordLen);
	return false;
    }
    if (!word_contains_only_letters(attempt)) {
	printf(MSG_INVALID_WORD_CHARACTERS);
	return false;
    }
    char* lastWord = previousSteps.wordArray[previousSteps.numWords - 1];
    if (!words_differ_by_one_char(attempt, lastWord)) {
	printf(MSG_INVALID_WORD_DIFFERENCE);
	return false;
    }
    if (is_word_in_list(attempt, previousSteps)) {
	printf(MSG_WORD_REPEATED);
	return false;
    }
    if (strcmp(attempt, targetWord) == 0) {
	// User has reached the target word
	return true;
    }
    if (!is_word_in_list(attempt, validWords)) {
	printf(MSG_WORD_NOT_IN_DICTIONARY);
	return false;
    }
    return true;	// The attempt is OK
}

/*
 * print_suggestions()
 * 	Print a list of all the words in the dictionary (validWords) that
 * 	are only one character different from the last word (last entry in
 * 	previousSteps) and are not the same as any of the words previously 
 * 	used. If the targetWord is a valid option then this is printed first
 * 	(and is not repeated if found in the dictionary)
 */
void print_suggestions(WordList previousSteps, WordList validWords, 
	const char* targetWord) {
    char* lastWord = previousSteps.wordArray[previousSteps.numWords - 1];
    bool havePrintedHeader = false;
    if (words_differ_by_one_char(lastWord, targetWord)) {
	printf(MSG_SUGGESTIONS_HEADER);
	havePrintedHeader = true;
	printf(" %s\n", targetWord);
    }
    // Iterate over all the valid words in the dictionary
    for (int i = 0; i < validWords.numWords; i++) {
	char* wordToCheck = validWords.wordArray[i];
	if (words_differ_by_one_char(lastWord, wordToCheck)  &&
		!is_word_in_list(wordToCheck, previousSteps) &&
		strcmp(targetWord, wordToCheck)) {
	    // Have found a word - print out the header if we haven't yet, 
	    // then print the word
	    if (!havePrintedHeader) {
		printf(MSG_SUGGESTIONS_HEADER);
		havePrintedHeader = true;
	    }
	    printf(" %s\n", wordToCheck);
	}
    }
    // if we've printed some suggestions, then print the "footer" for this
    // list, otherwise print that we didn't find any.
    if (havePrintedHeader) {
	printf(MSG_SUGGESTIONS_FOOTER);
    } else {
	printf(MSG_SUGGESTIONS_NONE_FOUND);
    }
}
