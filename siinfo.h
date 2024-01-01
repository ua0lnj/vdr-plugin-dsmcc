#ifndef DSMCC_SIIINFO
#define DSMCC_SIIINFO

#include <signal.h>
#include "dsmcc.h"

struct application_hint {
	unsigned short type;
	char priority;
	unsigned char data_len;
	char *data;
};

struct data_broadcast_desc {
	unsigned int count;
	struct application_hint *hints;
};

struct standard_boot {
	char *data;
};

struct enhanced_boot {
	 unsigned char mod_versoin;
	 unsigned short mod_id;
	 unsigned short block_size;
	 unsigned long module_size;
	 unsigned char compression_method;
	 unsigned long orig_size;
	 unsigned char timeout;
	 unsigned char objkey_len;;
	 char *objkey;
	 char *priv_data;
};

struct carousel_desc {
	unsigned long id;
	char format_id;

	union {
		struct standard_boot sboot;
		struct enhanced_boot eboot;
	} boot;
};


struct struPids {
  int pids[256];
  int pidCount;
};
static struct struPids Pids;


/*
 * find the mheg info in the PMT via the PAT, try first with the SID
 * and if that fails with the VPID
 * return <> 0 on error;
 */
int GetMhegInfo(int, unsigned short, struct dsmcc_status *);

void KillMetadataPids();
void ResetMetadataPids();

#endif
