#pragma once

#define TableNameForSSH "activeHostsSSH"
#define TableNameForGoodHosts "goodHosts"
#define TableNameForProgErrorHosts "progErrorHosts"
#define TableNameForErrorHosts "errorHosts"
#define TableNameForIdentify "identify"
#define TableNameForTELNET "activeHostsTELNET"

struct COMMANDS
{
    std::string cmd;
    std::string expect;
    std::string send_to_step;
    std::string not_expect;

    COMMANDS() {}
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
    uint32_t network; // часть маски (1)
    uint32_t hosts;   // часть маски (0)
};
struct ActiveHOSTS
{
    std::vector<HOST> ssh;
    std::vector<HOST> onlyTelnet;
};