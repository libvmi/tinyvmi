/* The LibVMI Library is an introspection library that simplifies access to 
 * memory in a target virtual machine or in a file containing a dump of 
 * a system's physical memory.  LibVMI is based on the XenAccess Library.
 *
 * Author: Matthew Fusaro (matthew.fusaro@zentific.com)
 *
 * This file is part of LibVMI.
 *
 * LibVMI is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * LibVMI is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with LibVMI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <inttypes.h>
#include <signal.h>

#include <tiny_libvmi.h>
// #include <libvmi/events.h>
#include <events.h>

#include "examples.h"

vmi_event_t single_event;

static int interrupted = 0;
static void close_handler(int sig){
    interrupted = sig;
}

event_response_t single_step_callback(vmi_instance_t vmi, vmi_event_t *event)
{
    ttprint(VMI_TEST_EVENTS, "Single-step event: VCPU:%u  GFN %"PRIx64" GLA %016"PRIx64"\n",
        event->vcpu_id,
        event->ss_event.gfn,
        event->ss_event.gla);

    return 0;
}

// int main (int argc, char **argv) {
status_t singlestep_event_example(char *vm_name){
    vmi_instance_t vmi;

    status_t ret = VMI_FAILURE;

    struct sigaction act;

    char *name = vm_name;

    // if(argc < 2){
    //     fprintf(stderr, "Usage: single_step_example <name of VM> \n");
    //     exit(1);
    // }

    // // Arg 1 is the VM name.
    // name = argv[1];

    /* for a clean exit */
    act.sa_handler = close_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGALRM, &act, NULL);

    // /* initialize the libvmi library */
    // if (VMI_FAILURE == vmi_init(&vmi, VMI_XEN, (void*)name, VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS, NULL, NULL))
    // {
    //     ttprint(VMI_TEST_EVENTS, "Failed to init LibVMI library.\n");
    //     return 1;
    // }
    if (VMI_FAILURE == 
        // vmi_init_complete(&vmi, name, VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS, NULL,
        //                   VMI_CONFIG_STRING, get_config_from_file_string(name), NULL))
        vmi_init(&vmi, VMI_XEN, (void*)name, VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS, NULL, NULL))
    {
        ttprint(VMI_TEST_EVENTS, "Failed to init LibVMI library.\n");
        goto _bail;
    }

    ttprint(VMI_TEST_EVENTS, "LibVMI init succeeded!\n");

    //Single step setup
    memset(&single_event, 0, sizeof(vmi_event_t));
    single_event.version = VMI_EVENTS_VERSION;
    single_event.type = VMI_EVENT_SINGLESTEP;
    single_event.callback = single_step_callback;
    single_event.ss_event.enable = 1;
    SET_VCPU_SINGLESTEP(single_event.ss_event, 0);
    vmi_register_event(vmi, &single_event);
    while(!interrupted ){
        ttprint(VMI_TEST_EVENTS, "Waiting for events...\n");
        ret = vmi_events_listen(vmi,500);
        if (ret == VMI_FAILURE){
            ttprint(VMI_TEST_EVENTS, "vmi events listen error\n");
            break;
        }
    }
    ttprint(VMI_TEST_EVENTS, "Finished with test.\n");

    // cleanup any memory associated with the libvmi instance
    vmi_destroy(vmi);

_bail:

    return ret;
}
