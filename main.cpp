
#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>
#include <vector>

using namespace std;

using namespace boost;
using namespace boost::asio;
using namespace boost::asio::ip;

void writestr( tcp::socket *sock, const string &str )
{
    int l = str.length();
    system::error_code err;

    asio::write( *sock, asio::buffer( (void*) &l, 4 ), transfer_all(), err );
    asio::write( *sock, asio::buffer(str), transfer_all(), err );

    cout << "    >>> (" << l << "): " << str << "\n";
}

string readstr( tcp::socket *sock )
{
    int l = 0;
    system::error_code err;
    string res;

    asio::read( *sock, asio::mutable_buffer((void*) &l, 4), transfer_all(), err );

    res.resize( l, ' ' );

    char buf[1000];

    asio::read( *sock, asio::mutable_buffer( buf, l ), transfer_all(), err );
    buf[l] = 0;

    cout << "    <<< (" << l << "): " << buf << "\n";

    return string( buf );
}

struct TClient
{
    string name;
    int state;
};

int cl_num = 0;
vector<TClient*> clients;


void workthr( tcp::socket *sock )
{
    int cl = cl_num++;

    cout << "Connect from: " << sock->remote_endpoint().address().to_string() << " (" << cl << ")\n";

    string name;

    name = readstr( sock );
    if (name == "helo") {
        writestr(sock, "ehlo");
    } else
    {
        cout << "Failed to sync (" << cl << ")\n";
        sock->close();

        return;
    }

    TClient *client = new TClient();
    client->name = readstr( sock );
    client->state = 1;

    cout << "new client: " << client->name << "\n";

    clients.push_back( client );

    string cmd, t;
    int time = 0, lastcl;

    lastcl = 0;

    while (true)
    {
        if (sock->available() > 0)
        {
            cmd = readstr(sock);

            if (cmd == "getclients")
            {
                string s;
                s = to_string( clients.size() );

                writestr( sock, s );

                for( int i = 0; i < clients.size(); i++ )
                    writestr( sock, clients[i]->name );
            }
            if (cmd == "buy")
            {
                writestr( sock, "buybuy" );
                client->state = 0;
                break;
            }
        }
        else
        {
            if (lastcl != clients.size())
            {
                writestr( sock, "clientschange" );
                lastcl = clients.size();
            }

            if (time++ > 5)
            {
                writestr( sock, "ping" );
                t = readstr( sock );

                if (t != "pong")
                {
                    client->state = -1;
                    cout << "Client " << client->name << " - no ping\n";
                    break;
                }

                time = 0;
            }

            //Sleep(1000);
        }
    }

    sock->close();

    cout << "Client " << client->name << " is end\n";
}

void listenthr( void )
{
    tcp::endpoint port { tcp::v4(), 2000 };
    io_service io_serv;

    tcp::acceptor a{ io_serv, port };


    while (true)
    {
        tcp::socket *sock = new tcp::socket( io_serv );

        a.accept( *sock );

        boost::thread *thr = new boost::thread( workthr, sock );
    }
}

int main()
{
    std::cout << "Starting server" << std::endl;

    boost::thread_group threads;

    threads.create_thread( &listenthr );

    threads.join_all();

    return 0;
}
