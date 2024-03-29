#pragma once

#define TableNameForSSH "activeHostsSSH"
#define TableNameForGoodHosts "goodHosts"
#define TableNameForProgErrorHosts "progErrorHosts"
#define TableNameForErrorHosts "errorHosts"

struct COMMANDS
{
    std::string cmd;
    std::string expect;
    std::string send_to_step;
    std::string not_expect;

    COMMANDS(){}
    COMMANDS(const std::string &cmd,
             const std::string &expect,
             const std::string &send_to_step,
             const std::string &not_expect) : //
                                    send_to_step(send_to_step),
                                    cmd(cmd),
                                    expect(expect),
                                    not_expect(not_expect)
    {
    }
};

struct SNparsedNetworkHost
{
    uint32_t network;
    uint32_t hosts;
};
struct ActiveHOSTS
{
    std::vector<HOST> ssh;
    std::vector<HOST> onlyTelnet;
};