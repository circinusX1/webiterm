/*

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/

    Author:  Marius O. Chincisan
    First Release: September 16 - 29 2016
*/


#define LIVEIMAGE_VERSION "1.0.0"
#define cimg_display 0
#define cimg_use_jpeg

#include <stdint.h>
#include <unistd.h>
#define  __USE_FILE_OFFSET64
#include <stdlib.h>
#include <sys/statvfs.h>
#include <iostream>
#include <string>
#include "sockserver.h"

/*
sudo apt-get install libpng-dev libv4l-dev libjpeg-dev
*/

using namespace std;
bool __alive = true;
bool _sig_proc_capture=false;


void ControlC (int i)
{
    __alive = false;
    printf("Exiting...\n");
}


void ControlP (int i)
{
}

void Capture (int i)
{
    _sig_proc_capture=true;
}

int main(int nargs, char* vargs[])
{

    //signal(SIGINT,  ControlC);
    //signal(SIGABRT, ControlC);
    //signal(SIGKILL, ControlC);
    //signal(SIGUSR2, Capture);
    //signal(SIGTRAP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    sockserver* ps = new sockserver(8888, "http");
    if(ps && ps->listen()==true)
    {
        while(__alive)
            ps->spin();
    }
    delete ps;
    return 0;
}
