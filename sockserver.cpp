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

#include <fstream>
#include <iostream>
#include <fcntl.h>
#include "sockserver.h"
#include "htmlenc.h"
#include "httprequestparser.h"
#include "httpresponseparser.h"

extern bool __alive;
using namespace httpparser;
static char HDR[] = "HTTP/1.0 200 OK\r\n"
                    "Connection: close\r\n"
                    "Server: v4l2net/1.0\r\n"
                    "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
                    "Pragma: no-cache\r\n"
                    "Expires: Mon, 3 Jan 2000 12:34:56 GMT\r\n"
                    "Content-type: text/html\r\n"
                    "Content-length: %d\r\n"
                    "X-Timestamp: %d.%06d\r\n\r\n";

sockserver::sockserver(int port, const string& proto):_port(port),_proto(proto),_headered(false)
{
    //ctor
}

sockserver::~sockserver()
{
    //dtor_headered
}

bool sockserver::listen()
{
    int ntry = 0;
AGAIN:
    if(__alive==false)
        return false;
    if(_s.create(_port, SO_REUSEADDR, 0)>0)
    {
        fcntl(_s.socket(), F_SETFD, FD_CLOEXEC);
        _s.set_blocking(0);
        if(_s.listen(4)!=0)
        {
            std::cout <<"socket can't listen. Trying "<< (ntry+1) << " out of 10 " << "\n";

            _s.destroy();
            sleep(1);
            if(++ntry<10)
                goto AGAIN;
            return false;
        }
        std::cout << "listening port"<< _port<<"\n";
        return true;
    }
    std::cout <<"create socket. Trying "<< (ntry+1) << " out of 10 \n";
    _s.destroy();
    sleep(2);
    if(++ntry<10)
        goto AGAIN;
    __alive=false;
    sleep(3);
    return false;
}

void sockserver::close()
{
    _s.destroy();
    for(auto& s : _clis)
    {
        delete s;
    }
}

bool sockserver::spin()
{
    fd_set  rd;
    int     ndfs = _s.socket();// _s.sock()+1;
    timeval tv {0,10000};

    FD_ZERO(&rd);
    FD_SET(_s.socket(), &rd);
    for(auto& s : _clis)
    {
        if(s->socket()>0)
        {
            FD_SET(s->socket(), &rd);
            ndfs = std::max(ndfs, s->socket());
        }
        else
            _dirty = true;
    }
    int is = ::select(ndfs+1, &rd, 0, 0, &tv);
    if(is ==-1) {
        std::cout << "socket select() " << "\n";
        __alive=false;
        return false;
    }
    if(is)
    {
        if(FD_ISSET(_s.socket(), &rd))
        {
            termsclis* cs = new termsclis();
            if(_s.accept(*cs)>0)
            {
                cs->set_blocking(0);
                std::cout <<"new connection \n";
                _clis.push_back(cs);
            }else{
                delete cs;
            }
        }

        for(auto& s : _clis)
        {
            if(s->socket()<=0)
                continue;
            if(FD_ISSET(s->socket(), &rd))
            {
                if(s->receive()<=0)
                {
                    std::cout << "client closed connection \n";
                    s->destroy();
                    _dirty = true;
                }else{
                    if(s->process()==true)
                    {
                        s->destroy();
                    }
                }

            }
        }
    }
    _clean();
    return true;
}

bool sockserver::has_clients()
{
    return _clis.size() > 0;
}


void sockserver::_clean()
{
    if(_dirty)
    {
AGAIN:
        size_t elems = _clis.size();
        for(std::vector<termsclis*>::iterator s=_clis.begin();s!=_clis.end();++s)
        {
            if((*s)->socket()<=0)
            {
                delete (*s);
                _clis.erase(s);
                std::cout << " client gone \n";
                goto AGAIN;
            }
        }
    }
    _dirty=false;
}

bool termsclis::process()
{
    bool                rv = false;
    Request             request;
    HttpRequestParser   parser;
    char                last = _request.back();
    HttpRequestParser::ParseResult res = parser.parse(request,
                                                      _request.c_str(),
                                                      &last);

    if( res == HttpRequestParser::ParsingCompleted )
    {
        std::cout << request.inspect() << std::endl;
        _request.clear();
        if(request.method=="GET"){
            if(request.uri=="/"){
                rv = _serve("./web/index.html");
            }else
                if(request.uri[0]=='/' && request.uri[1]=='?'){
                char header[512];
                std::string cmd = request.uri.substr(2);
                size_t lh = ::sprintf(header,
                        "HTTP/1.1 200 OK\r\n"
                        "Content-length: %d\r\n"
                        "Content-Type: text/html\r\n\r\n%s",16,"1234567890123456");
                this->sendall(header, lh);
            }
            else{
                std::string d = "./web";
                            d += request.uri;
                rv = _serve(d.c_str());
            }
        }
    }
    return rv;
}

bool   termsclis::_serve(const char* file, int len)
{
    return this->sendall(file,len)==len;
}

#define BUFFER_SIZE 4095
bool   termsclis::_serve(const char* file)
{
    std::vector<char> buffer (BUFFER_SIZE,0);
    std::ifstream fin(file, std::ifstream::binary);

    if(fin.is_open())
    {
        fin.seekg( 0, std::ios::end );

        size_t fsize = fin.tellg();
        std::streamsize s = 1;
        char header[512];

        size_t lh = ::sprintf(header,
                "HTTP/1.1 200 OK\r\n"
                "Content-length: %d\r\n"
                "Content-Type: text/html\r\n\r\n",fsize);
        this->sendall(header,lh);
        fin.seekg( 0, std::ios::beg );
        while(s)
        {
            fin.read(buffer.data(), buffer.size()-1);
            s = fin.gcount();
            if(s>0){
                buffer[s] = 0;
                this->sendall(buffer.data(), s);
                std::cout << buffer.data() << "\n";
            }
        }
        fin.close();
    }


    return true;
}



