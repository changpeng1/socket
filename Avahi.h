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
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/timeval.h>
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
        static char * name;

    private:
        static AvahiEntryGroup *group;
    public:
        Avahi();
        ~Avahi();
        void Start_Browser();
        void Stop_Browser();
        void Start_Publish();
        void Stop_Publish();
        void Update_Publish();
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


static void browser_client_callback(AvahiClient *c, AvahiClientState state, void * userdata); 

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, void *userdata); 
static void create_services(AvahiClient *c) ;
static void publish_client_callback(AvahiClient *c, AvahiClientState state, void * userdata); 
};
#endif
