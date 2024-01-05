//#include "dsmcc-decoder.h"
#include "dsmcc-monitor.h"
#include "dsmcc-siinfo.h"
#include "dsmcc-cache.h"
// #include <mpatrol.h>
#include <vdr/dvbdevice.h>
#include <vdr/plugin.h>

cDsmccMonitor::cDsmccMonitor(void)
{
  receiver = NULL;
}

cDsmccMonitor::~cDsmccMonitor()
{
  delete receiver;
}

void cDsmccMonitor::Scan(int ChannelNumber)
{
  int i;

  cDevice *device;
  device = cDevice::ActualDevice();

  delete receiver;
  receiver = NULL;
  if (!ChannelNumber) ChannelNumber = device->CurrentChannel();

  LOCK_CHANNELS_READ;
  const cChannel *c = Channels->GetByNumber(ChannelNumber);

  esyslog("[dsmcc] Scanning channel %d", ChannelNumber);
  esyslog("[dsmcc] Collecting MHEG info %s %d/%d", (const char*)device->DeviceName(), device->DeviceNumber(), c->Sid());

  receiver = new cDsmccReceiver(c->GetChannelID().ToString());
  GetMhegInfo(device, c->Sid(), receiver->status);

  if(receiver->status->carousels[0].streams != NULL) {
    esyslog("[dsmcc] Found Object Carousel");
    device->AttachReceiver(receiver);

    for(i=0;i<MAXCAROUSELS;i++) {
//	ObjCarousel *cart;
//	cart = &receiver->carousels[i];
	if(receiver->status->carousels[i].streams != NULL) {
	   receiver->AddStream(receiver->status, receiver->status->carousels[i].streams->pid); 
         esyslog("[dsmcc] Receiving pid %d", receiver->status->carousels[i].streams->pid);
	}
    }
  }
}

void cDsmccMonitor::ChannelSwitch(const cDevice *Device, int ChannelNumber, bool LiveView)
{
  //For LiveView, the hbbtvng plugin uses the "Scan dsmcc" service
  if (cPluginManager::GetPlugin("hbbtvng") && LiveView) return;

  if(Device->IsPrimaryDevice() && ChannelNumber != 0) {
    esyslog("[dsmcc] Got change to channel");
    Scan(ChannelNumber);
  }
}

void cDsmccMonitor::ScanChannels(int numChannels) {
  for (int i = 1166; i <= (60+numChannels); i++) {
    LOCK_CHANNELS_READ;
    const cChannel *c = Channels->GetByNumber(i);
    cDevice *dev = cDevice::ActualDevice();
    esyslog("[dsmcc] Switching to new channel after sleep");
    dev->SwitchChannel(c, true);
    while(receiver && receiver->Active()) { sleep(5); }
  }
}
