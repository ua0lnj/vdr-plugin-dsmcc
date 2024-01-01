#include "dsmcc-decoder.h"
#include "dsmcc-monitor.h"
#include "dsmcc-siinfo.h"
#include "dsmcc-cache.h"
// #include <mpatrol.h>

cDsmccMonitor::cDsmccMonitor(void)
{
  receiver = NULL;
}

cDsmccMonitor::~cDsmccMonitor()
{
  delete receiver;
}

void cDsmccMonitor::ChannelSwitch(const cDevice *Device, int ChannelNumber)
{
//  struct stream *streams = NULL, *str;
  int i;

  if(Device->IsPrimaryDevice() && ChannelNumber != 0) {
    delete receiver;
    receiver = NULL;
    cChannel *c = Channels.GetByNumber(ChannelNumber);

//    esyslog("Scanning channel %d", ChannelNumber);

//  esyslog("Collecting MHEG info %d/%d", Device->DeviceNumber(), c->Sid());

    receiver = new cDsmccReceiver(c->Name()); 

    GetMhegInfo(0, c->Sid(), receiver->status);

    if(receiver->status->carousels[0].streams != NULL) {
 //     esyslog("Found Object Carousel");
	cDevice *device = cDevice::ActualDevice();
//	esyslog("Got change to channel");

//	esyslog("Device = %p", device);

	device->AttachReceiver(receiver);

	for(i=0;i<MAXCAROUSELS;i++) {
//		ObjCarousel *cart;
//		cart = &receiver->carousels[i];
		if(receiver->status->carousels[i].streams != NULL) {
		   receiver->AddStream(receiver->status, receiver->status->carousels[i].streams->pid); 
//                 esyslog("Receiving pid %d", receiver->status->carousels[i].streams->pid);
		}
	}
    }
  }

}

void cDsmccMonitor::ScanChannels(int numChannels) {
  for (int i = 60; i <= (60+numChannels); i++) {
    cChannel *c = Channels.GetByNumber(i);
    cDevice *dev = cDevice::ActualDevice();
//    esyslog("Switching to new channel after sleep");
    dev->SwitchChannel(c, true);
    while(receiver && receiver->Active()) { sleep(5); }
  }
}
