#ifndef __AVAHI__SUPPORT__
#define __AVAHI__SUPPORT__
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>

#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
/*
        AvahiSimplePoll *simple_poll;
        AvahiClient *client;
        AvahiServiceBrowser *sb;
        */
class Avahi
{
    private:
        static AvahiSimplePoll *simple_poll;
        static AvahiClient *client;
        static AvahiServiceBrowser *sb;
    public:
        Avahi();
        ~Avahi();
        void Start_Browser();
        void Stop_Browser();
    public:
static void resolve_callback(
    AvahiServiceResolver *r,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* userdata); 

static void browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AvahiLookupResultFlags flags,
    void* userdata) ;


static void client_callback(AvahiClient *c, AvahiClientState state, void * userdata); 
};
#endif
