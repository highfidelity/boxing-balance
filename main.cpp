//
//  
//  Balance
//   
//  Show a big white dot to indicate a point of balance, reading data from 
//  serial port connected to Maple board/arduino. 
//
//  Keyboard Commands: 
//
//  / = toggle stats display
//  n = toggle noise in firing on/off
//  c = clear all cells and synapses to zero
//  s = clear cells to zero but preserve synapse weights
//

#include <stdlib.h>
#include <GLUT/glut.h>

//#include "main.h"
#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

//  These includes are for the serial port reading/writing
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

using namespace std;

//   Junk for talking to the Serial Port 
int serial_fd;
const int MAX_BUFFER = 100;
char serial_buffer[MAX_BUFFER];
int serial_buffer_pos = 0;
int serial_on = 1;                  //  Are we using serial port for I/O
int ping_test = 1; 
double ping = 0; 
clock_t begin_ping, end_ping;


#define WIDTH 1000					//  Width,Height of simulation area in cells
#define HEIGHT 600
#define BOTTOM_MARGIN 0				
#define RIGHT_MARGIN 0
#define TEXT_HEIGHT 14

#define MAX_FILE_CHARS 100000		//  Biggest file size that can be read to the system

int stats_on = 1;					//  Whether to show onscreen text overlay with stats
int noise_on = 0;					//  Whether to fire randomly
int step_on = 0;                    

int mouse_x, mouse_y;				//  Where is the mouse 
int mouse_pressed = 0;				//  true if mouse has been pressed (clear when finished)

float dot_x, dot_y;
int accel_x, accel_y;

int corners[4];                     //  Measured weights on corner
int first_measurement = 1;

int framecount = 0;

//  For accessing the serial port 
void init_port(int *fd, int baud)
{
    struct termios options;
    tcgetattr(*fd,&options);
    switch(baud)
    {
		case 9600: cfsetispeed(&options,B9600);
			cfsetospeed(&options,B9600);
			break;
		case 19200: cfsetispeed(&options,B19200);
			cfsetospeed(&options,B19200);
			break;
		case 38400: cfsetispeed(&options,B38400);
			cfsetospeed(&options,B38400);
			break;
		case 115200: cfsetispeed(&options,B115200);
			cfsetospeed(&options,B115200);
			break;
		default:cfsetispeed(&options,B9600);
			cfsetospeed(&options,B9600);
			break;
    }
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(*fd,TCSANOW,&options);
}

void output(int x, int y, char *string)
{
	//  Writes a text string to the screen as a bitmap at location x,y
	int len, i;
	glRasterPos2f(x, y);
	len = (int) strlen(string);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, string[i]);
	}
}

double diffclock(clock_t clock1,clock_t clock2)
{
	double diffticks=clock1-clock2;
	double diffms=(diffticks*10)/CLOCKS_PER_SEC;
	return diffms;
}

void Timer(int extra)
{
	char title[100];
	sprintf(title, "FPS = %i, ping(msec) = %4.4f", framecount, ping);
	glutSetWindowTitle(title);
	framecount = 0;
    if (serial_on && ping_test)
    {
        char buf[] = "ping";
        write(serial_fd,buf,4);
        write(serial_fd, "\r", 1);
        begin_ping = clock();
    }
	glutTimerFunc(1000,Timer,0);
}

void display_stats(void)
{
	//  bitmap chars are about 10 pels high 
	glColor3f(1.0f, 1.0f, 1.0f);
    char legend[] = "/ - toggle this display, Q - exit, N - toggle noise, M - toggle map";
	output(10,15,legend);
    if (serial_on)
    {
        /*char stuff[50];
        if (bytes_read > 0)
        {
            sprintf(stuff, "bytes=%i", bytes_read);
            output(10,60,stuff);
        }*/
        output(100,60,serial_buffer);
    }
	char mouse[50];
	sprintf(mouse, "mouse_x = %i, mouse_y = %i, pressed = %i", mouse_x, mouse_y, mouse_pressed);
	output(10,35,mouse);
	
}

void initDisplay(void)
{
    //  Set up blending function so that we can NOT clear the display
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void init(void)
{
    dot_x = WIDTH/2.f;
    dot_y = HEIGHT/2.f;
}

void update_pos(void)
{
    if (noise_on)
    {
        dot_x += (float)(rand()%11) - 5.f;
        dot_y += (float)(rand()%11) - 5.f;
    }
}

void display(void)
{
    // Trails - Draw a single Quad to blend instead of clear screen 
    glColor4f(0.f, 0.f, 0.f, 0.01f);
    glBegin(GL_QUADS);                    
    glVertex2f(0.f, HEIGHT);              
    glVertex2f(WIDTH, HEIGHT);            
    glVertex2f( WIDTH,0.f);               
    glVertex2f(0.f,0.f);                  
    
    //  But totally clear stats display area
    glColor4f(0.f, 0.f, 0.f, 1.f);
    glVertex2f(0.f, HEIGHT/10.f);              
    glVertex2f(WIDTH, HEIGHT/10.f);              
    glVertex2f( WIDTH,0.f);              
    glVertex2f(0.f,0.f);              
    glEnd();    

    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glColor4f(1.f, 1.f, 1.f, 1.f);
    glVertex2f(WIDTH, HEIGHT);
    
    //  Draw a big white DOT
    glPointSize(200.f);
    glEnable(GL_POINT_SMOOTH);
        
    glBegin(GL_POINTS);
    glColor4f(1.f, 1.f, 1.f, 0.9f);
    glVertex2f(dot_x, dot_y);
    glEnd();
    
    //  Draw a centered red dot for perspective 
    glPointSize(10.f);
    glColor4f(1.f, 0.f, 0.f, 1.f);
    glBegin(GL_POINTS);
    glVertex2f(WIDTH/2.f, HEIGHT/2.f);
    glEnd();
    
    usleep(1000);
    
   	if (stats_on) display_stats(); 
        
    glutSwapBuffers();
    framecount++;
}

// Collect sensor data from serial port 
void read_sensors(void)
{
    if (serial_on)
    {
        char bufchar[1];
        while (read(serial_fd, bufchar, 1) > 0)
        {
            serial_buffer[serial_buffer_pos] = bufchar[0];
            serial_buffer_pos++;
            //  Have we reached end of a line of input?
            if ((bufchar[0] == '\n') || (serial_buffer_pos >= MAX_BUFFER))
            {
                //  At end - Extract value from string to variables
                if (serial_buffer[0] != 'p')
                    scanf(serial_buffer, "%i %i %i %i", corners[0], 
                                                        corners[1], 
                                                        corners[2], 
                                                        corners[3]);
                //  Clear rest of string for printing onscreen
                while(serial_buffer_pos++ < MAX_BUFFER) serial_buffer[serial_buffer_pos] = ' ';
                serial_buffer_pos = 0;
            }
            if (bufchar[0] == 'p')
            {
                end_ping = clock();
                ping = diffclock(end_ping,begin_ping);
            }
        }
    }
}
void key(unsigned char k, int x, int y)
{
	//  Process keypresses 
	
	if (k == 'q')  exit(0);
	if (k == '/')  stats_on = !stats_on;		// toggle stats
	if (k == 'n') noise_on = !noise_on;			//  toggle random mutation
    if (k == 's') step_on = !step_on;           //  single step mode
}

void idle(void)
{
    if (!step_on) glutPostRedisplay();
    read_sensors();
    update_pos(); 
}

void reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0);
    glMatrixMode(GL_MODELVIEW);
}

void mouseFunc( int button, int state, int x, int y ) 
{
    if( button == GLUT_LEFT_BUTTON && state == GLUT_DOWN )
    {
		mouse_x = x;
		mouse_y = y;
		mouse_pressed = 1;
    }
	if( button == GLUT_LEFT_BUTTON && state == GLUT_UP )
    {
		mouse_x = x;
		mouse_y = y;
		mouse_pressed = 0;
    }
	
}

void motionFunc( int x, int y)
{
	mouse_x = x;
	mouse_y = y;
	
}

int main(int argc, char** argv)
{
    //  Try to setup the serial port I/O 
    if (serial_on)
    {
        serial_fd = open("/dev/tty.usbmodem411", O_RDWR | O_NOCTTY | O_NDELAY); // List usbSerial devices using Terminal ls /dev/tty.*
    
    
        if(serial_fd == -1) {				// Check for port errors
            cout << serial_fd;
            perror("Unable to open serial port\n");
            return (0);
        }
        else init_port(&serial_fd, 115200);        
    }

    glutInit(&argc, argv);
    
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	
    glutInitWindowSize(RIGHT_MARGIN + WIDTH, BOTTOM_MARGIN + HEIGHT);
    
    glutCreateWindow("Balance Platform");
    
    initDisplay();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
	glutKeyboardFunc(key);
	glutMotionFunc(motionFunc);
	glutMouseFunc(mouseFunc);
    glutIdleFunc(idle);
	
    init();
    
    glutTimerFunc(1000,Timer,0);
    glutMainLoop();
    
    // Close serial port
    close(serial_fd);
    
    return EXIT_SUCCESS;
}