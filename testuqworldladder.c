/*
 * testuqwordladder.c
 * Tests for Word Ladder
 * Created by: Adnaan Buksh
 * Student number: 47435568
 */

// includes
#include <time.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <csse2310a3.h>

// constants
#define GOOD_PROG "good-uqwordladder"
#define JOBDIR_DEF "./tmp"
#define USAGE_ERROR "Usage: testuqwordladder [--diffshow N] [--jobdir dir]\
 [--regenerate] jobfile program"
#define USAGE_ERROR_CODE 3
#define JOBFILE_ERROR "testuqwordladder: Unable to open file \"%s\"\n"
#define JOBFILE_ERROR_CODE 6
#define JOB_ERROR "testuqwordladder: Syntax error on line %i of job spec file\
 \"%s\"\n"
#define JOB_ERROR_CODE 19
#define JOB_DUP "testuqwordladder: Duplicate Job ID on line %i of job file\
 \"%s\"\n"
#define JOB_DUP_CODE 1
#define INFILE_ERROR "testuqwordladder: Unable to open file \"%s\" specified\
 on line %i of file \"%s\"\n"
#define INFILE_ERROR_CODE 20
#define JOBFILE_EMPTY "testuqwordladder: Job spec file \"%s\" is empty\n"
#define JOBFILE_EMPTY_CODE 4
#define DIR_FAIL "testuqwordladder: Can't create directory named \"%s\"\n"
#define DIR_FAIL_CODE 16
#define FILE_FAIL "testuqwordladder: Unable to open file \"%s\" for writing\n"
#define FILE_FAIL_CODE 15
#define NO_TESTS "testuqwordladder: No tests have been finished\n"
#define NO_TESTS_CODE 9
#define STDOUT_MATCH "Job %s: Stdout matches\n"
#define STDOUT_DIFF "Job %s: Stdout differs\n"
#define STDERR_MATCH "Job %s: Stderr matches\n"
#define STDERR_DIFF "Job %s: Stderr differs\n"
#define EXIT_MATCH "Job %s: Exit status matches\n"
#define EXIT_DIFF "Job %s: Exit status differs\n"
#define UNABLE_EXEC "Unable to execute test job %s\n"
#define RUN_TEST "Running test: %s\n"
#define PASSED_TESTS "testuqwordladder: %d out of %d tests passed\n"
#define EXIT_STAT_FILE "%s/%s.exitstatus"
#define STDOUT_FILE "%s/%s.stdout"
#define STDERR_FILE "%s/%s.stderr"
#define NULL_FILE "/dev/null"
#define READ_END 0
#define WRITE_END 1
#define PROG_FAIL 12
#define PROG_SUCCESS 0

// global variable

/*
 * interupt flag for signal SIGINT
 * false if not interupted
 * true when interupted
 */
bool interrupted = false;

// Structure type that hold infomatino about a job to be run
typedef struct {
    int numArgs;
    char* testId;
    char* inFileName;
    char** givenArgs;
} Job;

// Structure type that holds all the jobs to be run and how many there are
typedef struct {
    int numJobs;
    Job* jobs;
} Alljobs;

// Structure type that holds all the data for the program
typedef struct {
    char* jobFile;
    char* program;
    bool regen;
    char* jobdir;
    Alljobs alljobs;
} Data;

// functions

/* interrupt_handler()
* −----------------
* Handling SIGINT signal
* aborts any test in progress and exits the program
* 
* sig : signal number 
*
* Global variable modified: interrupted
*/
void interrupt_handler(int s) {
    interrupted = true;
}

/* free_job()
* −----------------
* Frees the memory allocated for all jobs
*
* data: Struct containing all the data for the program.
*/
void free_job(Data data) {
    for (int i = 0; i < data.alljobs.numJobs; i++) {
        free(data.alljobs.jobs[i].testId);
        free(data.alljobs.jobs[i].inFileName);
        for (int j = 0; j < data.alljobs.jobs[i].numArgs - 2; j++) {
            free(data.alljobs.jobs[i].givenArgs[j]);
        }
        free(data.alljobs.jobs[i].givenArgs);
    }
    free(data.alljobs.jobs);
}

/* error_exit()
* −−−−−−−−−−−−−−−
* Exits program with given error message and exit code 
*
* message: Error message to be printed to stderr.
* exitCode: Specified exit code for the program to exit with.
*/
void error_exit(char* message, int exitCode) {
    fprintf(stderr, "%s\n", message);
    exit(exitCode);
}

/* regen_message()
* −----------------
* Prints message to stdout that the expected output is being regenerated
*
* testId: The testId of the test that is being regenerated
*/
void regen_message(char* testId) {
    fprintf(stdout, "Regenerating expected output for test %s\n", testId);
}

/* gen_message()
* −----------------
* Prints message to stdout and flushes stdout
*
* message: The message to be printed to stdout
* curJob: The current job that is being run
*/
void gen_message(char* message, Job curJob){
    fprintf(stdout, message, curJob.testId);
    fflush(stdout);
}

/* file_fail()
* −----------------
* Prints message to stderr that the file failed to open
*
* fileName: The name of the file that failed to open
*/
void file_fail(char* fileName) {
    fprintf(stderr, FILE_FAIL, fileName);
    exit(FILE_FAIL_CODE);
}

/* error_message()
* −----------------
* Prints message to stderr depending on the exit code given
*
* exitCode: The exit code that determines which message to print
* data: Struct containing all the data for the program.
*/
void error_message(int exitCode, Data data) {
    switch (exitCode) {
        case USAGE_ERROR_CODE:
            error_exit(USAGE_ERROR, USAGE_ERROR_CODE);
            break;
        case JOBFILE_ERROR_CODE:
            fprintf(stderr, JOBFILE_ERROR, data.jobFile);
            break;
        case JOBFILE_EMPTY_CODE:
            fprintf(stderr, JOBFILE_EMPTY, data.jobFile);
            break;
        case DIR_FAIL_CODE:
            fprintf(stderr, DIR_FAIL, data.jobdir);
            break;
        default:
            break;
    }
    exit(exitCode);
}

/* check_command_line()
* −−−−−−−−−−−−−−−
* Checks to see if command line inputs given are valid
*
* data: Struct containing all the data for the program.
* argc: Number of command line arguments
* argv: Array of command line arguments
*
* Returns: updated data struct
* Errors: if insufficient or too many arguments are given or invalid
*/
Data check_command_line(Data data, int argc, char* argv[]) {

    if (argc < 3 || argc > 6) {
        error_exit(USAGE_ERROR, USAGE_ERROR_CODE);
    }

    if (argc > 3) {
        for (int i = 1; i < argc - 2; i++) {
            if (strcmp(argv[i], "--jobdir") == 0 && !data.jobdir) {
                if (i + 1 < argc - 2) {
                    data.jobdir = argv[i + 1];
                    i++; // skip file name argument
                } else {
                    error_message(USAGE_ERROR_CODE, data);
                }
            } else if (strcmp(argv[i], "--regenerate") == 0 && !data.regen) {
                data.regen = true;
            } else {
                error_message(USAGE_ERROR_CODE, data);
            }
        }
    }
    if (!data.jobdir) {
        data.jobdir = JOBDIR_DEF;
    }

    data.jobFile = argv[argc - 2];
    data.program = argv[argc - 1];

    // check if jobFile or program start with --
    if (strncmp(data.jobFile, "--", 2) == 0 
            || strncmp(data.program, "--", 2) == 0) {
        error_message(USAGE_ERROR_CODE, data);
    }
    
    return data;
}

/* free_jobfile_one_plus()
* −----------------
* Frees the memory allocated for all jobs if there is more than 1 job stored
*
* data: Struct containing all the data for the program.
* jobFileCount: The line number of the current line of the jobfile
*/
void free_jobfile_one_plus(Data data, int jobFileCount) {
    if (jobFileCount > 1) {
        free_job(data);
    }
}

/* check_jobfile()
* −----------------
* Checks to see if the jobfile is valid
*
* data: Struct containing all the data for the program.
* numArgs: Number of arguments in the current line of the jobfile
* testId: The testId of the current line of the jobfile
* jobFileCount: The line number of the current line of the jobfile
* jobs: Struct containing all the jobs to be run
*
* Errors: if the jobfile contents is invalid
*/
void check_jobfile(Data data, int numArgs, char* testId, int jobFileCount, 
        Alljobs jobs) {
    if (numArgs < 2) {
        fprintf(stderr, JOB_ERROR, jobFileCount, data.jobFile);
        free_jobfile_one_plus(data, jobFileCount);
        exit(JOB_ERROR_CODE);
    }
    for (int i = 0; i < strlen(testId); i++) {
        if (testId[i] == '/') {
            fprintf(stderr, JOB_ERROR, jobFileCount, data.jobFile);
            free_jobfile_one_plus(data, jobFileCount);
            exit(JOB_ERROR_CODE);
        }
    }
    for (int i = 0; i < jobs.numJobs; i++) {
        if (strcmp(jobs.jobs[i].testId, testId) == 0) {
            fprintf(stderr, JOB_DUP, jobFileCount, data.jobFile);
            free_jobfile_one_plus(data, jobFileCount);
            exit(JOB_DUP_CODE);
        }
    }
}

/* reead_jobfile()
* −----------------
* Reads the jobfile and stores the information in a Job struct, then stored in 
* an array in Alljobs struct
*
* data: Struct containing all the data for the program.
*
* Returns: updated Alljobs struct
* Errors: if the jobfile unopenable or empty
*/
Alljobs read_jobfile(Data data) {
    Alljobs jobs = { .numJobs = 0, .jobs = NULL};
    FILE* jobFile = fopen(data.jobFile, "r");
    if (!jobFile) { 
        error_message(JOBFILE_ERROR_CODE, data); 
    }
    char* line;
    int jobFileCount = 0;
    while ((line = read_line(jobFile)) != NULL) {
        int numArgs = 0;
        jobFileCount++;
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') {
            free(line);
            continue;
        }
        char** values = split_string(line, '\t');
        while (values[numArgs] != NULL && values[numArgs][0] != '\0') {
            numArgs++;
        }
        check_jobfile(data, numArgs, values[0], jobFileCount, jobs);
        char* testId = strdup(values[0]);
        char* inFileName = strdup(values[1]);
        FILE* inFile = fopen(inFileName, "r");
        if (!inFile) {
            fprintf(stderr, INFILE_ERROR, inFileName, jobFileCount, 
                    data.jobFile);
            free_jobfile_one_plus(data, jobFileCount);
            exit(INFILE_ERROR_CODE);  
        }
        jobs.numJobs++;
        jobs.jobs = (Job*) realloc(jobs.jobs, jobs.numJobs * sizeof(Job));
        Job curJob = { .numArgs = numArgs, .testId = testId,
            .inFileName = inFileName, .givenArgs = NULL};
        curJob.givenArgs = malloc((curJob.numArgs - 2 + 1) * sizeof(char*));
        for (int i = 2; i < numArgs; i++) {
            curJob.givenArgs[i - 2] = strdup(values[i]);
        }
        curJob.givenArgs[numArgs - 2] = NULL;
        jobs.jobs[jobs.numJobs - 1] = curJob;
        free(line);
        free(values);
        fclose(inFile);
    }
    fclose(jobFile);
    if (jobs.numJobs == 0) {
        error_message(JOBFILE_EMPTY_CODE, data);
    }
    return jobs;
}

/* make_jobdir()
* −----------------
* Creates the job directory to read, write, and execute for the owner of the 
* directory
*
* data: Struct containing all the data for the program.
*
* Returns: updated data struct
* Errors: if the jobdir cannot be created
*/
Data make_jobdir(Data data) {
    if (mkdir(data.jobdir, S_IRWXU) != 0) {
        if (errno != EEXIST) {
            error_message(DIR_FAIL_CODE, data);
        }
    }
    return data;
}

/* open_files()
* −----------------
* Opens the files for the given test job
* Runs the good-uqwordladder program with the given arguments
* Stores its output in the corresponding files
*
* inFileName: The name of the input file
* outFileName: The name of the output file
* errFileName: The name of the error file
* statusFileName: The name of the status file
* modArgs: The arguments for the uqwordladder with the program name
* testId: The testId of the current test job
*
* Errors: if the files cannot be opened
*/
void open_files(char* inFileName, char* outFileName, char* errFileName, 
        char* statusFileName, char** modArgs, char* testId) {
    regen_message(testId);
    FILE* inFile = fopen(inFileName, "r");
    FILE* outFile = fopen(outFileName, "w");
    if (!outFile) {
        fclose(inFile); 
        file_fail(outFileName);
    }
    FILE* errFile = fopen(errFileName, "w");
    if (!errFile) {
        fclose(inFile); 
        fclose(outFile);
        file_fail(errFileName);
    }
    FILE* statusFile = fopen(statusFileName, "w");
    if (!statusFile) {
        fclose(inFile); 
        fclose(outFile);
        fclose(errFile);
        file_fail(statusFileName);
    }

    pid_t pid = fork();
    if (pid == 0) {
        // redirect stdin, stdout, stderr to corresponding files
        dup2(fileno(inFile), STDIN_FILENO);
        dup2(fileno(outFile), STDOUT_FILENO);
        dup2(fileno(errFile), STDERR_FILENO);
        fclose(inFile); 
        fclose(outFile);
        fclose(errFile);
        fclose(statusFile);
        execvp(GOOD_PROG, modArgs);
    } else {
        int status;
        waitpid(pid, &status, 0);
        fprintf(statusFile, "%d\n", WEXITSTATUS(status));
    }
    fclose(inFile); 
    fclose(outFile);
    fclose(errFile);
    fclose(statusFile);
}

/* check_regen()
* −----------------
* Checks to see if the expected output files needs to be regenerated
*
* data: Struct containing all the data for the program.
* outFileName: The name of the output file
* errFileName: The name of the error file
* statusFileName: The name of the status file
*
* Returns: updated data struct
*/
Data check_regen(Data data, char* outFileName, char* errFileName, 
        char* statusFileName) {
    struct stat jobSpecStat, outFileStat, errFileStat, statusFileStat;

    stat(data.jobFile, &jobSpecStat);

    // check if files exist
    if (stat(outFileName, &outFileStat) == -1 ||
            stat(errFileName, &errFileStat) == -1 ||
            stat(statusFileName, &statusFileStat) == -1) {
        data.regen = true;
    }

    // check if jobSpecStat is older than any of the files
    if (compare_timespecs(jobSpecStat.st_mtim, statusFileStat.st_mtim) > 0 ||
            compare_timespecs(jobSpecStat.st_mtim, outFileStat.st_mtim) > 0 ||
            compare_timespecs(jobSpecStat.st_mtim, errFileStat.st_mtim) > 0) {
        data.regen = true;
    }

    return data;
}

/* generate_output()
* −----------------
* Generates the expected output for the test jobs
*
* data: Struct containing all the data for the program.
*
* Returns: updated data struct
*/
Data generate_output(Data data) {
    data = make_jobdir(data);
    // iterate over test job and need 3 expected output
    for (int i = 0; i < data.alljobs.numJobs; i++) {
        Job curJob = data.alljobs.jobs[i];
        char* modArgs[curJob.numArgs];
        modArgs[0] = GOOD_PROG; 
        int buffer = strlen(data.jobdir) + strlen(curJob.testId);
        for (int j = 0; j < curJob.numArgs - 2; j++) {
            modArgs[j + 1] = curJob.givenArgs[j];
        }
        modArgs[curJob.numArgs - 1] = NULL;

        char errFileName[buffer + 9]; // +9 = strlen("/.stderr\0")
        sprintf(errFileName, STDERR_FILE, data.jobdir, curJob.testId);

        char statusFileName[buffer + 13]; // +13 = strlen("/.exitstatus\0")
        sprintf(statusFileName, EXIT_STAT_FILE, data.jobdir, 
                curJob.testId);

        char outFileName[buffer + 9]; // +9 = strlen("/.stdout\0")
        sprintf(outFileName, STDOUT_FILE, data.jobdir, curJob.testId);

        data = check_regen(data, outFileName, errFileName, statusFileName);
        if (data.regen) {
            open_files(curJob.inFileName, outFileName, errFileName, 
                    statusFileName, modArgs, curJob.testId);
        }
    }
    return data;
}

/* run_prog_child()
* −----------------
* Runs the specified program with the given arguments
* Redirects stdin to the input file
* Redirects stdout and stderr to the corresponding pipes
*
* data: Struct containing all the data for the program.
* curJob: The current job that is being run
* cmpOutPipe: The pipe for the stdout comparison
* cmpErrPipe: The pipe for the stderr comparison
*
* Returns: pid of the child process
* Errors: if the execvp fails
*/
pid_t run_prog_child(Data data, Job curJob, int* cmpOutPipe, 
        int* cmpErrPipe) {
    pid_t pid = fork();
    if (!pid) {
        close(cmpOutPipe[READ_END]);
        close(cmpErrPipe[READ_END]);
        int inFile = open(curJob.inFileName, O_RDONLY);
        dup2(inFile, STDIN_FILENO);
        close(inFile);
        dup2(cmpOutPipe[WRITE_END], STDOUT_FILENO);
        dup2(cmpErrPipe[WRITE_END], STDERR_FILENO);
        close(cmpOutPipe[WRITE_END]);
        close(cmpErrPipe[WRITE_END]);
        char* modArgs[curJob.numArgs];
        modArgs[0] = data.program; 
        for (int k = 0; k < curJob.numArgs - 2; k++) {
            modArgs[k + 1] = curJob.givenArgs[k];
        }
        modArgs[curJob.numArgs - 1] = NULL;
        execvp(modArgs[0], modArgs);
        exit(99);
    }
    return pid;
}

/* run_cmp_out_child()
* −----------------
* Runs the cmp command to compare the stdout of the program with the expected
* output
* Redirects stdin to the cmpOutPipe
* Redirects stdout and stderr to /dev/null
*
* data: Struct containing all the data for the program.
* curJob: The current job that is being run
* cmpOutPipe: The pipe for the stdout comparison
* cmpErrPipe: The pipe for the stderr comparison
*
* Returns: pid of the child process
* Errors: if the execvp fails
*/
pid_t run_cmp_out_child(Data data, Job curJob, int* cmpOutPipe, 
        int* cmpErrPipe) {
    pid_t pid = fork();
    if (!pid) {
        close(cmpErrPipe[READ_END]);
        close(cmpErrPipe[WRITE_END]);
        close(cmpOutPipe[WRITE_END]);
        dup2(cmpOutPipe[READ_END], STDIN_FILENO);
        close(cmpOutPipe[READ_END]);
        int nullFd = open(NULL_FILE, O_WRONLY);
        dup2(nullFd, STDERR_FILENO);
        dup2(nullFd, STDOUT_FILENO);
        close(nullFd);
        char outFileName[strlen(data.jobdir) + strlen(curJob.testId) + 9];
        sprintf(outFileName, STDOUT_FILE, data.jobdir, curJob.testId);
        char* cmpOutArgs[3] = {"cmp", outFileName, NULL};
        execvp(cmpOutArgs[0], cmpOutArgs);
        exit(99);
    }
    return pid;
}

/* run_cmp_err_child()
* −----------------
* Runs the cmp command to compare the stderr of the program with the expected
* output
* Redirects stdin to the cmpErrPipe
* Redirects stdout and stderr to /dev/null
*
* data: Struct containing all the data for the program.
* curJob: The current job that is being run
* cmpOutPipe: The pipe for the stdout comparison
* cmpErrPipe: The pipe for the stderr comparison
*
* Returns: pid of the child process
* Errors: if the execvp fails
*/
pid_t run_cmp_err_child(Data data, Job curJob, int* cmpOutPipe, 
        int* cmpErrPipe) {
    pid_t pid = fork();
    if (!pid) {
        close(cmpOutPipe[READ_END]);
        close(cmpOutPipe[WRITE_END]);
        close(cmpErrPipe[WRITE_END]);
        dup2(cmpErrPipe[READ_END], STDIN_FILENO);
        close(cmpErrPipe[READ_END]);
        int nullFd = open(NULL_FILE, O_WRONLY);
        dup2(nullFd, STDERR_FILENO);
        dup2(nullFd, STDOUT_FILENO);
        close(nullFd);
        char errFileName[strlen(data.jobdir) + strlen(curJob.testId) + 9];
        sprintf(errFileName, STDERR_FILE, data.jobdir, curJob.testId);
        char* cmpErrArgs[3] = {"cmp", errFileName, NULL};
        execvp(cmpErrArgs[0], cmpErrArgs);
        exit(99);
    }
    return pid;
}

/* report_results()
* −----------------
* Reports the results of the test job
* Reports if the stdout, stderr, and exit status matches or differs
*
* statusA: The exit status of the program
* statusB: The exit status of the cmp command for stdout
* statusC: The exit status of the cmp command for stderr
* curJob: The current job that is being run
* data: Struct containing all the data for the program.
* failed: Flag to determine if the test job failed
* execFailed: Flag to determine if the test job failed to execute
*
* Returns: updated failed flag
*/
bool report_results(int statusA, int statusB, int statusC, Job curJob, 
        Data data, bool failed, bool execFailed){
    if (WEXITSTATUS(statusB) == 99 || WEXITSTATUS(statusC) == 99 || 
            WEXITSTATUS(statusA) == 99) {
        execFailed = true;
        failed = true;
        gen_message(UNABLE_EXEC, curJob);
    }
    if (execFailed == false) {
        if (WEXITSTATUS(statusB) == 0) {
            gen_message(STDOUT_MATCH, curJob);
        } else {
            gen_message(STDOUT_DIFF, curJob);
            failed = true;
        }
        if (WEXITSTATUS(statusC) == 0) {
            gen_message(STDERR_MATCH, curJob);
        } else {
            gen_message(STDERR_DIFF, curJob);
            failed = true;
        } 
        
        int actual = WEXITSTATUS(statusA);
        char expected[strlen(data.jobdir) + 
                strlen(curJob.testId) + 13];
        sprintf(expected, EXIT_STAT_FILE, data.jobdir,
                curJob.testId);
        int expectedStatus;
        FILE* exitStatusFile = fopen(expected, "r");
        fscanf(exitStatusFile, "%d", &expectedStatus);
        fclose(exitStatusFile);
        if (actual != expectedStatus) {
            gen_message(EXIT_DIFF, curJob);
            failed = true;
        } else {
            gen_message(EXIT_MATCH, curJob);
        }
    }
    return failed;
}

/* over_all_result()
* −----------------
* Prints the overall result of the test jobs
*
* passed: The number of test jobs that passed
* run: The number of test jobs that ran
*/
void over_all_result(int passed, int run) {
    fprintf(stdout, PASSED_TESTS, passed, run);
    fflush(stdout);
    if (passed > 0 && passed == run) {
        exit(PROG_SUCCESS);
    } else {
        exit(PROG_FAIL);
    }
}

/* check_interrupted()
* −----------------
* Checks to see if the program has been interrupted
* Closes the pipes and exits the program if it has been interrupted
*
* passed: The number of test jobs that passed
* run: The number of test jobs that ran
* cmpOutPipe: The pipe for the stdout comparison
* cmpErrPipe: The pipe for the stderr comparison
*/
void check_interrupted(int passed, int run, int* cmpOutPipe, 
        int* cmpErrPipe, Data data) {
    if (interrupted) {
        close(cmpOutPipe[READ_END]);
        close(cmpOutPipe[WRITE_END]);
        close(cmpErrPipe[READ_END]);
        close(cmpErrPipe[WRITE_END]);
        free_job(data);
        if (interrupted && passed == 0) {
            fprintf(stdout, NO_TESTS);
            exit(NO_TESTS_CODE);
        } else {
            over_all_result(passed, run);
        }
    }
}

/* run_test_job()
* −----------------
* Runs the test jobs
* Each job has 1.5 seconds to run before it is killed, then reaped
* Or if the program is interrupted, the test jobs are killed and reaped
*
* data: Struct containing all the data for the program.
*/
void run_test_job(Data data) { 
    int passed = 0;
    int run = 0; 
    for (int i = 0; i < data.alljobs.numJobs; i++) {
        Job curJob = data.alljobs.jobs[i];
        gen_message(RUN_TEST, curJob);
        bool failed = false;
        bool execFailed = false;
        int cmpOutPipe[2], cmpErrPipe[2];
        int statusA, statusB, statusC;
        // create pipes
        pipe(cmpOutPipe);
        pipe(cmpErrPipe);
        pid_t runProg = run_prog_child(data, curJob, cmpOutPipe, cmpErrPipe);
        pid_t cmpOut = run_cmp_out_child(data, curJob, cmpOutPipe, cmpErrPipe);
        pid_t cmpErr = run_cmp_err_child(data, curJob, cmpOutPipe, cmpErrPipe);
        close(cmpOutPipe[READ_END]);
        close(cmpOutPipe[WRITE_END]);
        close(cmpErrPipe[READ_END]);
        close(cmpErrPipe[WRITE_END]);
        // wait 1.5 seconds then kill children
        usleep(1500000);
        kill(runProg, SIGKILL);
        kill(cmpOut, SIGKILL);
        kill(cmpErr, SIGKILL);
        waitpid(runProg, &statusA, 0);
        waitpid(cmpOut, &statusB, 0);
        waitpid(cmpErr, &statusC, 0);
        check_interrupted(passed, run, cmpOutPipe, cmpErrPipe, data);
        failed = report_results(statusA, statusB, statusC, curJob, data, 
                failed, execFailed);
        run++;
        if (failed == false) {
            passed++;
        }
    }
    if (!interrupted) {
        free_job(data);
        over_all_result(passed, run);
    }
}

/* main()
* −----------------
* Main function of the program
* Calls all the functions to run the test jobs
*
* argc: Number of command line arguments
* argv: Array of command line arguments
*/
int main(int argc, char* argv[]) {
    // handle SIGINT
    struct sigaction sigIntHandler;
    memset(&sigIntHandler, 0, sizeof(sigIntHandler));
    sigIntHandler.sa_handler = interrupt_handler;
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    
    Data data = { .jobFile = NULL, .program = NULL, .regen = false,
        .jobdir = NULL};
    data = check_command_line(data, argc, argv);
    data.alljobs = read_jobfile(data);
    data = generate_output(data);
    run_test_job(data);
}