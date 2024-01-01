#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/poll.h>
#include <time.h>
#include <netinet/in.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <linux/unistd.h>
#include <unistd.h>
#include <syslog.h>

#include <linux/dvb/dmx.h>
#include "dsmcc-siinfo.h"

#define DATA_CAROUSEL_ID	0x13
#define DATA_BROADCAST_ID	0x66
#define DATA_STREAM		0x52
#define MHP_BROADCAST_ID	0x6F
#define UK_MHEG_DATA		0x106
#define RU_MHEG_DATA		0x123
#define MHP_DATA		0xF0

#define PACK  __attribute__ ((__packed__))

struct sect_header {
  uint8_t table_id   PACK;
  uint16_t syntax_len   PACK;
  uint16_t transport_stream_id   PACK;
  uint8_t ver_cur   PACK;
  uint8_t section_number   PACK;
  uint8_t last_section_number   PACK;
};

// H.222.0 2.4.4.3, Table 2-25
struct PAT_sect {
  uint8_t table_id   PACK;
  uint16_t syntax_len   PACK;
  uint16_t transport_stream_id   PACK;
  uint8_t vers_curr   PACK;
  uint8_t sect_no   PACK;
  uint8_t last_sect   PACK;
  struct {
    uint16_t program_number   PACK;
    uint16_t res_PMTPID   PACK;
  } d[1] PACK;
};

// H.222.0 2.4.4.8, Table 2-28
struct PMT_stream {
  uint8_t stream_type   PACK;
  uint16_t res_PID   PACK;
  uint16_t res_ES_info_len   PACK;
  uint8_t descrs[2]   PACK;
};

struct PMT_sect {
  uint8_t table_id   PACK;
  uint16_t syntax_len   PACK;
  uint16_t program_number   PACK;
  uint8_t vers_curr   PACK;
  uint8_t sect_no   PACK;
  uint8_t last_sect   PACK;
  uint16_t res_pcr   PACK;
  uint16_t res_program_info_length   PACK;
  struct PMT_stream s   PACK;
};

struct Metadata_sect {
  unsigned int table_id;
  u_char section_syntax_indicator :1;
  u_char private_indicator :1;
  u_char random_access_indicator :1;
  u_char decoder_config_flag :1;
  unsigned int metadata_section_length: 12;
  unsigned int metadata_service_id;
  unsigned int reserved;
  u_char section_fragment_indication :2;
  u_char version_number :5;
  u_char current_next_indicator :1;
  unsigned int section_number;
  unsigned int last_section_number;
  char *metadata_byte;
  unsigned long CRC_32;

  int metadata_length;
};

struct app_profile {
	unsigned short profile;
	unsigned char major;
	unsigned char minor;
	unsigned char micro;
};

struct app_descr {
	unsigned short app_profile_len;
	struct app_profile *profiles;
	unsigned char flags;	/* 11111223 -
				3 - Service_bound_flag
				2 - Visibility
				1 - reserved */
	unsigned char priority;
	char *proto_labels;
};

struct app_name {
	unsigned long iso_639_lang;
	unsigned char name_length;
	char *name;
};

struct app_name_descr {
	struct app_name *names;
};

struct app_icon_descr {
	unsigned char icon_loc_len;
	char *icon_loc;
	unsigned short flags;
	char *reserved;
};

struct app_auth {
	char app_identifier[6];
	unsigned char priority;
};

struct app_auth_descr {
	struct app_auth *auths;
};

struct trans_proto_desc {
	unsigned short id;
	unsigned char proto_label;
	char *selector;
};

struct trans_proto_label {
	unsigned char len;
	char *label;
	unsigned char priority;
};

struct oc_prefetch_descr {
	unsigned char proto_label;
	struct trans_proto_label *labels;
};

struct oc_dii_descr {
	unsigned char proto_label;
	char *dii_location[4];
};

struct dvbj_app_descr {
	unsigned char param_len;
	char *param;
};

struct dvbj_loc_descr {
	unsigned char base_dir_len;
	char base_dir;

	unsigned char classpath_ext_len;
	char classpath_ext;

	char initial_class;
};

struct dvbh_app_descr {
	unsigned char appid_set_len;
	char application_id;

	char parameter;
};

struct dvbh_loc_descr {
	unsigned char physical_root_len;
	char *physical_root;

	char *inital_path;
};

struct dvbh_boundary_descr {
	unsigned char label_len;
	char *label;
	char *regular_expression;
};

struct route_ipv4_descr {
	unsigned char tag;
	unsigned long address;
	unsigned short port;
	unsigned long mask;
};

struct route_ipv6_descr {
	unsigned char tag;
	unsigned char address[16];
	unsigned short port;
	unsigned char mask[16];
};
	
struct mhp_descr {
	unsigned short tag;
	unsigned short len;
	union {
		struct app_descr app;
		struct app_name_descr app_name;
		struct app_icon_descr app_icon;
		struct app_auth_descr app_auth;

		struct trans_proto_desc trans_proto;
		struct trans_proto_label trans_label;

		struct oc_prefetch_descr oc_prefetch;

		struct dvbj_app_descr dvbj_app;
		struct dvbj_loc_descr dvbj_loc;
		struct dvbh_app_descr dvbh_app;
		struct dvbh_loc_descr dvbh_loc;
		struct dvbh_boundary_descr dvbh_bound;

		struct route_ipv4_descr route_ipv4;
		struct route_ipv6_descr route_ipv6;
	} descr;
};

struct app_info {
	unsigned long app_identifier;
	unsigned char app_control_code;
	unsigned short app_desc_length;
	struct mhp_descr *descr;
};

struct app_info_sect {
	unsigned char section_number;
	unsigned char last_section_number;
	unsigned short descr_length;
	struct mhp_descr *common_descrs;
	unsigned short app_loop_length;
	struct app_info *apps;
	unsigned long crc32;
	struct app_info_sect *next;
};

struct app_info_table {
	struct app_info_sect *sections;
};

static int SetSectFilt(int fd, unsigned short pid, uint8_t tnr, uint8_t mask)
{
  int ret;

  struct dmx_sct_filter_params p;

  memset(&p, 0, sizeof(p));

  p.filter.filter[0] = tnr;
  p.filter.mask[0] = mask;
  p.pid     = pid;
  p.timeout = 800;
  p.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  if ((ret = ioctl(fd, DMX_SET_FILTER, &p)) < 0)
    perror("DMX SET FILTER:");

  return ret;
}

static int SetSectFilt2(int fd, unsigned short pid, uint8_t tnr, uint8_t mask)
{
  int ret;

  struct dmx_sct_filter_params p;

  memset(&p, 0, sizeof(p));

  p.filter.filter[0] = tnr;
  p.filter.mask[0] = mask;
  p.pid     = pid;
  //p.timeout = 600;
  p.flags = DMX_IMMEDIATE_START | DMX_CHECK_CRC;

  if ((ret = ioctl(fd, DMX_SET_FILTER, &p)) < 0)
    perror("DMX SET FILTER:");

  return ret;
}

/*
 * PID - pid to collect on
 * table_id - table id to filter out (H.222.0 table 2-26)
 * sects - pointer to char *[256], points to array of table sections, initially empty
 * numsects - number of sections in table
 * returns - 0 if everything ok
 */
#define SECTSIZE 1024
static int CollectSections(int card_no, int pid, int table_id, char **sects, int *numsects)
{
  int fd;
  int ret = -1;
  int last_section = 0;
  int done = 0;
  char *p = NULL;
  int n;
  char name[100];

  syslog(LOG_ERR, "COllectSections");

  syslog(LOG_ERR, "opening  adapter%d", card_no);
  snprintf(name, sizeof(name), "/dev/dvb/adapter%d/demux0", card_no);

  memset(sects, 0, sizeof(char*) * 256);

  if((fd = open(name, O_RDWR)) < 0){
    syslog(LOG_ERR, "failed to open adapter%d", card_no);
    perror("DEMUX DEVICE 1: ");
    return -1;
  }


  if(SetSectFilt(fd, pid, table_id, 0xff)) {
    syslog(LOG_ERR, "setsectfilter failed");
    ret = -1;
    goto bail;
  }


  do {
    struct sect_header *h;
    int i;

    if(p == NULL)
      p = (char *) malloc(SECTSIZE);

    n = read(fd, p, SECTSIZE);

    if(n < 8) {
      syslog(LOG_ERR, "n < 8");
      break;
    }

    h = (struct sect_header *) p;

    if(n != ((ntohs(h->syntax_len) & 0xfff) + 3)) {
      fprintf(stderr, "bad section length: %x / %x!\n", n, ntohs(h->syntax_len) & 0xfff);
      continue;
    }
    //syslog(LOG_ERR,"id %x\n",h->table_id);
    //syslog(LOG_ERR,"ver %x\n",h->ver_cur);

    last_section = h->last_section_number;

    if(!sects[h->section_number]) { // section_number
      sects[h->section_number] = p;
      p = NULL;
    }

    for(i = 0; i <= last_section; i++) {
      if(!sects[i]) {
        syslog(LOG_ERR, "!sects[%d]", i);
	break;
      }

      if(i == last_section) {
        syslog(LOG_ERR, "Set  numsects = %d", last_section+1);
	*numsects = last_section + 1;
	ret = 0;
	done = 1;
      }
    }

  } while (!done);

   syslog(LOG_ERR, "ret = %d", ret);

 bail:
  close(fd);
  return ret;
}

#define METADATA_SECT_SIZE 4096

static int CollectMetadataSections(int card_no, int pid, int table_id, struct Metadata_sect **sects, int *numsects)
{
  int fd;
  int ret = -1;
  int last_section = 0;
  int done = 0;
  char *p = NULL;
  int j, n;
  int len;
  char name[100];

  if (table_id != 0x06) {
    perror("NOT A VALID Metadata Section: ");
    return -1;
  }

  snprintf(name, sizeof(name), "/dev/dvb/adapter%d/demux0", card_no);
  memset(sects, 0, sizeof(char*) * 256);

  if((fd = open(name, O_RDWR)) < 0){
    perror("DEMUX DEVICE 1: ");
    return -1;
  }

  if(SetSectFilt2(fd, pid, table_id, 0xff)) {
    ret = -1;
    goto bail;
  }

  do {
    struct sect_header *h;
    int i;

    if(p == NULL)
      p = (char *) malloc(METADATA_SECT_SIZE);

    n = read(fd, p, METADATA_SECT_SIZE);

    printf("n = %d\n", n);

    if(n < 8)
      break;

    h = (struct sect_header *) p;

    if (n != ((ntohs(h->syntax_len) & 0xfff) + 3)) {
      fprintf(stderr, "bad section length: %x / %x!\n", n, ntohs(h->syntax_len) & 0xfff);
      continue;
    }

    if(!(h->ver_cur & 0x01)) // if not current
      continue;

    last_section = h->last_section_number;

    if(!sects[h->section_number]) { // section_number

      sects[h->section_number] = (struct Metadata_sect *)malloc(sizeof(struct Metadata_sect));

      sects[h->section_number]->table_id = p[0];
      sects[h->section_number]->section_syntax_indicator = (p[1] >> 7) & 0x01;
      sects[h->section_number]->private_indicator = (p[1] >> 6) & 0x01;
      sects[h->section_number]->random_access_indicator = (p[1] >> 5) & 0x01;
      sects[h->section_number]->decoder_config_flag = (p[1] >> 4) & 0x01;
      sects[h->section_number]->metadata_section_length = (0xFF & p[2]) | ((0x0F & p[1]) << 8);
      sects[h->section_number]->metadata_service_id = p[3];
      sects[h->section_number]->reserved = p[4];
      sects[h->section_number]->section_fragment_indication = (p[5] >> 6) & 0x03;
      sects[h->section_number]->version_number = (p[5] >> 3) & 0x1F;
      sects[h->section_number]->current_next_indicator = p[5] & 0x01;
      sects[h->section_number]->section_number = p[6];
      sects[h->section_number]->last_section_number = p[7];

      printf("table_id = %d\n", sects[h->section_number]->table_id);
      printf("section_syntax_indicator = %d\n", sects[h->section_number]->section_syntax_indicator);
      printf("private_indicator = %d\n", sects[h->section_number]->private_indicator);
      printf("random_access_indicator = %d\n", sects[h->section_number]->random_access_indicator);
      printf("decoder_config_flag = %d\n", sects[h->section_number]->decoder_config_flag);
      printf("metadata_section_length = %d\n", sects[h->section_number]->metadata_section_length);
      printf("metadata_service_id = %d\n", sects[h->section_number]->metadata_service_id);
      printf("reserved = %d\n", sects[h->section_number]->reserved);
      printf("section_fragment_indication = %d\n", sects[h->section_number]->section_fragment_indication);
      printf("version_number = %d\n", sects[h->section_number]->version_number);
      printf("current_next_indicator = %d\n", sects[h->section_number]->current_next_indicator);
      printf("section_number = %d\n", sects[h->section_number]->section_number);
      printf("last_section_number = %d\n", sects[h->section_number]->last_section_number);

      len = sects[h->section_number]->metadata_section_length - 9;
      sects[h->section_number]->metadata_length = len;

      printf("len = %d\n", len);

      sects[h->section_number]->metadata_byte = (char *)malloc(len);
      for (j = 0; j < len; j++) {
        sects[h->section_number]->metadata_byte[j] = p[8 + j];
      }

//      printf("metadata_byte = %d\n", sects[h->section_number]->metadata_byte);

      sects[h->section_number]->CRC_32 = p[8 + len];

      printf("CRC_32 = %ld\n", sects[h->section_number]->CRC_32);

      p = NULL;
    }

    for(i = 0; i <= last_section; i++) {
      if(!sects[i]) {
	break;
      }

      if(i == last_section) {
	*numsects = last_section + 1;
	ret = 0;
	done = 1;
      }
    }

  } while (!done);

 bail:
  close(fd);
  return ret;
}

static void FreeSects(char **sects)
{
  int i;

  for(i = 0; i < 256; i++) {
    if(sects[i])
      free(sects[i]);
  }
}

static void FreeMetadataSects(struct Metadata_sect **sects)
{
  int i;

  for(i = 0; i < 256; i++) {
    if(sects[i]) {
      if (sects[i]->metadata_byte) free(sects[i]->metadata_byte);
      free(sects[i]);
      sects[i]->metadata_byte = NULL;
      sects[i] = NULL;
    }
  }
}

static void ExtractMetadataSection(int card_no, int PID, char *chaname) {
  FILE *metadata_fd;
  int numsects;
  char dirbuf1[256];
  char dirbuf2[256];
  char filebuf[256];

  struct Metadata_sect *metadatasects[256];

  int j, i = 0;
  while (1) {
    CollectMetadataSections(card_no, PID, 0x06, metadatasects, &numsects);

    sprintf(dirbuf1, "%s/%s", "/tmp/cache", chaname);
    mkdir(dirbuf1, 0755);

    printf("First directory: %s\n", dirbuf1);

    sprintf(dirbuf2, "%s/%s/%s", "/tmp/cache", chaname, "MetadataSections");
    mkdir(dirbuf2, 0755);

    printf("Second directory: %s\n", dirbuf2);

    sprintf(filebuf, "%s/%s/%s/%s%d%s%d%s", "/tmp/cache", chaname, "MetadataSections", "file_", j, "_", PID, ".mp7");

    metadata_fd = fopen(filebuf, "w");

    printf("numsects: %d\n", numsects);
    for(i = 0; i < numsects; i++) {
      fwrite(metadatasects[i]->metadata_byte, 1, metadatasects[i]->metadata_length, metadata_fd);
    }

    if (fclose(metadata_fd) != 0) {
      perror("fclose: ");
      exit(1);
    }

    FreeMetadataSects(metadatasects);
    j++;
  }
}

static void ExtractMhegInfo(int card_no, char **pmtsects, int numsects, struct dsmcc_status *status)
{
  int i;
  int car_num = 0;

  for(i = 0; i < numsects; i++) {
    struct PMT_sect *psect;
    char *sp;
    char *end;

    if(status->debug_fd != NULL) {
    	fprintf(status->debug_fd, "[pmtparse] Parsing PMT\n");
    }

    psect = (struct PMT_sect *) pmtsects[i];
    end = pmtsects[i] + (psect->syntax_len & 0x3ff) - 7;
    // skip extra program info
    sp = ((char*)&(psect->s)) + (ntohs(psect->res_program_info_length) & 0xfff);

    while(sp < end) {
      struct PMT_stream *s = (struct PMT_stream *) sp;

      if(status->debug_fd != NULL) {
        fprintf(status->debug_fd, "[pmtparse] Stream Type %X\n",s->stream_type);
      }

      if(s->stream_type == 0xB) { // ISO/IEC 13818-6 Type B (DSMCC)
	uint8_t *descr;
	unsigned int data_id = 0 ;	/* TODO these can probably be     */
	unsigned int component_tag = 0; /* legally set to 0 ? Check spec. */
	unsigned long carousel_id = 0;
	unsigned short app_type_code;
	unsigned char boot_pri_hint, app_data_length, format_id;
	char *app_data;

		
	for(descr = s->descrs;
	    descr < s->descrs + (ntohs(s->res_ES_info_len) & 0xfff);
	    descr += descr[1] + 2) {

	      if(descr[0] == DATA_BROADCAST_ID) {
		int length = descr[1];
		data_id = descr[2] << 8 | descr[3];

		if(data_id == RU_MHEG_DATA || data_id == UK_MHEG_DATA) {
		  int index = 4;
		  while(index < length+2) {	/* Only 1 app defined... */
		   app_type_code = descr[index] << 8 | descr[index+1];
		   index+=2;
		   boot_pri_hint = descr[index++];
		   app_data_length = descr[index++];
		   app_data = (char *)malloc(app_data_length);
		   memcpy(app_data, descr+index, app_data_length);
		   /* TODO BBC Parliament seems to have some data here,
		    * no idea what as UKProfile 1.05 says there is
		    * no app data defined. */
		  } /* TODO verify / save this data */
		} else if(data_id == MHP_DATA) { /* MHP Object Carousel */
		  syslog(LOG_ERR, "MHP Object Carousel data_broadcast_id");
		  int index = 4;
		  while(index < length) {
		    app_type_code = descr[index] << 8 | descr[index+1];
		    index+=2;
		  }
		}
	      } else if(descr[0] == DATA_STREAM) {
		component_tag = descr[2];
	      } else if(descr[0] == DATA_CAROUSEL_ID) {
		/* Set carousel this stream belongs to */
		carousel_id = (descr[2] << 24) | (descr[3] << 16) |
			      (descr[4] << 8)  | descr[5];
		format_id = descr[6];
		if(format_id == 0x00) {
			/* Standard Boot - do nothing */
		} else if(format_id == 0x01) {
			/* Enhanced Boot - TODO handle! */
		}
	      } else {
		 syslog(LOG_ERR, "Descriptor - %X", descr[0]);
	      }
	}
	/* Now save stream info to correct carousel. If no carousel
	 * descriptor / data_broadcast_id descriptor given we cannot
 	 * guess where to save it to yet, so we save the details to
	 * a list of known channels, where it can be collected from later
 	 * from information in DSI/DII messages.
	 * If we have data_broadcast_id but no carousel_id we create a
	 * new carousel but do not assign a carousel_id, when parsing the
	 * DSI message we try and find which stream
	 * Hmm, the teletext stream does not send a carousel_id for either
	 * of its carousels... luckily it has no other streams for each
	 * but if it did not sure how to identify which belongs to which
	*/
      	if(data_id != 0) {
	    struct stream *str = (struct stream *)malloc(sizeof(struct stream));
	  /* New object carousel */
	  if(carousel_id) {
	    /* Hurrah! We can create a new object carousel with all the info */
	    status->carousels[car_num].id = carousel_id;
					/* TODO Init cache from saved data */
	    str->pid = ntohs(s->res_PID) & 0x1fff;
	    str->assoc_tag = component_tag;
	    str->next = str->prev = NULL;
	    status->carousels[car_num].streams = str;
      	    if(status->debug_fd != NULL) {
	    	fprintf(status->debug_fd, "[pmtparse] Initing carousel %d to pid %d (tag %X)\n", car_num, str->pid, str->assoc_tag);
      	    }
	    car_num++;
	  } else {
	    /* Hmmm, no carousel_id, set to zero and find out
	     * later hopefully (this is all for you teletext) */
	    status->carousels[car_num].id = 0;
					/* TODO Init cache from saved data */
	    str->pid = ntohs(s->res_PID) & 0x1fff;
	    str->assoc_tag = component_tag;
	    str->next = str->prev = NULL;
	    status->carousels[car_num].streams = str;
      	    if(status->debug_fd != NULL) {
	    	fprintf(status->debug_fd, "[pmtparse] Initing carousel %d to pid %d (tag %X)\n", car_num, str->pid, str->assoc_tag);
      	    }
	    car_num++;
	  }
	} else {
	  /* New stream belong to unknown carousel */
	  struct stream *str= (struct stream *)malloc(sizeof(struct stream));
	  str->pid = ntohs(s->res_PID) & 0x1fff;
	  str->assoc_tag = component_tag;
      	  if(status->debug_fd != NULL) {
	    	fprintf(status->debug_fd, "[pmtparse] Unassigned stream on pid %d (tag %X)\n", str->pid, str->assoc_tag);
      	  }
	  if(status->newstreams == NULL) {
	    status->newstreams = str;
	    str->prev = NULL;
	  } else {
	     struct stream *last;
	     for(last=status->newstreams;last->next!=NULL;last=last->next) {;}
	     last->next = str;
	     str->prev = last;
	  }
	  str->next = NULL;
	}

      } else if(s->stream_type == 0x18) {   /* Metadata carried in ISO/IEC 1318-6 (DSM-CC) Object Carousel */
	uint8_t *descr;
	unsigned int data_id = 0 ;	/* TODO these can probably be     */
	unsigned int component_tag = 0; /* legally set to 0 ? Check spec. */
	unsigned long carousel_id = 0;
	unsigned short app_type_code;
	unsigned char boot_pri_hint, app_data_length, format_id;
	char *app_data;

	for(descr = s->descrs;
	    descr < s->descrs + (ntohs(s->res_ES_info_len) & 0xfff);
	    descr += descr[1] + 2) {

	      if(descr[0] == DATA_BROADCAST_ID) {
		int length = descr[1];
		data_id = descr[2] << 8 | descr[3];

		if(data_id == RU_MHEG_DATA || data_id == UK_MHEG_DATA) {
		  int index = 4;
		  while(index < length+2) {	/* Only 1 app defined... */
		   app_type_code = descr[index] << 8 | descr[index+1];
		   index+=2;
		   boot_pri_hint = descr[index++];
		   app_data_length = descr[index++];
		   app_data = (char *)malloc(app_data_length);
		   memcpy(app_data, descr+index, app_data_length);
		   /* TODO BBC Parliament seems to have some data here,
		    * no idea what as UKProfile 1.05 says there is
		    * no app data defined. */
		  } /* TODO verify / save this data */
		} else if(data_id == MHP_DATA) { /* MHP Object Carousel */
//		  syslog(LOG_ERR, "MHP Object Carousel data_broadcast_id");
		  int index = 4;
		  while(index < length+2) {
		    app_type_code = descr[index] << 8 | descr[index+1];
		    index+=2;
		  }
		}
	      } else if(descr[0] == DATA_STREAM) {
		component_tag = descr[2];
	      } else if(descr[0] == DATA_CAROUSEL_ID) {
		/* Set carousel this stream belongs to */
		carousel_id = (descr[2] << 24) | (descr[3] << 16) |
			      (descr[4] << 8)  | descr[5];
		format_id = descr[6];
		if(format_id == 0x00) {
			/* Standard Boot - do nothing */
		} else if(format_id == 0x01) {
			/* Enhanced Boot - TODO handle! */
		}
	      } else {
		// syslog(LOG_ERR, "Descriptor - %X", descr[0]);
	      }
	}
	/* Now save stream info to correct carousel. If no carousel
	 * descriptor / data_broadcast_id descriptor given we cannot
 	 * guess where to save it to yet, so we save the details to
	 * a list of known channels, where it can be collected from later
 	 * from information in DSI/DII messages.
	 * If we have data_broadcast_id but no carousel_id we create a
	 * new carousel but do not assign a carousel_id, when parsing the
	 * DSI message we try and find which stream
	 * Hmm, the teletext stream does not send a carousel_id for either
	 * of its carousels... luckily it has no other streams for each
	 * but if it did not sure how to identify which belongs to which
	*/
      	if(data_id != 0) {
	    struct stream *str = (struct stream *)malloc(sizeof(struct stream));
	  /* New object carousel */
	  if(carousel_id) {
	    /* Hurrah! We can create a new object carousel with all the info */
	    status->carousels[car_num].id = carousel_id;
					/* TODO Init cache from saved data */
	    str->pid = ntohs(s->res_PID) & 0x1fff;
	    str->assoc_tag = component_tag;
	    str->next = str->prev = NULL;
	    status->carousels[car_num].streams = str;
	    syslog(LOG_ERR, "Initing carousel %d to pid %d", car_num, str->pid);
	    car_num++;
	  } else {
	    /* Hmmm, no carousel_id, set to zero and find out
	     * later hopefully (this is all for you teletext) */
	    status->carousels[car_num].id = 0;
					/* TODO Init cache from saved data */
	    str->pid = ntohs(s->res_PID) & 0x1fff;
	    str->assoc_tag = component_tag;
	    str->next = str->prev = NULL;
	    status->carousels[car_num].streams = str;
	    syslog(LOG_ERR, "Initing carousel %d to pid %d", car_num, str->pid);
	    car_num++;
	  }
	} else {
	  /* New stream belong to unknown carousel */
	  struct stream *str= (struct stream *)malloc(sizeof(struct stream));
	  str->pid = ntohs(s->res_PID) & 0x1fff;
//    	  syslog(LOG_ERR, "Creating stream on pid %d", str->pid);
	  str->assoc_tag = component_tag;
	  if(status->newstreams == NULL) {
	    status->newstreams = str;
	    str->prev = NULL;
	  } else {
	     struct stream *last;
	     for(last=status->newstreams;last->next!=NULL;last=last->next) {;}
	     last->next = str;
	     str->prev = last;
	  }
	  str->next = NULL;
	}
      } else if(s->stream_type == 0x16) { ;   /* Metadata carried in Metadata Section */
printf("------------------ METADATA carried in MetadataSection ----------------------------------------------");

	uint8_t *descr;
	int pidchild;

	for(descr = s->descrs;
	    descr < s->descrs + (ntohs(s->res_ES_info_len) & 0xfff);
	    descr += descr[1] + 2) {

	    printf("PID: %d\n", ntohs(s->res_PID) & 0x1fff);

            //ExtractMetadataSTDDescr(descr);

            pidchild = fork();
            if (pidchild == 0) {
              printf("Child Process!\n");

	      ExtractMetadataSection(card_no, ntohs(s->res_PID) & 0x1fff, status->channel_name);

	    } else if (pidchild == -1) {
              perror("FORK ");
              exit(1);

            } else {
              printf("Parent Process!\n");

	      Pids.pids[++Pids.pidCount] = pidchild;
            }

	}

      } else if(s->stream_type == 0xD) { ;  /* TODO ISO/IEC 13818-6 Type D */
      } else if(s->stream_type == 0x6) { ;   /* TODO PES packets video/audio */
      } else if(s->stream_type == 0x5) {
	uint8_t *descr;

	syslog(LOG_ERR, "Detected MHP stream carrying AIT (PID %d)",
					ntohs(s->res_PID) & 0x1ffff);
	for(descr = s->descrs;
	    descr < s->descrs + (ntohs(s->res_ES_info_len) & 0xfff);
	    descr += descr[1] + 2) {

	      if(descr[0] == MHP_BROADCAST_ID) {
		int length = descr[1];
		int index = 2;
		FILE *ait_fd;
		char *aitsects[256];
		int numsects;
		ait_fd = fopen("/tmp/ait.version.descr", "a");
		while(index < length+2) {
		  int type = descr[index++];
		  int version = descr[index++] & 0x1F;
		  fprintf(ait_fd, "AIT sub table type %d version %d\n", type,
				  		version);
		  /* fprintf(ait_fd, "AIT sub tabla tipo %d version %d", type, version): */
		}
		fclose(ait_fd);

		/* Receive AIT sections from stream and write to /tmp
		 * TODO - process AIT table */
		CollectSections(card_no, ntohs(s->res_PID) & 0x1fff, 0x74,
				aitsects, &numsects);
		if(aitsects != NULL) {
			ait_fd = fopen("/tmp/ait.table", "a");
			for(i = 0;i < numsects; i++) {
				fwrite(aitsects[i], 1, 1024, ait_fd);
			}
			fclose(ait_fd);
			FreeSects(aitsects);
		}

		/* This pid has the AIT sections as described above. Subscribe
		   to pid and build up the AIT table. */
	      }
	}
      }
      sp += (ntohs(s->res_ES_info_len) & 0xfff) + 5;
    }
  }

}

/*
 * Extract descriptors from PMT ( no vpid in all cases so cannot use that as
 * a fallback ). 
 */

static int FindMhegInfoInPMT(int card_no, int pid, struct dsmcc_status *status)
{
  int ret = -1;
  char *pmtsects[256];
  int numsects;

  ret = CollectSections(card_no, pid, 0x02, pmtsects, &numsects);

  if(ret)
    goto bail;

  ExtractMhegInfo(card_no, pmtsects, numsects, status);

 bail:
  FreeSects(pmtsects);
  return ret;
}


/*
 * find the ttxt_info in the PMT via the PAT, try first with the SID
 * and if that fails with the VPID
 * return <> 0 on error;
 */
int GetMhegInfo(int card_no, unsigned short sid, struct dsmcc_status *status)
{
  int ret;
  char *patsects[256];
  int numsects;
  int i;
  int j;
  uint16_t pmt_pid = 0;

  if(status->debug_fd != NULL) {
	  fprintf(status->debug_fd, "[pmtparse] Collecting MhegInfo\n");
  }

  ret = CollectSections(card_no, 0, 0, patsects, &numsects);

  if(ret) {
    syslog(LOG_ERR, "MhegInfo ret - %d", ret);
    goto bail;
  }

  if(status->debug_fd != NULL) {
	  fprintf(status->debug_fd, "[pmtparse] SID - %d\n", sid);
  }

  if(sid != 0) {
    int found;

    for(i = 0, found = 0; i < numsects && !found; i++) {
      int numdescrs;
      struct PAT_sect *s = (struct PAT_sect *) patsects[i];
      
      numdescrs = ((ntohs(s->syntax_len) & 0x3FF) - 7) / 4;
      
	/* Mapping Pid to channel number */
      for(j = 0; j < numdescrs && !found; j++) {
	uint16_t pno = ntohs(s->d[j].program_number);

	if(pno == 0)
	  continue; // network pid
	
  	if(status->debug_fd != NULL) {
  		fprintf(status->debug_fd, "[pmtparse] Program Number %d / Pid %d\n", pno, ntohs(s->d[j].res_PMTPID) & 0x1fff);
  	}

	if(pno == sid) {	/* Found our channel ? */
	  pmt_pid = ntohs(s->d[j].res_PMTPID) & 0x1fff;
  	  if(status->debug_fd != NULL) {
  		fprintf(status->debug_fd, "[pmtparse] Found Correct channel, retrieving PMT on %d\n", pmt_pid);
  	   }
	  found = 1;
	}
      }
    }
  }

  if(pmt_pid != 0) {
    syslog(LOG_ERR, "pmt pid %x\n",pmt_pid);
    ret = FindMhegInfoInPMT(card_no, pmt_pid, status);
  }
    
bail:
  syslog(LOG_ERR, "Bailed");
  FreeSects(patsects);
  return ret;
}

void KillMetadataPids() {
	int i;

  // Loop throught pid_array and kill all sons' pids

  printf("pid_count = %d\n", Pids.pidCount);

  //int count = pid_count;
  //for (int i = 0; i <= count; i++) {
  for (i = 0; i <= Pids.pidCount; i++) {

    printf("pid to kill = %d\n", Pids.pids[i]);

    if (kill(Pids.pids[i], SIGKILL) < 0) {
      perror("Kill ERROR ");
      exit(1);
    }
  }

  Pids.pidCount = -1;
}

void ResetMetadataPids() {
  Pids.pidCount = -1;
}
