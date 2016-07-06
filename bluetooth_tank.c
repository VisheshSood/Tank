#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>     //define O_WONLY and O_RDONLY
#include <stddef.h>
#include <termios.h>
#include <sys/types.h>

#define BONEPATH        "/sys/devices/bone_capemgr.9/slots"
#define SIG_USER 20
#define AIN1 67
#define AIN2 68
#define BIN1 44
#define BIN2 26
#define STBY 66
#define PWMA 46
#define PWMB 65

/*
AIN0 = Front
AIN1 = Left
AIN2 = Right
AIN3 = Back
*/
int volatile front_sensor_val = 0;
int volatile left_sensor_val = 0;
int volatile right_sensor_val = 0;
int volatile back_sensor_val = 0;
// Status will be used to control between user and auto mode
int volatile status =0;
// Store the command passed from the phone
void* newCommand;
int volatile H = 1; // high
int volatile L = 0; // low
int volatile fd; // file directory
FILE *sys, *dir, *FAIN1, *FPWMA,  *FPWMB, *dir2, *FAIN2, *dir3, *FBIN1, *dir4,
*FBIN2, *dir5, *FPWMA_Period, *FPWMA_duty,  *FPWMB_Period, *FPWMB_duty,
*dir6, *dir7, *FSTBY;

// Initialize the signal handler parameters
struct sigaction sa;
struct itimerval timer;
struct sigaction saio;
struct itimerval timervalue;
struct sigaction new_handler, old_handler;

// Declare function prototypes
void timer_Init();
void cntrlMotor(char direction);
void (*timer_func_handler_pntr)(void);
void timer_handler(int);
void hw_timer_handler(void);
void timer_sig_handler(int);
void AutoDrive(void);

// start_timer function for the timer interupt
int start_timer(int mSec, void (*timer_func_handler)(void))
{
    timer_func_handler_pntr = timer_func_handler;
    timervalue.it_interval.tv_sec = mSec / 1000;
    timervalue.it_interval.tv_usec = (mSec % 1000) * 1000;
    timervalue.it_value.tv_sec = mSec / 1000;
    timervalue.it_value.tv_usec = (mSec % 1000) * 1000;
    // Check if the timer was not set up correctly.
    if(setitimer(ITIMER_REAL, &timervalue, NULL))
    {
        printf("\nsetitimer() error\n");
        return(1);
    }
    // set the signal handler addresses
    new_handler.sa_handler = &timer_sig_handler;
    new_handler.sa_flags = SA_NOMASK;
    // if the handler is not set correctly, print an error.
    if(sigaction(SIGALRM, &new_handler, &old_handler))
    {
        printf("\nsigaction() error\n");
        return(1);
    }
    return(0);
}

// timer handler function
void timer_sig_handler(int arg)
{
    timer_func_handler_pntr();
}

// stop_timer function to set the timer values to 0.
void stop_timer(void)
{
    timervalue.it_interval.tv_sec = 0;
    timervalue.it_interval.tv_usec = 0;
    timervalue.it_value.tv_sec = 0;
    timervalue.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timervalue, NULL);
    sigaction(SIGALRM, &old_handler, NULL);
}


// timer interrupt handler, read the ADC values and set the parameters
// for the sensor values, and prints them to console.
void hw_timer_handler(void)
{
    front_sensor_val = readADC(0);
    left_sensor_val = readADC(1);
    right_sensor_val = readADC(2);
    back_sensor_val = readADC(3);
    printf ("Front value: %d \nBack value: %d \nRight value: %d \nLeft value: %d \n",
    front_sensor_val, back_sensor_val, right_sensor_val, left_sensor_val);
}

// Initialize a string for the command from the user.
char command[5];

// Our most important function, the signal handler function.
// This function reads the file that contains what the bluetooth sent, and
// based on that, we set the status of the tank.
void signal_handler_IO (int st)
{
    FILE * f = fopen("/dev/ttyO1", "r");
    if(fgets(command, 2, f) > 0){
        // If we get something, print it.
        printf("command: %c\n", (char) command[0]);
        if ((char) command[0] == 'A') {
            // If the character is A, set to Auto Mode.
            printf("Changed State to Auto State!");
            status= 1;
        } else if ((char) command[0] == 'U') {
            // If the character is U, set to User mode status.
            printf("Changed State to User State!");
            status = 0;
            // Stop the motor because now the user will control it.
            cntrlMotor('s');
        } else  {
            // If the user sent a direction, set the motor to that direction
            cntrlMotor((char) command[0]);
        }
    }
    fclose(f);
}

// Main Function. Does the set up required, and a bunch of other stuff.
int main(int argc, char *argv[])
{
    int count;
    //timer_Init();
    if(start_timer(250, &hw_timer_handler))
    {
        printf("\n timer error\n");
        return(1);
    }
    
    //Enable ADC pins within code
    //REMEMBER TO RUN THIS BEFORE TIMER
    system("echo BB-ADC > /sys/devices/bone_capemgr.9/slots");
    system("echo cape-bone-iio > /sys/devices/bone_capemgr.9/slots");
    printf ("This program was called with \"%s\".\n",argv[0]);
    
    /******** Setup of the Button GPIOs ********/
    // Specifies the file that the pointer will be used for (w = //write)
    sys = fopen("/sys/class/gpio/export", "w");
    fseek(sys, 0, SEEK_SET);
    
    fprintf(sys, "%d", AIN1);
    fprintf(sys, "%d", AIN2);
    fprintf(sys, "%d", BIN1);
    fprintf(sys, "%d", BIN2);
    fprintf(sys, "%d", STBY);
    
    // Clears the FILE stream for sys file object.
    fflush(sys);
    
    //Set the gpio to INPUT
    dir = fopen("/sys/class/gpio/gpio67/direction", "w");
    fseek(dir, 0, SEEK_SET);
    fprintf(dir, "out");
    fflush(dir);
    
    dir2 = fopen("/sys/class/gpio/gpio68/direction", "w");
    fseek(dir2, 0, SEEK_SET);
    fprintf(dir2, "out");
    fflush(dir2);
    
    dir3 = fopen("/sys/class/gpio/gpio44/direction", "w");
    fseek(dir3, 0, SEEK_SET);
    fprintf(dir3, "out");
    fflush(dir3);
    
    dir4 = fopen("/sys/class/gpio/gpio26/direction", "w");
    fseek(dir4, 0, SEEK_SET);
    fprintf(dir4, "out");
    fflush(dir4);
    
    dir7 = fopen("/sys/class/gpio/gpio66/direction", "w");
    fseek(dir7, 0, SEEK_SET);
    fprintf(dir7, "out");
    fflush(dir7);
        
    dir5 = fopen("/sys/class/gpio/gpio46/direction", "w");
    fseek(dir5, 0, SEEK_SET);
    fprintf(dir5, "out");
    fflush(dir5);
    
    dir6 = fopen("/sys/class/gpio/gpio65/direction", "w");
    fseek(dir6, 0, SEEK_SET);
    fprintf(dir6, "out");
    fflush(dir6);
    
    fclose(sys);
    fclose(dir);
    fclose(dir2);
    fclose(dir3);
    fclose(dir4);
    fclose(dir5);
    fclose(dir6);
    fclose(dir7);

    //load the overlay for UART5
    FILE *uart;
    int n;
    int connected;
    
    uart = fopen(BONEPATH, "w");
    if(uart == NULL) printf("slots didn't open\n");
    fseek(uart,0,SEEK_SET);
    fprintf(uart, "BB-UART1");
    fflush(uart);
    fclose(uart);
    
    
    fd = open("/dev/ttyO1", O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        perror("open_port: Unable to open /dev/ttyO1\n");
        return 0;
    }
    
    /********** THIS CODE SAVED EVERYONE'S LIFE. **********/
    /********** IT SETS THE SAME FLAGS AS THE MINICOM DOES, ALLOWING US **********/
    /********** TO INITIALIZE THE BLUETOOTH MODULE IN OUR CODE ON START UP **********/
    /********** RATHER THAN USING MINI COM **********/
    /********** I REPEAT, EVERYONE USED THIS CODE. **********/
    /********** WRITTEN BY US. **********/
    saio.sa_handler = signal_handler_IO;
    saio.sa_flags = 0;
    saio.sa_restorer = NULL;
    sigaction(SIGIO,&saio,NULL);
    // FILE CONTROLS TO SET MORE FLAGS ON FILES
    fcntl(fd, F_SETFL, FNDELAY);
    fcntl(fd, F_SETOWN, getpid());
    fcntl(fd, F_SETFL,  O_ASYNC ); /**<<<<<<------This line made it work.**/
    // SET FILE TARGET ATTRIBUTE TO STRUCT TERMIOS
    tcgetattr(fd,&termAttr);
    // SET THE BAUD RATE TO 115,200
    cfsetispeed(&termAttr,B115200);
    cfsetospeed(&termAttr,B115200);
    // SET MORE FLAGS THAT WE LEARNT FROM MINICOM
    termAttr.c_cflag &= ~PARENB;
    termAttr.c_cflag &= ~CSTOPB;
    termAttr.c_cflag &= ~CSIZE;
    termAttr.c_cflag |= CS8;
    termAttr.c_cflag |= (CLOCAL | CREAD);
    termAttr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    termAttr.c_iflag &= ~(IXON | IXOFF | IXANY);
    termAttr.c_oflag &= ~OPOST;
    tcsetattr(fd,TCSANOW,&termAttr);
    // ONCE ALL THAT IS COMPLETE, PRINT TO CONSOLE.
    printf("UART1 configured....\n");
    connected = 1;
    while(connected == 1){
    // Here we know bluetooth is connected        
        while (status == 1) {
            // This is what the tank does if it is un Auto mode
            cntrlMotor('f');
            
            if (front_sensor_val > 1550 && right_sensor_val > 1520 
                                        && left_sensor_val > 1500) {
                cntrlMotor('s');
                usleep(1000000000);
                cntrlMotor('b');
                usleep(1000000000);
                cntrlMotor('s');
                usleep(1000000000);
                cntrlMotor('l');
                usleep(1500000000);
                cntrlMotor('s');
                usleep(1000000000);
                cntrlMotor('f');
                usleep(1000000000);
            }    
            
            if (front_sensor_val < 1400) {
                cntrlMotor('f');
            }   
        }   
    }
    close(fd);
    return 0;
}

/*
readADC function - takes a pin number and reads the value at that moment from
the ADC.
*/ 
int readADC(unsigned int pin)  
{  
    int fd;          //file pointer  
    char buf[64];     //file buffer  
    char val[4];     //holds up to 4 digits for ADC value  

    //Create the file path by concatenating the ADC pin number to the end of the string  
    //Stores the file path name string into "buf"  
    snprintf(buf, sizeof(buf), "/sys/devices/ocp.3/helper.15/AIN%d", pin);     
    //Concatenate ADC file name  

    fd = open(buf, O_RDONLY);     //open ADC as read only  

    //Will trigger if the ADC is not enabled  
    if (fd < 0) {  
       perror("ADC - problem opening ADC");  
    }

    read(fd, &val, 4);     //read ADC ing val (up to 4 digits 0-1799)  
    close(fd);     //close file and stop reading  

    return atoi(val);     //returns an integer value (rather than ascii)  
}

/*
s = stop
r = right
l = left
f = forwards
b = backwards

This function will initialize the motor to make sure it goes in the direction
the user requested.
*/
void cntrlMotor(char direction)  
{
    // Initialize all the files for the Hbridge
    FAIN1 = fopen("/sys/class/gpio/gpio67/value", "w");
    fseek(FAIN1, 0, SEEK_SET);
    FAIN2 = fopen("/sys/class/gpio/gpio68/value", "w");
    fseek(FAIN1, 0, SEEK_SET);
    FBIN1 = fopen("/sys/class/gpio/gpio44/value", "w");
    fseek(FAIN1, 0, SEEK_SET);
    FBIN2 = fopen("/sys/class/gpio/gpio26/value", "w");
    fseek(FAIN1, 0, SEEK_SET);
    FPWMA = fopen("/sys/class/gpio/gpio46/value", "w");
    fseek(FAIN1, 0, SEEK_SET);
    FPWMB = fopen("/sys/class/gpio/gpio65/value", "w");
    fseek(FAIN1, 0, SEEK_SET);
    FSTBY = fopen("/sys/class/gpio/gpio66/value", "w");
    fseek(FAIN1, 0, SEEK_SET);
    // Based on the direction, set the values.
    switch(direction)
    {
        case 's':
        fprintf(FPWMA, "%d", 0);
        fflush(FPWMA);
        fprintf(FPWMB, "%d", 0);
        fflush(FPWMB);
        break;
        case 'r':
        fprintf(FAIN1, "%d", 1);
        fflush(FAIN1);
        fprintf(FAIN2, "%d", 0);
        fflush(FAIN2);
        fprintf(FBIN1, "%d", 1);
        fflush(FBIN1);
        fprintf(FBIN2, "%d", 0);
        fflush(FBIN2);
        fprintf(FPWMA, "%d", 1);
        fflush(FPWMA);
        fprintf(FPWMB, "%d", 1);
        fflush(FPWMB);
        fprintf(FSTBY, "%d", 1);
        fflush(FSTBY);
        break;
        case 'l':
        fprintf(FAIN1, "%d", 0);
        fflush(FAIN1);
        fprintf(FAIN2, "%d", 1);
        fflush(FAIN2);
        fprintf(FBIN1, "%d", 0);
        fflush(FBIN1);
        fprintf(FBIN2, "%d", 1);
        fflush(FBIN2);
        fprintf(FPWMA, "%d", 1);
        fflush(FPWMA);
        fprintf(FPWMB, "%d", 1);
        fflush(FPWMB);
        fprintf(FSTBY, "%d", 1);
        fflush(FSTBY);
        break;
        case 'f':
        fprintf(FAIN1, "%d", 1);
        fflush(FAIN1);
        fprintf(FAIN2, "%d", 0);
        fflush(FAIN2);
        fprintf(FBIN1, "%d", 0);
        fflush(FBIN1);
        fprintf(FBIN2, "%d", 1);
        fflush(FBIN2);
        fprintf(FPWMA, "%d", 1);
        fflush(FPWMA);
        fprintf(FPWMB, "%d", 1);
        fflush(FPWMB);
        fprintf(FSTBY, "%d", 1);
        fflush(FSTBY);
        break;
        case 'b':
        fprintf(FAIN1, "%d", 0);
        fflush(FAIN1);
        fprintf(FAIN2, "%d", 1);
        fflush(FAIN2);
        fprintf(FBIN1, "%d", 1);
        fflush(FBIN1);
        fprintf(FBIN2, "%d", 0);
        fflush(FBIN2);
        fprintf(FPWMA, "%d", 1);
        fflush(FPWMA);
        fprintf(FPWMB, "%d", 1);
        fflush(FPWMB);
        fprintf(FSTBY, "%d", 1);
        fflush(FSTBY);
        break;
    }
    fclose(FAIN1);
    fclose(FAIN2);
    fclose(FBIN1);
    fclose(FBIN2);
    fclose(FPWMA);
    fclose(FPWMB);
    fclose(FSTBY);
}

