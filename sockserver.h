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
#ifndef SOCKSERVER_H
#define SOCKSERVER_H

#include "sock.h"
#include <string>
#include <vector>


class termsclis : public tcp_cli_sock
{
public:
    termsclis(){}
    ~termsclis(){}

    int receive(){
        char buf[512] = {0};
        int rt = tcp_cli_sock::receive(buf, 511);
        if(rt>0)
            _request.append(buf);
        return rt;
    }
    bool process();
private:
    bool   _serve(const char* file, int len);
    bool   _serve(const char* file);
private:
    std::string  _request;
};


class sockserver
{
public:
    sockserver(int port, const string& proto);
    virtual ~sockserver();

    bool listen();
    void close();
    bool spin();
    int  socket() {return _s.socket();}
    bool has_clients();

private:
    void _clean();
    tcp_srv_sock _s;
    tcp_srv_sock _h;
    int          _port;
    string       _proto;
    std::vector<termsclis*> _clis;
    bool        _dirty;
    bool        _headered;
};

#endif // SOCKSERVER_H
