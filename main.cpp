#include <iostream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <boost/system.hpp>


namespace boost
{
    template<typename T>
    boost::optional<T> lexical_cast_optional(const std::string &s)
    {
        try
        {
            return boost::lexical_cast<T>(s);
        }
        catch (const boost::bad_lexical_cast &ex)
        {
            return boost::none;
        }
    }
}

struct CommandLineArgs
{
    std::string ipFrom;
    std::string ipTo;
    unsigned short port;
};

bool IsValidIpOctet(const boost::optional<int> &octet)
{
    return octet && *octet >= 0 && *octet <= 255;
}

boost::optional<std::string> TryParseIp(const std::string &s)
{
    std::vector<std::string> octets;
    boost::split(octets, s, boost::is_any_of("."));

    if (octets.size() != 4)
    {
        return boost::none;
    }

    for (const auto &octet: octets)
    {
        if (!boost::lexical_cast_optional<int>(octet))
        {
            return boost::none;
        }
    }

    return s;
}

boost::optional<CommandLineArgs> TryParseCommandLineArgs(int argc, char *argv[])
{
    if (argc != 3 && argc != 4)
    {
        return boost::none;
    }

    boost::optional<std::string> ipFrom;
    boost::optional<std::string> ipTo;
    boost::optional<unsigned short> port;

    if (argc == 3)
    {
        ipFrom = TryParseIp(argv[1]);
        ipTo = ipFrom;
        port = boost::lexical_cast_optional<unsigned short>(argv[2]);
    }
    else
    {
        ipFrom = TryParseIp(argv[1]);
        ipTo = TryParseIp(argv[2]);
        port = boost::lexical_cast_optional<unsigned short>(argv[3]);
    }

    if (ipFrom && ipTo && port)
    {
        return CommandLineArgs{*ipFrom, *ipTo, *port};
    }

    return boost::none;
}

boost::optional<std::string> GetNextIp(const std::string &ip)
{
    std::vector<std::string> octetStrings;
    boost::split(octetStrings, ip, boost::is_any_of("."));

    for (int i = 3; i >= 0; --i)
    {
        int octet = boost::lexical_cast<int>(octetStrings[i]) + 1;
        bool overflow = false;
        if (octet > 255)
        {
            octet -= 255;
            overflow = true;
        }

        octetStrings[i] = boost::lexical_cast<std::string>(octet);
    
        if (!overflow)
        {
            break;
        }

        if (overflow && i == 0)
        {
            return boost::none;
        }
    }

    return boost::join(octetStrings, ".");
}

bool TryConnect(boost::asio::io_context &ioc, const std::string &ip, unsigned short port)
{
    boost::system::error_code ec;

    boost::asio::ip::tcp::socket socket{ioc};

    auto address = boost::asio::ip::address::from_string(ip);
    boost::asio::ip::tcp::endpoint endpoint{address, port};

    socket.connect(endpoint, ec);

    return !ec.failed();
}

int main(int argc, char *argv[])
{
    auto cmdLineArgsOptional = TryParseCommandLineArgs(argc, argv);

    if (!cmdLineArgsOptional)
    {
        std::cout << "Usage: psc <ip-from> [<ip-to>] <port>" << std::endl;
        return 0;
    }

    
    auto cmdLineArgs = *cmdLineArgsOptional;
    auto nextIp = cmdLineArgs.ipFrom;

    boost::asio::io_context ioc;

    while (true)
    {
        if (TryConnect(ioc, nextIp, cmdLineArgs.port))
        {
            std::cout << "AVAILABLE " << nextIp << ":" << cmdLineArgs.port << std::endl;
        }
        else
        {
            std::cout << "unavailable " << nextIp << ":" << cmdLineArgs.port << std::endl;
        }

        if (nextIp == cmdLineArgs.ipTo)
        {
            break;
        }

        auto nextIpOptional = GetNextIp(nextIp);
        if (!nextIpOptional)
        {
            break;
        }

        nextIp = *nextIpOptional;
    }

    return 0;
}
