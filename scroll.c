#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <stdbool.h>

int currIndex;
long bufferSize;
char *buffer;
char promptCurrent[256];
char promptDefault[256] = "| Space: move forward 1 screenful | Enter: toggle scrolling | f: scroll 20\% faster [off] | s: scroll 20\% slower [off] | q: quit | ";
char promptFast[256] = "| Space: move forward 1 screenful | Enter: toggle scrolling | f: scroll 20\% faster [ON] | s: scroll 20\% slower [off] | q: quit | ";
char promptSlow[256] = "| Space: move forward 1 screenful | Enter: toggle scrolling | f: scroll 20\% faster [off] | s: scroll 20\% slower [ON] | q: quit | ";
char input[128];
bool isFast = false;
bool isSlow = false;
bool isScrolling = false;
bool isEnded = false;
struct winsize winsize;
struct itimerval timer;
struct termios oldTermios, newTermios;

void setTimer(int value_sec, int value_usec, int interval_sec, int interval_usec, const char* src) {
    timer.it_value.tv_sec = value_sec;
    timer.it_value.tv_usec = value_usec;
    timer.it_interval.tv_sec = interval_sec;
    timer.it_interval.tv_usec = interval_usec;
    setitimer(ITIMER_REAL, &timer, NULL);
    strcpy(promptCurrent, src);
}

int printLines(char *buffer, int lineCount, int index)
{
    int i;
    int currLength = 0;

    for (i = 0; i < lineCount; i++)
    {
        if (currIndex >= bufferSize)
        {
            printf("\n");
            strcpy(promptCurrent, " ");
            isEnded = true;
            return 0;
        }
        while (buffer[currIndex] != '\n' && currLength < winsize.ws_col && currIndex < bufferSize)
        {
            printf("%c", buffer[currIndex]);
            if(buffer[currIndex] == '\t') {
                int spaces = 8 - (currLength % 8);
                if(currLength + spaces > winsize.ws_col) {
                    currIndex++;
                    break;
                }
                currLength += spaces;
            } else {
                currLength++;
            }
            currIndex++;
        }
        if(buffer[currIndex] == '\n') {
            currIndex++;
        }
        currLength = 0;
        printf("\n");
    }
    return currIndex;
}

void handler(int param)
{
    if (isEnded)
    {
        isScrolling = false;
        setTimer(0, 0, 0, 0, " ");
        printf("\33[2K\r");
        printf("\033[7m| End of file: press q to quit |\033[0m");
        for (;;)
        {
            fgets(input, 2, stdin);
            if (input[0] == 'q')
            {
                tcsetattr(0, TCSANOW, &oldTermios);
                printf("\n\r");
                exit(0);
            }
        }
    }
    else
    {
        printf("\33[2K\r");
        printLines(buffer, 1, currIndex);
        printf("\033[7m%s\033[0m", promptCurrent);
        fflush(stdout);
    }
}

int main(int argc, char const *argv[])
{
    signal(SIGALRM, handler);
    ioctl(0, TIOCGWINSZ, &winsize);
    tcgetattr(0, &oldTermios);
    newTermios = oldTermios;
    newTermios.c_lflag &= ~ICANON;
    newTermios.c_lflag &= ~ECHO;
    tcsetattr(0, TCSANOW, &newTermios);

    FILE *fp;

    if (!(fp = fopen(argv[1], "r")))
    {
        perror(argv[1]);
    }

    if (fp != NULL)
    {
        fseek(fp, 0L, SEEK_END);
        bufferSize = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        buffer = malloc(sizeof(char) * (bufferSize + 1));
        fread(buffer, sizeof(char), bufferSize, fp);
    }
    else
    {
        printf("File is NULL");
    }

    printLines(buffer, winsize.ws_row - 1, 0);
    strcpy(promptCurrent, promptDefault);

    for (;;)
    {
        if (!isScrolling && !isEnded)
        {
            printf("\033[7m%s\033[0m", promptCurrent);
        }

        if (!isEnded)
        {
            fgets(input, 2, stdin);
        }
        else
        {
            printf("\033[A\33[2K\r\n");
            printf("\033[7m| End of file: press q to quit |\033[0m");
            for (;;)
            {
                fgets(input, 2, stdin);
                if (input[0] == 'q')
                {
                    tcsetattr(0, TCSANOW, &oldTermios);
                    printf("\n");
                    exit(0);
                }
            }
        }

        if (input[0] == ' ' && !isEnded)
        {
            //print screenful
            if (isScrolling)
            {
                isScrolling = false;
                isFast = false;
                isSlow = false;
                setTimer(0, 0, 0, 0, promptDefault);
                printf("\33[2K\r");
            }
            printf("\33[2K\r"); // remove prompt, removes everything on the line, then returns to column 0
            printLines(buffer, winsize.ws_row - 1, currIndex);
        }
        if (input[0] == '\n' && !isEnded)
        {
            //scroll on/off
            if (isScrolling)
            {
                //turn scroll off
                isScrolling = false;
                isFast = false;
                isSlow = false;
                setTimer(0, 0, 0, 0, promptDefault);
                printf("\33[2K\r");
            }
            else
            {
                //turn scroll on
                isScrolling = true;
                setTimer(1, 0, 2, 0, promptDefault);
            }
        }
        else if (input[0] == 'f' && isScrolling && !isEnded)
        {
            //scroll 20% faster
            if (!isScrolling)
            {
                printf("\33[2K\r");
            }
            else if (isFast)
            {
                isFast = false;
                setTimer(1, 0, 2, 0, promptDefault);
            }
            else
            {
                isFast = true;
                isSlow = false;
                setTimer(1, 0, 1, 600000, promptFast);
            }
        }
        else if (input[0] == 's' && isScrolling && !isEnded)
        {
            //scroll 20% slower
            if (isSlow)
            {
                isSlow = false;
                setTimer(1, 0, 2, 0, promptDefault);
            }
            else
            {
                isSlow = true;
                isFast = false;
                setTimer(1, 0, 2, 400000, promptSlow);
            }
        }
        else if (input[0] == 'q')
        {
            //quit
            tcsetattr(0, TCSANOW, &oldTermios);
            printf("\n");
            exit(0);
        }
        else
        {
            printf("\33[2K\r");
        }
    }

    return 0;
}
