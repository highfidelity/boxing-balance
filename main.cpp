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
#include <sys/time.h>

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
int serial_on = 0;                  //  Are we using serial port for I/O
int ping_test = 1; 
int display_ping = 0; 

timeval begin_ping, end_ping;
double elapsedTime;

double ping = 0; 
//clock_t begin_ping, end_ping;


#define WIDTH 1000					//  Width,Height of simulation area in cells
#define HEIGHT 600
#define BOTTOM_MARGIN 0				
#define RIGHT_MARGIN 0
#define TEXT_HEIGHT 14

#define MAX_FILE_CHARS 100000		//  Biggest file size that can be read to the system

int stats_on = 1;					//  Whether to show onscreen text overlay with stats
int noise_on = 0;					//  Whether to fire randomly
int step_on = 0;                    
int display_levels = 1;

int mouse_x, mouse_y;				//  Where is the mouse 
int mouse_pressed = 0;				//  true if mouse has been pressed (clear when finished)

float dot_x, dot_y;
int accel_x, accel_y;
const float AVG_RATE = 0.0001;

float mag_imbalance = 0.f;

int corners[4];                     //  Measured weights on corner
float avg_corners[4];

int first_measurement = 1;

int framecount = 0;
int samplecount = 0;

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

double diffclock(timeval clock1,timeval clock2)
{
	double diffms = (clock2.tv_sec - clock1.tv_sec) * 1000.0;
    diffms += (clock2.tv_usec - clock1.tv_usec) / 1000.0;   // us to ms

	return diffms;
}

void Timer(int extra)
{
	char title[100];
    
    //gettimeofday(&end_ping, NULL);
    //ping = diffclock(begin_ping,end_ping);

	sprintf(title, "FPS = %d, samples/sec = %d, ping(msec) = %4.4f", framecount, samplecount, ping);
	glutSetWindowTitle(title);
	framecount = 0;
    samplecount = 1;   
    
    if (serial_on && ping_test)
    {
        char buf[] = "ping";
        write(serial_fd,buf,4);
        write(serial_fd, "\r", 1);
        gettimeofday(&begin_ping, NULL);
        display_ping = 2;
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
    avg_corners[0] = avg_corners[1] = avg_corners[2] = avg_corners[3] = 0.f;
}

void update_pos(void)
//  Using sampled data, update the onscreen position of the balance point
{
    /*
    const int SPRING_BACK = 0.1f;
    if (noise_on)
    {
        dot_x += (float)(rand()%11) - 5.f;
        dot_y += (float)(rand()%11) - 5.f;
    }
    if (samplecount > 0)
    {
        dot_x += 0.1f*(corners[0] - last_corners[0]);
        dot_y += 0.1f*(corners[1] - last_corners[1]);
    }
    //  Option to use spring force to pull back toward center always
    if (SPRING_BACK > 0.f)
    {
        dot_x -= (WIDTH/2.f - dot_x)*SPRING_BACK;
        dot_y -= (HEIGHT/2.f - dot_y)*SPRING_BACK;
    }
     */
    
    if (noise_on)
    {
        if (rand()%100 == 1)
        {
            dot_x = rand()%400;  //  += (float)(rand()%11) - 5.f;
            dot_y =  rand()%500;  // 300;  // += (float)(rand()%11) - 5.f;

            //dot_x = (float)(rand()%1000);  // (float)(rand()%WIDTH);
            //dot_y = (float)(rand()%1000);  //200;  //(float)(rand()%HEIGHT);
        }
    }
    else {
        dot_x = WIDTH/2.f - (((corners[0] - avg_corners[0]) - (corners[1]-avg_corners[1])) + 
                             ((corners[2] - avg_corners[2]) - (corners[3]-avg_corners[3])))*1.f;
        dot_y = HEIGHT/2.f - (((corners[2]- avg_corners[2]) - (corners[0] - avg_corners[0])) + 
                              ((corners[3]- avg_corners[3]) - (corners[1]- avg_corners[1])))*1.f;
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

    if (display_ping)
    {
        //  Draw a green dot to indicate receipt of ping signal 
        glPointSize(30.f);
        if (display_ping == 2)
            glColor4f(1.f, 0.f, 0.f, 1.f);
        else 
            glColor4f(0.f, 1.f, 0.f, 1.f);
        glBegin(GL_POINTS);
        glVertex2f(50, 400);
        glEnd();
        display_ping = 0;
    }
    if (display_levels)
    {
        glColor4f(1.f, 1.f, 1.f, 1.f);
        glBegin(GL_LINES);
        glVertex2f(10, HEIGHT*0.95);
        glVertex2f(10, HEIGHT*(0.25 + 0.75f*corners[0]/4096));
        
        glVertex2f(20, HEIGHT*0.95);
        glVertex2f(20, HEIGHT*(0.25 + 0.75f*corners[1]/4096));
        
        glVertex2f(30, HEIGHT*0.95);
        glVertex2f(30, HEIGHT*(0.25 + 0.75f*corners[2]/4096));
        
        glVertex2f(40, HEIGHT*0.95);
        glVertex2f(40, HEIGHT*(0.25 + 0.75f*corners[3]/4096));
        glEnd();
        
    }
    
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
                {
                    samplecount++;
                    sscanf(serial_buffer, "%d %d %d %d", &corners[0], 
                                                        &corners[1], 
                                                        &corners[2], 
                                                        &corners[3]);
                    for (int i = 0; i < 4; i++)
                    {
                        if (!first_measurement)
                            avg_corners[i] = (1.f - AVG_RATE)*avg_corners[i] + 
                                            AVG_RATE*(float)corners[i];
                        else
                        {
                            avg_corners[i] = (float)corners[i];
                        }
                    }
                    first_measurement = 0;
                }
                //  Clear rest of string for printing onscreen
                while(serial_buffer_pos++ < MAX_BUFFER) serial_buffer[serial_buffer_pos] = ' ';
                serial_buffer_pos = 0;
            }
            if (bufchar[0] == 'p')
            {
                gettimeofday(&end_ping, NULL);
                ping = diffclock(begin_ping,end_ping);
                display_ping = 1; 
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
    usleep(5000);

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