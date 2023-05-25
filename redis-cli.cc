#include <unistd.h>
#include <json11.hpp>
#include <stdexcept>
#include <sys/socket.h>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
auto RawCommandToRequest(const std::string &raw) -> json11::Json::array
{
    auto it = raw.cbegin();
    auto isTokenMember = [](char c)
    {
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z');
    };
    auto nextToken = [&raw, &it, &isTokenMember]() mutable -> std::string
    {
        if (it == raw.cend())
        {
            return {};
        }
        while (!isTokenMember(*it))
        {
            it++;
            if (it == raw.cend())
            {
                return {};
            }
        }
        auto end = it;
        while (isTokenMember(*end))
        {
            end++;
            if (end == raw.end())
            {
                break;
            }
        }
        std::string ret;
        std::copy(it, end, std::back_inserter(ret));
        it = end;
        return ret;
    };

    json11::Json::array ret;
    for (std::string token = nextToken(); !token.empty(); token = nextToken())
    {
        ret.push_back(std::move(token));
    }
    return ret;
}

int main()
{
    std::cout << "connecting..." << std::endl;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        throw std::runtime_error("socket");
    }
    sockaddr_in sa;
    sa.sin_addr.s_addr = inet_addr("127.0.0.1");
    sa.sin_family = AF_INET;
    sa.sin_port = htons(8080);
    int ret = connect(fd, reinterpret_cast<sockaddr *>(&sa), sizeof(sa));
    if (ret == -1)
    {
        throw std::runtime_error("connect");
    }
    std::cout << "connected." << std::endl;
    std::thread t([](int conn)
                  {
                      char buf[65535];
                      while (1)
                      {
                          int n = read(conn, buf, 65535);
                          if(n<=0){
                              break;
                          }
                          buf[n] = 0;
                          printf("%s\n", buf);
                      } },
                  fd);
    std::string input;
    while (1)
    {
        std::getline(std::cin, input);
        json11::Json js(RawCommandToRequest(input));
        auto cmd = js.dump();
#ifndef NDEBUG
        std::cout << "send: " << cmd << std::endl;
#endif
        int n = write(fd, cmd.data(), cmd.size());
        if (n < 0)
        {
            break;
        }
    }
}