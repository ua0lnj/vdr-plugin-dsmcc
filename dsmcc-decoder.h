#ifndef DSMCC_DECODER_H
#define DSMCC_DECODER_H

#include "vdr/receiver.h"
#include "libdsmcc.h"

class cDsmccReceiver : public cReceiver, cThread {
private:
//	FILE *debug_fd, *test_fd, *dsi_debug;
	bool active;
	char *name;

protected:
	virtual void Receive(uchar *Data, int Length);
	virtual void Activate(bool ON);
	virtual void Action() { ; };
public:
	cDsmccReceiver(const char *);
	void AddStream(struct dsmcc_status *, int pid);
	char *Name(void) { return name; }
	bool Active();
	struct dsmcc_status *status;

	int scanning;
	~cDsmccReceiver();
};

#endif

