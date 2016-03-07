#include <iostream>
#include <string>
#include "Avahi.h"
char* Avahi::name=NULL;
AvahiSimplePoll * Avahi::simple_poll=NULL;
AvahiClient *Avahi::client=NULL;
AvahiServiceBrowser *Avahi::sb=NULL;
AvahiEntryGroup *Avahi::group=NULL;
void Avahi::resolve_callback(
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
    void* userdata) 
{

    assert(r);

    /* Called whenever a service has been resolved successfully or timed out */

    switch (event) {
        case AVAHI_RESOLVER_FAILURE:
            fprintf(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
            break;

        case AVAHI_RESOLVER_FOUND: {
            char a[AVAHI_ADDRESS_STR_MAX], *t;

            fprintf(stderr, "Service '%s' of type '%s' in domain '%s':\n", name, type, domain);

            avahi_address_snprint(a, sizeof(a), address);
            t = avahi_string_list_to_string(txt);
            fprintf(stderr,
                    "\t%s:%u (%s)\n"
                    "\tTXT=%s\n",
                    host_name, port, a,
                    t);
            avahi_free(t);
        }
    }

    avahi_service_resolver_free(r);
}

void Avahi::browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AvahiLookupResultFlags flags,
    void* userdata) 
{

    AvahiClient *c = (AvahiClient*)userdata;

    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */

    switch (event) {
        case AVAHI_BROWSER_FAILURE:

            fprintf(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
            avahi_simple_poll_quit(simple_poll);
            return;

        case AVAHI_BROWSER_NEW:
            fprintf(stderr, "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);

            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */

            if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, (AvahiLookupFlags)0, resolve_callback, c)))
                fprintf(stderr, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));

            break;

        case AVAHI_BROWSER_REMOVE:
            fprintf(stderr, "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
            break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            fprintf(stderr, "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
            break;
    }
}

void Avahi::browser_client_callback(AvahiClient *c, AvahiClientState state, void * userdata) 
{
    assert(c);

    /* Called whenever the client or server state changes */

    if (state == AVAHI_CLIENT_FAILURE) {
        fprintf(stderr, "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
        avahi_simple_poll_quit(simple_poll);
    }
}


void Avahi::entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
    assert(g == group || group == NULL);
    group = g;

    /* Called whenever the entry group state changes */

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            fprintf(stderr, "Service '%s' successfully established.\n", name);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name(name);
            avahi_free(name);
            name = n;

            fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

            /* And recreate the services */
            create_services(avahi_entry_group_get_client(g));
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            fprintf(stderr, "Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

            /* Some kind of failure happened while we were registering our services */
            avahi_simple_poll_quit(simple_poll);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

void Avahi::create_services(AvahiClient *c) {
    char *n, r[128];
    int ret;
    assert(c);

    /* If this is the first time we're called, let's create a new
     * entry group if necessary */

    if (!group)
        if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
            fprintf(stderr, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
            goto fail;
        }

    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */

    if (avahi_entry_group_is_empty(group)) {
        fprintf(stderr, "Adding service '%s'\n", name);

        /* Create some random TXT data */
        snprintf(r, sizeof(r), "HOST=192.168.1.101\r\nPORT=5000\r\nTYPE=Master\r\nID=1234567898\r\n");

        /* We will now add two services and one subtype to the entry
         * group. The two services have the same name, but differ in
         * the service type (IPP vs. BSD LPR). Only services with the
         * same name should be put in the same entry group. */

        /* Add the service for IPP */
        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, (AvahiPublishFlags)0, name, "_auralic._tcp", NULL, NULL, 6510, r, NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            fprintf(stderr, "Failed to add _ipp._tcp service: %s\n", avahi_strerror(ret));
            goto fail;
        }
        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0) {
            fprintf(stderr, "Failed to commit entry group: %s\n", avahi_strerror(ret));
            goto fail;
        }
    }

    return;

collision:

    /* A service name collision with a local service happened. Let's
     * pick a new name */
    n = avahi_alternative_service_name(name);
    avahi_free(name);
    name = n;

    fprintf(stderr, "Service name collision, renaming service to '%s'\n", name);

    avahi_entry_group_reset(group);

    create_services(c);
    return;

fail:
    avahi_simple_poll_quit(simple_poll);
}

void Avahi::publish_client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
    assert(c);

    /* Called whenever the client or server state changes */

    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:

            /* The server has startup successfully and registered its host
             * name on the network, so it's time to create our services */
            create_services(c);
            break;

        case AVAHI_CLIENT_FAILURE:

            fprintf(stderr, "Client failure: %s\n", avahi_strerror(avahi_client_errno(c)));
            avahi_simple_poll_quit(simple_poll);

            break;

        case AVAHI_CLIENT_S_COLLISION:

            /* Let's drop our registered services. When the server is back
             * in AVAHI_SERVER_RUNNING state we will register them
             * again with the new host name. */

        case AVAHI_CLIENT_S_REGISTERING:

            /* The server records are now being established. This
             * might be caused by a host name change. We need to wait
             * for our own records to register until the host name is
             * properly esatblished. */

            if (group)
                avahi_entry_group_reset(group);

            break;

        case AVAHI_CLIENT_CONNECTING:
            ;
    }
}

Avahi::Avahi()
{

    /* Allocate main loop object */
    if (!(simple_poll = avahi_simple_poll_new())) {
        fprintf(stderr, "Failed to create simple poll object.\n");
    }

}
Avahi::~Avahi()
{
}
void Avahi::Start_Browser()
{
    int error;
    int ret = 1;
    /* Allocate a new client */
    client = avahi_client_new(avahi_simple_poll_get(simple_poll), (AvahiClientFlags)0, browser_client_callback, NULL, &error);

    /* Check wether creating the client object succeeded */
    if (!client) {
        fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
    }
    /* Create the service browser */
    if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_auralic._tcp", NULL, (AvahiLookupFlags)0, browse_callback, client))) {
        fprintf(stderr, "Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client)));
    }
    avahi_simple_poll_loop(simple_poll);
}
void Avahi::Stop_Browser()
{
    avahi_simple_poll_quit(simple_poll);
    if (sb)
        avahi_service_browser_free(sb);

    if (client)
        avahi_client_free(client);

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);
}
void Avahi::Start_Publish()
{
    int error;
    name = avahi_strdup("GroupService");

    /* Allocate a new client */
    client = avahi_client_new(avahi_simple_poll_get(simple_poll), (AvahiClientFlags)0, publish_client_callback, NULL, &error);

    /* Check wether creating the client object succeeded */
    if (!client) {
        fprintf(stderr, "Failed to create client: %s\n", avahi_strerror(error));
    }

    /* After 10s do some weird modification to the service */
    /*
    avahi_simple_poll_get(simple_poll)->timeout_new(
        avahi_simple_poll_get(simple_poll),
        avahi_elapse_time(&tv, 1000*10, 0),
        modify_callback,
        client);
*/
    /* Run the main loop */
    avahi_simple_poll_loop(simple_poll);
}
void Avahi::Stop_Publish()
{
    avahi_simple_poll_quit(simple_poll);
    if (client)
        avahi_client_free(client);

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);

    avahi_free(name);

}
void Avahi::Update_Publish()
{
    fprintf(stderr, "Doing some weird modification\n");

    avahi_free(name);
    name = avahi_strdup("Modified MegaPrinter");

    /* If the server is currently running, we need to remove our
     * service and create it anew */
    if (avahi_client_get_state(client) == AVAHI_CLIENT_S_RUNNING) {

        /* Remove the old services */
        if (group)
            avahi_entry_group_reset(group);

        /* And create them again with the new name */
        create_services(client);
    }
}
int main()
{
    Avahi *s= new Avahi;
    s->Start_Browser();
//    s->Start_Publish();
    delete s;
}
