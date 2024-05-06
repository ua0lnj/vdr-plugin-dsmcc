#include <stdlib.h>
#include <zlib.h>
//#include <vdr/receiver.h>
#include "dsmcc-decoder.h"
//#include "libdsmcc.h"
// #include <mpatrol.h>

cDsmccReceiver::cDsmccReceiver(const char *channel) : cReceiver(0, -1) {

	char *cache;
	cache = (char*)malloc(strlen(CACHEDIR)+6+1);
	strcpy(cache, CACHEDIR);
	strcpy(cache + strlen(CACHEDIR), "/dsmcc");

	if(channel != NULL) {
		name = (char*)malloc(strlen(channel)+1);
		strcpy(name, channel);
	} else {
		name = (char*)malloc(7+1);
		strcpy(name, "Unknown");
	}

	status = dsmcc_open(channel, cache, NULL);	/* XXX pass log_fd */

	free(cache);
        scanning = 0;   /* Set to 1 to scan thorugh all channels for carousel
                           Set to 0 to disable
                        */


}

cDsmccReceiver::~cDsmccReceiver() {

	/* Free any carousel data and cached data. 
	 * TODO - actually cache on disk the cache data
	 * TODO - more terrible memory madness, this all needs improving
	 */

	/* Free any unattached streams */

	if(name)
		free(name);

//      if(debug_fd != NULL) fclose(debug_fd);
//      if(test_fd != NULL) fclose(test_fd);

        Detach(); /* TODO ??? */
}

bool cDsmccReceiver::Active() {
	return active;
}

void cDsmccReceiver::Activate(bool On) {
	;
}

void cDsmccReceiver::AddStream(struct dsmcc_status *status, int pid) {
	struct pid_buffer *buf, *lbuf;

	for(lbuf=status->buffers;lbuf!=NULL;lbuf=lbuf->next) { 
		if(lbuf->pid == pid) { return; }
	}

//	esyslog("Created buffer for pid %d\n", pid);

	buf = (struct pid_buffer *)malloc(sizeof(struct pid_buffer));
	buf->pid = pid;
	buf->pointer_field = 0;
	buf->in_section = 0;
	buf->cont = -1;
	buf->next = NULL;

	if(status->buffers == NULL) {
		status->buffers = buf;
	} else {
		for(lbuf=status->buffers;lbuf->next!=NULL;lbuf=lbuf->next) { ; }
		lbuf->next = buf;
	}

	AddPid(pid);

	if(!Active()) {
		Start();
		active = true;
	}
}

/* Start of new logging stuff */
#define ilog(a...)
#define dlog(a...)
#define elog(a...)
	
void cDsmccReceiver::Receive(const uchar *Data, int Length) {
	int full_cache = 0;
	int i;

	dsmcc_receive(status, Data, Length);

        for(i=0;i<MAXCAROUSELS;i++) {
                if(status->carousels[i].streams != NULL) {
                        if((status->carousels[i].filecache->total_files > 0) && ((status->carousels[i].filecache->num_files <= 0) && (status->carousels[i].filecache->num_dirs <= 0))) {
                                full_cache = 1;
                                esyslog("[dsmcc] Finished receiving cache for carousel %d", i);
                        } else {
                                full_cache = 0;
                        }
                }
        }

        if(full_cache) {
//		esyslog("Detaching receiver");
		Detach();
		active = false;
/*          if(scaning) {
                cDevice *dev;
//                esyslog("Switching channel after full cache");
                dev = cDevice::PrimaryDevice();
                result = dev->SwitchChannel(1);
                if(result == false) {
                         Gone through all channels 
//                        esyslog("Finished switching channels for cache");
                        scanning = 0;
                }
            }
*/
        }

	if(status->newstreams) {
		struct stream *s;

        	for(s=status->newstreams;s!=NULL;s=s->next) {
                	AddStream(status, s->pid);
		}

	        if(status->streams == NULL) {
                    status->streams = status->newstreams;
                } else {
                    struct stream *str;
                    for(str=status->streams;str!=NULL;str=str->next){;}
                    str->next = status->newstreams;
                }
                    status->newstreams = NULL;
        }

}
