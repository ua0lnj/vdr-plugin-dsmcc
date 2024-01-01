#ifndef MONITOR_H
#define MONITOR_H

#include <vdr/status.h>
#include "dsmcc-decoder.h"


class cDsmccMonitor : public cStatus {
private:
	cDsmccReceiver *receiver;
protected:
	virtual void ChannelSwitch(const cDevice *device,  int ChannelNumber);
public:
	cDsmccMonitor(void);
	~cDsmccMonitor();

	void ScanChannels(int numChannels);
};


#endif
